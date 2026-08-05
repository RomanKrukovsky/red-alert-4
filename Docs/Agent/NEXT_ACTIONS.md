# Agent Active Action Queue

**Last Updated**: 2026-08-05
**Current Milestone Target**: Functional Skirmish (Vertical Slice → Playable Build)

---

## Milestone Status (Honest Assessment)

| # | Milestone | Status | Evidence |
|---|-----------|--------|----------|
| 1 | Architecture Baseline | **PASS** | Engine-free core solid, 23 ADRs, all tests pass |
| 2 | Industrial Vertical Slice | **PASS** | Headless full-match deterministic, replay works |
| 3 | Systems Complete | **PASS** (headless only) | 308 tests, stress to 2000 entities |
| 4 | Content Complete | **BLOCKED** | 78 unit types defined, visual assets are placeholders |
| 5 | Feature Complete | **FAIL** | Core gameplay works headless, UE integration unverified |
| 6 | Alpha | **FAIL** | No packaged build, no visual verification |
| 7 | Beta | **FAIL** | Cannot proceed without Alpha |
| 8 | Release Candidate | **FAIL** | Cannot proceed without Beta |
| 9 | Gold Master | **FAIL** | Cannot proceed without RC |
| 10 | Launch Readiness | **FAIL** | Cannot proceed without Gold Master |

---

## Priority Queue (Next Allowed Actions)

### P0: Foundation (unblocks everything else)
- [ ] Verify UE editor integration: simulation drives visual layer
- [ ] Fix OpponentModel (.cpp implementation needed)
- [ ] Create packaged Shipping build script

### P1: Content Pipeline
- [ ] Audit ThirdParty asset licenses for redistribution
- [ ] Begin IP migration planning ("Red Alert 4" → original name)
- [ ] Replace placeholder art with blockout/PBR models

### P2: Gameplay Polish
- [ ] Author first campaign mission
- [ ] Extend CI to build UE targets
- [ ] Add reconnect and spectator to lockstep tests
- [ ] Implement missing command types (UpgradeBuilding, DeployMCV, SetStance)

### P3: External Dependencies (cannot be solved by code alone)
- [ ] Commission voice acting for EVA lines
- [ ] Compose/source music
- [ ] Commission final 3D art
- [ ] Human localization QA for en/ru
- [ ] Mass playtesting for balance

---

## Execution Guidelines

- **Current Branch**: `feat/soviet-asset-integration`
- **Do NOT commit to main directly**
- **Verify changes**: `cmake --build build && ctest --test-dir build`
- **Commit format**: `type(scope): short description`
- **No fabricated completion claims** — only record verified facts
