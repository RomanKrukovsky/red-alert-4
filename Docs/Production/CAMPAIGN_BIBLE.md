# Single-Player & Co-op Campaign Design (`CAMPAIGN_BIBLE.md`)

**Document Version**: 2.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  
**Mission Count**: 38 Data-Driven Missions across 4 Chapters  
**Co-op Support**: 2-Player Co-op Campaign Mode  

---

## 1. Campaign Structure Overview

The campaign is structured as an epic four-part narrative arc, allowing players to experience the global Aethelite conflict from the perspective of all four major factions.

```
  +-----------------------+     +-----------------------+
  |  CHAPTER 1: RSU       | --> |  CHAPTER 2: GDC       |
  |  (10 Missions)        |     |  (10 Missions)        |
  +-----------------------+     +-----------------------+
              |                             |
              v                             v
  +-----------------------+     +-----------------------+
  |  CHAPTER 3: PAS       | --> |  CHAPTER 4: TRO       |
  |  (9 Missions)         |     |  (9 Missions)         |
  +-----------------------+     +-----------------------+
```

---

## 2. Chapter Breakdown & Narrative Arcs

### Chapter 1: Red Star Union (RSU) — *Iron Dawn* (10 Missions)
- **Protagonist**: General Morozova.
- **Narrative Arc**: RSU forces launch a surprise offensive to secure the Arctic Aethelite fields. Morozova uncovers a secret plot by rogue PAS infiltrators attempting to trigger a global power grid collapse.
- **Key Mission**: Mission 1 (*Sokolov Demonstration*) introduces basic base building and heavy armored tank tactics.

### Chapter 2: Global Defense Coalition (GDC) — *Shield of Freedom* (10 Missions)
- **Protagonist**: Commander Hart.
- **Narrative Arc**: GDC mobilizes to defend Western Europe from RSU expansion. Hart uses precision air strikes and stealth infiltration to neutralize RSU Tesla batteries.

### Chapter 3: Pan-Asian Syndicate (PAS) — *Shadow Protocol* (9 Missions)
- **Protagonist**: Cybernetic Commander Mei.
- **Narrative Arc**: Operating from hidden subterranean facilities, PAS executes covert sabotage operations against both RSU and GDC, seeking to capture an experimental TRO Temporal Transposer.

### Chapter 4: Temporal Resonance Order (TRO) — *Chrono Convergence* (9 Missions)
- **Protagonist**: Time Commander Voss.
- **Narrative Arc**: TRO emerges from temporal isolation to prevent an impending Aethelite cataclysm capable of destroying Earth's atmosphere. Voss unites key factions in a final battle against rogue automated AI defense networks.

---

## 3. Data-Driven Objective System

Missions are specified as structured JSON data loaded by `RA4Campaign`:
- **Primary Objectives**: Must be completed to pass the mission (e.g. `Destroy Enemy Headquarters`, `Survive for 15 Minutes`).
- **Secondary Objectives**: Optional tactical goals granting bonus resources or unit reinforcements (e.g. `Capture Tech Hospital`, `Destroy Secondary Radar Array`).
- **Bonus Objectives**: High-difficulty achievements rewarding player profile XP and cosmetic campaign badges.

---

## 4. Co-op Campaign Execution

- **2-Player Co-op**: All 38 campaign missions support 2-player online co-op via lockstep networking.
- **Co-op Resource Modes**:
  1. *Shared Base*: Players share a single base build queue and credit pool.
  2. *Split Base*: Each player controls their own base footprint and army commander on the same tactical map.
