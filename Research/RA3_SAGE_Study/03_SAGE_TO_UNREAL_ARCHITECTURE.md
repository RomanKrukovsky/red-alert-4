# 03. От SAGE к самостоятельной Unreal-архитектуре

## 3.1. Наблюдаемое разделение в Generals / Zero Hour

Официальное дерево SAGE разделяет код на:

- `Common` — общая инфраструктура, данные, файловая система, состояние, аудио-описания, shared definitions;
- `GameClient` — drawable/presentation, UI, selection, input translation, camera/view, локализация и shell;
- `GameLogic` — object model, AI, pathfinding, locomotion, weapons, armor, scripts, terrain logic, powers и victory;
- `GameNetwork` — connections, frame data, command messages, packets, LAN/NAT/transport;
- `GameEngineDevice` — rendering, audio, video и platform adapters.

Это полезная граница, но переносить старые классы один к одному нельзя.

## 3.2. Предлагаемые Unreal modules

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

## 3.3. Ключевой архитектурный инвариант

```text
Input / AI decision
  → validated command
  → deterministic simulation mutation
  → presentation events
  → Unreal visual/audio response
```

Ни Niagara, ни Animation Blueprint, ни Slate, ни client-only audio callback не должны напрямую менять деньги, здоровье, position-in-simulation, cooldown или tech state.

## 3.4. Сеть

Имена `FrameData`, `NetCommand*`, `GameMessageParser` и инфраструктура CRC/state в Generals указывают на command/frame-oriented deterministic architecture. Это сильный референс, но не причина слепо повторять P2P lockstep.

Для RA4 следует сравнить три режима:

1. **Deterministic peer lockstep** — низкий трафик, тяжёлые требования к детерминизму и защите от desync.
2. **Server-authoritative deterministic simulation** — команды проходят через сервер; проще контроль честности, дороже инфраструктура.
3. **Hybrid command stream + snapshots** — основной поток команд, периодические authoritative snapshots/recovery.

Предварительная рекомендация: проектировать simulation/commands/replay так, чтобы они поддерживали deterministic command stream, а конкретный transport выбрать после нагрузочного прототипа на целевом количестве юнитов.

## 3.5. Что не копировать из SAGE

- глобальные singleton-подобные подсистемы;
- тесную связанность renderer/platform с simulation;
- старые Win32/GameSpy/DirectX adapters;
- огромные наследуемые object/module trees без строгих ownership rules;
- floating-point assumptions старого x86;
- UI, который напрямую знает внутренности simulation objects;
- сетевой протокол без versioned schemas и diagnostics.

## 3.6. Минимальный вертикальный архитектурный тест

Не MVP продукта, а инженерный proving ground:

- 2 игрока;
- 500–1000 simulation entities;
- selection + move + attack + production;
- одна ground navigation domain;
- command recording/replay;
- hash состояния каждые N кадров;
- принудительное внесение расхождения и понятный desync report;
- headless запуск двух симуляций на одинаковом command stream;
- одинаковый финальный hash на Windows/macOS/Linux либо документированная граница платформ.
