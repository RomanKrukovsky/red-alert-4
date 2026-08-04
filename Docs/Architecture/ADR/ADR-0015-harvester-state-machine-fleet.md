# ADR-0015: Harvester State Machine and Fleet Coordination

## Context

The current harvester implementation (`SimWorld::SystemHarvesters`) uses a 5-state FSM: Idle, MovingToResource, Harvesting, MovingToRefinery, Unloading. This is functional but lacks: threat response, retreat, blocked-path handling, deposit destruction recovery, refinery destruction recovery, ownership change handling, and fleet-level coordination (preventing multiple harvesters from queuing at the same depleted node). The economy design requires harvesters to be reliable first, intelligent second.

## Decision

### State Machine (10 States)

```
enum class HarvesterState : uint8_t
{
    Idle                = 0,  // No task; selecting next action
    MovingToResource    = 1,  // Traveling to assigned resource node
    Harvesting          = 2,  // Extracting resources at the node
    MovingToRefinery    = 3,  // Traveling to assigned refinery with cargo
    Unloading           = 4,  // Docked at refinery, offloading cargo
    ThreatDetected      = 5,  // Enemy nearby; evaluating retreat
    Retreating          = 6,  // Moving toward safe zone (refinery or base)
    Blocked             = 7,  // Path obstructed; waiting or rerouting
    WaitingForDock      = 8,  // At refinery but dock occupied; in queue
    ReturningToWork     = 9,  // After retreat/threat; resuming previous task
};
```

### State Transitions

```
Idle
  → MovingToResource     : resource node found via FindBestNode()
  → (stay Idle)          : no available nodes

MovingToResource
  → Harvesting           : arrived at node (distance ≤ DockRadius)
  → Idle                 : assigned node destroyed or depleted (reassign failed)
  → ThreatDetected       : hostile unit within threat radius
  → Blocked              : path obstructed for > kBlockedTickThreshold ticks

Harvesting
  → MovingToRefinery     : cargo full OR node depleted
  → Idle                 : node depleted AND no cargo (shouldn't happen, but safe)
  → ThreatDetected       : hostile unit within threat radius

MovingToRefinery
  → WaitingForDock       : arrived at refinery, dock occupied by another harvester
  → Unloading            : arrived at refinery, dock available
  → ThreatDetected       : hostile unit within threat radius
  → Blocked              : path obstructed for > kBlockedTickThreshold ticks
  → Idle                 : all refineries destroyed

Unloading
  → MovingToResource     : cargo empty; node still has resources
  → Idle                 : cargo empty; no available nodes
  → MovingToRefinery     : cargo empty; assigned node depleted, reassign

WaitingForDock
  → Unloading            : dock freed (previous harvester left)
  → Idle                 : assigned refinery destroyed
  → ThreatDetected       : hostile unit within threat radius

ThreatDetected
  → Retreating           : threat confirmed (not a false alarm after 10-tick scan)
  → MovingToResource     : threat passed (hostile left radius)
  → Harvesting           : threat passed while harvesting (resume)
  → MovingToRefinery     : threat passed while en route to refinery (resume)

Retreating
  → Idle                 : arrived at safe zone; cargo lost (dropped on ground, partially recoverable)
  → ReturningToWork      : threat eliminated while retreating

ReturningToWork
  → MovingToResource     : returning to previous node (if still valid)
  → MovingToRefinery     : had cargo, going to refinery
  → Idle                 : previous target no longer valid

Blocked
  → MovingToResource     : obstacle cleared (unit moved, path rerouted)
  → MovingToRefinery     : obstacle cleared, heading to refinery
  → Idle                 : blocked for > kBlockedGiveUpTicks (give up, reassess)
```

### Deposit Selection and Reservation

When a harvester enters Idle and selects a node via `FindBestNode()`:

1. **Candidate filter**: Node must be alive, have `Amount > 0`, and not be owned by an enemy.
2. **Scoring function** (integer arithmetic):
   ```
   Score = (NodeAmount * 1000) / (DistanceToHarvester + 100)
          + (bIsRichField ? 250 : 0)
          - (CurrentHarvestersOnNode * 150)  // congestion penalty
   ```
3. **Reservation**: Upon selecting a node, increment `Node.ReservedHarvesters`. When a harvester leaves a node (cargo full, node depleted, retreat), decrement `Node.ReservedHarvesters`. This prevents N harvesters from all converging on the same half-depleted node.

### Fleet Coordination

The `FindBestNode()` function is called per-harvester but uses global state (`ReservedHarvesters` per node) to distribute the fleet. Additional rules:

- **No more than 3 harvesters per node** (soft limit). If all nodes have 3+ harvesters, the 4th harvester picks the least congested one anyway (no hard block).
- **Refinery queuing**: harvesters queue at the refinery dock. Maximum queue length: 5. If queue is full, harvester waits at a holding position (150 units from refinery) rather than blocking the path.
- **Cargo capacity reservation**: when a harvester begins Unloading, the refinery reserves `CargoCapacity` credits worth of "pending income" so the player's UI shows expected income.

### Threat Response

When a hostile unit enters the harvester's threat radius (defined per-harvester type, default 400 units):

1. **Scan phase** (10 ticks / ~167ms): harvester pauses extraction, assesses threat.
2. **If threat is combat unit**: transition to Retreating.
3. **If threat is scout/non-threatening**: ignore and resume.
4. **Retreat target**: nearest refinery, or base HQ if no refinery reachable.
5. **Cargo behavior on retreat**: cargo is preserved if harvester reaches safe zone. If harvester is destroyed during retreat, 30% of cargo is lost (spill on ground, recoverable by nearby units).

### Blocked Path Handling

When `MovementComp.BlockedTicks > kBlockedTickThreshold` (30 ticks / 500ms):

1. Try to reroute (recalculate path).
2. If reroute fails, wait up to `kBlockedGiveUpTicks` (180 ticks / 3 seconds).
3. If still blocked, transition to Idle and reassign.

### Deposit Destruction

When a resource node is destroyed (amount reaches 0):

1. All harvesters with `AssignedNode == destroyed_node` transition:
   - If cargo > 0: → MovingToRefinery
   - If cargo == 0: → Idle
2. The destroyed node is removed from `PendingDestroy` as before.
3. `ReservedHarvesters` count is reset.

### Refinery Destruction

When a refinery is destroyed:

1. All harvesters with `AssignedRefinery == destroyed_refinery` transition:
   - If cargo > 0: find nearest remaining refinery via `FindNearestRefinery()`
   - If no refinery exists: → Idle (harvester is effectively useless until new refinery built)
2. `DockedHarvester` and `UnloadingQueue` are cleared.

### Ownership Change

When a harvester's owner changes (engineer capture scenario, which doesn't apply to units currently, but future-proofing):

1. Harvester is immediately reassigned: all node/refinery assignments cleared.
2. State → Idle.
3. The new owner's `CommandLimitUsed` is updated.

### Serialization

`HarvesterState`, `Cargo`, `AssignedNode`, `AssignedRefinery`, and `ReservedHarvesters` (on nodes) are serialized as part of entity state.

### State Hash

All harvester component fields plus `ReservedHarvesters` on resource nodes are included in `ComputeStateChecksum`.

### Replay Behavior

Harvester state transitions are deterministic given the same command stream and seed. Manual Harvest orders, Move orders, and threat events (which are deterministic from unit positions) drive all transitions.

### AI Evaluation

AI queries:
- `HarvesterState` per harvester for fleet status.
- `Cargo` level for income forecasting.
- `ReservedHarvesters` per node for congestion analysis.
- Fleet utilization: `HarvestersInState(Harvesting) / TotalHarvesters`.

## Rationale

- 10-state FSM covers all edge cases (threat, block, dock queue) without over-engineering.
- Reservation system prevents fleet congestion without requiring centralized coordination.
- Threat response is simple (scan → decide → retreat/ignore) and deterministic.
- Blocked-path handling prevents harvesters from getting stuck permanently.
- Refinery destruction gracefully degrades rather than crashing the economy.
- Minimal reliable machine first; advanced intelligence (route optimization, threat prediction) can be layered later.

## Status

**ACCEPTED**. Pending implementation. Will be validated by stress tests with 20+ harvesters on varied map layouts.
