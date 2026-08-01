# 03. From SAGE to independent Unreal architecture
## 3.1. Observed division in Generals/Zero Hour
The official SAGE tree divides the code into:
- `Common` - common infrastructure, data, file system, State, audio descriptions, shared definitions;
- `GameClient` - drawable/presentation, UI, selection, input translation, camera/view, localization and shell;
- `GameLogic` - object model, AI, pathfinding, locomotion, weapons, armor, scripts, terrain logic, powers and victory;- `GameNetwork` — connections, frame data, command messages, packets, LAN/NAT/transport;
- `GameEngineDevice` - rendering, audio, video and platform adapters.
This is a useful boundary, but you can't carry over old classes one-to-one.
## 3.2. Suggested Unreal modules
```text
Source/
  RA4Core/
  RA4Data/
  RA4Simulation/
  RA4Commands/
  RA4Navigation/
  RA4Combat/
  RA4Economy/
  RA4Production/
  RA4AI/
  RA4Scripting/
  RA4Net/
  RA4Replay/
  RA4Presentation/
  RA4UI/
  RA4Audio/
  RA4Editor/
  RA4Developer/
```

### RA4Core

- stable IDs;
- deterministic containers/utilities;
- common result/error types;
- logging categories;
- time/frame types;
- no dependency on Slate, Niagara or high-level UI.

### RA4Data

- primary definitions;
- registries;
- validation;
- dependency graph;
- cooked content manifest;
- schema versioning and migrations.

### RA4Simulation

- fixed simulation tick;
- authoritative world state;
- entity lifecycle;
- spatial query abstraction;
- snapshot/hash interface;
- never relies on frame-rate-dependent Actor Tick for game rules.

### RA4Commands

- immutable player/AI commands;
- validation before execution;
- deterministic ordering;
- command serialization;
- command history for replay and debugging.

### RA4Navigation

- navigation domains: ground, infantry, naval, air, amphibious;
- path requests and group paths;
- local avoidance;
- formation slots;
- dynamic blockers;
- deterministic movement contract where multiplayer demands it.

### RA4Combat

- targeting;
- weapon state machines;
- projectiles and instant effects;
- armor/damage calculation;
- status effects;
- veterancy;
- death resolution.

### RA4Economy / RA4Production

- resource accounts and transactions;
- power/supply/capacity systems;
- build queues;
- construction lifecycle;
- tech dependency evaluation;
- cancellation/refund rules.

### RA4AI

- strategic goals;
- economy and production planning;
- army composition;
- tactical groups;
- target heuristics;
- unit micro;
- deterministic AI decisions in synchronized modes.

### RA4Net

- lobby/session transport is separate from simulation protocol;
- command exchange;
- frame barriers/input delay;
- state hashes and desync report;
- reconnect/resync policy;
- protocol version and content manifest handshake.

### RA4Replay

- header with build/protocol/content hashes;
- initial match config and random seeds;
- ordered command stream;
- periodic checkpoints;
- seek index;
- deterministic verification mode.

### RA4Presentation

- simulation entity → Actor/proxy mapping;
- interpolation;
- animation/FX/audio event bridge;
- fog-visible representation;
- visual state must not mutate authoritative simulation.

## 3.3. Key architectural invariant
```text
Input / AI decision
  → validated command
  → deterministic simulation mutation
  → presentation events
  → Unreal visual/audio response
```

Neither Niagara, nor Animation Blueprint, nor Slate, nor the client-only audio callback should directly change money, health, position-in-simulation, cooldown or tech state.
## 3.4. Net
The names `FrameData`, `NetCommand*`, `GameMessageParser` and the CRC/state framework in Generals indicate a command/frame-oriented deterministic architecture. This is a strong reference, but there is no reason to blindly repeat P2P lockstep.
for RA4 there are three modes to compare:
1. **Deterministic peer lockstep** - low traffic, heavy Requirements for determinism and protection against desync.
2. **Server-authoritative deterministic simulation** - commands pass via the server; integrity control is simpler, infrastructure is more expensive.
3. **Hybrid command stream + snapshots** - main command stream, periodic authoritative snapshots/recovery.
Preliminary recommendation: design simulation/commands/replay so that they support deterministic command stream, and select a specific transport after the load prototype on the target number of units.
## 3.5. What not to copy from SAGE
- global singleton-like subsystems;
- close connection between renderer/platform and simulation;
- old Win32/GameSpy/DirectX adapters;
- huge inherited object/module trees without strict ownership rules;
- floating-point assumptions of the old x86;
- UI that directly knows the internals of simulation objects;
- network protocol without versioned schemas and diagnostics.
## 3.6. Minimum Vertical Architectural Test
Not a product MVP, but an engineering proving ground:
- 2 players;- 500–1000 simulation entities;
- selection + move + attack + production;
- one ground navigation domain;- command recording/replay;
- hash state every N frames;
- forced entry of discrepancies and clear desync report;
- headless launch of two simulations on the same command stream;
- the same final hash on Windows/macOS/Linux or a documented platform boundary.