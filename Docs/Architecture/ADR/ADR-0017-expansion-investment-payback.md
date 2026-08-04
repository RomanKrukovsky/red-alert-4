# ADR-0017: Expansion Investment and Payback

## Context

Expansion (building a second base) is one of the most important strategic decisions in RTS. If expansion pays off too quickly, the optimal strategy is always "expand first." If it pays off too slowly, players turtle on one base. The economy design requires expansion to cost "several combat units' worth" and pay back in 2.5–4 minutes undisturbed. This ADR defines the cost model, payback mechanics, and loss impact.

## Decision

### Expansion Cost Model

A "full expansion" consists of:

| Component | Cost (credits) | Build Time (s) | Purpose |
|---|---|---|---|
| Refinery + Harvester | 2,400 + 1,400 = 3,800 | 45 + 28 = 73 | Core income |
| Power Plant | 800–950 | 18–22 | Energy for refinery |
| Defensive structure | 700–1,100 | 15–25 | Protect investment |
| **Total minimum** | **~5,300–5,850** | | |

This is equivalent to 2–4 combat tanks (depending on faction), which is the intended tradeoff: "army now vs. economy later."

### Payback Formula

Payback time is defined as:

```
PaybackTicks = TotalExpansionCost / (IncomePerTick - MaintenancePerTick)
```

Where:
- `TotalExpansionCost` = credits spent on expansion infrastructure
- `IncomePerTick` = credits earned per tick from new harvesters at new node
- `MaintenancePerTick` = ongoing costs (power consumption converted to credit equivalent)

### Target Payback Time

For a standard expansion (refinery + harvester + power + basic defense):

```
IncomePerTick ≈ 35 credits/second (typical harvester cycle at medium distance)
MaintenancePerTick ≈ 2 credits/second (power plant amortized)
NetIncomePerTick ≈ 33 credits/second
TotalCost ≈ 5,500 credits
PaybackTime ≈ 5,500 / 33 ≈ 167 seconds ≈ 2.8 minutes
```

This falls within the 2.5–4 minute target range. Exact payback varies by:
- Distance to resource node (longer route = slower income)
- Harvester losses during expansion
- Power availability at new base
- Defensive investment level

### Expansion Zones (Content-Driven)

Each map defines expansion zones with properties:

```
struct ExpansionZone
{
    TileCoord    Center;
    int32_t      Radius;            // buildable area radius
    ResourceType PrimaryResource;   // what resource node is nearby
    int32_t      ResourceAmount;    // starting amount
    int32_t      RouteRisk;         // 0–100, how exposed the route is
    float        IncomeMultiplier;  // 1.0 = standard, 1.3 = rich, etc.
};
```

### Loss Impact

When an expansion is lost (all buildings destroyed):

1. **Immediate loss**: all invested credits are gone.
2. **Harvester loss**: if harvester is destroyed, 1,400 credits lost. If harvester survives, it can be reassigned.
3. **Recovery time**: building a new expansion takes 73+ seconds. During this time, the player is behind by the full investment.
4. **No insurance**: the economy design explicitly avoids compensating players for lost expansions. The loss is the consequence of poor defense.

### Expansion vs. Army Tradeoff

The key decision at 4–7 minutes into a match:

| Option A: Army | Option B: Expansion |
|---|---|
| Spend 5,500 on 3–4 tanks | Spend 5,500 on expansion |
| Immediate military advantage | No immediate advantage |
| No ongoing income increase | +33 credits/second ongoing |
| If attack fails, investment is lost | If expansion survives, pays back in ~3 min |
| Risk: opponent expands and outproduces | Risk: opponent attacks and destroys expansion |

This is the healthy RTS decision the economy design targets.

### Serialization

Expansion zone definitions are part of map data (content), not simulation state. No serialization needed beyond map data.

### State Hash

Expansion zone definitions are part of `ContentDatabase`, which is already hashed. No additional state needed.

### Replay Behavior

Expansion decisions are player commands (build orders). No special replay handling needed.

### UI Representation

- **Expansion indicator**: minimap shows expansion zones with resource amounts and risk levels.
- **Payback calculator**: when placing a refinery, tooltip shows estimated payback time based on distance to resource and current harvester speed.
- **Loss warning**: when expansion is under attack, highlight the investment value at risk.

### AI Evaluation

AI evaluates expansion decisions using:
- `PaybackTime` estimate based on current game state.
- `RouteRisk` of the expansion zone.
- Current army strength vs. threat assessment.
- `IncomePerTick` from expansion vs. cost of army units.
- Decision threshold: expand if `PaybackTime < 240s` AND `RouteRisk < 60` AND `ArmyStrength > defensive_minimum`.

## Rationale

- Cost equivalent to 2–4 combat units creates a meaningful army-vs-economy tradeoff.
- 2.5–4 minute payback is long enough to be risky but short enough to be worthwhile.
- No insurance for lost expansions惩罚 poor defense decisions.
- Content-driven expansion zones allow per-map tuning.
- Payback calculator in UI helps new players understand the tradeoff.

## Status

**ACCEPTED**. Pending implementation. Payback time will be validated by headless simulator across different map sizes and risk levels.
