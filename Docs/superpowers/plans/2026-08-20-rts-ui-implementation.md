# Native RTS UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Реализовать все 24 референсных экрана как нативные UMG/Slate-интерфейсы Unreal с реальными состояниями и без использования скриншотов как готового UI.

**Architecture:** UMG отвечает за экранную композицию и повторно используемые панели. Slate используется только для мини-карты, массовых маркеров и рамки выделения. Состояние приходит через C++ MVVM; экранные варианты описываются единым каталогом и не дублируют целые деревья виджетов.

**Tech Stack:** Unreal Engine 5.8, C++17, UMG, Slate, CommonUI, ModelViewViewModel, Enhanced Input, Primary Data Assets, Unreal Automation Tests.

**Spec:** `Docs/superpowers/specs/2026-08-20-rts-ui-design.md`

## Global Constraints

- Эталонная композиция: 1920×1080; обязательное сравнение также при 1672×941.
- Проверяемые размеры: 1280×720, 1672×941, 1920×1080, 2560×1440, 3440×1440 и 3840×2160.
- UI не читает и не изменяет `SimWorld` напрямую.
- Скриншоты из `SCREENSHOTS` не используются как финальные фоновые изображения с уже нарисованным UI.
- Все пользовательские строки проходят через `FText`, `LOCTEXT` или String Table.
- Импорты остаются в начале модулей; новые перечисления обрабатываются исчерпывающе.
- Каждый пакет заканчивается компиляцией, тестами, запуском в Unreal, визуальной проверкой и отдельным коммитом.

---

## File Structure

- `Source/RA4UI/Public/RA4UIScreenCatalog.h` — чистый C++-каталог 24 референсов и их вариантов.
- `Source/RA4UI/Public/RA4UIScreenContract.h` — Unreal-адаптер каталога к экранным ID и темам.
- `Source/RA4UI/Private/RA4UIScreenContract.cpp` — исчерпывающие преобразования и валидация.
- `Source/RA4UI/Public/RA4ScreenRootWidget.h` — общий полноэкранный UMG-контейнер.
- `Source/RA4UI/Private/RA4ScreenRootWidget.cpp` — SafeZone, фон, reference frame и chrome/content layers.
- `Source/RA4UI/Public/RA4AngularPanelWidget.h` — UMG-обёртка общей угловой панели.
- `Source/RA4UI/Private/RA4AngularPanelWidget.cpp` — применение темы и 9-slice/material brush.
- `Source/RA4UI/Public/RA4MainMenuViewModel.h` — данные заставки и главного меню.
- `Source/RA4UI/Private/RA4MainMenuViewModel.cpp` — команды и значения меню.
- `Source/RA4UI/Public/RA4CampaignViewModel.h` — состояние выбора кампании, миссий и брифинга.
- `Source/RA4UI/Private/RA4CampaignViewModel.cpp` — переключение фракций, миссий и загрузки.
- `Source/RA4UI/Public/RA4LobbyViewModel.h` — игроки, чат, готовность и настройки матча.
- `Source/RA4UI/Private/RA4LobbyViewModel.cpp` — проверка лобби и команды.
- `Source/RA4UI/Public/RA4HUDViewModel.h` — боевое состояние HUD.
- `Source/RA4UI/Private/RA4HUDViewModel.cpp` — событийная проекция `HudSnapshot`.
- `Source/RA4UI/Public/Slate/SRA4Minimap.h` — один батчированный Slate-виджет мини-карты.
- `Source/RA4UI/Private/Slate/SRA4Minimap.cpp` — отрисовка карты, контактов и viewport polygon.
- `Source/RA4UI/Public/Slate/SRA4WorldMarkerLayer.h` — батчированный слой маркеров мира.
- `Source/RA4UI/Private/Slate/SRA4WorldMarkerLayer.cpp` — health bars, selection markers и intel states.
- `Tools/Editor/CreateRA4UIAssets.py` — идемпотентная сборка Widget Blueprint оболочек.
- `Tools/Editor/ValidateRA4UI.py` — проверка классов, привязок, ассетов и компиляции.
- `Source/RA4Tests/Private/TestUI.cpp` — чистые тесты каталога и presentation-контрактов.
- `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp` — Unreal Automation Tests для ViewModel и виджетов.
- `Content/RA4UI/Widgets/*.uasset` — сгенерированные/собранные UMG-экраны.
- `Content/RA4UI/Components/*.uasset` — общие визуальные компоненты.

---

### Task 1: Канонический контракт 24 референсов

**Files:**
- Modify: `Source/RA4UI/Public/RA4UIScreenCatalog.h`
- Modify: `Source/RA4Tests/Private/TestUI.cpp`

**Interfaces:**
- Consumes: существующие `RA4::UI::ScreenId`, `FactionTheme`, `ScreenCatalog`.
- Produces: `ScreenFamily`, `ScreenVariant`, `InputPolicy`, `ScreenReferenceDefinition`, `FindScreenByReference(int)`.

- [ ] **Step 1: Write the failing catalog tests**

Добавить тесты, которые требуют полного покрытия номеров 1–24, корректную тему, layout family, input policy и соответствие HUD-вариантов базовой семье. Повторные кадры 11 и 19 должны сохранять общий экранный ID, но иметь собственный визуальный вариант; `PauseMenu` и `Victory` не должны притворяться скриншотами:

```cpp
RA4_TEST(UI, ReferenceCatalogCoversEveryScreenshotExactlyOnce)
{
    std::array<bool, 25> Seen{};
    for (int Reference = 1; Reference <= 24; ++Reference)
    {
        const RA4::UI::ScreenReferenceDefinition* Screen =
            RA4::UI::FindScreenByReference(Reference);
        RA4_EXPECT(Screen != nullptr);
        Seen[static_cast<std::size_t>(Reference)] = true;
    }
    for (std::size_t Reference = 1; Reference < Seen.size(); ++Reference)
    {
        RA4_EXPECT(Seen[Reference]);
    }
}
```

- [ ] **Step 2: Run the focused headless test and verify failure**

Run:

```bash
cmake -S Tools/HeadlessBuild -B build/ui-foundation
cmake --build build/ui-foundation --target RA4Tests -j 8
./build/ui-foundation/RA4Tests --filter UI
```

Expected: compilation fails because `ScreenReferenceDefinition` and `FindScreenByReference` do not exist.

- [ ] **Step 3: Extend the catalog without creating another registry**

Add:

```cpp
enum class ScreenFamily : unsigned char
{
    Splash, MainMenu, CampaignSelect, FactionCampaign, MissionMap,
    Briefing, VideoComms, Loading, MultiplayerLobby, InGameHud,
    PauseMenu, Victory
};

enum class ScreenVariant : unsigned char
{
    Default, AlliesAlternate, EasternDetail, SovietBattle, SovietAlert,
    AlliesNaval, AlliesAir, ChronoSuperweapon
};

enum class InputPolicy : unsigned char { MenuOnly, GameAndUI };
```

Extend `ScreenDefinition` with family and input policy. Add a normalized 24-entry `ScreenReferenceCatalog` whose rows contain reference number, existing screen ID and visual variant. Implement `FindScreenByReference` as a constexpr linear lookup. References 5/11 share `AlliesCampaign`; 12/19 share `SovietLoading`, while the second frame in each pair receives its own variant. Non-reference `PauseMenu` and `Victory` remain only in `ScreenCatalog`.

- [ ] **Step 4: Run the focused tests and verify pass**

Run the command from Step 2.

Expected: all `UI` tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/RA4UI/Public/RA4UIScreenCatalog.h Source/RA4Tests/Private/TestUI.cpp
git commit -m "feat(ui): define canonical reference screen contract"
```

### Task 2: Unreal screen contract and input policy

**Files:**
- Create: `Source/RA4UI/Public/RA4UIScreenContract.h`
- Create: `Source/RA4UI/Private/RA4UIScreenContract.cpp`
- Modify: `Source/RA4UI/Private/RA4UINavigationService.cpp`
- Create: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`

**Interfaces:**
- Consumes: `ERA4UIScreenId`, `ERA4FactionTheme`, `RA4::UI::ScreenDefinition`.
- Produces: `FRA4UIScreenContract ResolveScreenContract(ERA4UIScreenId, ERA4UIScreenVariant)` and `ERA4UIInputMode ResolveInputMode(...)`.

- [ ] **Step 1: Write failing Unreal automation tests**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRA4ScreenContractTest,
    "RA4.UI.Contracts.AllReferenceVariantsResolve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4ScreenContractTest::RunTest(const FString& Parameters)
{
    const FRA4UIScreenContract Contract = ResolveScreenContract(
        ERA4UIScreenId::AlliesHud, ERA4UIScreenVariant::Air);
    TestEqual(TEXT("reference"), Contract.ReferenceNumber, 23);
    TestEqual(TEXT("input"), Contract.InputMode, ERA4UIInputMode::GameAndUI);
    return true;
}
```

- [ ] **Step 2: Build and verify failure**

Run:

```bash
"<UE_ROOT>/Engine/Build/BatchFiles/Mac/Build.sh" RedAlert4Editor Mac Development "RedAlert4.uproject" -WaitMutex
```

Expected: compilation fails because the screen contract does not exist.

- [ ] **Step 3: Implement exhaustive mappings**

Define the variant enum and contract USTRUCT. Use a `switch` over `ERA4UIScreenId` and a second exhaustive switch for the variant. Every `default` branch must call `checkNoEntry()` before returning a safe value.

- [ ] **Step 4: Replace duplicated navigation classification**

Change `URA4UINavigationService::ApplyInputModeForScreen` to use `ResolveScreenContract(Screen, Default).InputMode`.

- [ ] **Step 5: Build and run automation tests**

```bash
"<UE_ROOT>/Engine/Binaries/Mac/UnrealEditor-Cmd" RedAlert4.uproject \
  -unattended -nop4 -nullrhi \
  -ExecCmds="Automation RunTests RA4.UI.Contracts; Quit" \
  -testexit="Automation Test Queue Empty"
```

Expected: `RA4.UI.Contracts` passes.

- [ ] **Step 6: Commit**

```bash
git add Source/RA4UI/Public/RA4UIScreenContract.h Source/RA4UI/Private/RA4UIScreenContract.cpp Source/RA4UI/Private/RA4UINavigationService.cpp Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp
git commit -m "feat(ui): map reference screens to runtime contracts"
```

### Task 3: Общий экранный контейнер и угловая панель

**Files:**
- Create: `Source/RA4UI/Public/RA4ScreenRootWidget.h`
- Create: `Source/RA4UI/Private/RA4ScreenRootWidget.cpp`
- Create: `Source/RA4UI/Public/RA4AngularPanelWidget.h`
- Create: `Source/RA4UI/Private/RA4AngularPanelWidget.cpp`
- Modify: `Source/RA4UI/Public/RA4ActivatableWidget.h`
- Modify: `Source/RA4UI/Private/RA4ActivatableWidget.cpp`
- Modify: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`

**Interfaces:**
- Consumes: `URA4UITheme`, `URA4UIScreenData`, `URA4ActivatableWidget`.
- Produces: `ApplyScreenData(const URA4UIScreenData*)`, `SetTheme(const URA4UITheme*)`, `SetPanelRole(ERA4PanelRole)`.

- [ ] **Step 1: Add failing widget construction tests**

Create widgets transiently and assert the root contains `SafeZone`, `BackgroundLayer`, `ReferenceFrame`, `ChromeLayer` and `ContentLayer`. Assert missing screen data produces a neutral brush and a validation error.

- [ ] **Step 2: Run `RA4.UI.Widgets.Foundation` and verify failure**

Use the Unreal automation command from Task 2 with test name `RA4.UI.Widgets.Foundation`.

- [ ] **Step 3: Implement `URA4ScreenRootWidget`**

Build this hierarchy in `RebuildWidget`:

```text
SafeZone
└─ Overlay
   ├─ Background Image
   └─ ScaleBox (ScaleToFit)
      └─ SizeBox (1920×1080)
         └─ Overlay
            ├─ ChromeLayer
            └─ ContentLayer
```

Expose the two overlay layers with getters for derived UMG widgets.

- [ ] **Step 4: Implement `URA4AngularPanelWidget`**

Wrap one child in a `Border`, apply `URA4UITheme::PanelBrush`, and set padding by semantic role: Compact 8, Standard 16, DenseHUD 10, Hero 24 logical pixels.

- [ ] **Step 5: Run tests and editor build**

Expected: construction tests pass and Widget Blueprint subclasses compile.

- [ ] **Step 6: Commit**

```bash
git add Source/RA4UI/Public/RA4ScreenRootWidget.h Source/RA4UI/Private/RA4ScreenRootWidget.cpp Source/RA4UI/Public/RA4AngularPanelWidget.h Source/RA4UI/Private/RA4AngularPanelWidget.cpp Source/RA4UI/Public/RA4ActivatableWidget.h Source/RA4UI/Private/RA4ActivatableWidget.cpp Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp
git commit -m "feat(ui): add reusable screen and angular panel widgets"
```

### Task 4: Заставка и главное меню

**Files:**
- Modify: `Source/RA4UI/Public/RA4MainMenuViewModel.h`
- Modify: `Source/RA4UI/Private/RA4MainMenuViewModel.cpp`
- Create: `Source/RA4UI/Public/RA4SplashScreenWidget.h`
- Create: `Source/RA4UI/Private/RA4SplashScreenWidget.cpp`
- Create: `Source/RA4UI/Public/RA4MainMenuScreenWidget.h`
- Create: `Source/RA4UI/Private/RA4MainMenuScreenWidget.cpp`
- Modify: `Tools/Editor/CreateRA4UIAssets.py`
- Modify: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`

**Interfaces:**
- Consumes: screen root, theme assets, router, main-menu ViewModel.
- Produces: functional screens 1 and 2 with `OpenCampaign`, `OpenMultiplayer`, `OpenSkirmish`, `OpenSettings`, `ExitToSplash`.

- [ ] **Step 1: Write failing tests for menu entries and navigation**

Assert eight ordered menu entries, selected state, press-any-key navigation, and that decorative widgets are not focusable.

- [ ] **Step 2: Verify test failure**

Run `RA4.UI.Screens.MainMenu` and expect missing screen classes.

- [ ] **Step 3: Implement splash composition**

Use separate background art, logo texture and prompt widget. Handle any keyboard key and primary pointer release through the screen class; navigate to `MainMenu` exactly once.

- [ ] **Step 4: Implement main-menu composition**

Build the left navigation, commander card, news carousel, operation summary and footer from shared panels. Use texture assets under `Content/RA4UI/Art`, never `SCREENSHOTS/2.png`.

- [ ] **Step 5: Generate and compile Widget Blueprint subclasses**

Update the editor script so `WBP_RA4_Splash` and `WBP_RA4_MainMenu` inherit the new concrete screen classes. Re-running the script must preserve existing assets and produce the same parent classes.

- [ ] **Step 6: Run tests, build and capture references 1–2**

Capture 1672×941 and 1920×1080. Confirm no duplicated baked UI is visible.

- [ ] **Step 7: Commit**

```bash
git add Source/RA4UI/Public/RA4MainMenuViewModel.h Source/RA4UI/Private/RA4MainMenuViewModel.cpp Source/RA4UI/Public/RA4SplashScreenWidget.h Source/RA4UI/Private/RA4SplashScreenWidget.cpp Source/RA4UI/Public/RA4MainMenuScreenWidget.h Source/RA4UI/Private/RA4MainMenuScreenWidget.cpp Tools/Editor/CreateRA4UIAssets.py Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp Content/RA4UI/Widgets/WBP_RA4_Splash.uasset Content/RA4UI/Widgets/WBP_RA4_MainMenu.uasset
git commit -m "feat(ui): build native splash and command menu"
```

### Task 5: Кампания, миссии, брифинг и загрузка

**Files:**
- Create: `Source/RA4UI/Public/RA4CampaignViewModel.h`
- Create: `Source/RA4UI/Private/RA4CampaignViewModel.cpp`
- Modify: `Source/RA4UI/Public/RA4CampaignSelectWidget.h`
- Modify: `Source/RA4UI/Private/RA4CampaignSelectWidget.cpp`
- Create: `Source/RA4UI/Public/RA4CampaignScreenWidget.h`
- Create: `Source/RA4UI/Private/RA4CampaignScreenWidget.cpp`
- Create: `Source/RA4UI/Public/RA4MissionFlowWidgets.h`
- Create: `Source/RA4UI/Private/RA4MissionFlowWidgets.cpp`
- Modify: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`

**Interfaces:**
- Consumes: campaign database/profile through an adapter, screen root and shared panels.
- Produces: screens 3–12, 18 and 19 as working states; `SelectFaction`, `SelectMission`, `SetDifficulty`, `StartMission`, `SkipBriefing`.

- [ ] **Step 1: Write failing ViewModel tests**

Cover four factions, locked chapters, progress clamping, mission selection, duplicate visual reference 11, loading states 12/19 and invalid content IDs.

- [ ] **Step 2: Verify test failure**

Run `RA4.UI.Screens.Campaign` and expect missing ViewModel/commands.

- [ ] **Step 3: Implement campaign ViewModel and adapters**

Expose `TArray<FRA4FactionCardView>`, `TArray<FRA4MissionNodeView>`, selected mission details, progress and commands with Field Notify.

- [ ] **Step 4: Build shared campaign compositions**

Create one faction campaign widget driven by theme and variant. Create separate mission-map, briefing, video-comms and loading classes only where composition truly differs.

- [ ] **Step 5: Bind all screen classes and validate assets**

Update `CreateRA4UIAssets.py`; compile every affected Widget Blueprint.

- [ ] **Step 6: Capture and compare references 3–12, 18–19**

Record visible differences by component, fix layout and repeat until no critical mismatch remains.

- [ ] **Step 7: Commit**

```bash
git add Source/RA4UI Tools/Editor/CreateRA4UIAssets.py Content/RA4UI/Widgets
git commit -m "feat(ui): implement campaign and mission screen flow"
```

### Task 6: Сетевое лобби

**Files:**
- Create: `Source/RA4UI/Public/RA4LobbyViewModel.h`
- Create: `Source/RA4UI/Private/RA4LobbyViewModel.cpp`
- Create: `Source/RA4UI/Public/RA4LobbyScreenWidget.h`
- Create: `Source/RA4UI/Private/RA4LobbyScreenWidget.cpp`
- Modify: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`

**Interfaces:**
- Consumes: network/session adapter, screen root and list views.
- Produces: screen 17; `SetReady`, `ChangeFaction`, `ChangeTeam`, `SendChat`, `StartMatch`, `LeaveLobby`.

- [ ] **Step 1: Write failing lobby tests**

Cover eight slots, duplicate colors, invalid teams, host-only start, all-ready requirement, empty chat rejection and disconnect state.

- [ ] **Step 2: Verify test failure**

Run `RA4.UI.Screens.Lobby`.

- [ ] **Step 3: Implement ViewModel validation and commands**

Use Field Notify and typed row view models. Do not call online APIs from the widget.

- [ ] **Step 4: Build lobby screen**

Use virtualized `ListView` for players/chat, a clean map preview texture, settings summary and explicit host/readiness states.

- [ ] **Step 5: Run tests and compare reference 17**

Verify mouse, keyboard focus, scrolling and disabled Start state.

- [ ] **Step 6: Commit**

```bash
git add Source/RA4UI Content/RA4UI/Widgets/WBP_RA4_MultiplayerLobby.uasset
git commit -m "feat(ui): implement multiplayer lobby interface"
```

### Task 7: Базовый RTS HUD и presentation adapter

**Files:**
- Modify: `Source/RA4UI/Public/RA4HUDViewModel.h`
- Modify: `Source/RA4UI/Private/RA4HUDViewModel.cpp`
- Modify: `Source/RA4UI/Public/RA4HUDWidget.h`
- Modify: `Source/RA4UI/Private/RA4HUDWidget.cpp`
- Modify: `Source/RA4UI/Private/RA4UIDataProviderSubsystem.cpp`
- Modify: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`

**Interfaces:**
- Consumes: immutable `RA4::Presentation::HudSnapshot` and player commands.
- Produces: event-driven resource bar, objectives, selection, production, commands and alerts shared by screens 13–16 and 20–24.

- [ ] **Step 1: Write failing projection tests**

Cover empty/single/multi selection, harvester cargo, blocked production, changing resources, objective states, queue progress and alert replacement without per-frame text churn.

- [ ] **Step 2: Verify test failure**

Run `RA4.UI.HUD.ViewModel`.

- [ ] **Step 3: Implement snapshot diff application**

Add `ApplySnapshot(const FRA4HUDSnapshotView&)`. Broadcast only fields whose values changed. Keep the presentation adapter outside widgets.

- [ ] **Step 4: Build the UMG HUD shell**

Anchor objectives top-left, resources top-right, sidebar right, selection bottom-left, production bottom-center and commands bottom-right. Keep the center world view unobstructed.

- [ ] **Step 5: Verify input blocking**

Add automation coverage ensuring pointer hits over every interactive HUD region suppress world commands while transparent world-view regions pass through.

- [ ] **Step 6: Build, test and commit**

```bash
git add Source/RA4UI
git commit -m "feat(ui): add event-driven native RTS HUD shell"
```

### Task 8: Slate mini-map and batched markers

**Files:**
- Create: `Source/RA4UI/Public/Slate/SRA4Minimap.h`
- Create: `Source/RA4UI/Private/Slate/SRA4Minimap.cpp`
- Create: `Source/RA4UI/Public/Slate/SRA4WorldMarkerLayer.h`
- Create: `Source/RA4UI/Private/Slate/SRA4WorldMarkerLayer.cpp`
- Create: `Source/RA4UI/Public/RA4MinimapWidget.h`
- Create: `Source/RA4UI/Private/RA4MinimapWidget.cpp`
- Modify: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`

**Interfaces:**
- Consumes: immutable minimap/marker arrays from the presentation adapter.
- Produces: one UMG wrapper per Slate layer, `SetSnapshot`, `OnMapCommand`, `OnCameraJump`.

- [ ] **Step 1: Write failing geometry and hit tests**

Cover world-to-map conversion, clamping, viewport polygon, team/intel glyph selection and map click conversion.

- [ ] **Step 2: Verify failure**

Run `RA4.UI.HUD.SlateLayers`.

- [ ] **Step 3: Implement one-pass painting**

Use `FSlateDrawElement` batches by brush/layer. Do not allocate child widgets for contacts or health bars.

- [ ] **Step 4: Implement UMG wrappers**

`RebuildWidget` creates the Slate object; `ReleaseSlateResources` clears it; snapshot setters update immutable arrays and invalidate paint only.

- [ ] **Step 5: Measure**

Capture Slate Insights with 0, 200, 1000 and 5000 markers. Record paint time and allocation count in `Docs/QA/UI_PERFORMANCE_REPORT.md`.

- [ ] **Step 6: Commit**

```bash
git add Source/RA4UI Docs/QA/UI_PERFORMANCE_REPORT.md
git commit -m "feat(ui): add batched minimap and world marker layers"
```

### Task 9: Фракционные HUD и боевые состояния

**Files:**
- Create: `Source/RA4UI/Public/RA4FactionHUDWidget.h`
- Create: `Source/RA4UI/Private/RA4FactionHUDWidget.cpp`
- Modify: `Tools/Editor/CreateRA4HUDComponents.py`
- Modify: `Tools/Editor/CreateRA4UIThemes.py`
- Modify: `Source/RA4UI/Private/Tests/RA4UIAutomationTests.cpp`
- Modify: `Content/RA4UI/Themes/*.uasset`
- Modify: `Content/RA4UI/Widgets/WBP_RA4_HUD_*.uasset`

**Interfaces:**
- Consumes: base HUD, `URA4UITheme`, `ERA4UIScreenVariant`.
- Produces: screens 13–16 and 20–24 without duplicated HUD logic.

- [ ] **Step 1: Write failing variant tests**

Assert that every combat reference resolves to the correct theme, tab set, selected panel, alerts and specialized widget: Soviet production, Allies aviation/naval, Eastern production, Chrono abilities/superweapon.

- [ ] **Step 2: Verify failure**

Run `RA4.UI.HUD.Factions`.

- [ ] **Step 3: Implement theme and variant slots**

Switch only data and optional component visibility. Keep resource bar, objectives, minimap, selection and command handling shared.

- [ ] **Step 4: Generate theme assets and Widget Blueprint subclasses**

Run editor scripts twice and verify the second run produces no asset changes.

- [ ] **Step 5: Capture references 13–16 and 20–24**

Test normal base, tank battle, alert, naval, air and superweapon states at 1672×941 and 3440×1440.

- [ ] **Step 6: Commit**

```bash
git add Source/RA4UI Tools/Editor Content/RA4UI/Themes Content/RA4UI/Widgets
git commit -m "feat(ui): implement faction HUD variants"
```

### Task 10: Asset validation, visual regression and release gate

**Files:**
- Create: `Tools/Editor/ValidateRA4UI.py`
- Create: `Config/Automation/RA4UIVisualTests.ini`
- Create: `Docs/QA/UI_VISUAL_ACCEPTANCE.md`
- Modify: `Docs/Production/UI_UX_BIBLE.md`
- Modify: `Docs/Agent/PROJECT_STATE.md`
- Modify: `Docs/Agent/NEXT_ACTIONS.md`

**Interfaces:**
- Consumes: all screen assets, themes, bindings and reference captures.
- Produces: deterministic validation report and documented acceptance status for all 24 references.

- [ ] **Step 1: Make the validator fail on one intentionally invalid fixture**

Validate required class, screen ID, theme, background policy, focusability, bindings and compile status. The invalid fixture must fail with the exact asset path and property name.

- [ ] **Step 2: Implement validator and run against real UI assets**

Run:

```bash
"<UE_ROOT>/Engine/Binaries/Mac/UnrealEditor-Cmd" RedAlert4.uproject \
  -run=pythonscript -script=Tools/Editor/ValidateRA4UI.py -unattended -nop4
```

Expected: 24 reference contracts and every required Widget Blueprint pass.

- [ ] **Step 3: Run complete test suite**

Run headless core tests, Unreal UI automation, Widget Blueprint compilation and packaged Development client smoke test.

- [ ] **Step 4: Run visual acceptance**

Capture all 24 screens at 1672×941. Classify differences as critical, major or minor. No critical difference may remain; major differences require an explicit recorded decision.

- [ ] **Step 5: Update documentation with measured facts**

Record exact test counts, build commands, performance measurements and remaining non-code art dependencies. Do not mark screens verified unless they were opened and captured in Unreal.

- [ ] **Step 6: Commit**

```bash
git add Tools/Editor/ValidateRA4UI.py Config/Automation/RA4UIVisualTests.ini Docs/QA/UI_VISUAL_ACCEPTANCE.md Docs/Production/UI_UX_BIBLE.md Docs/Agent/PROJECT_STATE.md Docs/Agent/NEXT_ACTIONS.md
git commit -m "test(ui): gate all reference screens for release"
```
