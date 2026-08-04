# Original IP Migration & Trademark Neutralization Plan (`ORIGINAL_IP_MIGRATION.md`)

**Document Version**: 2.0  
**Project Codename**: RA4  
**Commercial Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Legal IP Separation Strategy

To ensure zero risk of trademark or copyright infringement claims from Electronic Arts Inc. (EA) or third parties, all protected identifiers, names, lore, and visual trademarks originating from the *Command & Conquer* / *Red Alert* franchise are systematically replaced with original, commercially safe intellectual property.

> [!IMPORTANT]
> **Legal Disclaimer**: This document establishes internal production design guidelines for IP replacement. Final legal clearance of title candidates, logos, and narrative text must be executed by specialized intellectual property legal counsel prior to commercial registration.

---

## 2. World Lore & Conflict Narrative

### The Universe: *The Great Resonance Era (2099)*
In the late 21st century, the discovery of **Aethelite**—a rare crystalline substrate exhibiting quantum resonance properties—revolutionized energy production and military technology. As Earth's traditional natural resources depleted, global super-states fractured into four hostile geopolitical blocs vying for control of Aethelite deposits and temporal distortion technology.

---

## 3. Commercial Name Candidates Evaluation

| Title Candidate | Legal Availability Assessment | Brand Strength | Verdict |
| :--- | :--- | :--- | :--- |
| **Candidate 1: Iron Resonance: Command of Tomorrow** | High (Unique combination) | Strong industrial military vibe | **SELECTED TITLE** |
| **Candidate 2: Chrono Vanguard: Global Escalation** | Medium ("Chrono" widely used) | Strong sci-fi RTS feel | Secondary Candidate |
| **Candidate 3: Tectonic Supremacy** | High | Industrial feel | Reserve Title |
| **Candidate 4: Aethelgard: Fractured World** | High | Fantasy leaning | Rejected |
| **Candidate 5: Vanguard 2099: Eclipse of Nations** | Medium | Modern action feel | Reserve Title |

---

## 4. Complete Terminology Translation Matrix

| Category | Legacy Prototype Term (C&C) | New Original IP Term | Narrative / Context Definition |
| :--- | :--- | :--- | :--- |
| **Project Title** | `RedAlert4` / `Red Alert 4` | *Iron Resonance* | Commercial product title. |
| **Faction 1** | `Soviet` / `Soviet Union` | **Red Star Union (RSU)** | Heavy industrial armored collective using kinetic firepower and Tesla resonance. |
| **Faction 2** | `Alliance` / `Allies` | **Global Defense Coalition (GDC)** | High-tech allied coalition utilizing precision optics, air superiority, and energy shields. |
| **Faction 3** | `Eastern Coalition` | **Pan-Asian Syndicate (PAS)** | Cybernetic swarm coalition utilizing stealth, automated drones, and subterranean mining. |
| **Faction 4** | `Chrono Legion` | **Temporal Resonance Order (TRO)** | Advanced faction utilizing quantum phase shifts, temporal stasis, and teleportation. |
| **Primary Resource**| `Ore` / `Tiberium` | **Aethelite Substrate** | Energy-dense crystalline mineral harvested from ground nodes. |
| **Audio Announcer** | `EVA` | **AURA** | *Automated Tactical Reconnaissance & Reaction Assistant*. |
| **Iconic Unit 1** | `Apocalypse Tank` | **RSU Mammoth Siege Engine** | Quad-track heavy assault fortress tank. |
| **Iconic Unit 2** | `Kirov Airship` | **RSU Dreadnought Leviathan** | Heavy armored atmospheric siege airship. |
| **Iconic Unit 3** | `Chrono Legionnaire` | **TRO Phase Stasis Trooper** | Infantry equipped with temporal freezing beam rifle. |

---

## 5. Code Migration Strategy

To avoid breaking existing simulation tests (`RA4Tests`, `RA4AITests`) during Phase 2 design authoring:
1. **Phase 2 (Current)**: Document all replacement names in production design bibles (`FACTION_BIBLE.md`, `GAME_DESIGN_DOCUMENT.md`). Code enums (`FactionId::Soviet`) remain intact in C++ source to preserve 378/378 test stability.
2. **Phase 3 (Migration Pass)**: Execute systematic refactoring of C++ symbols, JSON schema keys, and asset paths using string mapping scripts.
