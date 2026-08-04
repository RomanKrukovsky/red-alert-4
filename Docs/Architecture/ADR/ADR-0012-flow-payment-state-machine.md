# ADR-0012: Flow-Payment State Machine

## Context

RA4 uses a C&C-style gradual payment model where credits are deducted incrementally over the production duration rather than charged upfront. This creates interesting cash-flow management but requires explicit state tracking for every production item across interruptions: power shortages, manual pauses, prerequisite loss, producer destruction, ownership changes, and credit starvation. The current implementation (`SimWorld::SystemProduction`) tracks only `PaidCredits` and `bPaused` on `ProductionItem`, which is insufficient for the full set of transitions described in the economy design.

## Decision

Define a `FlowPaymentState` enum and expand `ProductionItem` to track the complete lifecycle of every queued production item.

### States

```
enum class FlowPaymentState : uint8_t
{
    Queued            = 0,  // In queue, not yet receiving credits
    Funding           = 1,  // Credits being deducted incrementally
    Paying            = 2,  // Fully funded; production ticks advancing
    Starved           = 3,  // Was Paying, but credits exhausted mid-cycle
    EnergyThrottled   = 4,  // Has credits but power deficit halts progress
    ManuallyPaused    = 5,  // Player paused this item
    Completed         = 6,  // Production finished; awaiting spawn/placement
    Cancelled         = 7,  // Player cancelled; refund issued
    ProducerDestroyed = 8,  // Producing building destroyed
    PrerequisiteLost  = 9,  // Required tech building destroyed
    OwnershipChanged  = 10, // Building captured or sold
};
```

### Expanded ProductionItem

```
struct ProductionItem
{
    ContentId         Content;
    FlowPaymentState  State           = FlowPaymentState::Queued;
    int32_t           TotalCost       = 0;      // full credit cost (fixed at queue time)
    int32_t           PaidCredits     = 0;      // credits deducted so far
    int32_t           ProgressTicks   = 0;      // production progress (advances only in Paying)
    int32_t           TotalTicks      = 0;      // total ticks to complete
    int32_t           TickAccumulator = 0;      // fractional-tick accumulator (kProgressScale)
    int32_t           Priority        = 0;      // player-assigned priority (higher = funded first)
    bool              bPaused         = false;  // legacy flag; State is authoritative
};
```

### Transition Rules

| From | To | Condition |
|---|---|---|
| Queued | Funding | Item is at head of queue AND credits > 0 AND energy ratio ≥ kMinPowerRatioPercent |
| Queued | Cancelled | Player issues cancel command |
| Queued | ProducerDestroyed | Producing building destroyed |
| Queued | OwnershipChanged | Building captured or sold |
| Funding | Paying | `PaidCredits >= TotalCost` (fully funded) |
| Funding | Starved | Player credits reach 0 before `PaidCredits >= TotalCost` |
| Funding | EnergyThrottled | Power ratio drops below `kMinPowerRatioPercent` |
| Funding | ManuallyPaused | Player issues pause command |
| Funding | Cancelled | Player issues cancel command |
| Funding | ProducerDestroyed | Building destroyed |
| Funding | PrerequisiteLost | Required tech building destroyed |
| Funding | OwnershipChanged | Building captured or sold |
| Paying | Completed | `ProgressTicks >= TotalTicks * kProgressScale` |
| Paying | Starved | Player credits reach 0 (for items with ongoing credit drain, e.g., superweapons) |
| Paying | EnergyThrottled | Power ratio drops below `kMinPowerRatioPercent` |
| Paying | ManuallyPaused | Player issues pause command |
| Paying | ProducerDestroyed | Building destroyed |
| Paying | PrerequisiteLost | Required tech building destroyed |
| Paying | OwnershipChanged | Building captured or sold |
| Starved | Funding | Player credits > 0 AND energy ratio ≥ minimum |
| Starved | ProducerDestroyed | Building destroyed |
| Starved | OwnershipChanged | Building captured or sold |
| EnergyThrottled | Paying | Power ratio recovers to ≥ `kMinPowerRatioPercent` |
| EnergyThrottled | Starved | Credits also reach 0 |
| EnergyThrottled | ProducerDestroyed | Building destroyed |
| EnergyThrottled | OwnershipChanged | Building captured or sold |
| ManuallyPaused | Queued | Player unpauses (returns to funding queue) |
| ManuallyPaused | ProducerDestroyed | Building destroyed |
| ManuallyPaused | OwnershipChanged | Building captured or sold |

### Credit Deduction Order

Each tick, across all active production items globally:

1. **Collect all items in Funding or Paying state** from all players.
2. **Sort by player-assigned Priority descending**, then by queue position ascending (FIFO within same priority).
3. **For each item in sorted order:**
   - Calculate `CreditBudget = min(CreditsRemainingThisTick, CostPerTick)`.
   - `CostPerTick = TotalCost / TotalTicks` (integer division; remainder applied on final tick).
   - Deduct `CreditBudget` from item `PaidCredits` and from player `Credits`.
   - If item transitions to Paying (`PaidCredits >= TotalCost`), it stops consuming credit budget.

`CostPerTick` is computed as:

```
int32_t CostPerTick(int32_t TotalCost, int32_t TotalTicks)
{
    return (TotalTicks > 0) ? (TotalCost + TotalTicks - 1) / TotalTicks : TotalCost;
}
```

This ceiling division ensures the full cost is covered within `TotalTicks`.

### Refund Rules (on Cancel)

| Condition | Refund |
|---|---|
| Building, `PaidCredits / TotalCost ≤ 25%` | 90% of `PaidCredits` |
| Building, `PaidCredits / TotalCost > 25%` | 60% of `PaidCredits` |
| Unit in queue | 80% of `PaidCredits` |
| Building sale (any state) | 50% of `TotalCost` + crew spawn |

Refunds are applied atomically: the player's `Credits` increases and the `ProductionItem` is removed in the same tick.

### Progress Regression

**Never.** `ProgressTicks` only advances or stays constant. When an item is Starved or EnergyThrottled, `ProgressTicks` freezes. When it resumes, it continues from where it stopped. This is critical for determinism: the same sequence of commands and seeds must produce the same final state regardless of the path through funding states.

### ProducerDestroyed / PrerequisiteLost / OwnershipChanged

These are terminal states. The item is immediately removed from the queue. Refund rules apply:

- **ProducerDestroyed**: 50% refund of `PaidCredits` (sympathy refund; the building is gone).
- **PrerequisiteLost**: 80% refund of `PaidCredits` (player didn't choose to lose the tech).
- **OwnershipChanged**: Item is removed without refund (new owner inherits completed items only).

### Interaction with Power Ratio

When `PowerRatioPercent < kMinPowerRatioPercent` (currently 50%):

- Items in **Funding** state transition to **EnergyThrottled**. Credit deduction pauses. `PaidCredits` is preserved.
- Items in **Paying** state transition to **EnergyThrottled**. `ProgressTicks` freezes. `PaidCredits` is preserved.
- When power recovers, items return to their pre-throttle state (Funding or Paying).

### Serialization

`FlowPaymentState`, `PaidCredits`, `ProgressTicks`, `TickAccumulator`, and `Priority` are included in `SimWorld::Serialize`. `TotalCost` and `TotalTicks` are recomputed from `ContentId` + `ContentDatabase` on deserialization (they are derived, not authoritative state).

### State Hash

`FlowPaymentState`, `PaidCredits`, `ProgressTicks`, `TickAccumulator`, and `Priority` are included in `ComputeStateChecksum`. `TotalCost` and `TotalTicks` are excluded (derived).

### UI Representation

The presentation layer reads `FlowPaymentState` to display:
- **Queued**: greyed out, position in queue shown
- **Funding**: progress bar showing `PaidCredits / TotalCost` with credit-per-second rate
- **Paying**: progress bar showing `ProgressTicks / (TotalTicks * kProgressScale)`
- **Starved**: red flash, "Insufficient credits" tooltip, resume button
- **EnergyThrottled**: yellow warning, "Power shortage" tooltip, priority button
- **ManuallyPaused**: pause icon, resume button
- **Completed**: green, "Ready" / "Select placement" for buildings

### Replay Behavior

Flow-payment state transitions are deterministic given the same command stream and seed. No special replay handling is needed beyond recording the commands that trigger state changes (pause, cancel, unpause). The simulation re-derives all other transitions.

### AI Evaluation

AI queries `FlowPaymentState` to:
- Detect Starved items and defer lower-priority production.
- Detect EnergyThrottled items and prioritize power infrastructure.
- Calculate `TimeToComplete = TotalTicks - ProgressTicks` for planning.
- Calculate `CreditsNeeded = TotalCost - PaidCredits` for budgeting.

## Rationale

- Every production interruption has an explicit state, eliminating implicit "paused but why?" ambiguity.
- Progress never regresses, which is critical for determinism and replay.
- Priority-based credit allocation across parallel queues prevents starvation of critical production.
- Refund rules are consistent with the bible specification.
- Serialization includes only authoritative state (PaidCredits, ProgressTicks), not derived values.

## Status

**ACCEPTED**. Pending implementation. Will be validated by headless economy simulator before production integration.
