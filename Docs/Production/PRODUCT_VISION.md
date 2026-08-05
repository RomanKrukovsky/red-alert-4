# Product Vision & Strategic Positioning (`PRODUCT_VISION.md`)

**Document Version**: 2.1  
**Project Codename**: RA4  
**Commercial Working Title**: *Iron Resonance: Command of Tomorrow*  
**Genre**: Industrial Lockstep Real-Time Strategy (RTS)  
**Distribution Model**: Buy-to-Play (Steam / Epic Games Store) + Cosmetic-Only Seasonal Battle Pass  

---

## 1. Executive Product Vision

*Iron Resonance: Command of Tomorrow* is a next-generation industrial RTS designed to revive the golden era of base-building tactical strategy while solving the netcode, onboarding, and visual clutter issues of modern strategy games. Built on a deterministic fixed-tick C++ simulation engine (20 Hz sim, 60+ FPS presentation), it delivers zero-latency lockstep networking, deep asymmetrical faction gameplay, physics-based battlefield destruction, and native modding capabilities.

Its differentiating claim is **perception warfare**: the player commands their *belief* about the battlefield rather than the battlefield itself. Intelligence arrives late and decays, orders travel through destructible command infrastructure, the enemy can falsify the picture, the terrain remembers every battle, and the AI opponent studies the player's habits between matches. No competitor in the genre combines these; each is a deterministic, testable extension of the existing simulation core rather than a cosmetic feature.

---

## 2. Product Directions Comparison & Strategic Choice

To establish the strongest market position, three strategic product directions were evaluated:

| Product Direction | Target Persona | Strengths | Major Weaknesses | Strategic Verdict |
| :--- | :--- | :--- | :--- | :--- |
| **Direction A: Retro Replica** | 90s Nostalgia Fans | High initial nostalgia impulse; simple mechanics. | Small total addressable market; perceived as cheap clone; low retention. | **REJECTED** |
| **Direction B: Ultra-Hardcore eSports** | Twitch APM Competitors | High streaming visibility; eSports tournament potential. | Extreme onboarding barrier; alienates 80% of casual campaign/co-op players. | **REJECTED** |
| **Direction C: Modern Industrial RTS (SELECTED)** | Campaign + Competitive Strategy Players | Broad appeal; deterministic lockstep; deep asymmetry; high micro ceiling with accessible macro. | Requires rigorous initial faction balancing and engine determinism. | **SELECTED FOR PRODUCTION** |

### Justification for Direction C
Direction C leverages our existing deterministic C++ simulation core (479/479 passing headless tests as measured 2026-08-05, superseding the earlier 378 figure), providing eSports-grade lockstep precision while preserving rich campaign narrative and accessible macro controls for casual/solo players.

**Direction C is further specialized as perception warfare** (see `PERCEPTION_WARFARE_DIRECTION.md`). Without this specialization, Direction C is a well-engineered but familiar product whose pitch reduces to "solid netcode and four factions" — insufficient differentiation against established franchises. The perception layer converts an engineering advantage (a deterministic, engine-free simulation core that can hold per-player belief state honestly) into a player-facing genre claim that competitors built on presentation-coupled engines cannot easily copy.

---

## 3. Target Audience & User Personas

1. **The Campaign Strategist (Solo Player)**: Demands a rich 38-mission single-player campaign, memorable character dialogue, varied objective types (infiltration, base building, defense, convoy escort), and adaptive AI difficulty.
2. **The Competitive Ladder Climber (eSports Player)**: Demands reliable lockstep netcode (20 Hz sim tick with 60+ FPS interpolated presentation), zero input lag, transparent MMR matchmaking, crisp hitboxes, deterministic pathfinding, and instantaneous spectator replay tools. Ladder play defaults to `CommandNetwork = Classic` so competitive results are never attributed to order-propagation latency.
3. **The Base-Building Architect (Casual / Co-op)**: Enjoys resource harvesting, base optimization, power grid management, mega-structure defenses, and co-op skirmishes against AI.
4. **The Modding Community (Content Creator)**: Demands an in-engine map editor, custom trigger scripting, custom unit JSON definitions, and integrated Steam Workshop sharing.

---

## 4. Key Genre Problems Solved

- **Problem 1: Clunky Netcode & Desyncs**: Solved via pure C++ lockstep engine with tick-level 64-bit state hashing.
- **Problem 2: Excessive Visual Clutter**: Solved via clean silhouette art direction, readable team-color masking, and distinct projectile trajectories.
- **Problem 3: Superficial Faction Asymmetry**: Solved by giving each of the 4 factions distinct economic models, construction mechanics, and superweapon doctrines.
- **Problem 4: Pay-to-Win Microtransactions**: Solved by strict cosmetic-only monetization (unit skins, commander voice packs, custom badges).
- **Problem 5: Scouting Is a Solved Checkbox**: In modern RTS, vision is binary and permanent — once seen, a base is known forever, and scouting collapses into an opening ritual. Solved by the **perceived world**: intel arrives as reports with honest confidence values that decay over time, can be falsified by the enemy, and must be refreshed. Reconnaissance becomes a continuous strategic activity rather than a two-minute chore. (ADR-0021, ADR-0023, ADR-0026.)
- **Problem 6: Command Is Telepathic**: Elsewhere, orders reach any unit instantly regardless of what the enemy destroys. Solved by **command infrastructure**: orders propagate through player-built relays, so jamming and decapitation strikes have real tactical value, while a `Classic` toggle preserves conventional behaviour for competitive play. (ADR-0022.)
- **Problem 7: Battles Leave No Trace**: Most RTS maps reset to pristine geometry, so victories have no physical memory. Solved by **battlefield memory**: craters, wrecks, contamination and player-worn supply roads persist within a match and, in campaign, across missions. (ADR-0024.)

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
