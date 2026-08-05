# ADR-0016: Resource Depletion and Regeneration

## Context

The bible specifies standard ore fields (45,000 credits) and rich fields (75,000 credits). The current implementation destroys resource nodes when depleted (`PendingDestroy.push_back(H.AssignedNode)`), which creates an irreversible economic loss. The economy design requires: starter field slow regeneration (15–20% of original yield), central high-risk resources (30–50% more income), strategic capture points (passive income), and late-game weak income sources. All must use integer arithmetic and simulation ticks.

## Decision

### Resource Node Types

```
enum class ResourceType : uint8_t
{
    StandardOre     = 0,  // 45,000 credits; slow regeneration after depletion
    RichOre         = 1,  // 75,000 credits; +25% harvest speed; slow regeneration
    StrategicPoint  = 2,  // Capturable; provides passive income while held
    OilDerrick      = 3,  // Capturable; provides 8 credits/second passive income
    LateGameNode    = 4,  // Weak but infinite; 2 credits/second effective yield
};
```

### ResourceNodeComp (Expanded)

```
struct ResourceNodeComp
{
    ResourceType    Type                = ResourceType::StandardOre;
    int32_t         Amount              = 0;      // current extractable credits
    int32_t         MaxAmount           = 0;      // initial amount (for UI and regeneration cap)
    int32_t         RegenPerTick        = 0;      // credits restored per tick (0 = no regen)
    int32_t         RegenDelayTicks     = 0;      // ticks after depletion before regen starts
    int32_t         RegenTimer          = 0;      // countdown timer
    bool            bDepleted           = false;  // true when Amount == 0
    int32_t         ReservedHarvesters  = 0;      // fleet coordination (ADR-0015)
    ContentId       Def;
};
```

### Regeneration Mechanics

When a node's `Amount` reaches 0:

1. Node is **not destroyed**. It remains on the map with `bDepleted = true`.
2. `RegenTimer` starts counting down from `RegenDelayTicks`.
3. When `RegenTimer` reaches 0, `Amount` begins increasing by `RegenPerTick` each tick.
4. `Amount` is capped at `MaxAmount * RegenCapPercent / 100` (the regeneration ceiling).

### Regeneration Parameters by Type

| Type | MaxAmount | RegenDelayTicks | RegenPerTick | RegenCapPercent | Notes |
|---|---|---|---|---|---|
| StandardOre | 45,000 | 3,600 (**180s at 20 Hz**, not 60s) | 12 (**unbalanced — see erratum**) | 20% (9,000) | Starter fields regenerate slowly |
| RichOre | 75,000 | 3,600 (**180s at 20 Hz**, not 60s) | 15 (**unbalanced — see erratum**) | 20% (15,000) | Rich fields also regenerate |
| StrategicPoint | N/A | N/A | N/A | N/A | Passive income, not extractable |
| OilDerrick | N/A | N/A | N/A | N/A | Passive income, not extractable |
| LateGameNode | ∞ | N/A | N/A | N/A | Infinite; yields ~2 credits/s |

**ERRATUM (2026-08-05)**: the original text read "All values are at 60Hz tick rate. `RegenPerTick` of
12 = 720 credits/minute = 12 credits/second", which was wrong twice: the simulation runs at 20 Hz
(`kTicksPerSecond`, `SimConfig.h`), and the arithmetic contradicted itself (720/minute is 12/second,
which at 60 Hz would require `RegenPerTick` of 0.2, not 12).

Corrected at the real 20 Hz tick: `RegenPerTick` of 12 = 240 credits/second = 14,400 credits/minute,
which is far too fast for "regenerate slowly" — a starter field's 9,000-credit regen cap would refill
in 38 seconds. The delay figures were also computed at 60 Hz: `RegenDelayTicks` of 3,600 is 180 seconds
at 20 Hz, not 60.

**These numbers require rebalancing against the intended rates, not mechanical conversion.** Until an
economy designer sets them, treat the table as intent-only: slow trickle regeneration after a delay,
capped at ~20% of the node's maximum. Intended-rate targets to convert with
`PerSecondToPerTick()`: starter fields ~0.5 credits/second (`RegenPerTick` = 0 at integer precision —
needs sub-integer accumulation or a longer interval), delay 60 seconds = `RegenDelayTicks` 1,200.
Tracked as NEXT_ACTIONS P-4.

### Depletion Behavior (Revised)

The current code destroys depleted nodes. Revised behavior:

1. When `Amount` reaches 0 during harvest:
   - Set `bDepleted = true`.
   - Do **NOT** add to `PendingDestroy`.
   - Clear the `Tile_Resource` flag (so minimap shows field as empty).
   - Start `RegenTimer`.
2. When `RegenTimer` reaches 0:
   - Begin restoring `Amount` by `RegenPerTick` each tick.
   - Re-set the `Tile_Resource` flag when `Amount > 0`.
3. When `Amount` reaches `MaxAmount * RegenCapPercent / 100`:
   - Stop regenerating. Node stays at ceiling until harvested again.
   - Harvester `FindBestNode()` can select it again.

### Harvester Interaction with Depleted Nodes

When a harvester arrives at a node with `Amount == 0` and `bDepleted == true`:

- If `RegenTimer > 0`: harvester waits (state → Idle, reassign to another node).
- If `RegenTimer == 0` and `Amount > 0`: harvester can extract (amount is regenerating).
- If `RegenTimer == 0` and `Amount == 0`: very early regen; harvester waits.

In practice, harvesters will reassign to other nodes rather than wait, which is the desired behavior.

### Strategic Points

Strategic points are not extractable. They provide passive income to the controlling player:

```
IncomePerTick = StrategicPointIncome  // defined per point in content data
```

Default `StrategicPointIncome`: 5 credits/tick = 300 credits/minute.

Strategic points are captured by engineers (same as oil derricks). They cannot be destroyed, only captured.

### Oil Derricks

Oil derricks provide 8 credits/second = ~480 credits/minute passive income. Captured by engineers. Cannot be destroyed. Already specified in the bible.

### Late-Game Nodes

Late-game nodes are infinite resource points placed far from starting positions. They provide slow but steady income:

- Yield: ~2 credits/second effective (harvest speed limited by distance).
- No depletion. No regeneration needed.
- Purpose: prevent economic stagnation in extended matches.

### Map Resource Distribution

| Zone | Risk | ResourceType | Yield vs Starter |
|---|---|---|---|
| Starter | Low | StandardOre | 1.0× (baseline) |
| Flank | Medium | StandardOre or RichOre | 1.0–1.25× |
| Center | High | RichOre | 1.25–1.5× |
| Far edges | High | LateGameNode | ~0.5× but infinite |
| Strategic | Very high | StrategicPoint | Passive 300/min |

### Serialization

`ResourceNodeComp` fields (`Amount`, `MaxAmount`, `RegenPerTick`, `RegenDelayTicks`, `RegenTimer`, `bDepleted`, `ReservedHarvesters`, `Type`) are serialized as part of entity state.

### State Hash

All `ResourceNodeComp` fields are included in `ComputeStateChecksum`.

### Replay Behavior

Regeneration is deterministic: same tick count → same `Amount`. No special replay handling needed.

### UI Representation

- **Depleted node**: greyed out on minimap; tooltip shows "Depleted — regenerating" with timer.
- **Regenerating node**: pulsing animation; progress bar showing `Amount / RegenCap`.
- **Strategic points**: ownership-colored icon; income rate shown on hover.
- **Oil derricks**: similar to strategic points.

## Rationale

- Regeneration prevents permanent economic death from a single bad engagement.
- 60-second delay + 20% ceiling ensures regenerated income is supplementary, not primary.
- Depleted nodes remaining on map (not destroyed) avoids information loss and enables comeback mechanics.
- Strategic points and late-game nodes create natural late-match objectives.
- All parameters are content-data driven, allowing per-map tuning.

## Status

**ACCEPTED**. Pending implementation. Regeneration parameters will be tuned by headless simulator.
