# Red Alert 4 — Deterministic RTS AI Architecture & Integration Specification

## 1. Executive Summary & Honest System Status

### Status: Architecture Specification & Engine Core Verification
- **Architecture & System Boundaries:** Fully designed and documented.
- **Simulation Harness & Tests:** Headless C++ test runner `RA4Tests` compiled and verified with strict zero-error flags (`100% tests passed` across `core`, `input`, `presentation`, `ai`).
- **Current Core AI Implementation:** Baseline `AICommander` & `AIStrategy` scoring loop active in `Source/RA4AI`.
- **Next Development Stage:** Implementation of the C++ pure vertical slice (Base Planner, Fog-of-War `EnemyMemory`, Budget Allocator, Operation Lifecycle) and headless cross-platform seed hashing.

---

## 2. Hard Anti-Desync & Simulation Rules

> **The simulation is the only source of truth, and it knows nothing about Unreal.**
> `RA4Core`, `RA4Content`, `RA4Simulation`, `RA4AI` contain zero `UObject` or `AActor` references.

```
       [ Unreal Presentation ]  <--- (Reads Snapshot, Visual Effects, Animations)
                  ^
                  |
     +--------------------------+
     |   RA4Simulation Core     | <=== [Authoritative Fixed-Point State]
     +--------------------------+
                  ^
                  | ApplyCommand()
     +--------------------------+
     |        RA4AI             | <--- (Reads IAIWorldView, Emits Commands)
     +--------------------------+
```

### 2.1 Wall Between Unreal AI Plugins & Authoritative Simulation
1. **Unreal StateTree / EQS / MassAI are strictly isolated to Presentation.**
   - Unreal's EQS or StateTree MUST NOT make authoritative decisions (target pick, unit coordinates, damage, resource cost, production).
   - `RA4Simulation` is the sole source of truth for entity coordinates, movement, damage, and target resolution.
2. **Determinism Standards:**
   - **No floating-point math in simulation.** All coordinates, velocities, ranges use `Fixed` (48.16).
   - **Multi-stream Deterministic PRNG (`DeterministicRngStreams`):**
     - `StrategyStream`, `CombatStream`, `NavigationStream`.
     - Cosmetic PRNG is isolated and never touches simulation streams.
   - **Container Order:** No iteration over `std::unordered_map`. Sorted vectors or insertion-ordered containers only.

---

## 3. Read-Only AI Interface (`IAIWorldView`) & Fog-of-War Memory

To prevent `AICommander` from cheating or mutating `SimWorld` directly, the AI inspects the world strictly via a read-only snapshot interface:

```cpp
namespace RA4::AI
{

struct EnemyMemory
{
    EntityId LastKnownEntity = 0;
    TileCoord LastKnownPosition{0, 0};
    TickIndex LastSeenTick = 0;
    ContentId LastKnownType = 0;
    Fixed Confidence = Fixed::FromInt(1); // Decays over time in fog
};

class IAIWorldView
{
public:
    virtual ~IAIWorldView() = default;
    virtual TickIndex GetCurrentTick() const = 0;
    virtual PlayerId GetPlayerId() const = 0;
    virtual int32_t GetCredits() const = 0;
    virtual int32_t GetPowerProduced() const = 0;
    virtual int32_t GetPowerConsumed() const = 0;
    virtual const std::vector<EntityId>& GetOwnedEntities() const = 0;
    virtual const std::vector<EnemyMemory>& GetKnownEnemies() const = 0;
    virtual bool IsTileExplored(TileCoord Tile) const = 0;
};

} // namespace RA4::AI
```

---

## 4. Modular 4-Tier Subsystem Architecture (`RA4AI`)

```
RA4AI
├── Strategic
│   ├── StrategySelector    (Scores ExpandEconomy, TechUp, Fortify, Assault)
│   ├── EconomyPlanner     (Harvesting ratio, refinery expansion)
│   ├── ProductionPlanner  (Unit composition & prerequisites)
│   └── HTNPlanner         (Multi-step operational trees)
├── Tactical
│   ├── ThreatMap          (Gridded enemy firepower concentration)
│   ├── InfluenceMap       (Territorial dominance assessment)
│   ├── ObjectiveSelector  (Target priority: Power Plants vs Harvesters vs HQ)
│   └── ArmyAllocator       (Forces assigned to active Operations)
├── Squad
│   ├── SquadManager       (Group formation & cohesion)
│   ├── FormationPlanner   (Flow-field formation slots)
│   └── EngagementEvaluator(Retreat vs Push calculation)
└── Knowledge
    ├── AIWorldModel       (Internal representation of match state)
    ├── EnemyMemory        (Fog-of-war tracking & confidence decay)
    └── ScoutingModel      (Reconnaissance unit dispatching)
```

---

## 5. Key Subsystem Specifications

### 5.1 Dynamic Budget Allocator
The AI Commander dynamically partitions available credits into spending pools:
- **Default Ratios:**
  - Economy: 25% | Army: 45% | Tech: 15% | Defense: 10% | Reserve: 5%
- **Dynamic Adjustments:**
  - *Under early rush:* Defense -> 35%, Army -> 50%, Tech -> 0%.
  - *Harvester lost:* Economy -> 45%, Army -> 35%.
  - *Tech advantage secured:* Army -> 60%, Tech -> 5%.

### 5.2 Tactical Operation Lifecycle
Attacks and harassment runs are managed as stateful operations:

```cpp
enum class OperationState : uint8_t
{
    Proposed,
    Gathering,
    Staging,
    Advancing,
    Engaging,
    Exploiting,
    Retreating,
    Completed,
    Aborted
};

struct TacticalOperation
{
    uint32_t OperationId = 0;
    OperationState State = OperationState::Proposed;
    TileCoord TargetLocation{0, 0};
    TileCoord StagingPoint{0, 0};
    int32_t RequiredCombatPower = 0;
    std::vector<EntityId> AssignedUnits;
    TickIndex StartTick = 0;
};
```

### 5.3 Base Planner
Dedicated placement logic prevents self-blocking and secures expansion:
- Validates footprint, power grid connectivity, harvester pathing clearings.
- Keeps vehicle factory exit lanes unobstructed.
- Distance-to-threat scoring for defensive structures.

---

## 6. Verification Pipeline & Headless Match Hashing

### 6.1 Strict Local Verification Command
```bash
set -euo pipefail
cmake -S Tools/HeadlessBuild -B build -DCMAKE_BUILD_TYPE=Development
cmake --build build --parallel
ctest --test-dir build --output-on-failure --no-tests=error
```

### 6.2 Determinism Verification Test (`ra4_headless`)
```bash
# Run 1
./build/RA4Tests --seed 1001 --ticks 100000 --state-hash-out run_a.txt

# Run 2
./build/RA4Tests --seed 1001 --ticks 100000 --state-hash-out run_b.txt

# Compare bit-exact match
cmp run_a.txt run_b.txt
```
