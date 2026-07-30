# AI Implementation Report

Validation of AI target priority scoring, automatic retreat logic, tactical formation offsets, and faction build-order profiles derived from Section 5 of `RA4_Factions_Units_Economy_Voice_Bible.md`.

## 1. Target Priority Scoring Formula
`TargetScore = BaseRolePriority + ThreatMultiplier - DistancePenalty + LowHealthBonus`
- Primary targets: High-threat armed units, sieging artillery, damaged enemy harvesters.
- Restricted targets: Anti-Air weapons do not target ground units; siege artillery maintains minimum range.

## 2. Automatic Retreat Thresholds
- **Infantry**: Retreat when HP drops below 25% (unless in Hold Position or under Commissar order).
- **Vehicles / Tanks**: Retreat when HP drops below 30%.
- **Heroes**: Retreat when HP drops below 35%.
- Retreat destination selects nearest friendly base or allied defensive tower cluster.

## 3. Formation Offsets by Tactical Role
- **Frontline**: Heavy Vehicles (`SU_Apocalypse`, `AL_GuardianTank`, `CO_JadeTank`, `CH_ContinuumTank`).
- **Skirmish**: Light Infantry & Scout Vehicles (`SU_Conscript`, `AL_Jackal`, `CO_Vanguard`, `CH_BlinkScout`).
- **Support & Ranged**: Artillery & AA (`SU_Buratino`, `AL_AthenaCannon`, `CO_LotusArtillery`, `CH_DelayArtillery`).
- **Command / Hero**: Center position behind frontline.

## 4. Faction AI Build-Order Profiles
Derived from Section 7 starter armies:
- **Soviet Aggressive**: Fast Mobilization -> Power -> Refinery -> Barracks -> War Factory -> Conscript spam -> Hammer Tank assault.
- **Alliance Technical**: Uplink scan rush -> Prospector outpost expansion -> Mirage / Pacifier ambushes.
- **Coalition Grid**: Synchronized power grid -> Bastion shield wall -> Lotus Artillery battery.
- **Chrono Tactics**: Temporal Stability regen -> Quantum Harvester teleport -> Stasis Projector lock down.
