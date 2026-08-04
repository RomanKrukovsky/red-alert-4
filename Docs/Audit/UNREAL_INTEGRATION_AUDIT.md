# Unreal Engine Integration & Build System Audit (`UNREAL_INTEGRATION_AUDIT.md`)

**Audit Date**: August 4, 2026  
**Unreal Target**: UE 5.8  
**Build Harness**: Dual UBT / CMake Headless System  

---

## 1. Project Configuration & `.uproject` Analysis

File: `RedAlert4.uproject`

```json
{
	"FileVersion": 3,
	"EngineAssociation": "5.8",
	"Category": "Strategy",
	"Description": "Red Alert 4 - real-time strategy on a deterministic C++ core."
}
```

### Configured Plugins Analysis
- `GameplayAbilities` (Enabled)
- `CommonUI` (Enabled)
- `ModelViewViewModel` (Enabled)
- `EnhancedInput` (Enabled)
- `FunctionalTestingEditor` (Enabled)
- `PythonScriptPlugin` (Enabled)
- `EditorScriptingUtilities` (Enabled)
- `ModelContextProtocol` (Enabled)
- `AllToolsets` (Enabled)
- `ToolsetRegistry` (Enabled)

**Critical Finding**: `NoesisGUI` plugin is **NOT listed** in `.uproject` plugins array, and the project root lacks a `Plugins/` folder. However, `Source/RA4UI` contains `RA4NoesisHUDViewModel.h/cpp` which were authored for Noesis XAML binding.

---

## 2. C++ Target Files (`Source/*.Target.cs`)

1. **`RedAlert4.Target.cs`**: Standard Desktop Game target. Defines build rules for standalone game client.
2. **`RedAlert4Editor.Target.cs`**: Editor target. Configures Unreal Editor extensions (`RA4Editor` module).
3. **`RedAlert4Server.Target.cs`**: Dedicated Server target. Configured for headless server builds without visual rendering pipeline.

---

## 3. Headless Build Harness (`Tools/HeadlessBuild/`)

The project features a **pure C++ CMake build system** in `Tools/HeadlessBuild/` that stubs out Unreal Engine headers (`UnrealStub/`), allowing full C++ simulation engine compilation and unit testing without launching Unreal Engine or requiring an installed UE5 engine binary.

### CMake Build Pipeline (`Tools/HeadlessBuild/CMakeLists.txt`)
- Compiles 13 C++ static libraries: `libRA4Core.a`, `libRA4Content.a`, `libRA4Simulation.a`, `libRA4Combat.a`, `libRA4Navigation.a`, `libRA4Input.a`, `libRA4Presentation.a`, `libRA4FogOfWar.a`, `libRA4AI.a`, `libRA4Network.a`, `libRA4Campaign.a`, `libRA4Replay.a`.
- Builds 4 test executables: `RA4Tests`, `RA4AITests`, `RA4InputTests`, `RA4PresentationTests`.
- **Execution Proof**: Builds cleanly in `build/hb/` and executes 378 unit tests in under 6 seconds on macOS ARM64 / Linux x86_64.

---

## 4. Config Directory Audit (`Config/`)

- `DefaultEngine.ini`: Configures asset manager scanning rules (`GameFeatureData`), viewport settings, game user settings, and MCP plugin settings.
- `DefaultGame.ini`: Project metadata, game instance class (`URA4GameInstance`), default map (`/Game/Maps/M_Skirmish_Desert`).
- `DefaultInput.ini`: Enhanced Input action mappings for WASD camera controls, selection hotkeys, control groups (0-9), and cheat console (`~` key).
- `DefaultUserInterface.ini`: Default font configuration and cursor visual mapping.

---

## 5. CI / Automation Scripts Audit

- **`.github/workflows/`**:
  - `ci.yml`: Automation pipeline that runs headless CMake compilation and unit tests on GitHub runners.
  - Verification: Clean pass on headless C++ suite.
- **Automation Tools (`Tools/`)**:
  - `Tools/HeadlessBuild`: Standalone CMake harness.
  - `Tools/ContentImport`: Python scripts for batch converting JSON bible definitions to Unreal Primary DataAssets.
  - `Tools/Art`: FBX blockout generation scripts and ArtMapping registry tools.
  - `Tools/Editor`: Unreal Editor Python automation scripts for map layout validation and screenshot capture.

---

## 6. Packaged Build Readiness Assessment

| Requirement | Status | Details / Blocker |
| :--- | :--- | :--- |
| **Headless Server Executable** | **READY** | Builds via CMake or `RedAlert4Server.Target.cs`. |
| **Pure C++ Sim Compilation** | **READY** | 0 compiler warnings, 100% test pass. |
| **Unreal Engine Desktop Client** | **BLOCKED** | Missing NoesisGUI plugin creates compilation error during standard UBT packaging if Noesis headers are referenced in `RA4UI`. |
| **Asset Registry & DataAssets** | **READY** | Normalized JSON and asset registry present in `Content/RA4/Data/`. |

### Remediation Strategy for Packaging
1. Download/install NoesisGUI Unreal Engine plugin into `Plugins/NoesisGUI` OR
2. Wrap NoesisGUI ViewModels in `#if WITH_NOESIS` preprocessor directives so standard UMG/Slate frontend builds without plugin dependency.
