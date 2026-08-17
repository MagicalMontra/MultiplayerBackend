# MultiplayerBackend.GameServer — Session Checkpoint

## Current milestone

The current walkthrough has established the intended foundation for:

- Linux non-blocking TCP;
- RAII file-descriptor ownership;
- epoll-driven connection I/O;
- timerfd-driven authoritative simulation;
- length-prefixed packet framing and incremental decoding;
- explicit `GameTime` / `SimulationStep` semantics;
- low-rate server simulation with sub-tick event placement;
- fixed-capacity rollback state and step history;
- replayable movement/spawn/despawn events;
- bounded/pruned rollback event history;
- stable `EntityId` plus dense `EntityStore`;
- sparse world chunks with 16×16 / 256-entry 1D cell storage;
- incremental spatial-grid maintenance during normal simulation;
- full grid reconstruction after rollback restore;
- persistent AOI interest diffing (`entered`, `stayed`, `left`).

## Architectural checkpoint

```text
GameServer
│
├── epoll / sockets / timerfd
│
├── WorldSimulation
│   ├── WorldState                 authoritative
│   ├── SimulationEvent timeline   replayable history
│   ├── StateHistory               rollback snapshots
│   ├── SimulationStep history     historical timing
│   └── SpatialGrid                derived world index
│
└── InterestTracker                replication knowledge
```

Rules to preserve:

```text
WorldState              rolls back
SpatialGrid             is rebuilt/derived
InterestTracker         does NOT roll back
EntityId                is stable identity
vector index            is storage location only
step interval           is [start_time, end_time)
StateHistory[N]         means state after Tick N
```

## Current experiment

The server examples are deliberately using a low simulation rate (12 Hz) while the eventual client may predict at 30/60 Hz and render independently. Inputs/events are mapped to authoritative `GameTime`; matching tick numbers are not required.

Temporary demo events currently prove two cases:

1. a late movement transition at 1.55 s causes restore + resimulation and corrects the present;
2. a late historical spawn at 2.55 s is replayed during rollback, after which the non-rollback `InterestTracker` can report that entity as newly entered AOI.

These demo paths should later be replaced by real session/network input.

## Recommended next implementation

Build a transport-independent replication snapshot model:

```text
InterestDelta
│
├── entered → SpawnEntityRecord
├── stayed  → EntityStateRecord
└── left    → DespawnEntityRecord
            ↓
      ReplicationSnapshot
```

Then continue in roughly this order:

1. bind `ClientConnection` / session state to a controlled `EntityId`;
2. schedule replication independently from simulation rate;
3. encode replication snapshots into protocol messages;
4. replace demo movement with actual network commands;
5. add client/server clock synchronization and validated GameTime mapping;
6. add bounded duplicate/out-of-order input sequence handling;
7. introduce a higher-rate client prediction/reconciliation model;
8. implement rollback-sensitive Attack / Dodge / Parry / action state;
9. add UDP for latency-sensitive gameplay where appropriate.

## Known follow-up issues

- `EPOLLERR` / `EPOLLHUP` need explicit policy.
- Pending send queues need a maximum size/backpressure rule.
- Packet types and payload sizes should receive stricter validation.
- Sequence replay protection must eventually be independent of pruned rollback history.
- Copying the complete entity lookup map into each rollback snapshot may later be optimized after profiling.
- Cell occupant vectors may later be optimized if allocation/churn becomes measurable.
- timerfd repeating periods are a tiny nanosecond approximation at rates like 12 Hz; authoritative `TimeAtTick()` already avoids cumulative game-time drift.
- simulation catch-up needs a maximum/overload policy to avoid a spiral of death.
- rollback-sensitive simulation must not directly duplicate irreversible side effects.

## Suggested next commit title

```text
Build snapshot replication model and client interest pipeline
```
