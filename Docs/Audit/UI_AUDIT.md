# RA4 — UI Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`

Supersedes the previous version, which reported the Noesis integration as a build blocker
(it is not) and described web-UI features (JSON WebSocket bridge, live match viewer) and
ultrawide scaling support that could not be confirmed.

## 1. Three parallel UI stacks exist; one is real

| Stack | Volume | Wired into the build? | Verdict |
| --- | --- | --- | --- |
| **UMG / CommonUI / MVVM** | 52 C++ files (7 120 lines), 67 `.uasset` widgets | **Yes** — `libUnrealEditor-RA4UI.dylib` builds (1.5 MB) | **LIVE PATH** |
| **NoesisGUI XAML** | 6 `.xaml` in `Assets/Noesis/` + 1 viewmodel pair | **No** — plugin absent from `.uproject`; XAML referenced by nothing | **ABANDONED** |
| **`ra4-ui` React/Vite** | 210 files + built `dist/` | **No** — not referenced by any target or CI | **ABANDONED** |

This is the clearest case of duplication in the repository: three attempts at the same
front-end, two of them dead but still present, none of them deleted.

## 2. The Noesis "blocker" is not a blocker

The previous audit's headline finding was that a missing `Plugins/NoesisGUI` blocks UBT
packaging. It does not, and the reason is that the Noesis integration was never actually
written against Noesis:

```cpp
// Source/RA4UI/Public/RA4NoesisHUDViewModel.h
#include "CoreMinimal.h"
#include "RA4HUDViewModel.h"
#include "RA4NoesisHUDViewModel.generated.h"

class RA4UI_API URA4NoesisHUDViewModel : public URA4HUDViewModel
```

No `NsGui/*`, no Noesis type, no conditional compilation. `RA4NoesisHUDViewModel.cpp`
contains only a default constructor. It is an empty subclass with a misleading name. That is
why `RedAlert4Editor` links cleanly with no Noesis plugin present.

The 6 XAML files (`App.xaml`, `Screens/MainMenuScreen.xaml`, `Screens/InGameHUD.xaml`,
`Controls/ButtonStyles.xaml`, `Themes/Typography.xaml`, `Themes/FactionSoviet.xaml`) live
in `Assets/Noesis/`, outside `Content/`, so they are not cooked and not loaded by anything.

**They are design artefacts, not code.** Keep them as reference if the visual language is
wanted, but the migration described in `Docs/UI/ReactToNoesisMigrationMap.md` and certified
in `Docs/UI/NoesisFinalAcceptanceReport.md` did not happen.

## 3. What the live UMG stack actually contains

Real, substantive C++ (`Source/RA4UI/Public/`):

- **ViewModels** — `RA4HUDViewModel`, `RA4MainMenuViewModel`, `RA4UIScreenViewModel`,
  `RA4ViewModelBase`, `RA4UIViewModelRegistry`
- **Screens/widgets** — `RA4HUDWidget`, `RA4SidebarWidget`, `RA4SkirmishSetupWidget`,
  `RA4CampaignSelectWidget`, `RA4MatchResultOverlayWidget`, `RA4CommandCentreMenuWidget`,
  `RA4HoverTooltipWidget`, `RA4ShowcaseWidget`
- **Infrastructure** — `RA4UIInputRouter`, `RA4UINavigationService`, `RA4UIRouterSubsystem`,
  `RA4UIScreenCatalog`, `RA4UIDataProviderSubsystem`, `RA4UITheme`, `RA4GameViewportClient`,
  `RA4ActivatableWidget`, `RA4ButtonBase`

67 `WBP_*.uasset` widgets exist in `Content/RA4UI/Widgets/`, including per-faction campaign
screens (`WBP_RA4_Campaign_{USSR,Allies,Eastern,Chrono}`, `WBP_RA4_MissionMap_USSR`).

### Data pipeline is genuinely tested — presentation is not

`HudSnapshot.cpp` (in `RA4Presentation`, engine-free via stub) is covered by **22 `Hud.*`
tests plus 2 `HudSnapshot.*` and 4 `UI.*`**, all passing:

- `Hud*` — resources, selection, health/name of primary selection
- `UI.BuildCardCostTimePowerAndBlockReasons`
- `UI.SelectionDetailsAndHarvesterCargo`
- `UI.SkirmishSetupOptionsAndConflictValidation`
- `UI.WASDCameraPanningAndBoundsClamping`

So the **data** the HUD needs is correct and verified. Whether any of it renders on screen
is **entirely unverified** — no widget was ever instantiated in a test or a running editor,
because `RA4Tests` is not a UBT module (ARCHITECTURE_AUDIT §2.4).

## 4. Minimap has no data source

`Minimap` appears in exactly three files — `RA4HUDWidget.h`, `RA4SidebarWidget.cpp`,
`RA4ShowcaseWidget.cpp` — all presentation-side. `HudSnapshot` produces no minimap payload
(no blip list, no fog-projected entity set), and no `Minimap.*` test exists. The widget has
a container to draw into and nothing to draw. See GAMEPLAY_AUDIT §2.2.

## 5. Input routing — partially substantiated

The previous audit claimed `UI.WASDCameraPanningAndBoundsClamping` and
`ClassicScheme.ArmedModesOwnTheClickInBothSchemes` prove that UI clicks are consumed before
reaching the world. Both tests exist and pass, but they test *camera panning* and *input
scheme arbitration inside the sim*, not widget hit-testing — `RA4UIInputRouter` is Unreal-side
and unreachable from the headless suite. The claim is plausible from the code but unproven.

The 15 `KeyBindings.*` and 11 `ClassicScheme.*` tests are real and do cover rebinding and
classic/modern scheme behaviour in the core.

## 6. `ra4-ui` — abandoned prototype, still shipped in-repo

210 files, React + Vite + TypeScript, with a committed `dist/` (`assets/`, `favicon.svg`,
`icons.svg`). Referenced by no target, no `.uproject` entry and no CI step — the previous
`BUILD_AND_TEST_AUDIT` described an `npm ci && npm run build` CI stage that does not exist in
`.github/workflows/core.yml`.

`f1c8f8e checkpoint: save current web UI prototype before NoesisGUI migration` marks the
point it was superseded. It should be removed or moved to an explicitly archival location;
leaving a buildable second front-end in the tree invites future confusion about which is
authoritative.

## 7. Localization is in better shape than the UI

261 keys, `en` + `ru`, with real `.po`, `.archive` and compiled `.locres`. Exactly **one**
untranslated `msgstr` in Russian. `Culture=ru` is the default in `DefaultGame.ini`. This is
genuinely production-shaped work.

## 8. Findings

| # | Finding | Severity |
| --- | --- | --- |
| 1 | Three parallel UI stacks; two dead, none removed | **IMPORTANT** (duplication) |
| 2 | `RA4NoesisHUDViewModel` is an empty subclass with a misleading name | **IMPORTANT** |
| 3 | `Docs/UI/NoesisFinalAcceptanceReport.md` certifies a migration that did not occur | **CRITICAL** (false documentation) |
| 4 | Minimap has no simulation-side data producer | **IMPORTANT** |
| 5 | No widget/rendering is testable — `RA4Tests` outside UBT | **BLOCKING** for UI verification |
| 6 | `ra4-ui` (210 files + `dist/`) is dead weight in the repo | **MEDIUM** |
| 7 | HUD data layer is correct and well tested | **OK** |
| 8 | Localization 260/261 complete, en + ru | **OK** |
