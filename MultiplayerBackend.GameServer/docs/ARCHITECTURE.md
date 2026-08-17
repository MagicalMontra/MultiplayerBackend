# MultiplayerBackend.GameServer — Architecture Notes

## Goal

The project is a Linux-focused authoritative multiplayer game server intended to expose systems that higher-level game networking frameworks normally hide. The current target is an instance-based action-combat server that can also retain MMO-style spatial/AOI functionality for larger maps.

The deliberate networking experiment is:

- low authoritative server simulation rate, currently demonstrated at 12 Hz;
- client prediction potentially at 30/60 Hz;
- rendering unrestricted and independent from simulation rate;
- input/events placed on a shared authoritative `GameTime` timeline rather than requiring matching client/server ticks;
- rollback and resimulation for late events;
- spatially filtered replication for larger worlds.

Core rule:

> Client tick rate does not have to equal server tick rate. Shared game time is the bridge.

## Current architecture

```text
GameServer
│
├── Networking
│   ├── UniqueFd
│   ├── TcpSocket
│   ├── epoll
│   ├── timerfd
│   └── ClientConnection
│
├── Protocol
│   ├── Packet
│   ├── PacketCodec
│   └── PacketDecoder
│
├── WorldSimulation
│   ├── SimulationStep / GameTime
│   ├── WorldState
│   ├── EntityStore
│   ├── SimulationEvent timeline
│   ├── StateHistory
│   ├── historical SimulationStep storage
│   └── rollback/resimulation
│
├── SpatialGrid
│   ├── sparse chunks
│   ├── dense 1D cells per chunk
│   └── EntityId membership
│
└── Replication
    └── InterestTracker
        ├── entered
        ├── stayed
        └── left
```

Three state categories must remain separate:

```text
1. Authoritative simulation state
   WorldState
   → saved/restored during rollback

2. Derived simulation state
   SpatialGrid
   → rebuilt from authoritative state when necessary

3. Replication/network knowledge state
   InterestTracker
   → represents what a remote client currently knows
   → must NOT be rewound with rollback
```

## Linux resource ownership

`UniqueFd` is the RAII owner for Linux file descriptors. Sockets, epoll instances and timerfd instances are all represented by file descriptors, so ownership must be explicit.

`UniqueFd` is non-copyable and movable. The destructor closes the descriptor. This prevents duplicated ownership and makes cleanup deterministic on every control-flow path.

`TcpSocket` composes `UniqueFd`, so socket ownership inherits the same rule.

## Non-blocking TCP and epoll

The server uses non-blocking sockets and a level-triggered epoll loop. A readiness notification means an operation can probably make progress; it does not mean a complete logical read/write will finish.

Important cases:

```text
recv > 0                         bytes received
recv == 0                        peer read-side EOF / orderly shutdown
recv < 0 + EAGAIN/EWOULDBLOCK    no data available now
send < requested amount          partial write
send < 0 + EAGAIN/EWOULDBLOCK    cannot accept more output now
EINTR                            interrupted syscall; retry as appropriate
```

`ClientConnection` therefore owns a send buffer and offset. `EPOLLOUT` is enabled only while pending bytes remain; otherwise writable readiness would cause unnecessary wakeups.

TCP half-close is handled separately from immediate destruction: after `recv() == 0`, pending server output may still be flushed before the connection is removed.

## Packet framing

TCP is a byte stream, not a message transport. The protocol therefore uses explicit framing:

```text
[ uint32 payload_size ][ uint16 packet_type ][ payload ]
```

The fixed header is 6 bytes. `PacketDecoder` accumulates arbitrary TCP fragments and emits complete packets only when enough bytes have arrived. Network byte order is used for multi-byte wire integers.

## Authoritative simulation time

The simulation defines:

```cpp
using GameTime = std::chrono::nanoseconds;

struct SimulationStep
{
    std::uint64_t tick;
    GameTime start_time;
    GameTime end_time;
    GameTime delta_time;
};
```

Tick identity and time are deliberately separate. That allows sub-tick events, replay and mismatched client/server rates.

At rates such as 12 Hz, one ideal step cannot be represented as an exact integer number of nanoseconds. The authoritative timeline is therefore derived from absolute tick index (`TimeAtTick`) rather than repeatedly adding one truncated period. This prevents cumulative truncation in game time.

## Different client/server rates

A possible target is:

```text
server simulation       12 Hz
snapshot replication     6 Hz
client prediction       60 Hz
rendering          unrestricted
```

The client may generate an input around prediction step C93, but the server does not need to understand C93. It needs a validated location on authoritative game time, e.g. `1.550 s`.

Example:

```text
Server Tick 19
1.500 s ------------------------------ 1.583 s
                    ↑
                 1.550 s
                 Backward
```

The server can simulate 1.500→1.550 using the old control state, apply the event at 1.550, then simulate 1.550→1.583 with the new state. Thus outer simulation frequency and event timing resolution are not identical concepts.

## Client authority model

Clients send intent, not results.

Allowed examples:

```text
Forward / Backward / Left / Right
Attack
Dodge
Parry
Jump
sequence/timing metadata
```

The server owns movement speed, authoritative delta time, positions, damage, hit/parry results and other game outcomes. Client-provided displacement/position/damage must not be treated as authoritative.

## Rollback history

`StateHistory<T, Capacity>` is a fixed-size ring buffer. A slot is selected with `tick % Capacity`, but every slot also stores its actual tick so that overwritten/stale entries cannot be mistaken for the requested history.

Snapshot semantics are explicit:

```text
history[0] = state before Tick 1
history[N] = state after Tick N
```

Therefore replaying Tick N requires restoring state N-1.

Historical `SimulationStep` values are stored as well so resimulation can replay the exact historical time intervals rather than reconstructing them from a current tick-rate assumption.

## Replayable event timeline

Rollback evolved from movement-only input history into a generic `SimulationEvent` timeline using `std::variant` payloads such as:

```text
MovementInput
SpawnPlayerEvent
DespawnEntityEvent
```

Each event has:

```text
GameTime
stable order value
payload
```

The stable order resolves deterministic ordering for events sharing the same timestamp.

Entity lifecycle must be replayable. If Player 5 spawned at 2.55 s and the server rolls back to 2.50 s, restoring the old state removes Player 5. Replaying the recorded spawn at 2.55 s recreates the correct present.

## Rollback procedure

For a late event:

```text
validate event/time
→ find the historical step containing that GameTime
→ restore the previous completed state
→ rebuild derived spatial state
→ replay events in chronological order
→ resimulate historical steps to the present
→ overwrite stale history with corrected states
```

A demonstrated toy case:

```text
present @ 2.000 s: x = 2.0
late Backward event @ 1.550 s
restore @ 1.500 s: x = 1.5, Forward
simulate Forward 1.500→1.550
apply Backward
simulate Backward 1.550→2.000
corrected present ≈ x = 1.1
```

The retained event history is pruned once its effects are already captured in the oldest restorable state. Step intervals use `[start_time, end_time)`, so events exactly at the replay boundary must remain.

## Entity identity and storage

`EntityId` is stable identity; vector index is not.

`EntityStore<T>` uses:

```text
dense std::vector<T>
+
unordered_map<EntityId, index>
```

This gives contiguous iteration plus average O(1) lookup. Removal uses swap-and-pop and repairs the moved entity's ID→index mapping.

Long-lived system references should use `EntityId`, not pointers into the dense vector, because vector reallocation and swap-and-pop can invalidate/move objects.

## Spatial AOI

AOI is initially a replication concern, not a rule deciding whether an authoritative entity exists or is simulated.

```text
WorldState.position
      ↓ canonical
SpatialGrid
      ↓ derived
nearby EntityIds
      ↓
InterestTracker / replication
```

This lets a small combat instance simply make most entities mutually relevant, while larger field maps can use spatial filtering within the same architecture.

## Chunked 1D grid storage

The world is sparse globally but dense locally:

```text
unordered_map<ChunkCoord, Chunk>

Chunk
└── 16 × 16 logical cells
    └── 256 physical 1D entries
```

Local 2D→1D conversion:

```text
index = y * ChunkWidth + x
```

For width 16:

```text
(0,0)   → 0
(15,0)  → 15
(0,1)   → 16
(5,3)   → 53
(15,15) → 255
```

Reverse:

```text
x = index % ChunkWidth
y = index / ChunkWidth
```

Negative global cells require mathematical floor division and positive modulo because C++ integer division truncates toward zero. Example:

```text
global cell -17, chunk width 16
→ chunk -2
→ local cell 15
```

Only populated chunks need to exist, while the local cell metadata remains contiguous.

## Incremental AOI updates

Normal simulation no longer needs to clear/rebuild the entire grid every tick.

```text
entity moves
→ compute new GridAddress
→ same cell: no membership mutation
→ different cell: swap-and-pop from old cell, push into new cell
```

Rollback remains intentionally different:

```text
restore WorldState
→ rebuild entire SpatialGrid once
→ historical resimulation continues with incremental updates
```

This keeps the common path efficient while preserving a simple, safe correction path.

## Interest tracking and replication semantics

`InterestTracker` stores the set of entities a remote observer is currently considered to know. Comparing its current sorted vector with the newly desired AOI produces:

```text
entered
stayed
left
```

This naturally maps toward:

```text
entered → SpawnEntity
stayed  → StateUpdate
left    → DespawnEntity
```

Crucially, replication knowledge is not rewound when simulation rolls back. If rollback reveals that Entity 5 should now exist in the client's AOI, the next interest diff reports Entity 5 as `entered`; the server tells the client about the corrected present rather than attempting to rewind packets already seen.

## Current invariants

1. Server simulation is authoritative.
2. Clients send intent, not authoritative results.
3. Client and server simulation rates do not need to match.
4. Shared `GameTime` bridges independent rates.
5. Server delta time is never trusted from the client.
6. `WorldState` is canonical rollback state.
7. `SpatialGrid` is derived from canonical state.
8. Replication state is not rollback state.
9. `EntityId` is stable identity; dense index is temporary storage location.
10. Long-lived references between systems should use IDs rather than pointers into movable storage.
11. Lifecycle transitions crossed by rollback must be replayable.
12. Same-time event ordering must be deterministic.
13. AOI initially filters replication rather than authoritative simulation existence.
14. Normal AOI updates are incremental; rollback may rebuild derived indexes.
15. Rendering remains independent from server/prediction cadence.

## Next architectural milestone

The next logical feature is a replication data model independent from transport:

```text
InterestDelta
│
├── entered → SpawnEntityRecord
├── stayed  → EntityStateRecord
└── left    → DespawnEntityRecord
            ↓
      ReplicationSnapshot
```

After that, the project can associate real connections/sessions with controlled EntityIds, run replication at its own cadence, serialize snapshots, replace demo events with network input, add time synchronization, and then build rollback-sensitive Attack/Dodge/Parry combat.

## Known follow-up work

- explicit `EPOLLERR` / `EPOLLHUP` handling;
- per-client pending-send limits / backpressure policy;
- stronger packet type and payload validation;
- UDP path for latency-sensitive gameplay;
- sequence/ack/out-of-order handling independent from pruned rollback history;
- client/server clock synchronization and timestamp validation;
- maximum rollback window;
- timer catch-up/overload policy to prevent spiral-of-death behavior;
- side-effect discipline so rollback does not duplicate irreversible DB/network/analytics effects;
- profiling before deeper entity/cell storage optimizations.
