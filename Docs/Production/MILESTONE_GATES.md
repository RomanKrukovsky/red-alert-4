# Milestone Gates & Binary Acceptance Criteria (`MILESTONE_GATES.md`)

**Document Version**: 4.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Milestone Gate Philosophy

Production progress is evaluated strictly through **Binary Pass/Fail Milestone Gates**. 
Phrases such as *"almost finished"*, *"90% ready"*, or *"minor unresolved bugs"* are **EXPLICITLY BANNED**. A gate is either 100% PASSED or FAILED.

---

## 2. The 11 Mandatory Milestone Gates

### Gate 1: Architecture Baseline
- **Status**: **PASSED** (Aug 4, 2026).
- **Binary Criteria**:
  - [X] Pure C++ simulation kernel builds cleanly with 0 compilation errors.
  - [X] 378 / 378 headless C++ unit tests pass (100% pass rate).
  - [X] All 16 module boundaries defined in `MODULE_BOUNDARIES.md`.
  - [X] Working directory relative path resolution verified for test suite data loading.

### Gate 2: Industrial Vertical Slice
- **Binary Criteria**:
  - [ ] 1 fully playable 1v1 skirmish map (`M_Skirmish_Desert`) running end-to-end in UE5.
  - [ ] 2 playable factions (Red Star Union vs Global Defense Coalition) with 10 units each.
  - [ ] Lockstep network 1v1 match completes 15 minutes without desync or frame drop below 60 FPS.
  - [ ] UMG HUD widgets render resource bar, minimap, and build cards from live `SimWorld` snapshots.

### Gate 3: Systems Complete
- **Binary Criteria**:
  - [ ] All 4 asymmetric factions implemented in C++ simulation kernel.
  - [ ] Superweapons, veterancy ranks, and destructible environment triggers functional in `SimWorld`.
  - [ ] AI Commander completes skirmish matches across all 3 difficulty profiles (Easy, Medium, Hard).

### Gate 4: Content Complete
- **Binary Criteria**:
  - [ ] All 78 unit 3D PBR models and 35 building meshes integrated into `Content/RA4/Art/`.
  - [ ] 38 campaign missions authored as valid data JSONs in `Content/RA4/Data/`.
  - [ ] 624 voice lines and SFX audio events integrated into `Audio/`.

### Gate 5: Feature Complete
- **Binary Criteria**:
  - [ ] All single-player, co-op, 1v1/2v2/3v3 ranked, replay, and map editor features fully playable.
  - [ ] Zero missing gameplay features from GDD v2.0.

### Gate 6: Alpha
- **Binary Criteria**:
  - [ ] Code freeze for new features.
  - [ ] 100 consecutive automated 8-player AI matches complete without desync or memory leaks.

### Gate 7: Beta
- **Binary Criteria**:
  - [ ] External closed beta playtest executed with >500 multiplayer matches.
  - [ ] Zero crash reports in telemetry log over 10,000 player-hours.

### Gate 8: Release Candidate (RC)
- **Binary Criteria**:
  - [ ] Final legal clearance sign-off received from IP counsel.
  - [ ] Platform certification (Steam / EGS) approved.

### Gate 9: Gold Master
- **Binary Criteria**:
  - [ ] Final shipping build packaged via UAT with Pak file encryption.

### Gate 10: Launch Readiness
- **Binary Criteria**:
  - [ ] Dedicated server clusters deployed and load-tested for 10,000 concurrent players.

### Gate 11: Post-Launch Baseline
- **Binary Criteria**:
  - [ ] Day 1 telemetry monitored; 24-hour hotfix deployment pipeline verified active.
