# ADR-0013: Energy Production, Consumption, Priority, and Deficit Effects

## Context

Energy is not a storable resource but a continuous balance: `NetPower = PowerProduced - PowerConsumed`. The current implementation (`SimWorld::SystemPower`) computes these totals and `PlayerState::GetPowerRatioPercent` returns a simple percentage, but the actual deficit effects are scattered across `SystemConstruction` and `SystemProduction` as a single `kMinPowerRatioPercent` throttle. The economy design requires a multi-tier degradation cascade with explicit priority controls, gradual degradation curves, and deterministic recovery.

## Decision

### Core Formula

```
PowerRatioPercent = (PowerProduced * 100) / max(PowerConsumed, 1)
```

All arithmetic is integer. `PowerProduced` is the sum of all completed buildings' `PowerProduced` field, scaled by building health ratio. `PowerConsumed` is the sum of all completed buildings' `PowerConsumed` fields.

### Deficit Tiers

| Tier | PowerRatioPercent | Effect |
|---|---|---|
| Normal | ≥ 100% | No penalties. All systems operate at full speed. |
| Mild | 70–99% | Construction and production speed scaled linearly: `SpeedMultiplier = PowerRatioPercent / 100`. A building at 85% power produces at 85% speed. |
| Moderate | 40–69% | Radar/minimap disabled. Repair speed halved. Construction/production at `PowerRatioPercent / 100` speed. |
| Severe | 10–39% | Radar disabled. Repair disabled. High-tech production paused. Static defense fire rate halved (cooldowns doubled). Construction/production at `PowerRatioPercent / 100` speed, minimum 10%. |
| Critical | < 10% | Only barracks and harvesters operate (at 50% speed). All other production paused. Superweapons paused. Static defense offline. |

### Affected Systems Per Tier

| System | Normal | Mild (70–99%) | Moderate (40–69%) | Severe (10–39%) | Critical (< 10%) |
|---|---|---|---|---|---|
| Building construction speed | 100% | `Ratio/100` | `Ratio/100` | `max(Ratio/100, 10%)` | 50% (barracks only) |
| Unit production speed | 100% | `Ratio/100` | `Ratio/100` | `max(Ratio/100, 10%)` | 50% (barracks only) |
| Radar / minimap | On | On | **Off** | **Off** | **Off** |
| Repair speed | 100% | 100% | **50%** | **Off** | **Off** |
| High-tech production | 100% | 100% | 100% | **Paused** | **Paused** |
| Static defense fire rate | 100% | 100% | 100% | **50%** (2× cooldown) | **Off** |
| Superweapons | 100% | 100% | 100% | 100% | **Paused** |
| Harvesters | 100% | 100% | 100% | 100% | **50%** |

"High-tech production" = any item whose `ContentId` resolves to a unit or building with `TechLevel >= T2` (requires Radar or equivalent).

### Priority System

Buildings are assigned one of four power priority levels (configurable per-building via player command):

| Priority | Default Assignment | Behavior at Deficit |
|---|---|---|
| 0 — Critical | HQ, Barracks, Harvester refinery | Last to degrade; never fully offline |
| 1 — Production | Factory, Airfield, Dock | Degrade at tier thresholds |
| 2 — Defense | Turrets, Shields, Walls | Degrade at tier thresholds; offline at Critical |
| 3 — Auxiliary | Radar, Repair, Tech Center, Superweapon | First to degrade; offline at Moderate+ |

Player can override any building's priority via command. Override is persisted in save/replay.

### Power Production Health Scaling

A damaged power plant produces proportionally less:

```
EffectivePower = BuildingPowerProduced * (CurrentHP * 100 / MaxHP) / 100
```

This is already implemented in `SystemPower`. No change needed.

### Recovery Behavior

When power ratio recovers from one tier to a better tier:

- **Radar**: Reactivates immediately on tick boundary.
- **Repair**: Resumes at the tier's speed on next repair tick.
- **High-tech production**: Items in `EnergyThrottled` state transition back to `Paying` (see ADR-0012).
- **Static defense**: Cooldowns resume at normal rate on next fire attempt.
- **Superweapons**: Resume charging from where they stopped.

No "warm-up" delay. Recovery is instantaneous on the tick the power ratio crosses the threshold. This is deterministic and avoids tracking "time since recovery."

### Serialization

Player priority overrides are serialized as part of `PlayerState`. The power tier itself is recomputed each tick from `PowerProduced`/`PowerConsumed` (derived, not stored).

### State Hash

`PowerProduced`, `PowerConsumed`, and per-building priority overrides are included in `ComputeStateChecksum`. The computed tier is excluded (derived).

### UI Representation

- **HUD power bar**: shows `PowerProduced` / `PowerConsumed` with color coding (green ≥100%, yellow 70–99%, orange 40–69%, red <40%).
- **Building power priority**: icon overlay on each building showing its priority level; click to cycle.
- **Deficit warnings**: EVA audio line + on-screen notification when crossing from Normal to Mild, and from Mild to Moderate. Additional warning at Severe.
- **Per-system status**: small icons on sidebar showing which systems are degraded (radar off, defense slow, etc.).

### Replay Behavior

Power ratio is recomputed from building states each tick. Priority overrides are part of the command stream. No special replay handling needed.

### AI Evaluation

AI queries:
- `PowerRatioPercent` for overall health.
- `PowerProduced` and `PowerConsumed` individually for planning.
- Per-building priority to deprioritize non-essential buildings during deficit.
- Predicts future power needs based on planned construction queue.

## Rationale

- Multi-tier degradation creates meaningful choices: players must prioritize which systems to keep online.
- Gradual speed scaling (not binary on/off) for construction/production avoids jarring gameplay interruptions.
- Clear threshold boundaries (10%, 40%, 70%) are easy to communicate via UI and easy to verify in tests.
- Priority system gives players agency during deficit rather than opaque automatic shutdown.
- Instantaneous recovery avoids tracking "recovery timers" which would complicate determinism.

## Status

**IMPLEMENTED** for every system that exists. Packages A through D are done and tested.

### Implemented (packages A through D)

Tier bands and the construction/production/harvester rows of the effect matrix;
`PowerTier` + `PowerTierForRatio` + `PowerSpeedPercentForTier` in `SimTypes.h`;
`SimWorld::PowerSpeedPercent` as the single ratio→speed authority;
`ProductionInfo::Tier` with the default content graded T0–T2; high-tech (T2+) pausing
via `FlowPaymentState::EnergyThrottled`; edge-triggered `PowerShortageStarted/Ended`
with `PlayerState::LastPowerTier` serialized and hashed; the four-band priority table
with `BuildingComp::Priority`, defaults derived from content, and the
`SetPowerPriority` player command (serialized, hashed, replayed).

Package D completes the effect matrix: radar goes dark from Moderate (which stops the
anonymous contacts the recon layer derives from its coverage, so the minimap really does
go quiet); repair exists at all -- `RepairBuilding` previously validated and then did
nothing -- and runs at full speed to Mild, half through Moderate and not at all from
Severe; static defence doubles its cooldown at Severe and stops firing at Critical.
Every one of those also respects the building's own priority band, which is what makes
the player's override meaningful.

Both mechanics now reach the player: `SelectionState` carries the priority, whether the
band is currently offline, and the repair state, and those cross into Blueprints through
`URA4UIDataProviderSubsystem`. `ARA4PlayerController::CycleSelectedPowerPriority` and
`ToggleSelectedRepair` issue ordinary validated commands, so both are replayed and
server-authoritative like any other decision.

### Pending

- **Superweapons** do not exist as a system, so the superweapon row is unreachable. It
  is the only row of the effect matrix with nothing behind it.
- **No radar in shipped content.** No definition sets `bIsRadar`, so the `Auxiliary`
  band has no default occupant and the radar tests author one. The code path works and is
  tested; a faction has to declare a radar for it to matter in play. This is a content
  task, not a code one.
- **No widget bound yet.** The controls are exposed to Blueprints and the commands are
  wired, but no `.uasset` calls them — that is editor work, and it cannot be done or
  verified headlessly.
- **AI does not use priority.** The commander never demotes a building during a deficit.
  Advisory only, so nothing is broken by its absence.
- **Priority is free and uncapped.** There is no cost, cooldown or per-building rate
  limit, so a player can set every building to `Vital` at match start and opt out of the
  shutdown dimension of the deficit entirely (speed scaling still applies, since that is
  player-wide and priority-independent). This is spec-faithful — the ADR places no
  constraint on the override — but the spec is exploitable, and balancing it wants play
  data rather than a number invented here.
- **Repair rates are placeholders.** `kRepairHealthPerTick` (4) and
  `kRepairCostPerHealthCenti` (25, a quarter-credit per hitpoint) are chosen to be
  slower and dearer per point than the original construction, so repair saves a building
  rather than replacing defence. Both want tuning against real matches.

### Amendments made during implementation

**1. Construction continues at every tier, including Critical.**

The effect matrix says building construction at Critical is "50% (barracks only)".
Implemented literally that is incoherent: construction progress belongs to the
building being built, not to a producer, so "barracks only" has no meaning on that
row. Worse, freezing construction at Critical strands a half-built power plant and
makes a blackout permanent. All in-progress construction therefore advances at the
Critical rate. The row should read "50%".

**2. The construction yard keeps producing at Critical.**

The production row's "barracks only" is implemented as "infantry producers *and the
construction yard*". Without the yard a blacked-out base cannot queue the power plant
that ends the blackout — a deadlock rather than a difficulty. Membership is derived
from content (which buildings appear in an Infantry-category item's `ProducedBy`)
rather than from a name list, so renaming or adding an infantry building needs no code
change.

**3. Payment and production share one stall predicate.**

Not a spec change but a correctness requirement discovered in review.
`SimWorld::IsProductionPowerStalled` is the single verdict on whether the power state
has stopped a queue head, and both `SystemFlowPayment` and `SystemProduction` consult
it. When the two decided independently, payment charged a slice every tick for an item
production refused to advance: the item froze at zero progress while draining the
treasury that should have finished the power plant, and the base stayed dark
permanently. Anything the power state stops must also stop being charged for.

**4. Throttle recovery is the inverse of the trigger by construction.**

An earlier attempt used a separate 50% recovery constant while the throttle fired
below 40%, which trapped any item stalled between 40% and 49% — both throttled and
refused recovery. There is no separate recovery threshold: reaching the
`EnergyThrottled` case in the funding pass already means the throttle test said no.

**5. Harvester scaling is approximate for small rates.**

The Critical harvest rate is `max(1, HarvestPerTick / 2)`. The clamp prevents a rate
of zero, which would be a deadlock rather than a penalty, so content with
`HarvestPerTick` of 1 runs at full speed at Critical and a rate of 3 runs at 33%
rather than 50%. Acceptable at current content values; worth revisiting if a faction
ships a very low per-tick rate.

**6. Deficit warnings are suppressed for defeated players.**

Defeat does not clear `bActive`, and a defeated player's last building dying takes
both power figures to zero — which `GetPowerRatioPercent` reports as a healthy 100%.
The tier would jump to Normal and announce that power had been restored to a player
who had just lost their base. The edge trigger therefore gates on `bDefeated` as well.

**7. The barracks is Vital, not Production.**

The priority table assigns "HQ, Barracks, Harvester refinery" to Vital and "Factory,
Airfield, Dock" to Production, which is what the implementation does — but the table
also has Production going offline at Critical, while the effect matrix says an
infantry producer keeps working there. Read literally the two rules contradict each
other, and a barracks left at Production priority was forced offline at exactly the
tier the carve-out exists to protect, making a blackout inescapable again.

Resolved by deriving the Vital band partly from the carve-out itself: anything
`ProducerRunsAtCriticalPower` keeps alive is Vital. The two rules now ask the same
function, so they cannot drift apart.

**8. Vital defaults are keyed on specific roles, not `BaseBuilding`.**

`EntityRole::BaseBuilding` is set on *every* building in the default content, turrets
and factories included, so using it as a Vital signal made almost the whole base Vital
and the priority table meaningless. Only `EntityRole::Refinery`, `EntityRole::Power`
and the `bIsConstructionYard` / `bIsRefinery` / `bIsPowerPlant` flags discriminate.

**9. Save versions 1–4 are refused, not migrated.**

The two branches independently stamped v4 on different byte layouts: one wrote
`bSelling` (one byte per building) and morale (28 bytes per entity), the other wrote
neither. Nothing in the stream distinguishes them, so a single reader cannot serve both
— gating morale on `Version >= 4` misaligns every entity of one, and skipping
`bSelling` misaligns every building of the other. Accepting either is a coin flip that
yields a silently corrupt world rather than an error. The same collision makes v2 and v3
ambiguous. Since no save ships yet, the minimum supported version is 5, and
`SaveVersion.AmbiguousLegacyVersionsAreRefusedRatherThanMisread` pins that.

**11. Radar shutdown is routed through the priority band, not the tier directly.**

The effect matrix says radar is off from Moderate, and a radar defaults to `Auxiliary`,
whose band stops at exactly Moderate — so the default behaviour matches the matrix. The
check is written against the band rather than the tier so that promoting a radar to
`Vital` actually keeps it lit, which is the point of letting the player choose. An earlier
version ANDed both tests, so promotion bought nothing and the override was decoration for
the one building it matters most for.

A radar's chain-of-command role is deliberately *not* switched off with its coverage:
relaying reports is a separate function with its own blackout rule in the recon layer.
Note that rule uses a 50% threshold, which sits inside the Moderate band — so between 50%
and 69% a radar is dark for coverage but a healthy relay. Recorded rather than unified,
because the two functions are genuinely independent.

**12. Repair skips a building already queued for destruction.**

`SystemDeaths` runs at the end of the tick, so a building sold earlier in the same tick is
still alive when `SystemRepair` sees it. Without the skip, selling a damaged building with
repair armed charged for hitpoints nobody ever saw, on top of the sale refund. Same check
`SystemFlowPayment` makes, for the same reason.

**13. Production is funded before repair, so production wins a contested tick.**

`SystemFlowPayment` runs before `SystemRepair`, and neither knows about the other; each
clamps to the balance it sees, so the treasury never overdraws (pinned by
`Repair.CompetingWithProductionNeverOverdrawsTheTreasury`). But the starvation is
one-directional: a queue that wants the whole treasury leaves repair nothing, every tick,
while repair never starves production. Defensible — a production order is the more
explicit commitment — but it is a consequence of list order rather than a decision, so it
is recorded here. Routing repair through the same priority-sorted funding pass would make
it a choice; that wants play data first.

**10. A pre-v7 save derives priority from content rather than defaulting.**

A v6 save has no priority byte. Leaving the `BuildingComp` default would hand every
building `Production`, which goes offline at Critical — putting the construction yard
and barracks in a band that stops them exactly where amendments 2 and 7 say they must
keep working, and recreating the inescapable blackout. The migration therefore calls
`DefaultPowerPriorityFor` on the entity's definition, which is available at that point
in the load.
