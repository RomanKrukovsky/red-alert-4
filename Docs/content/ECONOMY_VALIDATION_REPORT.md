# Economy Validation Report

Validation report for global economy, harvesting loops, power cascades, command caps, and financial transaction rules from `RA4_Factions_Units_Economy_Voice_Bible.md`.

## 1. Starting Conditions & Ore Fields
- **Starting Match Credits**: 10,000 credits per player.
- **Starting Command Cap**: 50 max (capped at 200).
- **Ore Fields**: Standard Ore Field (45,000 credits), Rich Ore Field (75,000 credits).
- **Harvester Capacity**: 1,200 credits per full cargo load.
- **Oil Derrick Income**: 8 credits per second continuous passive income.

## 2. Cancellation, Sale, Repair & Capture Rules
- **Building Cancellation**:
  - `<= 25%` construction progress: **90% refund**.
  - `> 25%` construction progress: **60% refund**.
- **Unit Queue Cancellation**: **80% refund** of unused cost.
- **Building Sale**: **50% refund** + spawns crew infantry group.
- **Building Repair**: Full repair cost capped at **30%** of initial building purchase price.
- **Vehicle Repair**: Field repair capped at **25%** of vehicle price.
- **Engineer Capture**: Consumes Engineer unit; disables captured building for **8 seconds**.

## 3. Power Cascade Shutdown Sequence
Under severe power shortage, systems deactivate in strict priority order:
1. Auxiliary Systems
2. Radar & Minimap
3. Repair Facilities
4. High-Tech Manufacturing
5. Static Defense
6. Superweapons
*Base Barracks and Harvester gather loops operate at 50% speed during severe shortage.*

## 4. Transaction Safety (`FRA4EconomyTransaction`)
Server maintains single-source-of-truth ownership. All credit mutations emit a journaled transaction log: `TransactionId`, `PlayerId`, `ReasonTag`, `Amount`, `BalanceBefore`, `BalanceAfter`, `RelatedEntityId`, and `SimulationTick`.
