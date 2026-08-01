# Red Alert 4 — EVA Event Catalog & Master Dispatch Architecture
**Document Status:** Production Master Reference  
**Version:** 2.0 (Naming Reset Compliant)  
**Encoding:** UTF-8  
**Target Subsystem:** Unreal Engine 5 `URA4VoiceSubsystem` & VoxCPM2 TTS Pipeline  

---

## 1. Executive Summary & Architecture Overview

The Red Alert 4 Electronic Voice Assistant (EVA) architecture provides immediate tactical, economic, and strategic situational awareness to players across all 4 playable factions:
- **SU (USSR):** Voice Operator **EVA Contour (`EVA_SU_KONTUR`)**
- **AL (Alliance):** Tactical AI **EVA Astra (`EVA_AL_ASTRA`)**
- **CO (Eastern Coalition):** Harmonic Network Node **EVA Harmony (`EVA_CO_HARMONIA`)**
- **CH (Chronolegion):** Temporal Causal Analyst **EVA Moira (`EVA_CH_MOIRA`)**
All voice assets, script events, and subtitles are strictly bound to the **v2.0 Naming Reset** specification (`RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md`). No legacy C&C terms, names, or deprecated IDs are utilized.

---

## 2. Faction EVA Voice Profiles & VoxCPM2 Control Parameters

### 2.1. EVA Contour (`EVA_SU_KONTUR`) - USSR / USSR
- **Role:** Central Command Network Voice Operator (Central operator of the USSR command network)- **Acoustic Profile:** Authoritative mature female military command voice, low contralto, precise Russian diction, restrained emotion, firm concise delivery, calm under pressure.
- **Tone & Pacing:** Commanding, industrial, resolute, unhurried under normal conditions, sharp and urgent during combat alerts.
- **Forbidden Traits:** Screaming, panic, theatrical accent, hysterical pitch spikes.
- **Control Instruction (VoxCPM2):** `Authoritative mature female military command voice, low contralto, precise Russian diction, restrained emotion, firm concise delivery, calm under pressure.`

### 2.2. EVA Astra (`EVA_AL_ASTRA`) – Alliance / Alliance
- **Role:** Tactical Logistics & Combat AI (Advanced Tactical AI System of the Alliance)- **Acoustic Profile:** Professional female tactical AI voice, clean and intelligent, confident, modern, precise, slightly warm, medium-fast pace, excellent Russian diction.
- **Tone & Pacing:** Crisp, high-tech, efficient, slightly warm yet strictly professional, clear analytical cadence.
- **Forbidden Traits:** Robotic monotone, casual slang, dramatic pauses, emotional instability.
- **Control Instruction (VoxCPM2):** `Professional female tactical AI voice, clean and intelligent, confident, modern, precise, slightly warm, medium-fast pace, excellent Russian diction.`

### 2.3. EVA Harmony (`EVA_CO_HARMONIA`) – Eastern Coalition / Eastern Coalition
- **Role:** Strategic Harmonic Network Node (Coalition Strategic Harmonic Node AI)- **Acoustic Profile:** Calm disciplined female strategic command voice, elegant and precise, controlled emotion, measured pace, subtle authority, clear standard Russian.
- **Tone & Pacing:** Serene, balanced, highly disciplined, unshakeable calm, elegant rhythm reflecting network cohesion.
- **Forbidden Traits:** Caricature accents, broken Russian, aggressive screaming, erratic speed shifts.
- **Control Instruction (VoxCPM2):** `Calm disciplined female strategic command voice, elegant and precise, controlled emotion, measured pace, subtle authority, clear standard Russian.`

### 2.4. EVA Moira (`EVA_CH_MOIRA`) - Chrono Legion / Chronolegion
- **Role:** Temporal Causal Analyst- **Acoustic Profile:** Androgynous timeless command voice, calm and unsettling, extremely precise, restrained emotion, deliberate micro-pauses, clear Russian, no baked-in audio effects.
- **Tone & Pacing:** Coldly analytical, detached, timeless, deliberate micro-pauses between sentences, treating past, present, and future events as editable facts.
- **Forbidden Traits:** Robotic digital distortion artifacts, screeching, emotional hysteria, theatrical villainy.
- **Control Instruction (VoxCPM2):** `Androgynous timeless command voice, calm and unsettling, extremely precise, restrained emotion, deliberate micro-pauses, clear Russian, no baked-in audio effects.`

---

## 3. Taxonomy, Dispatch Rules & Concurrency Matrix

### 3.1. Priority Scale
1. **Priority 1 (Critical):** Match flow events (`MATCH_START`, `MATCH_VICTORY`, `MATCH_DEFEAT`), structural losses (`COMMAND_CENTER_DESTROYED`, `HERO_LOST`), and superweapons (`OWN_SUPERWEAPON_FIRED`, `ENEMY_SUPERWEAPON_FIRED`).
2. **Priority 2 (High):** Tactical combat alerts (`BASE_UNDER_ATTACK`, `HARVESTER_UNDER_ATTACK`, `POWER_LOW`, `STEALTH_UNIT_DETECTED`).
3. **Priority 3 (Normal):** Production and economic notifications (`UNIT_READY_GENERIC`, `CONSTRUCTION_COMPLETE`, `INSUFFICIENT_FUNDS`, `UPGRADE_COMPLETE`).
4. **Priority 4 (Low / Info):** Information and status updates.

### 3.2. Cooldowns & Deduplication
To eliminate audio clutter in high-APM scenarios, events utilize per-category cooldown timers:
- **Combat Alerts (`BASE_UNDER_ATTACK`):** 5.0 seconds
- **Harvester Alerts (`HARVESTER_UNDER_ATTACK`):** 8.0 seconds
- **Economic Deficits (`INSUFFICIENT_FUNDS`, `POWER_LOW`):** 10.0 seconds
- **Production (`UNIT_READY_GENERIC`):** 2.0 seconds
- **Critical Match Flow Events:** 0.0 seconds (Always dispatch)

### 3.3. Concurrency Groups (`ConcurrencyGroup`)
- `CG_MatchFlow`: Match state changes and catastrophic structure/hero destruction.
- `CG_Superweapon`: Superweapon countdowns, firings, and warnings.
- `CG_CombatAlert`: Base attack, harvester under fire, structure damage.
- `CG_Production`: Unit construction, structure complete, upgrade complete, sales, cancellations.
- `CG_Economy`: Resource depletion, low power, insufficient funds, command cap.
- `CG_Intel`: Stealth detection, faction ability status, full resource charge.

### 3.4. Interrupt Policies (`InterruptPolicy`)
- **`NeverInterrupt`:** Cannot be interrupted under any circumstance (e.g. `MATCH_START`, `MATCH_VICTORY`, `MATCH_DEFEAT`).
- **`InterruptImmediate`:** Instantly preempts lower priority audio playing on the same channel (e.g. `BASE_UNDER_ATTACK`, `ENEMY_SUPERWEAPON_FIRED`).
- **`Queue`:** Waits in line for current voice output to complete (e.g. `UNIT_READY_GENERIC`, `UPGRADE_COMPLETE`).
- **`DropIfBusy`:** Discarded if the channel or group is actively playing another line (e.g. `INSUFFICIENT_FUNDS`).

---

## 4. Master Event Catalog & Canonical Mapping

Below is the complete event taxonomy across all 32 EventId types (12 Frequent, 9 Rare, 11 Standard).

| EventId | FrequencyTier | Variants / Faction | Priority | Cooldown (s) | ConcurrencyGroup | InterruptPolicy | Canonical Line (Var 01) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `MATCH_START` | Rare | 2 | 1 | 0.0 | `CG_MatchFlow` | `NeverInterrupt` | **SU:** The command has been deployed. Industry awaits orders.<br>**AL:** The command network is active. all channels are protected.<br>**CO:** The command palace is connected to the network. Harmony is established.<br>**CH:** The causal chain is secured. This is available for editing. |
| `POWER_LOW` | Frequent | 4 | 2 | 10.0 | `CG_Economy` | `InterruptImmediate` | **SU:** The power grid is overloaded. Production is slowing down.<br>**AL:** Energy reserve is below safe level.<br>**CO:** Energy connections are unstable.<br>**CH:** Energy circuit is behind the timeline. |
| `BASE_UNDER_ATTACK` | Frequent | 4 | 2 | 5.0 | `CG_CombatAlert` | `InterruptImmediate` | **SU:** The enemy is attacking our production facilities.<br>**AL:** Attack on critical infrastructure detected.<br>**CO:** The enemy is compromising the integrity of our network.<br>**CH:** In the current future, the base is under attack. |
| `UNIT_READY_GENERIC` | Frequent | 4 | 3 | 2.0 | `CG_Production` | `Queue` | **SU:** The unit is ready to depart.<br>**AL:** The unit has completed training.<br>**CO:** The new unit has entered service.<br>**CH:** The unit is synchronized with the present. || `ENEMY_SUPERWEAPON_FIRED` | Rare | 2 | 1 | 0.0 | `CG_Superweapon` | `InterruptImmediate` | **SU:** Preparation of a strategic strike has been recorded.<br>**AL:** Strategic threat confirmed. Trajectory calculation in progress.<br>**CO:** Strategic disturbance detected.<br>**CH:** Mass casualty outcome recorded. The likelihood is growing. |
| `FACTION_RESOURCE_MAX` / `CRITICAL` | Rare | 2 | 2 | 15.0 | `CG_Intel` | `Queue` | **SU:** Mobilization complete. The army is ready for the general pressure.<br>**AL:** The complete intelligence package has been formed.<br>**CO:** The network has reached full synchronization.<br>**CH:** Temporal stability has been critically reduced. |
| `MATCH_VICTORY` | Rare | 2 | 1 | 0.0 | `CG_MatchFlow` | `NeverInterrupt` | **SU:** Resistance crushed. The territory comes under our control.<br>**AL:** The objectives of the operation have been achieved. Losses are within acceptable limits.<br>**CO:** The enemy is uncoordinated. The field has been stabilized.<br>**CH:** The enemy line of events has been completed. |
| `MATCH_DEFEAT` | Rare | 2 | 1 | 0.0 | `CG_MatchFlow` | `NeverInterrupt` | **SU:** Command network lost. Organized resistance has ceased.<br>**AL:** The control network has been destroyed. The operation has been terminated.<br>**CO:** Communication between nodes has been lost. The system has collapsed.<br>**CH:** This story option is no longer supported. || `INFANTRY_READY` | Frequent | 4 | 3 | 2.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `VEHICLE_READY` | Frequent | 4 | 3 | 2.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `AIRCRAFT_READY` | Frequent | 4 | 3 | 2.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `NAVAL_UNIT_READY` | Frequent | 4 | 3 | 2.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `CONSTRUCTION_COMPLETE` | Frequent | 4 | 3 | 2.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `INSUFFICIENT_FUNDS` | Frequent | 4 | 3 | 10.0 | `CG_Economy` | `DropIfBusy` | *Generated Faction Variants* |
| `HARVESTER_UNDER_ATTACK` | Frequent | 4 | 2 | 8.0 | `CG_CombatAlert` | `InterruptImmediate` | *Generated Faction Variants* |
| `COMMAND_CAP_REACHED` | Frequent | 4 | 3 | 10.0 | `CG_Economy` | `DropIfBusy` | *Generated Faction Variants* |
| `STEALTH_UNIT_DETECTED` | Frequent | 4 | 2 | 6.0 | `CG_Intel` | `InterruptImmediate` | *Generated Faction Variants* |
| `COMMAND_CENTER_DESTROYED` | Rare | 2 | 1 | 0.0 | `CG_MatchFlow` | `InterruptImmediate` | *Generated Faction Variants* |
| `MOBILE_COMMAND_UNIT_DESTROYED` | Rare | 2 | 1 | 0.0 | `CG_MatchFlow` | `InterruptImmediate` | *Generated Faction Variants* |
| `HERO_LOST` | Rare | 2 | 1 | 0.0 | `CG_MatchFlow` | `InterruptImmediate` | *Generated Faction Variants* |
| `OWN_SUPERWEAPON_FIRED` | Rare | 2 | 1 | 0.0 | `CG_Superweapon` | `InterruptImmediate` | *Generated Faction Variants* |
| `BUILDING_UNDER_ATTACK` | Standard | 3 | 2 | 5.0 | `CG_CombatAlert` | `DropIfBusy` | *Generated Faction Variants* |
| `STRUCTURE_SOLD` | Standard | 3 | 3 | 2.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `CONSTRUCTION_CANCELLED` | Standard | 3 | 3 | 2.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `UPGRADE_COMPLETE` | Standard | 3 | 3 | 3.0 | `CG_Production` | `Queue` | *Generated Faction Variants* |
| `SUPERWEAPON_READY` | Standard | 3 | 1 | 0.0 | `CG_Superweapon` | `InterruptImmediate` | *Generated Faction Variants* |
| `SUPERWEAPON_COUNTDOWN_STARTED` | Standard | 3 | 1 | 0.0 | `CG_Superweapon` | `InterruptImmediate` | *Generated Faction Variants* |
| `FACTION_ABILITY_READY` | Standard | 3 | 3 | 5.0 | `CG_Intel` | `Queue` | *Generated Faction Variants* |
| `FACTION_ABILITY_ACTIVATED` | Standard | 3 | 2 | 3.0 | `CG_Intel` | `Queue` | *Generated Faction Variants* |
| `ALLIED_BASE_UNDER_ATTACK` | Standard | 3 | 2 | 6.0 | `CG_CombatAlert` | `InterruptImmediate` | *Generated Faction Variants* |
| `RADAR_OFFLINE` | Standard | 3 | 2 | 5.0 | `CG_Intel` | `InterruptImmediate` | *Generated Faction Variants* |
| `RESOURCES_DEPLETED` | Standard | 3 | 3 | 10.0 | `CG_Economy` | `DropIfBusy` | *Generated Faction Variants* |

---

## 5. CSV Master File Details

The generated CSV manifest is located at `Content/RA4/Audio/Generated/eva_script_master.csv`.
- **Total Lines:** 396 dialogue entries (99 entries per faction across 4 factions).
- **Exact Schema:** `Faction,VoiceId,EventId,Variant,FrequencyTier,Priority,CooldownSeconds,ConcurrencyGroup,InterruptPolicy,ContextDescription,TextRu,SpokenTextRu,Emotion,Pace,Intensity,ControlInstruction,CanonicalSource,CanonicalPreserved,Status,Notes`
- **Canonical Verification:** All 32 canonical lines (Variant 01 of the 8 canonical events per faction) match `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` exactly, with `CanonicalPreserved=true` and `CanonicalSource=RA4_Bible_v2`.
