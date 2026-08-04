# ADR-0014: Soft Command-Limit Penalty Curve

## Context

The command limit (CL) gates army size. The bible specifies a hard cap of 200, with buildings providing +5 to +20 each. The economy design calls for a **soft cap**: below the limit, no penalty; above it, increasing penalties that discourage unlimited spam without making one additional unit unexpectedly cripple the entire army. This avoids the hard-cap frustration of "you cannot build one more soldier" while still creating meaningful army-size decisions.

### Current Implementation

`SimWorld::SystemPower` computes `CommandLimitMax` (sum of building contributions, capped at 200) and `CommandLimitUsed` (sum of unit costs). `SystemProduction` rejects new units when `CommandLimitUsed + NewUnitCost > CommandLimitMax`. This is a hard cap.

### Design Constraints

1. Below the cap: zero penalty.
2. Just above the cap: mild, barely noticeable penalty.
3. Far above the cap: severe penalty that makes further expansion very costly.
4. The penalty must not make one additional unit unexpectedly cripple the army.
5. Must be deterministic and use integer arithmetic.
6. Must be reversible (penalty disappears when units are lost).
7. Must apply to production speed, not movement or combat stats.

## Decision

### Candidate Curves

Three candidate curves were evaluated. All use the overflow ratio `r = (CommandLimitUsed - CommandLimitMax) / CommandLimitMax` clamped to [0, 1].

**Curve A — Linear:**
```
PenaltyPercent = min(r * 50, 50)
```
At 0% overflow: 0% penalty. At 100% overflow (double the cap): 50% penalty.

**Curve B — Quadratic:**
```
PenaltyPercent = min(r * r * 100, 60)
```
At 0% overflow: 0%. At 25% overflow: 6.25%. At 50% overflow: 25%. At 100% overflow: 60%.

**Curve C — Piecewise (recommended):**
```
if r <= 0.0:     penalty = 0
elif r <= 0.10:  penalty = r * 100          // 0–10% overflow: 0–10% penalty (linear)
elif r <= 0.30:  penalty = 10 + (r-0.10)*150  // 10–30% overflow: 10–40% penalty
elif r <= 0.50:  penalty = 40 + (r-0.30)*100  // 30–50% overflow: 40–60% penalty
else:            penalty = 60 + (r-0.50)*80    // 50–100% overflow: 60–100% penalty
```

### Candidate Comparison

| Scenario | CL Used | CL Max | Overflow | Curve A | Curve B | Curve C |
|---|---|---|---|---|---|---|
| At cap | 200 | 200 | 0% | 0% | 0% | 0% |
| +10 units (5% over) | 210 | 200 | 5% | 2.5% | 0.25% | 5% |
| +20 units (10% over) | 220 | 200 | 10% | 5% | 1% | 10% |
| +40 units (20% over) | 240 | 200 | 20% | 10% | 4% | 25% |
| +60 units (30% over) | 260 | 200 | 30% | 15% | 9% | 40% |
| +100 units (50% over) | 300 | 200 | 50% | 25% | 25% | 60% |
| +200 units (100% over) | 400 | 200 | 100% | 50% | 60% | 100% |

### Evaluation Criteria

| Criterion | Curve A | Curve B | Curve C |
|---|---|---|---|
| Near-cap sensitivity (0–10% over) | Too gentle (2.5–5%) | Too gentle (0.25–1%) | Good (5–10%) |
| Mid-range pressure (20–30% over) | Moderate (10–15%) | Too gentle (4–9%) | Strong (25–40%) |
| Spam deterrence (50%+ over) | Moderate (25%+) | Strong (25%+) | Very strong (60%+) |
| Linearity near cap | Smooth | Too smooth (invisible) | Piecewise but predictable |
| One-unit突变 | None | None | None (continuous) |

### Recommendation: Curve C (Piecewise)

Curve C is recommended because:
1. **Near-cap sensitivity**: 5–10% penalty at 5–10% overflow is noticeable but not punishing. Players feel the cost of going over.
2. **Mid-range pressure**: At 20–30% over cap, the 25–40% penalty makes further expansion clearly costly. This is the "decision zone" where players must choose between army size and economy.
3. **Spam deterrence**: At 50%+ overflow, the 60–100% penalty makes unlimited spam impractical without being a hard wall.
4. **No突变**: The piecewise function is continuous at boundary points (verified: at r=0.10, both segments give 10%; at r=0.30, both give 40%; at r=0.50, both give 60%).

### Integer Implementation

To avoid floating-point, all calculations use fixed-point with 1/100 precision:

```
// r = overflow ratio * 100 (so r is 0..100 for 0%..100%)
int32_t CommandLimitPenalty(int32_t OverflowRatio100)
{
    if (OverflowRatio100 <= 0) return 0;
    if (OverflowRatio100 <= 10) return OverflowRatio100;              // 0–10%
    if (OverflowRatio100 <= 30) return 10 + (OverflowRatio100 - 10) * 3 / 2; // 10–40%
    if (OverflowRatio100 <= 50) return 40 + (OverflowRatio100 - 30) * 1;      // 40–60%
    return 60 + (OverflowRatio100 - 50) * 8 / 10;                              // 60–100%
}
```

### What the Penalty Affects

The penalty applies **only to production speed** (both building construction and unit production):

```
EffectiveSpeedMultiplier = max(100 - CommandLimitPenalty(Overflow), 10) / 100
```

Minimum 10% speed (never zero). This means even at extreme overflow, production continues but very slowly.

The penalty does **NOT** affect:
- Movement speed
- Combat stats (damage, health, range)
- Harvester efficiency
- Energy production
- Repair speed

### Overflow Calculation

```
Overflow = max(0, CommandLimitUsed - CommandLimitMax)
OverflowRatio100 = (Overflow * 100) / max(CommandLimitMax, 1)
OverflowRatio100 = min(OverflowRatio100, 100)  // clamp to prevent >100%
```

### No Hard Cap

There is no hard cap. A player can exceed the command limit indefinitely, but the penalty makes it impractical beyond ~50% overflow. This is a design choice: the penalty is the wall, not an arbitrary number.

### Serialization

`CommandLimitUsed`, `CommandLimitMax` are already serialized as part of `PlayerState`. The penalty is recomputed each tick (derived).

### State Hash

`CommandLimitUsed` and `CommandLimitMax` are included in `ComputeStateChecksum`. The penalty itself is excluded (derived).

### UI Representation

- **Command bar**: shows `CommandLimitUsed / CommandLimitMax` with color coding (green when under, yellow at 0–20% over, orange at 20–50% over, red at 50%+ over).
- **Penalty indicator**: small icon showing current production speed penalty percentage.
- **Tooltips**: "Production speed: -X%" on hover over the command bar.

### Replay Behavior

Command limit is recomputed from unit states each tick. No special replay handling needed.

### AI Evaluation

AI queries:
- `CommandLimitUsed` and `CommandLimitMax` for current state.
- `CommandLimitPenalty(Overflow)` for production speed impact.
- Predicts future CL usage based on production queue.
- Avoids exceeding cap unless strategic advantage justifies the penalty.

## Rationale

- Piecewise curve provides the right sensitivity at each overflow level.
- No突变 at boundary points ensures smooth gameplay experience.
- Production-speed-only penalty keeps the penalty focused and understandable.
- Integer arithmetic ensures determinism across platforms.
- No hard cap eliminates frustrating "cannot build" messages while still discouraging spam.

## Status

**ACCEPTED**. Pending implementation. Will be validated by headless economy simulator comparing win rates across different CL strategies.
