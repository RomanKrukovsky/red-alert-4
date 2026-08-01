# Assembling the Red Alert 4 interface in Unreal Editor
This document is an accurate visual assembly map for the Unreal Engine 5.6 editor. It is based on 24 files from `SCREENSHOTS`. `.uasset` files are intentionally not created as text: they are Unreal binary objects that must be created by the editor.
## Common root of each screen
Each `WBP_RA4_*` inherits from the specified C++ class and is rooted using `SafeZone → Overlay`. The main composition is built in `ScaleBox` with `Scale To Fit`, control resolution - 1920x1080. The side HUD panels snap to the edges, the center panels to the center, and the bottom command panel to the bottom edge. Do not use `CanvasPanel` with fixed coordinates as the only container.
Bind text and State only to `URA4UIScreenViewModel` via MVVM. Buttons call `NavigateToScreen`, `NavigateBack` or ViewModel methods. Do not access `SimWorld`, actors or unit components from widgets.
## Themes
| Data Asset | Frame color | Second color | Screen backgrounds || --- | --- | --- | --- |
| `DA_UITheme_USSR` | `#D92525` | `#160607` | `1, 2, 4, 8–13, 19–21` |
| `DA_UITheme_Allies` | `#3D9BE9` | `#0A1726` | `5, 11, 14, 22, 23` |
| `DA_UITheme_Eastern` | `#D7B14E` | `#123025` | `6, 15, 18` |
| `DA_UITheme_Chrono` | `#A95CFF` | `#160B25` | `7, 16, 24` |

For all themes, create one basic panel material `M_UI_AngularPanel`: black translucent fill, two-pixel outer border, cut corners, `AccentColor` parameter and weak outer glow. Instances of `MI_UI_*` get their color from `DA_UITheme_*`.
## Widget Blueprints based on references
| Reference | Widget Blueprint | C++ class | Key Present Elements || --- | --- | --- | --- |
| 1 | `WBP_RA4_Splash` | `URA4SplashWidget` | logo, background illustration, continuation hint |
| 2 | `WBP_RA4_MainMenu` | `URA4MainMenuWidget` | vertical menu, profile card, news, network status |
| 3 | `WBP_RA4_FactionSelect` | `URA4FactionSelectWidget` | four faction cards, Description, continue button |
| 4 | `WBP_RA4_Campaign_USSR` | `URA4SovietCampaignWidget` | portrait of a marshal, goals, choice of operation |
| 5 and 11 | `WBP_RA4_Campaign_Allies` | `URA4AlliesCampaignWidget` | admiral portrait, objectives, campaign tabs |
| 6 | `WBP_RA4_Campaign_Eastern` | `URA4EasternCampaignWidget` | portrait of a general, operation data |
| 7 | `WBP_RA4_Campaign_Chrono` | `URA4ChronoCampaignWidget` | time markers, operation data |
| 8 | `WBP_RA4_MissionMap_USSR` | `URA4MissionMapWidget` | map, mission nodes, difficulty, start of operation |
| 9 | `WBP_RA4_Briefing_USSR` | `URA4BriefingWidget` | summary, tasks, operation parameters |
| 10 | `WBP_RA4_VideoComms` | `URA4VideoCommsWidget` | two video channels, timecode, dialogue || 12 and 19 | `WBP_RA4_Loading_USSR` | `URA4LoadingWidget` | progress, hint, initialization status |
| 13 | `WBP_RA4_HUD_USSR` | `URA4SovietHUDWidget` | resources, mini-map, queue, unit cards |
| 14 | `WBP_RA4_HUD_Allies` | `URA4AlliesHUDWidget` | resources, mini-map, aviation queue |
| 15 | `WBP_RA4_HUD_Eastern` | `URA4EasternHUDWidget` | resources, mini-map, production queue |
| 16 | `WBP_RA4_HUD_Chrono` | `URA4ChronoHUDWidget` | resources, mini-map, Chrono indicators |
| 17 | `WBP_RA4_MultiplayerLobby` | `URA4LobbyWidget` | list of players, chat, map, slots, readiness |
| 18 | `WBP_RA4_Campaign_EasternDetail` | `URA4EasternCampaignWidget` | expanded card of the Eastern Coalition |
| 20 | `WBP_RA4_HUD_USSR_Battle` | `URA4SovietHUDWidget` | combat goals, selection, Production |
| 21 | `WBP_RA4_HUD_USSR_Alert` | `URA4SovietHUDWidget` | alarm, red warnings, target list |
| 22 | `WBP_RA4_HUD_Allies_Naval` | `URA4AlliesHUDWidget` | naval tactics, squadron, mini-map || 23 | `WBP_RA4_HUD_Allies_Air` | `URA4AlliesHUDWidget` | air wing, abilities, queue |
| 24 | `WBP_RA4_HUD_ChronoSuperweapon` | `URA4ChronoHUDWidget` | superweapon charge, timer, target |
Additional screens without a separate reference: `WBP_RA4_Pause` (`URA4PauseWidget`), `WBP_RA4_Victory` (`URA4MatchResultWidget`), `WBP_RA4_Encyclopedia` (`URA4EncyclopediaWidget`), `WBP_RA4_TechTree` (`URA4TechTreeWidget`), `WBP_RA4_Mods` (`URA4ModsWidget`), `WBP_RA4_Settings` (`URA4SettingsWidget`). They use the same panel, theme, typography and navigation.
## Interactivity and states
Create a `WBP_RA4_Button` from `URA4ButtonBase` and four `CommonButtonStyle` (`CBS_RA4_USSR`, `CBS_RA4_Allies`, `CBS_RA4_Eastern`, `CBS_RA4_Chrono`). for each state, set separate brushes/colors: `Normal` - dark panel and thin frame, `Hovered` - bright frame and slight glow, `Pressed` - dark fill and offset by 2 px, `Selected` - thick accent frame, `Disabled` - 35% opacity without glow.
Modal windows are added to the top `CommonActivatableWidgetStack`; closure is called via `NavigateBack`. for tabs use `CommonTabListWidgetBase`, for queues - `ListView` with `FRA4ProductionQueueItem`, for lobbies - `ListView` with `FRA4LobbyPlayerSlot`.
## Input, localization and verification
Create `IA_UI_Back`, `IA_UI_Confirm`, `IA_UI_Pause` in Enhanced Input and add them to CommonUI action data. Source language - `ru`; all custom strings must be `FText`/`LOCTEXT` or String Table strings `ST_RA4_UI_RU`.
In the editor, check each screen at `1280x720`, `1920x1080`, `2560x1440`, `3840x2160` and `3440x1440`. At 21:9, the central composition remains in the 16:9 `ScaleBox`, and the free side zones are filled with the background; The HUD remains anchored to safe edges.