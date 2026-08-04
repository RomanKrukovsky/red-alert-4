# Git Commit History & Regression Audit (`GIT_REGRESSION_AUDIT.md`)

**Audit Date**: August 4, 2026  
**Repository Branch**: `main`  
**Latest Commit**: `d0b7813` (`fix(slice): reconnect selection, presentation, combat visuals and attack-move`)  

---

## 1. Merged Feature Branches & Commit Lineage

The repository history reveals major parallel feature integration passes:
- `35a8613`: `merge(skirmish): merge integration/skirmish-final into main`
  - Merged sub-agent branches: `agents/skirmish-gameplay`, `agents/skirmish-ai`, `agents/skirmish-art`, `agents/skirmish-map`, `agents/skirmish-ui`.
- `2fb5c9c`: `merge: integrate NoesisGUI C++ architecture and XAML assets into main`
- `7930dc1`: `merge: integrate PBR 3D models and artwork into main`

---

## 2. Regression & Disconnected Feature Audit

### A. Missing NoesisGUI Engine Plugin (Commit `2a8ae8b`, `cc8e68c`)
- **What Was Added**: C++ ViewModels (`RA4NoesisHUDViewModel`), Navigation Service, Input Router, and XAML assets.
- **What Was Lost/Omitted**: The `Plugins/NoesisGUI` plugin binaries/headers were not included in the merge to `main`.
- **Impact**: UBT build errors occur when building `RA4UI` for Unreal Engine unless preprocessor guards or Noesis plugin is supplied.

### B. Dual UI Architecture (Commit `f1c8f8e` vs `2a8ae8b`)
- **What Happened**: Before migrating to NoesisGUI, a React/Vite Web UI prototype was developed in `ra4-ui/` (checkpoint `f1c8f8e`).
- **Current State**: Both `ra4-ui/` and NoesisGUI C++ ViewModels exist side-by-side in `main`, creating architectural duality.

### C. Direct Control Possession Mode (`F` Key) (Commit `efcad14`)
- **What Was Implemented**: `feat(input): implement Direct Control unit possession mode with F key` allowing direct WASD unit movement.
- **Current State**: C++ logic exists in `RA4Input`, but input binding in UE `APlayerController` requires explicit activation.

### D. Blockout Models vs PBR Replacement (Commits `c0df9c2` vs `af513bb`)
- **What Happened**: 142 FBX blockouts were imported into `Content/RA4/Art/Blockout/`. Later, 36 PBR models were integrated in `af513bb`.
- **Current State**: `URA4ArtMapping` supports fallback from PBR mesh to Blockout mesh, but unused blockout assets add repository weight.

---

## 3. History Commit Log (Key Milestones)

| Commit Hash | Author / Message | Feature / Component | Status in Main |
| :--- | :--- | :--- | :--- |
| `d0b7813` | `fix(slice): reconnect selection, presentation, combat visuals` | Visual Selection | **Active** |
| `68a4334` | `tests: run from the repository root regardless of launch directory` | Test Suite | **Active** |
| `0915e30` | `docs(adr): record the multiplayer scope decision` | LAN Lockstep Scope | **Active** |
| `08fc128` | `feat(campaign): author missions as data with runtime-checkable objectives` | Campaign System | **Active** |
| `959368c` | `feat(net): drive the simulation from the lockstep session` | Lockstep Net | **Active** |
| `cc8e68c` | `feat(ui): add C++ ViewModels... correct audit reports to BLOCKED_PLUGIN_MISSING` | UI ViewModels | **Active (Blocked)** |
| `efcad14` | `feat(input): implement Direct Control unit possession mode with F key` | Direct Control | **Dormant** |
| `a7fbfca` | `feat(ui): implement in-game cheat console with C&C cheat codes` | Cheat Console | **Active** |
