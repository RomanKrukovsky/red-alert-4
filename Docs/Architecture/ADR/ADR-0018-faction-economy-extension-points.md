# ADR-0018: Faction Economy Extension Points

## Context

The bible defines four factions with distinct economic identities: Soviet (mass industry), Alliance (technological efficiency), Eastern Coalition (territorial network), and Chronolegion (temporal economy). ADR-007 in `Docs/ADRs/` used pre-rename faction names and incomplete mechanics. This ADR supersedes ADR-007 with the complete faction economy design, using current faction names and exact formulas. All faction economies must extend a shared base model without requiring separate balancing.

## Decision

### Shared Base Model

All factions share:
- Credits (primary currency, harvested from resource nodes)
- Energy (production/consumption balance, ADR-0013)
- Command Limit (soft cap, ADR-0014)
- Harvester state machine (ADR-0015)
- Resource depletion/regeneration (ADR-0016)
- Flow-payment production (ADR-0012)

Faction differences are expressed through **content data overrides** and **per-faction resource mechanics**, not separate code paths.

### Extension Points

Faction economies are configured via data, not code:

```
struct FactionEconomyConfig
{
    // Harvester overrides
    int32_t     HarvesterCargoCapacity;     // default: 1200
    Fixed       HarvesterSpeed;             // default: 2.5
    Fixed       HarvestPerTick;             // default: defined per content
    Fixed       UnloadPerTick;              // default: defined per content
    int32_t     HarvesterCost;              // default: 1400
    int32_t     HarvesterCommandLimit;      // default: 4

    // Refinery overrides
    int32_t     RefineryCost;               // default: 2400
    int32_t     RefineryBuildTime;          // default: 45s
    int32_t     RefineryPowerConsumed;      // default: 20
    int32_t     MaxDockedHarvesters;        // default: 1

    // Power plant overrides
    int32_t     PowerPlantCost;             // default: 800
    int32_t     PowerPlantPowerProduced;    // default: 120
    int32_t     PowerPlantBuildTime;        // default: 18s

    // Income modifiers (applied as multipliers to base income)
    int32_t     HarvestIncomeMultiplier;    // basis points (10000 = 1.0×)
    int32_t     PassiveIncomeMultiplier;    // basis points

    // Special mechanics (faction-specific flags)
    bool        bCanRecycleWreckage;        // Soviet: harvest destroyed vehicles
    bool        bHasAutomatedMiners;        // Alliance: reduced harvester count needed
    bool        bHasDistributedNetwork;     // Eastern Coalition: bonus from connected bases
    bool        bHasTemporalDebt;           // Chronolegion: borrow from future
};
```

### Faction: Soviet Union (RSU) — Industrial Mass

**Identity**: Large harvesters, big cargo, slow movement, cheap refineries, mass production bonus, wreckage recycling.

| Parameter | Value | vs. Base |
|---|---|---|
| HarvesterCargoCapacity | 1,500 | +25% |
| HarvesterSpeed | 2.0 | -20% |
| HarvesterCost | 1,400 | same |
| RefineryCost | 2,000 | -17% |
| HarvestIncomeMultiplier | 10,000 | 1.0× |
| bCanRecycleWreckage | true | unique |

**Wreckage Recycling**: Soviet harvesters can harvest destroyed vehicles (any faction's wrecks). Wreckage yields 30% of the original unit's credit value. Wreckage is a resource node with `ResourceType::Wreckage` and limited amount.

**Strength**: Massive income when infrastructure is protected. Weakness: large, slow harvesters are vulnerable on long routes.

### Faction: Alliance (ALC) — Technological Efficiency

**Identity**: Expensive but fast automated miners, fewer harvesters needed, compact refineries, remote mining boost, expensive early economy, high per-unit efficiency.

| Parameter | Value | vs. Base |
|---|---|---|
| HarvesterCargoCapacity | 800 | -33% |
| HarvesterSpeed | 3.5 | +40% |
| HarvesterCost | 1,800 | +29% |
| RefineryCost | 2,500 | +4% |
| HarvestIncomeMultiplier | 12,000 | +20% |
| MaxDockedHarvesters | 2 | +1 |
| bHasAutomatedMiners | true | unique |

**Automated Mining**: Alliance refineries can process 2 harvesters simultaneously. Combined with faster harvesters, this means fewer harvesters are needed for equivalent income, reducing vulnerability.

**Strength**: Fewer vulnerable units on the map. Weakness: losing one harvester is proportionally more painful.

### Faction: Eastern Coalition (ECO) — Territorial Network

**Identity**: Distributed resource nodes, infrastructure connection bonuses, cheap remote outposts, income depends on territory control, trade routes between nodes.

| Parameter | Value | vs. Base |
|---|---|---|
| HarvesterCargoCapacity | 1,200 | same |
| HarvesterSpeed | 2.8 | +12% |
| RefineryCost | 2,200 | -8% |
| HarvestIncomeMultiplier | 10,000 | 1.0× |
| bHasDistributedNetwork | true | unique |

**Network Bonus**: Eastern Coalition gains income bonus from connected infrastructure:
- 2 connected bases: +5% income
- 3 connected bases: +12% income
- 4+ connected bases: +20% income

"Connected" means a continuous chain of power plants or communication buildings linking the bases. If the chain is broken (building destroyed), the bonus is lost immediately.

**Trade Routes**: Between connected bases, a passive "trade income" of 3 credits/second per connection.

**Strength**: Fast spread across the map with compounding income. Weakness: network can be severed by targeted strikes.

### Faction: Chronolegion (CHL) — Temporal Economy

**Identity**: Time reserve accumulation, production acceleration, future income borrowing, temporary resource restoration, partial refund for recent losses, risk of "temporal debt."

| Parameter | Value | vs. Base |
|---|---|---|
| HarvesterCargoCapacity | 1,000 | -17% |
| HarvesterSpeed | 3.0 | +20% |
| RefineryCost | 2,600 | +8% |
| HarvestIncomeMultiplier | 10,000 | 1.0× |
| bHasTemporalDebt | true | unique |

**Temporal Debt** (detailed in ADR-0020): Chronolegion can borrow future income for present use, with a repayment penalty. This is their unique economic mechanic.

**Strength**: Powerful economic bursts at critical moments. Weakness: wrong timing leaves the faction without income.

### All Factions Share

- Same base resource node types and amounts
- Same harvester state machine (ADR-0015)
- Same energy system (ADR-0013)
- Same command limit curve (ADR-0014)
- Same flow-payment production (ADR-0012)
- Same expansion payback model (ADR-0017)

Faction differences are purely data-driven: different numbers in `FactionEconomyConfig`, plus the faction-specific flags that enable unique mechanics.

### Serialization

`FactionEconomyConfig` is derived from `FactionId` + `ContentDatabase`. It is not serialized separately. On deserialization, the config is recomputed from faction identity.

### State Hash

Faction-specific state (Wreckage nodes, Network connections, Temporal Debt) is included in `ComputeStateChecksum`. The config itself is excluded (derived from content).

### Replay Behavior

All faction mechanics are deterministic. Temporal Debt state transitions are driven by player commands (activate/deactivate). Network bonuses are computed from building states. No special replay handling needed.

### UI Representation

- **Faction resource gauge**: unique UI element per faction showing the faction-specific resource (Mobilization, Intelligence, Synchronization, Temporal Stability).
- **Network map** (Eastern Coalition): overlay showing connected bases and bonus level.
- **Temporal debt indicator** (Chronolegion): shows current debt, repayment timer, and income penalty.

### AI Evaluation

AI queries `FactionEconomyConfig` to:
- Plan harvester count based on cargo capacity and speed.
- Evaluate expansion timing based on refinery cost and payback time.
- Use faction-specific mechanics (e.g., Soviet wreckage harvesting, Alliance dual-dock).

## Rationale

- Data-driven faction differences avoid separate code paths and simplify balancing.
- All factions share the same economic systems, reducing testing surface.
- Faction-specific flags enable unique mechanics without architectural changes.
- Supersedes ADR-007 with current faction names and complete mechanics.

## Status

**ACCEPTED**. Supersedes `Docs/ADRs/ADR-007-Faction-Economy.md`. Pending implementation.
