# RA4 Content Bible Implementation Plan

Integration of the full 3,520-line specification (`RA4_Factions_Units_Economy_Voice_Bible.md`) into the Red Alert 4 C++ strategy engine and Unreal Engine 5.6 frontend. This plan establishes an automated, deterministic content import pipeline, expands core simulation systems (economy, power, command cap, 9x9 damage matrix, veterancy, 4 unique faction resources, GAS abilities), implements all 4 factions and 78 unique units, connects HUD UI and EVA audio, and verifies authority, save/load, and headless + engine testing.

---

## 1. Executive Summary & Goals

- **Single Source of Truth**: Parse and enforce `RA4_Factions_Units_Economy_Voice_Bible.md` deterministically.
- **4 Factions**: Soviet Union, Alliance, Eastern Coalition, ChronoLegion.
- **78 Unique Units**: 19 Soviet, 20 Alliance, 20 Coalition, 19 ChronoLegion.
- **Data-Driven Architecture**: Fully engine-free simulation data definitions + Unreal Engine Primary Data Assets & Data Tables.
- **Idempotent Importer Pipeline**: Commandlet `RA4ContentImportCommandlet` generating `ra4_content.normalized.json`, verifying JSON Schema, and generating asset definitions and `voice_manifest.csv`.

---

## 2. Phase Breakdown

### Phase 1: Content Importer & Schema Pipeline (`RA4Editor`)
- Parse Bible markdown via structural C++ / Python parser script.
- Generate `Content/RA4/Data/Generated/ra4_content.normalized.json` with SHA-256 source hash and timestamp.
- Generate Primary Data Assets, Data Tables, String Tables, Gameplay Tags, and Voice Manifest.
- Report generation: `docs/content/CONTENT_IMPORT_REPORT.md` & `Content/RA4/Data/Generated/content_import_report.json`.

### Phase 2: Data Models & Taxonomy Expansion (`RA4Content`, `RA4Core`)
- Expand `ArmorClass` to 9 types: `LightInfantry`, `HeavyInfantry`, `LightVehicle`, `HeavyVehicle`, `SiegeVehicle`, `Air`, `Naval`, `Building`, `Shielded`.
- Expand `WarheadClass` (DamageType) to 9 types: `Ballistic`, `Fragmentation`, `ArmorPiercing`, `Siege`, `Electric`, `Plasma`, `Cryogenic`, `Temporal`, `AntiAir`.
- Implement `DamageMatrix.h` with exact 9x9 multiplier matrix from Section 3.
- Implement `VeterancyDef.h` (Recruit 0x, Veteran 1x cost, Elite 2.5x cost, Heroic 5x cost).
- Implement Command Cap, Power priority levels, and Faction Resource structs.

### Phase 3: Core Simulation Subsystems (`RA4Economy`, `RA4Combat`, `RA4Abilities`, `RA4Factions`)
- Command limit reservation & cap generation (HQ, Barracks, Factories).
- Power cascade degradation (Aux -> Radar/Minimap -> Repair -> High-Tech -> Static Defense -> Superweapons).
- Economy transaction journaling (`FRA4EconomyTransaction`) with refund/sell/repair/capture math.
- 4 Faction resources (`IRA4FactionResourceStrategy`):
  - Soviet Mobilization (0-100 scale, General Push ability).
  - Alliance Intelligence (scans, hacks, precision strikes).
  - Coalition Synchronization (network grid, joint shields, fast production).
  - ChronoLegion Temporal Stability (teleport/rewind costs & penalty <30).

### Phase 4: Units, Buildings & AI (`RA4Units`, `RA4Buildings`, `RA4Abilities`, `RA4AI`)
- Implement definitions & C++ logic for all 78 units.
- Implement building definitions, prerequisites graph validator, and superweapons.
- GAS integration for abilities, cooldowns, shields, and buffs.
- AI target priority scoring, HP retreat logic, tactical formation offsets, and 4 faction profiles.

### Phase 5: Voice, EVA & UI Binding (`RA4Voice`, `RA4UI`, `RA4Network`, `RA4SaveSystem`)
- Unit voice events & system EVA priority queue via `URA4VoiceSubsystem`.
- Fallback localized subtitles for missing audio files (`MissingSoundWave`).
- UI binding for command cap, faction resources, build cards, selection info, EVA feed.
- Server RPC authority & save/load match state serialization.

### Phase 6: Automated Testing & Validation (`RA4Tests`)
- Data-driven tests for all 78 units, buildings, damage matrix, economy rules, abilities, AI, save/load, and headless determinism.
