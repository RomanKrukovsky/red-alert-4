# Red Alert 4: NoesisGUI C++ & XAML Architecture Blueprint

> **Status Notice**: Plugin Status: `BLOCKED_PLUGIN_MISSING`. `Plugins/NoesisGUI` is not installed in the repository. The architecture defined below represents the target specification.

## 1. System Overview

This document defines the production C++ and XAML architecture for Red Alert 4's native NoesisGUI user interface in Unreal Engine 5.

```
                      +-----------------------------+
                      |    Unreal Engine 5 Engine   |
                      +-----------------------------+
                                     |
                      +-----------------------------+
                      | URA4UINavigationService     |
                      | URA4UIInputRouter           |
                      +-----------------------------+
                                     |
    +--------------------------------+--------------------------------+
    |                                |                                |
+-----------------------+  +-----------------------+  +-----------------------+
| URA4MainMenuViewModel |  | URA4SkirmishViewModel |  | URA4HUDViewModel      |
+-----------------------+  +-----------------------+  +-----------------------+
            |                          |                          |
    (Data Binding)             (Data Binding)             (Data Binding)
            v                          v                          v
+-----------------------+  +-----------------------+  +-----------------------+
|   MainMenuScreen.xaml |  | SkirmishSetupScreen.xaml| |      InGameHUD.xaml  |
+-----------------------+  +-----------------------+  +-----------------------+
```

---

## 2. Core C++ Services & Registries

### 2.1 Navigation & Layer Service (`URA4UINavigationService`)
* **Role**: Manages the UI view stack, modal overlays, transitions, and map load screen handoffs.
* **Key API**:
  * `PushScreen(EUIScreen ScreenId)`
  * `PopScreen()`
  * `ShowModal(EUIModal ModalId)`
  * `CloseModal()`

### 2.2 Input Router (`URA4UIInputRouter`)
* **Role**: Integrates Enhanced Input with NoesisGUI focus management.
* **Input Modes**:
  * `EUIInputMode::GameOnly`: Camera & RTS controls active, UI non-interactive.
  * `EUIInputMode::UIOnly`: Modal dialogs, menus. Camera & game input suppressed.
  * `EUIInputMode::GameAndUI`: HUD overlay during gameplay. WASD camera active, clicks on HUD captured by XAML.

### 2.3 Subsystem ViewModels
* **`URA4HUDViewModel`**: Exposes real-time resource snapshots (Credits, Power, Intel), unit selection arrays, build queues, and command grid states.
* **`URA4EconomyViewModel`**: Listens to economy subsystem events and updates credit growth animations.
* **`URA4MinimapViewModel`**: Binds the RenderTarget texture to XAML `<Image>` and translates XAML pointer coordinates to world-space vectors for RTS camera placement.

---

## 3. Shipping Build & Cook Validation

* **No Web Runtime**: `WebBrowser`, `Chromium`, `CEF`, `Node.js`, and `npm` modules are excluded from Client and Shipping builds.
* **Asset Cooking**: All XAML files in `Assets/Noesis/` are cooked as native `NoesisXaml` assets into the pak file. Textures are baked as `Texture2D`, fonts as `FontFace`.
