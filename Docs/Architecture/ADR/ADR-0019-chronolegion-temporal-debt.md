# ADR-0019: Chronolegion Temporal Debt

## Context

The Chronolegion faction's unique economic mechanic is "temporal debt": the ability to borrow future income for present use, with a repayment penalty. The economy design specifies: "Get 3000 credits now, but next 90 seconds produce 30% slower." This must be defined as explicit deterministic simulation state with exact stacking rules, maximum debt, duration, repayment curve, cancellation rules, serialization, save migration, state hashing, replay behavior, UI representation, AI evaluation, and interactions with income modifiers. All arithmetic must use integers/fixed-point and simulation ticks.

## Decision

### Temporal Debt State

```
struct TemporalDebtState
{
    int32_t  DebtAmount          = 0;      // total credits borrowed (not yet repaid)
    int32_t  DebtPrincipal       = 0;      // original amount borrowed (for UI display)
    int32_t  RepaymentTicksRemaining = 0;  // ticks until debt expires
    int32_t  RepaymentTotalTicks = 0;      // total repayment duration (for UI progress)
    int32_t  IncomePenaltyBPS    = 0;      // income penalty in basis points (10000 = 100%)
    bool     bActive             = false;  // whether debt is currently active
};
```

Stored in `PlayerState` as a sub-struct. Only Chronolegion players use this field.

### Activation Rules

When a Chronolegion player activates "Temporal Borrowing" ability:

1. **Cost**: 30 Temporal Stability points (faction resource, ADR-0018).
2. **Immediate gain**: `DebtAmount` credits added to player's `Credits`.
3. **Parameters** (from content data, tunable per ability level):

| Parameter | Value |
|---|---|
| CreditsGranted | 3,000 |
| RepaymentDurationTicks | 5,400 (90 seconds at 60Hz) |
| IncomePenaltyBPS | 3,000 (30% penalty) |
| TemporalStabilityCost | 30 |
| CooldownTicks | 3,600 (60 seconds) |

4. **Activation command**: Player issues `Command::ActivateAbility` with `AbilityId::TemporalBorrowing`. The simulation validates: faction is Chronolegion, cooldown is 0, Temporal Stability ≥ 30.

### Stacking Rules

**Stacking is limited.** Maximum concurrent debts:

| Concurrent Debts | Combined Penalty Cap |
|---|---|
| 1 | 30% (3,000 BPS) |
| 2 | 50% (5,000 BPS) |
| 3 | 65% (6,500 BPS) |
| 4+ | Not allowed (ability greyed out) |

Stacking formula:
```
TotalPenaltyBPS = 0
for each active debt:
    TotalPenaltyBPS += debt.IncomePenaltyBPS
TotalPenaltyBPS = min(TotalPenaltyBPS, MaxPenaltyBPS)  // MaxPenaltyBPS = 8,000 (80%)
```

If activating a new debt would cause `TotalPenaltyBPS > MaxPenaltyBPS`, the ability is rejected.

### Repayment Curve

Repayment is **linear**. Each tick, the debt reduces the player's effective income:

```
EffectiveIncome = BaseIncome * (10000 - TotalPenaltyBPS) / 10000
```

Where `BaseIncome` is the sum of all income sources (harvesting + passive + trade).

Additionally, each tick:
```
DebtPrincipal -= (DebtPrincipal + RepaymentTotalTicks - 1) / RepaymentTotalTicks  // ceiling division
RepaymentTicksRemaining -= 1
```

When `RepaymentTicksRemaining == 0`:
- `DebtPrincipal = 0`
- `DebtAmount = 0`
- `bActive = false`
- Income penalty from this debt is removed

### Maximum Debt

| Limit | Value | Enforcement |
|---|---|---|
| Concurrent debts | 4 | Rejected at activation |
| Total penalty cap | 80% (8,000 BPS) | Rejected at activation |
| Maximum duration | 5,400 ticks (90s) | Content data, not hardcoded |
| Maximum credits per debt | 3,000 | Content data |

### Cancellation Rules

A player can **cancel** active debt early:

1. **Cancellation command**: `Command::CancelTemporalDebt` with `DebtId`.
2. **Immediate cost**: `DebtPrincipal * 20%` additional credits (early repayment penalty).
3. **Effect**: Debt is immediately cleared. Income penalty removed.
4. **Partial refund**: No refund of the original `CreditsGranted`. The player already spent them.
5. **Temporal Stability**: No refund of the 30 stability points.

Cancellation is a strategic decision: pay 20% of remaining principal now to remove the income penalty immediately.

### Interaction with Income Modifiers

The income penalty applies **after** all other modifiers:

```
1. Calculate BaseIncome (harvest + passive + trade)
2. Apply faction bonuses (Eastern Coalition network, etc.)
3. Apply economic upgrade modifiers
4. Apply temporal debt penalty: EffectiveIncome = Income * (10000 - TotalPenaltyBPS) / 10000
5. EffectiveIncome is what the player actually receives
```

This means temporal debt penalty is multiplicative with other modifiers, not additive. A 30% debt penalty on top of a 10% upgrade bonus results in `Income * 1.1 * 0.7 = Income * 0.77`, not `Income * 0.8`.

### Serialization

`TemporalDebtState` fields are serialized as part of `PlayerState`:
- `DebtAmount`, `DebtPrincipal`, `RepaymentTicksRemaining`, `RepaymentTotalTicks`, `IncomePenaltyBPS`, `bActive`
- Additionally, the **array of active debts** is serialized (up to 4 entries).

```
struct TemporalDebtEntry
{
    int32_t  DebtAmount;
    int32_t  DebtPrincipal;
    int32_t  RepaymentTicksRemaining;
    int32_t  RepaymentTotalTicks;
    int32_t  IncomePenaltyBPS;
};

// In PlayerState:
TemporalDebtEntry ActiveDebts[4];
int32_t           ActiveDebtCount = 0;
int32_t           TotalPenaltyBPS = 0;  // cached sum for efficiency
```

### Save Migration

Save format version is incremented when `TemporalDebtState` fields are added. Migration rule:

- **Version N to N+1**: If `ActiveDebtCount` is missing (old save), default to 0. If `TotalPenaltyBPS` is missing, recompute from `ActiveDebts` array.
- **Backward compatibility**: Old saves without temporal debt data load correctly with no active debts.

### State Hash

All `TemporalDebtEntry` fields and `ActiveDebtCount` are included in `ComputeStateChecksum`. The cached `TotalPenaltyBPS` is excluded (derived from entries).

### Replay Behavior

Temporal debt state transitions are driven by player commands:
- `ActivateAbility(TemporalBorrowing)`: adds a new debt entry
- `CancelTemporalDebt(DebtId)`: removes a debt entry

During replay, the same commands produce the same debt states. The tick-by-tick repayment is deterministic (same tick count → same principal reduction).

### UI Representation

- **Debt gauge**: Below the credits display, a row of up to 4 debt indicators.
  - Each shows: `DebtPrincipal` remaining, `RepaymentTicksRemaining` as a countdown, `IncomePenaltyBPS` as a percentage.
  - Color: green (low penalty) → yellow (moderate) → red (high).
- **Income impact**: Credits/second display shows "before debt" and "after debt" values.
- **Activation button**: Greyed out when at max debts or insufficient Temporal Stability. Shows tooltip with exact numbers: "Borrow 3,000 credits. Income -30% for 90s. Cost: 30 Stability."
- **Cancellation button**: On each active debt indicator. Shows cost: "Cancel debt for X credits (20% of remaining)."
- **Warning indicator**: Flash + EVA audio when total penalty exceeds 50%.

### AI Evaluation

AI evaluates temporal debt decisions using:

```
ExpectedBenefit = CreditsGranted  // 3,000
CostPerTick = BaseIncome * IncomePenaltyBPS / 10000
TotalCost = CostPerTick * RepaymentTotalTicks
NetValue = ExpectedBenefit - TotalCost
```

AI activates debt when:
- `NetValue > 0` (profitable)
- AND current tactical situation justifies the income reduction
- AND `TotalPenaltyBPS + NewDebtPenalty <= MaxPenaltyBPS`
- AND `TemporalStability >= 30`

AI cancels debt when:
- Income penalty is causing starvation in critical production
- AND cancellation cost < expected income loss over remaining debt duration

### Interaction with Other Chronolegion Mechanics

| Mechanic | Interaction |
|---|---|
| Temporal Stability (faction resource) | Debt activation costs 30 stability. Stability regenerates at 1/2s. |
| Production acceleration ability | Can be used during debt. Acceleration applies to production speed, not income. |
| Resource restoration ability | Can be used during debt. Restores node amount independently of income. |
| Partial refund ability | Can be used during debt. Refund is added to credits directly, not affected by debt penalty. |

### Determinism Verification

The temporal debt system is verified by:
1. **Activation test**: Activate debt, verify credits added, penalty applied, state serialized correctly.
2. **Repayment test**: Run for `RepaymentTotalTicks`, verify principal reaches 0, penalty removed.
3. **Stacking test**: Activate 2–3 debts, verify penalty sums correctly, verify cap enforcement.
4. **Cancellation test**: Cancel mid-repayment, verify penalty removed, cancellation cost deducted.
5. **Serialization round-trip**: Serialize/deserialize with active debts, verify identical state hash.
6. **Replay test**: Replay activation commands, verify identical debt states on each tick.

## Rationale

- Explicit state array (up to 4 debts) makes stacking rules clear and testable.
- Linear repayment is simple, deterministic, and easy to understand.
- 80% maximum penalty prevents economic suicide from excessive borrowing.
- Cancellation with 20% penalty creates meaningful risk/reward decisions.
- Integer arithmetic (basis points, tick counts) ensures cross-platform determinism.
- Content-data-driven parameters allow balancing without code changes.

## Status

**ACCEPTED**. Pending implementation. Will be validated by headless simulator testing various debt usage patterns.
