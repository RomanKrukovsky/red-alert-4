# Product Vision & Strategic Positioning (`PRODUCT_VISION.md`)

**Document Version**: 2.0  
**Project Codename**: RA4  
**Commercial Working Title**: *Iron Resonance: Command of Tomorrow*  
**Genre**: Industrial Lockstep Real-Time Strategy (RTS)  
**Distribution Model**: Buy-to-Play (Steam / Epic Games Store) + Cosmetic-Only Seasonal Battle Pass  

---

## 1. Executive Product Vision

*Iron Resonance: Command of Tomorrow* is a next-generation industrial RTS designed to revive the golden era of base-building tactical strategy while solving the netcode, onboarding, and visual clutter issues of modern strategy games. Built on a deterministic 60Hz C++ simulation engine, it delivers zero-latency lockstep networking, deep asymmetrical faction gameplay, physics-based battlefield destruction, and native modding capabilities.

---

## 2. Product Directions Comparison & Strategic Choice

To establish the strongest market position, three strategic product directions were evaluated:

| Product Direction | Target Persona | Strengths | Major Weaknesses | Strategic Verdict |
| :--- | :--- | :--- | :--- | :--- |
| **Direction A: Retro Replica** | 90s Nostalgia Fans | High initial nostalgia impulse; simple mechanics. | Small total addressable market; perceived as cheap clone; low retention. | **REJECTED** |
| **Direction B: Ultra-Hardcore eSports** | Twitch APM Competitors | High streaming visibility; eSports tournament potential. | Extreme onboarding barrier; alienates 80% of casual campaign/co-op players. | **REJECTED** |
| **Direction C: Modern Industrial RTS (SELECTED)** | Campaign + Competitive Strategy Players | Broad appeal; deterministic lockstep; deep asymmetry; high micro ceiling with accessible macro. | Requires rigorous initial faction balancing and engine determinism. | **SELECTED FOR PRODUCTION** |

### Justification for Direction C
Direction C leverages our existing deterministic C++ simulation core (378/378 passing unit tests), providing eSports-grade lockstep precision while preserving rich campaign narrative and accessible macro controls for casual/solo players.

---

## 3. Target Audience & User Personas

1. **The Campaign Strategist (Solo Player)**: Demands a rich 38-mission single-player campaign, memorable character dialogue, varied objective types (infiltration, base building, defense, convoy escort), and adaptive AI difficulty.
2. **The Competitive Ladder Climber (eSports Player)**: Demands 60Hz lockstep netcode, zero input lag, transparent MMR matchmaking, crisp hitboxes, deterministic pathfinding, and instantaneous spectator replay tools.
3. **The Base-Building Architect (Casual / Co-op)**: Enjoys resource harvesting, base optimization, power grid management, mega-structure defenses, and co-op skirmishes against AI.
4. **The Modding Community (Content Creator)**: Demands an in-engine map editor, custom trigger scripting, custom unit JSON definitions, and integrated Steam Workshop sharing.

---

## 4. Key Genre Problems Solved

- **Problem 1: Clunky Netcode & Desyncs**: Solved via pure C++ lockstep engine with tick-level 64-bit state hashing.
- **Problem 2: Excessive Visual Clutter**: Solved via clean silhouette art direction, readable team-color masking, and distinct projectile trajectories.
- **Problem 3: Superficial Faction Asymmetry**: Solved by giving each of the 4 factions distinct economic models, construction mechanics, and superweapon doctrines.
- **Problem 4: Pay-to-Win Microtransactions**: Solved by strict cosmetic-only monetization (unit skins, commander voice packs, custom badges).

---

## 5. Commercial Positioning & Monetization Model

- **Base Product**: Premium Buy-to-Play ($39.99 USD) including full 38-mission campaign, 4 asymmetric factions, offline/online skirmish, and map editor.
- **Monetization (Zero Pay-to-Win)**:
  - Seasonal Cosmetic Battle Pass (Battlefield visual themes, unit decals, victory banners).
  - Commander Voice Packs (Alternate EVA voice announcers).
  - Cosmetic Faction Skin Sets.
- **Scope Boundaries for Version 1.0**:
  - 4 Asymmetric Factions.
  - 78 Unique Units & 35 Structures.
  - 38 Campaign Missions across 4 Chapters.
  - 1v1, 2v2, 3v3 Ranked & Unranked Skirmish Maps.
  - Full Headless Dedicated Server & Replay System.
  - Built-in Map Editor & Custom Mission Scripting.
