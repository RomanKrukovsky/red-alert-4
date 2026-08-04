# User Interface Systems Audit (`UI_AUDIT.md`)

**Audit Date**: August 4, 2026  
**UI Architecture**: Tri-Layer Framework (NoesisGUI + UMG/Slate + Web UI Prototype)  

---

## 1. UI Architecture Overview

The RA4 UI architecture underwent multiple evolutionary iterations recorded in Git history:
1. **Layer 1: NoesisGUI (Production Design)** - High-performance XAML visual vector UI system.
2. **Layer 2: Unreal UMG / Slate (Fallback / Engine Native)** - Standard Unreal Engine Widget Blueprints in `Content/RA4UI/`.
3. **Layer 3: Web UI (`ra4-ui`)** - React + Vite + TypeScript web application used for out-of-engine visual debugging and browser-based match viewing.

```
                  +-----------------------------------+
                  |        RA4 UI FRONTENDS           |
                  +-----------------------------------+
                    /               |               \
                   v                v                v
          +---------------+  +---------------+  +---------------+
          |  NoesisGUI    |  |  Unreal UMG   |  |   Web UI      |
          |  (XAML Assets)|  |  (WBP Widgets)|  |  (ra4-ui)     |
          +---------------+  +---------------+  +---------------+
                   \                |               /
                    v               v              v
          +--------------------------------------------------+
          |            C++ VIEWMODEL LAYER                   |
          |  RA4HUDViewModel | RA4MainMenuViewModel          |
          |  RA4UIInputRouter | RA4UINavigationService       |
          +--------------------------------------------------+
                                    |
                                    v
          +--------------------------------------------------+
          |             SIMULATION ENGINE CORE               |
          |               (SimWorld / CommandBus)            |
          +--------------------------------------------------+
```

---

## 2. NoesisGUI Integration Audit

### Current Status: `DECLARED / BLOCKED_PLUGIN_MISSING`
- **C++ Architecture**: Fully implemented in `Source/RA4UI/`:
  - `RA4NoesisHUDViewModel.h/cpp`: Pure C++ MVVM provider for HUD radar, resource counters, build card lists, active production queues, and tactical alert logs.
  - `RA4UIViewModelRegistry.h/cpp`: Central registry mapping view model types to XAML visual targets.
  - `RA4UIInputRouter.h/cpp`: Priority-based input router preventing mouse clicks on UI elements from leaking to world selection/orders.
  - `RA4UINavigationService.h/cpp`: Screen transition state machine (Main Menu -> Skirmish Setup -> In-Game HUD -> Victory/Defeat Overlay).
- **XAML Assets**: Authored and stored in `Assets/Noesis/Themes/` and `Content/RA4/UI/`.
- **Blocker**: The NoesisGUI plugin is **missing** from `Plugins/NoesisGUI` and `.uproject`. As a result, standard UBT builds cannot link against Noesis runtime libraries until the plugin is placed in `Plugins/`.

---

## 3. UMG / Slate Integration Audit

### Current Status: `FUNCTIONAL`
- **Location**: `Content/RA4UI/`
- **Key Widgets**:
  - `WBP_RA4HUD`: Main combat screen HUD layout with resource bar, selection info panel, minimap container, and command bar.
  - `WBP_SkirmishSetup`: Skirmish lobby UI for selecting map, player faction, AI difficulty, and starting credits.
  - `WBP_CheatConsole`: In-game C&C style cheat console toggled via `~` key.
- **Verification**: UMG widgets function cleanly in Unreal Editor viewports and bind directly to `URA4HUDViewModel`.

---

## 4. Web UI (`ra4-ui`) Prototype Audit

### Current Status: `FUNCTIONAL PROTOTYPE`
- **Directory**: `ra4-ui/`
- **Technology Stack**: React 18, Vite 5, TypeScript, TailwindCSS / Vanilla CSS, Oxlint.
- **Features**:
  - Full Skirmish setup interface.
  - Live HUD snapshot visualizer reading simulated match frames via JSON WebSocket bridge.
  - Command bar, build cards, and minimap canvas rendering.
- **Build Status**: Compiles cleanly with zero errors (`npm run build` generates static assets in `ra4-ui/dist/`).

---

## 5. UI Input Router & Event Isolation Audit

- **Input Leaks**: Unit test `UI.WASDCameraPanningAndBoundsClamping` and `ClassicScheme.ArmedModesOwnTheClickInBothSchemes` confirm that when UI buttons or HUD overlays are clicked, click events are consumed by `RA4UIInputRouter` and do not trigger unintended ground movement orders.
- **Resolution & Aspect Ratio**: ViewModels support dynamic scaling across 16:9, 16:10, and 21:9 ultrawide monitor formats.
