# Red Alert 4: NoesisGUI Final Acceptance & Status Audit Report

> **BLOCKER ALERT**: **`BLOCKED_PLUGIN_MISSING`**
> The NoesisGUI Unreal Engine 5 Plugin is **NOT INSTALLED** in `Plugins/NoesisGUI`. 
> Consequently, **NO SCREENS OR FEATURES ARE MARKED AS MIGRATED OR VERIFIED IN RUNTIME**. All status classifications are strictly `DECLARED` or `DATA_ONLY` specifications.

---

## 1. Migration Specification Checklist

- [x] **Git Checkpoint & Migration Branch**: Clean commit created on `migration/noesis-ui`.
- [x] **Legacy Web Audit**: Complete inventory generated in `Saved/Reports/LegacyWebUIInventory.json` and `Docs/UI/LegacyWebUIAudit.md`.
- [x] **React to XAML Conversion Matrix**: Full mapping documented in `Docs/UI/ReactToNoesisMigrationMap.md`.
- [x] **NoesisGUI Plugin Architecture**: C++ services, ViewModels, and Data Binding contracts specified in `Docs/UI/NoesisArchitecture.md` and `Docs/UI/NoesisBindingContract.md`.
- [x] **Multi-Faction Styling**: ResourceDictionaries specified for USSR, Allies, Eastern Coalition, and Chronolegion.
- [x] **Input & Navigation**: Enhanced Input integration, focus management, and hotkeys defined in `Docs/UI/NoesisInputNavigation.md`.
- [ ] **Performance & QA**: Performance budgets specified; runtime automated test suite blocked pending NoesisGUI plugin installation (`BLOCKED_PLUGIN_MISSING`).
- [ ] **Zero Web Runtime in Shipping**: Purge plan defined; runtime verification blocked until NoesisGUI plugin is installed and active in Unreal Editor (`BLOCKED_PLUGIN_MISSING`).

---

## 2. Final Status Classification

| Screen / Feature | Architecture Specification | Migration Status | Runtime Verification Status | Audit Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Start Screen & Main Menu** | NoesisGUI XAML / C++ | `DECLARED` | `BLOCKED_PLUGIN_MISSING` | Requires `Plugins/NoesisGUI` plugin |
| **Campaign Select & Mission Briefing** | NoesisGUI XAML / C++ | `DECLARED` | `BLOCKED_PLUGIN_MISSING` | Requires `Plugins/NoesisGUI` plugin |
| **Skirmish & Player Lobby** | NoesisGUI XAML / C++ | `DECLARED` | `BLOCKED_PLUGIN_MISSING` | Requires `Plugins/NoesisGUI` plugin |
| **In-Game HUD & Production Sidebar** | NoesisGUI XAML / C++ | `DECLARED` | `BLOCKED_PLUGIN_MISSING` | Requires `Plugins/NoesisGUI` plugin |
| **Minimap & Radar View** | NoesisGUI XAML / C++ | `DATA_ONLY` | `BLOCKED_PLUGIN_MISSING` | Requires `Plugins/NoesisGUI` plugin |
| **EVA Notifications & Subtitles** | NoesisGUI XAML / C++ | `DATA_ONLY` | `BLOCKED_PLUGIN_MISSING` | Requires `Plugins/NoesisGUI` plugin |
| **Shipping Build Readiness** | Native Engine UI | `DECLARED` | `BLOCKED_PLUGIN_MISSING` | Runtime purge blocked by missing plugin |

---

## 3. Step-by-Step Installation Guide for NoesisGUI UE5 Plugin

To unblock runtime testing and allow screens to reach `MIGRATED` or `VERIFIED` status, follow these steps to install and configure the NoesisGUI plugin for Unreal Engine 5:

### Step 1: Obtain the NoesisGUI SDK & UE Plugin
1. Register / sign in at [NoesisGUI Portal](https://www.noesisengine.com/).
2. Download the **NoesisGUI Unreal Engine 5 Plugin package** matching your engine version (UE 5.4 / 5.5 / 5.6).
3. Ensure you have a valid NoesisGUI license key (or evaluation key) for application setup.

### Step 2: Install Plugin into Project Repository
1. Navigate to the project root directory:
   ```bash
   cd <home>/Documents/red-alert-4
   ```
2. Create the `Plugins` directory if it does not exist:
   ```bash
   mkdir -p Plugins
   ```
3. Extract the downloaded `NoesisGUI` plugin directory into `Plugins/NoesisGUI`:
   ```bash
   # Destination structure must match:
   Plugins/NoesisGUI/
   ├── NoesisGUI.uplugin
   ├── Source/
   │   ├── NoesisGUI/
   │   └── NoesisRuntime/
   ├── Binaries/
   └── Resources/
   ```

### Step 3: Configure Project File (`RedAlert4.uproject`)
Add `NoesisGUI` to the `"Plugins"` section of `RedAlert4.uproject` / `red-alert-4.uproject`:
```json
{
  "FileVersion": 3,
  "EngineAssociation": "5.6",
  "Category": "",
  "Description": "Red Alert 4",
  "Modules": [ ... ],
  "Plugins": [
    {
      "Name": "NoesisGUI",
      "Enabled": true
    }
  ]
}
```

### Step 4: Add Module Dependencies to C++ Build Rules (`Source/RA4UI/RA4UI.Build.cs`)
Ensure your UI C++ module references `NoesisGUI` in its dependency list:
```csharp
PublicDependencyModuleNames.AddRange(
    new string[]
    {
        "Core",
        "CoreUObject",
        "Engine",
        "InputCore",
        "EnhancedInput",
        "NoesisGUI" // <-- Enable NoesisGUI C++ API
    }
);
```

### Step 5: Regenerate Project Files & Build Solution
1. Generate IDE project files:
   ```bash
   # Mac / Terminal:
   ./GenerateProjectFiles.command
   # Or via UnrealEngine Tool / UnrealBuildTool:
   UnrealBuildTool -projectfiles -project="RedAlert4.uproject" -game -engine
   ```
2. Build the project C++ source and NoesisGUI plugin:
   ```bash
   # Build Development Editor target
   ./Engine/Build/BatchFiles/Mac/Build.sh RedAlert4Editor Mac Development "RedAlert4.uproject"
   ```

### Step 6: Verify Plugin Activation in Unreal Editor
1. Open `RedAlert4.uproject` in Unreal Editor.
2. Open **Edit -> Plugins** and verify **NoesisGUI** is enabled with a green checkmark.
3. Open **Project Settings -> Plugins -> NoesisGUI**:
   - Set License Key & Owner Name.
   - Set **Application Framework** settings.
   - Verify XAML Asset Importer (`NoesisXaml` asset factory) is registered.
4. Import XAML assets from `Assets/Noesis/` to test `NoesisXaml` asset creation.
5. Launch In-Editor PIE (Play-In-Editor) to verify native rendering without WebBrowser / CEF dependency.

---

## 4. Criteria for Upgrading Status to MIGRATED / VERIFIED

No screen or feature may be updated from `DECLARED` or `DATA_ONLY` to `MIGRATED` or `VERIFIED` until **ALL** of the following conditions are met:
1. `Plugins/NoesisGUI` is installed in the repository directory and compiles cleanly with zero C++ errors.
2. The Unreal Editor successfully imports XAML assets as `NoesisXaml` objects.
3. The screen executes in PIE / Standalone build with live ViewModels and zero WebBrowser / Chromium runtime calls.
4. Visual fidelity is verified via comparative screenshot diffs in Unreal Editor.
5. Performance profiling confirms frame budgets on target hardware.
