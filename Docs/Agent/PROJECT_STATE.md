# Project State Master Reference (`PROJECT_STATE.md`)

**Last Updated**: August 4, 2026  
**Current Phase**: Phase 1 Complete (Full Project Audit & Baseline Verification)  
**Main Branch**: `main`  
**Latest Commit Hash**: `d0b7813`  

---

## 1. Project Overview & Quick Reference

Red Alert 4 (RA4) is a deterministic real-time strategy (RTS) game engine built on a pure C++ simulation kernel with an Unreal Engine 5 presentation layer.

### Primary Metrics
- **C++ Headless Test Suite**: **378 passed, 0 failed** (5.616s runtime).
  - `RA4Tests`: 258/258 PASS
  - `RA4AITests`: 46/46 PASS
  - `RA4InputTests`: 51/51 PASS
  - `RA4PresentationTests`: 23/23 PASS
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
| **AI Commander** | **100% Functional** | `AICommander.h`, `HTNWorldState.h` | 46/46 tests pass. Zero-cheat fog compliance. |
| **Input & WASD Camera** | **100% Functional** | `RA4InputRouter.h`, `RA4CameraComponent.h` | 51/51 input tests pass. WASD bounds clamped. |
| **Presentation Mapping** | **Functional** | `URA4PresentationSubsystem`, `URA4ArtMapping` | Maps entity IDs to skeletal/static meshes. |
| **UI Framework (NoesisGUI)** | **Blocked** | `RA4NoesisHUDViewModel.h` | Missing `Plugins/NoesisGUI` in repo. |
| **UI Framework (UMG)** | **Functional** | `Content/RA4UI/Widgets/` | Native Unreal UMG fallback widgets. |
| **Web UI Prototype** | **Functional Prototype** | `ra4-ui/` | React/Vite web application (`npm run build` PASS). |

---

## 3. Phase 1 Audit Reports Sitemap (`Docs/Audit/`)

- [`CURRENT_STATE.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/CURRENT_STATE.md): Executive summary and overall project health baseline.
- [`ARCHITECTURE_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/ARCHITECTURE_AUDIT.md): C++ module breakdown, sim vs presentation separation, lockstep determinism.
- [`GAMEPLAY_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/GAMEPLAY_AUDIT.md): Harvesters, camera controls, placement grid, combat, pathfinding, victory/defeat.
- [`UNREAL_INTEGRATION_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/UNREAL_INTEGRATION_AUDIT.md): `.uproject`, build targets, CMake headless harness, CI workflows, packaging status.
- [`UI_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/UI_AUDIT.md): Tri-layer UI audit (NoesisGUI, Slate/UMG, `ra4-ui`).
- [`AI_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/AI_AUDIT.md): `AICommander` utility strategy loop, difficulty profiles, fog-of-war zero-cheat policy.
- [`CONTENT_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/CONTENT_AUDIT.md): Data bible, 3D models (142 blockout + 36 PBR), 624 voice lines, maps, Niagara VFX.
- [`ASSET_AND_LICENSE_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/ASSET_AND_LICENSE_AUDIT.md): Legal audit, C&C trademark usage, 3D/audio asset licenses, `Druk Cyr` font risk.
- [`BUILD_AND_TEST_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/BUILD_AND_TEST_AUDIT.md): UBT vs CMake build harness, 378 test suite inventory, timing benchmarks.
- [`GIT_REGRESSION_AUDIT.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/GIT_REGRESSION_AUDIT.md): History analysis, merged feature branches, disconnected features (Noesis plugin missing, direct control).
- [`GAP_ANALYSIS.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Audit/GAP_ANALYSIS.md): Categorized issue register (Blocker, Critical, Important, Medium, Cosmetic, External).
- [`RISK_REGISTER.md`](file:///Users/romanmolodyko/Documents/red-alert-4/Docs/Production/RISK_REGISTER.md): Legal/IP, architectural, UI, build, and team handoff risks.

---

## 4. Key Rules for Autonomous Agents

1. **Maintain Determinism**: Never introduce non-deterministic C++ operations (e.g. `std::rand()`, unseeded engine floats, pointer-address hashing) into `Source/RA4Simulation`, `Source/RA4Core`, or `Source/RA4AI`.
2. **Execute Tests From Root**: Always execute C++ test binaries (`./build/hb/RA4Tests`, etc.) with current working directory set to the project root directory.
3. **Respect IP Neutralization Rules**: Use safe faction identifiers (`Red Star Union`, `Global Alliance`, `AURA`) for new feature development.
