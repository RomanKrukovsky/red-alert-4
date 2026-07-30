# Faction Mechanics Report

Validation report for the four unique faction resources and gameplay mechanics defined in `RA4_Factions_Units_Economy_Voice_Bible.md`.

## 1. Soviet Union: Mobilization (0–100 Scale)
- **Accumulation**: +1 point per 200 damage dealt, +1 point per 300 damage taken, holding front lines.
- **Passive Tiers**:
  - `0–24`: Base state (no bonus).
  - `25–49`: +5% Infantry production speed.
  - `50–74`: +8% Vehicle production speed, +5% movement speed near HQ.
  - `75–100`: +10% Heavy Vehicle attack damage.
- **Active Ability**: *General Push* (Cost: 50 Mobilization). Grants selected units +20% movement speed and suppression immunity for 12 seconds.
- **Implementation Class**: `IRA4FactionResourceStrategy` -> `FSovietMobilizationStrategy`.

## 2. Alliance: Intelligence / Data (0–100 Scale)
- **Accumulation**: Enemy unit discovery, radar scans, destroying key enemy structures.
- **Active Abilities**:
  - *Radar Scan* (Cost: 20 pts): Reveals target shrouded fog-of-war area for 15s.
  - *Network Hack* (Cost: 35 pts): Disables target enemy structure for 10s.
  - *Air Wing Boost* (Cost: 30 pts): Instantly rearm all landed aircraft.
  - *Precision Strike* (Cost: 60 pts): Guided air strike on pinpoint coordinates.
- **Implementation Class**: `IRA4FactionResourceStrategy` -> `FAllianceIntelligenceStrategy`.

## 3. Eastern Coalition: Synchronization (0–100 Scale)
- **Accumulation**: Maintaining connected power grid, tight unit formations, joint attacks.
- **Passive Effects**: Shared shield absorption across adjacent units, +10% firing accuracy, +15% production speed. Drops rapidly if formations scatter or power grid breaks.
- **Implementation Class**: `IRA4FactionResourceStrategy` -> `FCoalitionSyncStrategy`.

## 4. ChronoLegion: Temporal Stability (0–100 Scale)
- **Accumulation**: Passive regeneration (+1 pt / 2s), controlling Chrono Beacons.
- **Costs**: Teleporting units (15–30 pts), Temporal Rewind (40 pts).
- **Penalty State**: If stability falls below 30, all units incur +15% damage taken and -10% movement speed.
- **Implementation Class**: `IRA4FactionResourceStrategy` -> `FChronoStabilityStrategy`.
