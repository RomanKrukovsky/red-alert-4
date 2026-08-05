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

**ACCEPTED**, implemented in `SimWorld::SystemFlowPayment` and covered by the
`FlowPayment.*` tests in `TestSimulation.cpp`.

### Amendments made during implementation

Two details in the sections above were wrong as written and were corrected in code.
They are recorded here rather than silently edited, because both change observable
behaviour.

**1. Payment and progress run concurrently, not sequentially.**

The transition table implies an item only advances once `PaidCredits >= TotalCost`
("`ProgressTicks` advances only in Paying"). Implemented literally, that makes every
build take twice as long: `TotalTicks` to pay, then `TotalTicks` to build. It also
contradicts `CostPerTick = TotalCost / TotalTicks`, which only sums to the full price
if payment is spread across the *build*, not across a separate funding phase.

`SystemProduction` therefore advances items in **Funding as well as Paying**. Payment
and construction finish on the same tick, and total build time is unchanged from the
pre-ADR behaviour. `Starved`, `EnergyThrottled`, `ManuallyPaused` and `Queued` still
do not advance, which preserves the property that actually matters: an item never
gains progress on a tick it failed to pay for. A guard in `SystemProduction` holds an
item one unit short of completion if rounding lets progress arrive before the final
credit, so nothing is ever delivered unpaid.

**2. `TickAccumulator` is not a separate field.**

`ProgressTicks` is already scaled by `kProgressScale` (100), so it *is* the
fractional-tick accumulator — the power ratio is added to it directly each tick. A
second accumulator field would have been dead state that still had to be serialized
and hashed. `ProductionItem` therefore carries `State`, `TotalCost`, `PaidCredits`,
`ProgressTicks`, `TotalTicks` and `Priority`, and the serialization and hash lists
above should be read with `TickAccumulator` folded into `ProgressTicks`.

### Deferred to other ADRs

- `EnergyThrottled` is defined and honoured by `SystemFlowPayment` but nothing sets
  it yet. Ordinary production *scales* with the power ratio rather than freezing, per
  the ADR-0013 effect matrix; the state is for the high-tech and superweapon
  categories ADR-0013 pauses outright. Recovery (`EnergyThrottled → Funding` once the
  ratio is back above `kMinPowerRatioPercent`) is already implemented here rather than
  left to ADR-0013, so the state cannot become a permanent trap for whoever sets it
  first. Note that `kMinPowerRatioPercent` in `SimWorld.cpp` is 20, not the 50 quoted
  above, and it is a floor multiplier rather than a freeze threshold.
- `PrerequisiteLost` is defined but unreachable: nothing currently re-checks
  prerequisites for in-flight items, so its 80% refund is unimplemented.
- `OwnershipChanged` is not yet used as a *state*, but its rule is enforced:
  `SellBuilding` marks `BuildingComp::bSelling` and `DestroyEntity` skips the queue
  refund for a sale, so selling returns the sale price only. Capture does not exist
  yet.
- `Priority` is stored, serialized, hashed and honoured by the allocation order, but
  no command sets it — every item is priority 0 and ties break on entity index. The
  command to set it belongs with the UI work.

`TotalCost` is serialized despite being derivable from `ContentId`, so that a save
made before a balance change still loads at the price the player agreed to pay. It
remains excluded from the state hash.

### Consequences outside the simulation

- `kReplayFormatVersion` is bumped to 2. `SystemFlowPayment` changed the tick order
  and the state hash gained the payment fields, so a v1 replay would replay to a
  different hash; without the bump it would be reported as a desync rather than as an
  old file.
- Affordability is no longer a HUD block reason. The simulation accepts an order the
  player cannot yet pay for, so greying the card out would forbid a command the
  simulation would take and make gradual payment unreachable through the UI.
  `CommandReject::InsufficientCredits` is consequently no longer produced for
  production; it is left in the enum because removing a public value is a separate
  interface change.
- `QueueEntry` carries `PaymentState`, `PaidCredits`, `TotalCost` and
  `bStarvedForCredits` so the sidebar can distinguish "out of money" from "you pressed
  pause" — the distinction the state enum exists for.
