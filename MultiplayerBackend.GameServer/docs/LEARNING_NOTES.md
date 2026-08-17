# MultiplayerBackend.GameServer — Detailed Learning Notes

## 1. C++ model and compilation

### Translation units

Each `.cpp` file is compiled independently as a translation unit. Headers expose declarations; source files normally provide definitions. The linker later resolves references between translation units.

This explains the difference between:

```text
compile-time error
→ syntax/type/declaration problem inside a translation unit

link-time error
→ a referenced symbol has no valid definition, or definitions conflict
```

### `#include`

`#include` is a preprocessor operation that makes declarations/content visible before C++ compilation. `#pragma once` prevents repeated inclusion of the same header within one translation unit.

### `const` and `constexpr`

`const` expresses non-modification through that declaration. `constexpr` expresses that a value/function can participate in compile-time evaluation where possible.

### References and pointers

```cpp
T&     // reference
T*     // pointer
&obj   // address-of
*ptr   // dereference
```

Pointers can represent no object with `nullptr`; references generally model an existing object.

### trailing `const`

```cpp
int Get() const;
```

means the member function will not modify the object's logical state through `this`, allowing it to be called through const-qualified objects/references.

### `explicit`

`explicit` prevents unintended implicit construction/conversion. It does not mean "initialize this object". Direct construction remains valid.

## 2. Lifetime, ownership and RAII

Automatic objects are destroyed when their scope ends. Dynamically allocated resources have separate lifetime management.

RAII ties resource lifetime to object lifetime:

```text
object acquires resource
→ object owns resource
→ destructor releases resource
```

`UniqueFd` applies this to Linux descriptors. It is non-copyable because two owners of the same fd could double-close it; it is movable so ownership can be transferred safely.

`std::move` only enables move semantics; the actual resource transfer occurs in the type's move constructor/assignment. Move operations that cannot throw are appropriately marked `noexcept`, which also helps standard containers during relocation.

## 3. Standard containers and views

### `std::vector`

```text
size     = number of live elements
capacity = allocated room before reallocation
```

`reserve()` changes capacity, not size. Vector storage is contiguous, which is good for iteration/cache locality, but reallocations invalidate pointers/references/iterators.

### `std::array`

Fixed-size contiguous storage. It became a natural match for a 16×16 chunk containing exactly 256 cell slots.

### `std::span`

A non-owning view over contiguous memory, useful for socket send/receive APIs without forcing one owning container type.

### `std::byte`

Represents raw bytes without character/arithmetic semantics, appropriate for binary packet buffers.

### templates

`StateHistory<T, Capacity>` and `EntityStore<T>` demonstrate templates: one generic definition produces concrete types for different `T` and capacities.

### `std::optional`

Represents either a value or no value. Useful for history slots and searches where absence is legitimate.

### `std::variant`

A type-safe tagged union. The simulation event payload can contain one of several event types without unsafe manual casts. `std::get_if<T>()` returns a pointer when the active alternative is `T`, otherwise `nullptr`.

## 4. Linux descriptors and non-blocking networking

Linux exposes sockets, epoll and timerfd as integer file descriptors. The integer is only a handle; correct ownership still matters.

A non-blocking socket does not wait for I/O to become possible. `EAGAIN/EWOULDBLOCK` means "try again later", not a fatal error.

A non-blocking `send()` may write only part of a buffer. Therefore the server needs a persistent send queue and offset. `EPOLLOUT` should be subscribed only while data remains queued.

`MSG_NOSIGNAL` prevents a failed socket write from killing the process via `SIGPIPE`, allowing the error to be handled normally.

## 5. epoll

`epoll` reports readiness:

```text
listener readable → accept
client readable   → recv until no more progress
client writable   → flush queued bytes
 timerfd readable  → advance simulation
```

Readiness means progress is possible, not that an entire message/write will complete.

The current implementation uses level-triggered semantics, which are simpler for learning than edge-triggered mode.

TCP directions are independent. `recv() == 0` means the peer will send no more bytes, but the server may still finish pending output before destruction.

## 6. TCP framing

TCP preserves byte order but not application message boundaries. One `send()` can arrive through several `recv()` calls, and several sends can arrive together.

Therefore the protocol uses a length-prefixed frame:

```text
uint32 payload size
uint16 packet type
payload bytes
```

The incremental decoder accumulates bytes until a full header and payload are available. Network byte order defines the wire representation of multi-byte integers.

## 7. Fixed-step simulation and timerfd

`timerfd` integrates simulation timing into epoll because timer expirations are readable file-descriptor events.

Simulation scheduling uses a monotonic clock because wall-clock time can jump due to clock adjustments and is unsuitable for measuring deterministic durations.

A tick is not the same thing as time. `SimulationStep` records tick identity plus exact start/end/delta `GameTime`.

At 12 Hz, 1/12 second is not an integer number of nanoseconds. Repeatedly adding one truncated duration would accumulate error, so the authoritative game timeline is derived from absolute tick index.

## 8. Different server/client rates

The important design shift from a matching-rate model is:

```text
old mental model:
one client command ≈ one server step ≈ one fixed movement increment

new model:
input changes control state at a time
server integrates that state over authoritative time
```

This allows configurations such as:

```text
server simulation  12 Hz
client prediction  60 Hz
rendering          144/240/unrestricted
```

The client does not need to send its authoritative displacement or delta time. It sends intent plus sequencing/timing information; the server determines the result.

## 9. Sub-tick simulation

A 12 Hz outer step is roughly 83 ms, but an event can occur inside it.

```text
1.500 -------- 1.550 -------- 1.583
Forward          event         Backward
```

The server can split the interval at the event time. This separates simulation-update frequency from input timing resolution and is a major mechanism for exploring responsive action gameplay on a relatively low-rate server.

## 10. Rollback semantics

Rollback means:

```text
restore historical state
→ modify/use corrected historical event timeline
→ run the same simulation rules forward
→ obtain corrected present
```

It is not merely editing the current position.

The state-history convention is deliberately explicit:

```text
history[N] = state after Tick N
```

So to redo Tick N, restore N-1.

Historical `SimulationStep`s are retained so replay uses the actual original time intervals.

## 11. Event sourcing inside the rollback window

Movement history alone is insufficient once entity lifecycle is introduced. A historical spawn/despawn must also be replayed if rollback crosses it.

A generic `SimulationEvent` timeline therefore holds multiple payload types with explicit `(time, order)` sorting.

The order field matters because two events can share a timestamp. Deterministic replay requires a deterministic tie-breaker.

Events older than the oldest replayable point can be pruned because their effects are already represented by the retained snapshot.

Step interval semantics are `[start, end)`: an event exactly at `end` belongs to the next step. This boundary rule prevents ambiguous/off-by-one replay behavior.

## 12. Server vs client determinism

The client prediction simulation may be approximate; its purpose is responsiveness and it can later reconcile to authoritative snapshots.

Server rollback has a stronger requirement: given the same restored state, event timeline and timing, original simulation and resimulation should reproduce the same authoritative result.

## 13. Stable entity identity

`EntityId` is identity; dense storage index is not.

`EntityStore` uses:

```text
vector<PlayerState>
unordered_map<EntityId, index>
```

This combines fast contiguous iteration with average O(1) lookup. Removal uses swap-and-pop, which makes vector order unstable but keeps removal cheap.

Because vector elements may move, long-lived cross-system references should use `EntityId`, not retained `PlayerState*` pointers.

## 14. AOI as derived state

The authoritative source of truth is entity position in `WorldState`.

Cell/chunk membership is an index derived from position. It should not become a second authoritative truth that can disagree with the world state.

Therefore rollback saves/restores `WorldState`, then rebuilds the spatial grid from restored positions.

## 15. Chunking and 1D cell indexing

A very large world should not require allocating one giant dense cell array. The chosen structure is:

```text
sparse global chunks
+
dense local cell storage
```

Each 16×16 chunk contains 256 cell entries in a single array. Local coordinates convert with:

```text
index = y * width + x
```

and reverse with:

```text
x = index % width
y = index / width
```

This preserves convenient 2D APIs while using a cache-friendly internal representation.

Negative world coordinates require custom floor division/positive modulo because C++ signed division truncates toward zero rather than mathematical floor.

## 16. Incremental spatial maintenance

Normal movement should not rebuild every AOI cell every server tick.

```text
position changes
→ compute new cell
→ if same cell: no structural update
→ if different: remove from old cell, add to new cell
```

Rollback is exceptional: restoring a historical world can move/remove/create many entities at once, so a one-time full rebuild is simpler and safer before incremental replay resumes.

This is a useful general systems lesson: the best common-path algorithm and best correction-path algorithm do not need to be identical.

## 17. Interest tracking

An AOI query answers only "what is relevant now?" Replication also needs to remember "what did the client already know?"

`InterestTracker` stores a sorted current set and compares it with a new desired set, producing:

```text
entered
stayed
left
```

For small interest sets, sorted vectors provide deterministic ordering, contiguous memory and an O(n+m) merge-style diff.

## 18. Replication state is not rollback state

This distinction is critical.

Packets already observed by a remote client cannot literally be undone when the server rolls back. Instead, after correcting the authoritative present, the replication system compares that present with its knowledge of what the client currently knows.

Example:

```text
client known set:       {2}
corrected authoritative AOI: {2,5}

result:
entered = {5}
```

The next replication pass can send SpawnEntity(5). This converges the remote client to the corrected present without pretending the network conversation itself was rewound.

## 19. Security lessons

The client should not control:

```text
position
displacement
movement speed
deltaTime
damage result
parry success
```

The future timing system must also avoid blindly trusting arbitrary client timestamps. It will need clock-offset/RTT estimation, sequence checks, rollback-window bounds and future/stale timestamp validation.

## 20. Rollback and side effects

Simulation state can be restored; external side effects often cannot.

Examples:

```text
database writes
reward grants
analytics events
packets already sent
client-side audio/VFX already played
```

Future rollback-sensitive combat code must distinguish deterministic/resimulatable simulation effects from committed external effects.

## 21. Current conceptual progression

```text
C++ ownership/lifetime
→ Linux descriptors
→ non-blocking sockets
→ epoll
→ TCP framing
→ timerfd simulation
→ authoritative GameTime
→ sub-tick input
→ rollback history
→ generic replayable events
→ stable entity storage
→ chunked 1D AOI
→ incremental spatial updates
→ persistent replication interest
```

The most important lesson is architectural separation. Networking bytes, authoritative world state, historical event state, spatial indexes and remote-client replication knowledge are different kinds of state with different ownership and rollback rules.
