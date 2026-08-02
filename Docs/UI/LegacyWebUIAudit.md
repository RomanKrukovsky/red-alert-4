# Red Alert 4: Legacy Web UI Audit & Migration Technical Blueprint

> **Notice**: This document represents the authoritative audit of all HTML, CSS, JavaScript, TypeScript, React components, npm dependencies, and WebBrowser Widget integration layers in Red Alert 4 prior to native NoesisGUI (C++ / XAML) migration.

---

## 1. Executive Summary & Audit Context

* **Branch**: `migration/noesis-ui`
* **Target Engine**: Unreal Engine 5.4+ with NoesisGUI Plugin (`Plugins/NoesisGUI`)
* **Primary Objective**: Complete elimination of Chromium/CEF, WebBrowser Widget, DOM, npm runtime, and JS bridges in Shipping builds while maintaining 100% visual and functional parity.

---

## 2. Legacy UI Inventory & Artifact Matrix

| Screen / Component | Original Path | Input Events | Output Commands | State & Data Dependencies | Target Noesis XAML | Target C++ ViewModel | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Start / Title Screen** | `ra4-ui/src/screens/StartScreen.tsx` | AnyKey, Click | NavigateToMenu | SplashAnimation, LogoGlow | `StartScreen.xaml` | `URA4StartScreenViewModel` | `LEGACY_PRESENT` |
| **Main Menu** | `ra4-ui/src/screens/MainMenu.tsx` | NavClick, Hotkey | OpenCampaign, OpenSkirmish, OpenSettings | CommanderLevel, NewsFeed | `MainMenuScreen.xaml` | `URA4MainMenuViewModel` | `LEGACY_PRESENT` |
| **Campaign Select** | `ra4-ui/src/screens/CampaignSelect.tsx` | CardHover, Click | SelectFaction, LaunchCampaign | FactionProgress, Difficulty | `CampaignSelectScreen.xaml` | `URA4CampaignSelectViewModel` | `LEGACY_PRESENT` |
| **Mission Briefing** | `ra4-ui/src/screens/FactionBriefing.tsx` | TabSelect, Click | StartMission, SetDifficulty | MissionObjectives, PortraitArt | `MissionBriefingScreen.xaml` | `URA4BriefingViewModel` | `LEGACY_PRESENT` |
| **Skirmish & Lobby** | `ra4-ui/src/screens/SkirmishScreen.tsx` | RowClick, ChatSend | StartMatch, SendChat, SetMap | PlayerList, LobbySettings, ChatLog | `SkirmishSetupScreen.xaml` | `URA4SkirmishLobbyViewModel` | `LEGACY_PRESENT` |
| **In-Game HUD** | `ra4-ui/src/screens/InGameHUD.tsx` | BuildClick, CmdClick | IssueCommand, QueueItem | Resources, Power, Selection, Queue | `InGameHUD.xaml` | `URA4HUDViewModel` | `LEGACY_PRESENT` |
| **Production Sidebar** | `ra4-ui/src/components/ProductionPanel.tsx` | TileClick, TabClick | QueueUnit, QueueBuilding | Prerequisites, Costs, QueueList | `ProductionPanel.xaml` | `URA4ProductionViewModel` | `LEGACY_PRESENT` |
| **Command Grid** | `ra4-ui/src/components/CommandBar.tsx` | GridClick, Hotkey | TriggerAbility, SetStance | AbilityCooldowns, Stances | `CommandGrid.xaml` | `URA4CommandGridViewModel` | `LEGACY_PRESENT` |
| **Minimap Widget** | `ra4-ui/src/components/Minimap.tsx` | MapClick, MapDrag | PanCamera, IssueMoveTarget | TerrainTexture, UnitPings, FogOfWar | `MinimapWidget.xaml` | `URA4MinimapViewModel` | `LEGACY_PRESENT` |
| **EVA & Subtitles** | `ra4-ui/src/components/EVALog.tsx` | EventTrigger | PlayEVAVoice, ShowSubtitle | NotificationQueue, Priority | `EVANotifications.xaml` | `URA4NotificationViewModel` | `LEGACY_PRESENT` |

---

## 3. Legacy Web Dependencies & Elimination Strategy

The following packages, configs, and runtimes will be fully uninstalled and purged from Client/Shipping builds once NoesisGUI functional parity is verified:

* `react` & `react-dom`: Replaced by NoesisGUI XAML DataTemplates and ControlTemplates.
* `react-router-dom`: Replaced by `URA4UINavigationService` (C++ navigation stack).
* `zustand`: Replaced by C++ `URA4UIViewModelRegistry` and UObject ViewModels.
* `zod`: Replaced by Unreal Engine native `FText` and `USTRUCT` data validation.
* `vite`: Replaced by Unreal Engine Asset Pipeline (`NoesisXaml` asset importer).

---

## 4. Faction Visual Language Mapping

```
      +-------------------------------------------------------+
      |               Red Alert 4 Theme Engine                |
      +-------------------------------------------------------+
        |                  |                  |             |
  +-----------+      +-----------+      +-----------+  +-----------+
  |   USSR    |      |  ALLIES   |      |    EC     |  | CHRONO    |
  | (Red/Gold)|      | (Blue/Slv)|      | (Grn/Gld) |  | (Prp/Vlt) |
  +-----------+      +-----------+      +-----------+  +-----------+
```

* **USSR ResourceDictionary**: Red glowing borders (`#ff1a1a`), gold accents (`#ffb700`), brutalist angular corners (`clip-path`).
* **Allies ResourceDictionary**: Cyan/Blue glow (`#0088ff`), silver borders (`#e0e0e0`), sleek hi-tech panels.
* **Eastern Coalition ResourceDictionary**: Emerald Green (`#00ff66`), dragon motifs, bronze accents.
* **Chronolegion ResourceDictionary**: Neon Violet (`#aa00ff`), dark metallic finish, energy pulse storyboards.
