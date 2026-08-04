# Technical Design Document (TDD) (`TECHNICAL_DESIGN_DOCUMENT.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  
**Engine Architecture**: Deterministic 60Hz C++ Simulation Core + UE5 Presentation Layer  

---

## 1. Executive Technical Architecture Summary

The technical architecture of *Iron Resonance* enforces a strict decoupling between the **Deterministic Simulation Core** (`Source/RA4Simulation`, `Source/RA4Core`) and the **Unreal Engine 5 Presentation Layer** (`Source/RA4Presentation`, `Source/RedAlert4`).

```
+-----------------------------------------------------------------------------------+
|                            UNREAL ENGINE 5 FRONTEND                               |
|   RA4PresentationSubsystem | APlayerController | UMG Widgets | Niagara | Audio  |
+-----------------------------------------------------------------------------------+
                                         |
                            Reads State Snapshots & Dispatches Commands
                                         v
+-----------------------------------------------------------------------------------+
|                        DETERMINISTIC SIMULATION KERNEL                            |
|   SimWorld (60Hz Fixed Tick) | CommandBus | LockstepSession | FixedPoint Math     |
|   RA4Combat | RA4Navigation (Flowfield) | RA4AI (HTN) | RA4FogOfWar                 |
+-----------------------------------------------------------------------------------+
```

---

## 2. Simulation Loop & Entity Component Layout

### Fixed 60Hz Simulation Loop
- **Tick Interval**: Fixed at 16.666 milliseconds (60 ticks/sec).
- **Time Representation**: All time calculations use 60Hz tick indices (`TickIndex`).
- **Math Engine**: Pure integer fixed-point arithmetic (`FixedPoint.h`) for all positions, velocities, hitboxes, and weapon ranges. Floating-point math is strictly forbidden in simulation code.

### Data-Oriented Component Model (`SimWorld`)
Entities are stored in contiguous memory arrays (Struct-of-Arrays layout) to maximize CPU L1/L2 cache hits during tick evaluation:
```cpp
struct SimWorld {
    std::vector<TransformComp> Transforms;
    std::vector<HealthComp>    Healths;
    std::vector<CombatComp>    Combats;
    std::vector<MovementComp>  Movements;
    std::vector<ProducerComp>  Producers;
    std::vector<HarvesterComp> Harvesters;
};
```

---

## 3. CommandBus, Lockstep & State Hashing

### CommandBus Dispatching
- Commands (`Move`, `Attack`, `PlaceBuilding`, `UseAbility`) are wrapped in `CommandFrame` structs.
- `CommandBus::DispatchTick` executes buffered frames in strict player ID order, guaranteeing identical state progression across all peers.

### State Hash Engine (`SimWorld::CalculateStateHash`)
- Generates a 64-bit FNV-1a checksum of all state variables at every 10th simulation tick.
- Checksums are exchanged in lockstep network frames. Any mismatch immediately flags a desync and triggers diagnostic log dumps.

---

## 4. Save System, Replays & Data Migration

### Binary Replay Format (`.ra4replay`)
- Header: Initial random seed, player IDs, map identifier, game version.
- Frame Stream: Per-tick `CommandFrame` array.
- Snapshot Checkpoints: Full `SimWorld` snapshots stored every 30 seconds (1,800 ticks) for instant replay seeking.

### Save Game Migration
- Save files store a schema version header (`uint32_t Version`).
- Migration functions (`MigrateSaveV1ToV2`) run sequentially upon loading older save formats to ensure backward compatibility.

---

## 5. Unreal Engine Presentation Layer

- **`URA4PresentationSubsystem`**: Polled once per frame during `Tick()` to reconcile visual UE `AActor` positions with simulation `SimWorld` transforms via linear interpolation.
- **Visual Selection & Decals**: `URA4SelectionDecalComponent` manages team-color selection rings.
- **Input & Camera**: `RA4UIInputRouter` traps UI clicks to prevent accidental ground orders; `RA4CameraComponent` handles WASD keyboard panning and edge scrolling.
- **VFX & Audio**: Triggered asynchronously via `SimEventType` notifications emitted by `SimWorld` (e.g. `BuildingPlaced`, `UnitPromoted`, `WeaponFired`).
