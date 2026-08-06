# Gap Analysis & Issue Categorization (`GAP_ANALYSIS.md`)

**Audit Date**: August 4, 2026  
**Scope**: Complete evidence-based classification of project deficiencies, architectural gaps, and technical debt.

---

## Issue Classification Taxonomy

Issues are categorized into 6 distinct severity bands:
1. **Development Blockers** (Блокирующие разработку)
2. **Critical Issues** (Критические)
3. **Important Issues** (Важные)
4. **Medium Issues** (Средние)
5. **Cosmetic Issues** (Косметические)
6. **External Dependencies** (Внешние зависимости)

---

## 1. Development Blockers (Блокирующие разработку)

### GAP-01: NoesisGUI Unreal Engine Plugin Missing from Project Structure
- **Evidence**: `Plugins/` folder does not exist in the root directory. `RedAlert4.uproject` does not include `NoesisGUI` in the `"Plugins"` array. `Source/RA4UI/RA4UI.Build.cs` omit link dependencies for Noesis.
- **Affected Files**:
  - [`RedAlert4.uproject`](file://RedAlert4.uproject)
  - [`Source/RA4UI/RA4UI.Build.cs`](file://Source/RA4UI/RA4UI.Build.cs)
  - [`Source/RA4UI/Public/RA4NoesisHUDViewModel.h`](file://Source/RA4UI/Public/RA4NoesisHUDViewModel.h)
- **Impact**: Standard Unreal Build Tool (UBT) compilation fails when attempting to compile native Noesis GUI integration for Unreal Engine.
- **Recommended Remediation Order**: **Step 1**. Acquire and install NoesisGUI UE5 plugin into `Plugins/NoesisGUI` OR implement preprocessor guards (`#if WITH_NOESIS`) around Noesis ViewModels.

### GAP-02: Headless Test Data Path Sensitivity
- **Evidence**: Running `./build/hb/RA4Tests` inside `build/hb` directory fails 18 tests (`File.is_open()` returns false for relative path `Content/RA4/Data/Generated/ra4_content.normalized.json`).
- **Affected Files**:
  - [`Source/RA4Tests/Private/TestBibleImport.cpp#L16`](file://Source/RA4Tests/Private/TestBibleImport.cpp#L16)
  - [`Tools/HeadlessBuild/CMakeLists.txt`](file://Tools/HeadlessBuild/CMakeLists.txt)
- **Impact**: Developers running `RA4Tests` directly from `build/hb/` observe test failures, creating false bug reports.
- **Recommended Remediation Order**: **Step 2**. Add working-directory detection or fallback relative path resolution in `BibleContentLoader.cpp` to check `../..` when executed from `build/hb/`.

---

## 2. Critical Issues (Критические)

### GAP-03: Trademark Legal Risks (Command & Conquer / Red Alert / EA Terms)
- **Evidence**: Core project files, module names, and data assets use trademarked names (`RedAlert4`, `Soviet`, `Allied`, `Tiberium`, `EVA`).
- **Affected Files**:
  - [`RedAlert4.uproject`](file://RedAlert4.uproject)
  - [`RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md`](file://RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md)
  - [`Content/RA4/Data/Generated/ra4_content.normalized.json`](file://Content/RA4/Data/Generated/ra4_content.normalized.json)
- **Impact**: Exposes the project to immediate trademark infringement claims from Electronic Arts Inc. upon commercial release.
- **Recommended Remediation Order**: **Step 3**. Execute faction and identifier neutralization pass (e.g. `Red Star Union`, `Global Alliance`, `AURA`).

### GAP-04: Proprietary Font Licensing (`Druk Cyr`)
- **Evidence**: `Assets/Noesis/Themes/Typography.xaml` references proprietary font `Druk Cyr` (`pack://application:,,,/RA4;component/Content/RA4/UI/Fonts/#Druk Cyr`).
- **Affected Files**:
  - [`Assets/Noesis/Themes/Typography.xaml#L5`](file://Assets/Noesis/Themes/Typography.xaml#L5)
- **Impact**: Redistributing commercial font files without an active enterprise license creates copyright infringement liabilities.
- **Recommended Remediation Order**: **Step 4**. Replace `Druk Cyr` font references with open-source SIL OFL fonts (`Oswald`, `Inter`, `Bebas Neue`).

---

## 3. Important Issues (Важные)

### GAP-05: Tri-Layer UI Framework Duplication & Maintenance Split
- **Evidence**: Project contains 3 active UI layers: NoesisGUI (ViewModels in C++), Slate/UMG (`Content/RA4UI/`), and Web UI prototype (`ra4-ui/`).
- **Affected Files**:
  - [`Source/RA4UI/`](file://Source/RA4UI)
  - [`Content/RA4UI/`](file://Content/RA4UI)
  - [`ra4-ui/`](file://ra4-ui)
- **Impact**: UI bug fixes must be duplicated across multiple frontend implementations, splitting developer focus.
- **Recommended Remediation Order**: **Step 5**. Formally deprecate `ra4-ui` web prototype and finalize UMG vs Noesis production choice.

### GAP-06: Direct Control Possession Mode Integration Gap
- **Evidence**: `feat(input): implement Direct Control unit possession mode with F key` was merged in commit `efcad14`, but `ARA4PlayerController` does not expose a UI toggle prompt or active possession camera lock.
- **Affected Files**:
  - [`Source/RA4Input/Private/RA4InputRouter.cpp`](file://Source/RA4Input/Private/RA4InputRouter.cpp)
  - [`Source/RedAlert4/Private/RA4PlayerController.cpp`](file://Source/RedAlert4/Private/RA4PlayerController.cpp)
- **Impact**: Direct WASD unit possession mode remains dormant and unreachable for standard players.
- **Recommended Remediation Order**: **Step 6**. Wire `F` key possession toggle event to `RA4PlayerController` HUD prompt.

---

## 4. Medium Issues (Средние)

### GAP-07: Unused Legacy Blockout Mesh Duplication
- **Evidence**: `Content/RA4/Art/Blockout/` contains 142 FBX blockouts. 36 PBR production models were added in `Content/RA4/Art/`, leaving blockout assets unreferenced for vertical slice units.
- **Affected Files**:
  - [`Content/RA4/Art/Blockout/`](file://Content/RA4/Art/Blockout)
  - [`Source/RA4Presentation/Private/RA4ArtMapping.cpp`](file://Source/RA4Presentation/Private/RA4ArtMapping.cpp)
- **Impact**: Increases repository size and cooking build time unnecessarily.
- **Recommended Remediation Order**: **Step 7**. Consolidate `URA4ArtMapping` rules and clean up orphaned FBX files.

---

## 5. Cosmetic Issues (Косметические)

### GAP-08: Redundant Log Output in Headless AI Benchmark
- **Evidence**: Running `RA4AITests` prints verbose decision strategy logs during `AI.RecentDamageTriggersDefenceStrategy` execution.
- **Affected Files**:
  - [`Source/RA4AI/Private/AICommander.cpp`](file://Source/RA4AI/Private/AICommander.cpp)
- **Impact**: Slightly clutters test runner stdout logs.
- **Recommended Remediation Order**: **Step 8**. Guard verbose stdout logs behind `bEnableVerboseLogging` flag.

---

## 6. External Dependencies (Внешние зависимости)

### GAP-09: Packaged Game Build Automation Missing
- **Evidence**: No automated script (e.g. `build_package.sh` or `RunUAT.sh`) exists to produce shipping standalone executable packages.
- **Affected Files**:
  - [`build/`](file://build)
- **Impact**: Packaging shipping builds relies on manual Unreal Editor operations.
- **Recommended Remediation Order**: **Step 9**. Author automated UAT build script in `Tools/Build/package.sh`.
