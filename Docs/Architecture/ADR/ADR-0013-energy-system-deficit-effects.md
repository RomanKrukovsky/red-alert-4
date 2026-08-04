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

**ACCEPTED**. Pending implementation. Will be validated by headless economy simulator.
