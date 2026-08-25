# Project State Master Reference (`PROJECT_STATE.md`)

**Last Updated**: August 25, 2026
**Current Phase**: Pre-alpha; native RTS UI reference screens captured; real gameplay behaviour remains to be verified
**Primary Branch**: `main`

---

## 1. Project Overview & Quick Reference

Red Alert 4 (RA4) is a deterministic real-time strategy (RTS) game engine built on a pure C++ simulation kernel with an Unreal Engine **5.8** presentation layer. The workspace is `Scarlet-Horizon`; the project file is `RedAlert4.uproject`.

### UI reference-pass evidence recorded 2026-08-20

- All 24 canonical screens from `screenshots/` were opened and captured in Unreal at 1672×941.
- HUD references 13–16 and 20–24 were additionally exercised at 3440×1440.
- Native stack: CommonUI routing, UMG layout, Slate minimap/world markers, C++ event snapshots.
- Validator result: 24 reference contracts, 24 Widget Blueprints and 4 faction themes passed.
- Minimap first-paint measurement: 58.08 µs at 1,000 contacts and 224.66 µs at 5,000;
  zero per-contact widgets.
- This is evidence of reference-screen capture and UI-contract validation only. It does not verify real gameplay, minimap interaction, packaged builds, or commercial readiness.

### Primary Metrics
- **C++ Headless Test Suite**: use the current CMake/CTest output instead of a fixed historical count:
  `cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release`,
  `cmake --build build/headless --parallel`, then
  `ctest --test-dir build/headless --output-on-failure`.
- **Factions Defined**: 4 (Soviets, Alliance, Eastern Coalition, Chrono Legion).
- **Units in Data Bible**: 78 unique unit types (`ra4_content.normalized.json`).
- **Buildings Defined**: 35 structure types with power consumption/production values.
- **C++ Modules**: 16 modules in `Source/`.

---

## 2. Component Health Matrix

| Subsystem | Health Status | Key Classes / Files | Notes / Blockers |
| :--- | :--- | :--- | :--- |
| **Deterministic Command Bus** | **100% Functional** | `CommandBus.h`, `LockstepSession.h` | Frame isolation & input delay buffering. |
| **Simulation Core (`SimWorld`)** | **100% Functional** | `SimWorld.h`, `SimTypes.h` | Hashes state deterministically every tick. |
| **AI Commander** | **100% Functional** | `AICommander.h`, `HTNWorldState.h` | Covered by the headless CTest suite. Zero-cheat fog compliance. |
| **Input & WASD Camera** | **100% Functional** | `RA4InputRouter.h`, `RA4CameraComponent.h` | Covered by the headless CTest suite. WASD bounds clamped. |
| **Presentation Mapping** | **Functional** | `URA4PresentationSubsystem`, `URA4ArtMapping` | Maps entity IDs to skeletal/static meshes. |
| **UI Framework (CommonUI + UMG + Slate)** | **Functional** | `Content/RA4UI/Widgets/`, native presentation code | The only supported production UI stack: CommonUI routing, UMG layout, Slate minimap/world markers, and C++ event snapshots. |
| **Minimap / radar panel** | **Reference-screen evidence; real gameplay behaviour unverified** | `HudSnapshot.cpp`, `RA4SidebarWidget.cpp`, `FogOfWarGrid.cpp` | The recorded reference pass supports the visual UI contract. Runtime radar contacts, blackout handling, drag-pan, right-click orders, camera frame, and alert pings still need explicit PIE/gameplay validation. The code was independently reviewed and fixes were recorded for a radar maphack and an unbounded fog leak. |

---

## 3. Phase 1 Audit Reports Sitemap (`Docs/Audit/`)

- [`CURRENT_STATE.md`](../Audit/CURRENT_STATE.md): Executive summary and overall project health baseline.
- [`ARCHITECTURE_AUDIT.md`](../Audit/ARCHITECTURE_AUDIT.md): C++ module breakdown, sim vs presentation separation, lockstep determinism.
- [`GAMEPLAY_AUDIT.md`](../Audit/GAMEPLAY_AUDIT.md): Harvesters, camera controls, placement grid, combat, pathfinding, victory/defeat.
- [`UNREAL_INTEGRATION_AUDIT.md`](../Audit/UNREAL_INTEGRATION_AUDIT.md): `.uproject`, build targets, CMake headless harness, CI workflows, packaging status.
- [`UI_AUDIT.md`](../Audit/UI_AUDIT.md): Historical pre-remediation UI audit; current stack is documented above.
- [`AI_AUDIT.md`](../Audit/AI_AUDIT.md): `AICommander` utility strategy loop, difficulty profiles, fog-of-war zero-cheat policy.
- [`CONTENT_AUDIT.md`](../Audit/CONTENT_AUDIT.md): Data bible, 3D models, voice lines, maps, and visual effects.
- [`ASSET_AND_LICENSE_AUDIT.md`](../Audit/ASSET_AND_LICENSE_AUDIT.md): Legal audit, trademark usage, and asset-license risks.
- [`BUILD_AND_TEST_AUDIT.md`](../Audit/BUILD_AND_TEST_AUDIT.md): Historical UBT/CMake harness and timing audit.
- [`GIT_REGRESSION_AUDIT.md`](../Audit/GIT_REGRESSION_AUDIT.md): History analysis, merged feature branches, and disconnected features.
- [`GAP_ANALYSIS.md`](../Audit/GAP_ANALYSIS.md): Categorized issue register (Blocker, Critical, Important, Medium, Cosmetic, External).
- [`RISK_REGISTER.md`](../Production/RISK_REGISTER.md): Legal/IP, architectural, UI, build, and team handoff risks.

---

## 4. Key Rules for Autonomous Agents

1. **Maintain Determinism**: Never introduce non-deterministic C++ operations (e.g. `std::rand()`, unseeded engine floats, pointer-address hashing) into `Source/RA4Simulation`, `Source/RA4Core`, or `Source/RA4AI`.
2. **Execute Tests Through CTest**: From the workspace root, configure `Tools/HeadlessBuild`, build it, then run `ctest --test-dir build/headless --output-on-failure`.
3. **Respect IP Neutralization Rules**: Use safe faction identifiers (`Red Star Union`, `Global Alliance`, `AURA`) for new feature development.
