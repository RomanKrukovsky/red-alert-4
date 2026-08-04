# Skeptical Design & Technical Feasibility Review (`SKEPTICAL_REVIEW.md`)

**Review Date**: August 4, 2026  
**Audited Documents**: `PRODUCT_VISION.md`, `ORIGINAL_IP_MIGRATION.md`, `GAME_DESIGN_DOCUMENT.md`, `FACTION_BIBLE.md`, `CAMPAIGN_BIBLE.md`, `MULTIPLAYER_DESIGN.md`, `MODDING_AND_EDITOR_DESIGN.md`, `LEGAL_AND_LICENSES.md`  

---

## 1. Executive Review Summary

This skeptical review evaluates the production design documentation for potential internal contradictions, feature bloat (scope creep), technical infeasibility, and residual IP risks.

---

## 2. Detailed Findings & Mitigation Actions

### A. Internal Contradictions

- **Finding 1: UI Framework Split (NoesisGUI vs UMG vs Web UI)**
  - *Contradiction*: `UI_AUDIT.md` and `PRODUCT_VISION.md` reference NoesisGUI as the primary UI, while `UNREAL_INTEGRATION_AUDIT.md` notes NoesisGUI plugin is missing and UMG widgets are used in current builds.
  - *Remedy*: Clearly designate UMG as the fallback active UI for Unreal Engine builds until NoesisGUI plugin binaries are integrated into `Plugins/NoesisGUI`.

- **Finding 2: Harvester Speed vs Map Scale**
  - *Contradiction*: `GAME_DESIGN_DOCUMENT.md` specifies 20-minute match pacing, but RSU heavy harvester speed (180 units/sec) would make long-distance mining inefficient on 2v2 maps.
  - *Remedy*: Adjust RSU heavy harvester capacity from 500 to 750 credits and increase base movement speed by 15%.

---

### B. Technical Feasibility & Performance Risks

- **Finding 3: Lockstep Replay Seeking Overhead**
  - *Risk*: Seeking forward in a lockstep replay requires re-simulating ticks from tick 0 up to target tick. For a 40-minute match (144,000 ticks), unoptimized re-simulation could freeze playback.
  - *Remedy*: `SimWorld` snapshots are saved into replay memory every 1,800 ticks (30 seconds), enabling instant seeking to nearest snapshot checkpoint.

- **Finding 4: Destructible Terrain Grid Collision**
  - *Risk*: Destroyable terrain objects changing navigation grid state dynamically during live match.
  - *Remedy*: `RA4Navigation` flowfield updates dynamically on `SimEventType::BuildingPlaced` and `BuildingDestroyed` events without full grid recalculation.

---

### C. Scope Creep & Production Reality Check

- **Finding 5: 38 Campaign Missions Baseline**
  - *Scope Risk*: Authoring 38 custom maps with full voice acting represents a major content pipeline load for Version 1.0.
  - *Remedy*: Implement data-driven mission templates (`RA4Campaign`) reusing skirmish map layouts for secondary chapters while reserving custom cinematic maps for core story missions.

---

## 3. Final Skeptical Review Verdict

> [!TIP]
> The Phase 2 production design documents are **APPROVED FOR PRODUCTION AGENT DEVELOPMENT**, subject to enforcing the remediation steps above.
