# План реализации интерфейса Scarlet Horizon (C++ / UMG / Slate)

> Документ отражает **проверенное** состояние. Всё, что помечено «сделано»,
> прошло компиляцию и автотесты; всё остальное честно помечено как незакрытое.

## 1. Аудит

### 1.1 Стек и команды

| Параметр | Факт |
|---|---|
| Проект | `/Users/romanmolodyko/Documents/Scarlet-Horizon/RedAlert4.uproject` |
| Движок | Unreal Engine **5.8**, Mac ARM64 (`CLAUDE.md` называет 5.6 — устарело) |
| Правила агентов | `AGENTS.md` в репозитории **отсутствует**; действует `CLAUDE.md` |
| Модуль UI | `Source/RA4UI` — C++, UMG, Slate, CommonUI, ModelViewViewModel |
| Связанные модули | `RA4Presentation`, `RA4Input`, `RA4Content`, `RA4Simulation`, `RA4Campaign`, `RedAlert4` |

```bash
# Сборка редактора
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4Editor Mac Development \
  -Project="/Users/romanmolodyko/Documents/Scarlet-Horizon/RedAlert4.uproject" -waitmutex

# Автотесты интерфейса
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "/Users/romanmolodyko/Documents/Scarlet-Horizon/RedAlert4.uproject" \
  -ExecCmds="Automation RunTests RA4.UI; Quit" \
  -unattended -nopause -nosplash -nullrhi -ReportExportPath=<dir>

# Детерминированное ядро
cmake -S Tools/HeadlessBuild -B build/hb && cmake --build build/hb -j8 && ./build/hb/RA4Tests
```

### 1.2 Ключевая находка аудита

Накопленная UI-работа (≈245 файлов, ~20 000 строк) **не компилировалась и ни разу
не проверялась**. Сборка падала на 21 ошибке, автотесты — на 4 проверках.
Первым делом чинилась сборка, только потом велась содержательная работа.

### 1.3 Матрица экранов

| Экран / класс | Референс | Сохранено | Заменено | Данные |
|---|---|---|---|---|
| `URA4SplashScreenWidget` | `01_title_screen.png` | Контракт роутинга, анимация входа | Логотип Scarlet Horizon, дальний горизонт | `ERA4UIScreenId::Splash` |
| `URA4MainMenuScreenWidget` | `02_main_menu.png` | Композиция, кнопки, карточки | Палитра из реестра, карточки «Сводка фронтов / Текущая операция / Состояние сети» | `URA4MainMenuViewModel` |
| `URA4CampaignSelectWidget` | `05`–`10_*_vivid.png` | Трёхшаговая навигация, хлебные крошки | Асимметричная композиция, вес/плотность/силуэт из данных | `FRA4BlocInfo`, `FRA4CountryInfo`, `FRA4DoctrineInfo` |
| `URA4MissionMapScreenWidget` | `07_campaign_map_eurasian.png` | Узлы-кнопки, прогресс главы | Сливовая палитра вместо красной, 8 операций главы 4 | `FRA4MissionNodeView` |
| `URA4BriefingScreenWidget` | `08_operation_briefing_eurasian.png` | Структура целей и разведданных | Операция «Тихий релей», командир Ирина Волкова | `URA4CampaignViewModel` |
| `URA4VideoCommsScreenWidget` | `09_secure_channel_eurasian_atlantic.png` | Два канала, управление сеансом | Волкова ↔ Рид вместо прежней пары | — |
| `URA4LoadingScreenWidget` | `10_mission_loading_eurasian.png` | Прогресс, подсказки | Сводка и цели операции | — |
| `URA4LobbyScreenWidget` | `11_multiplayer_lobby.png` | Виртуализованный список, чат, готовность | Отдельные поля блока, страны и доктрины | `FRA4LobbyPlayerView` |
| `URA4FactionHUDWidget` / `URA4HUDWidget` / `URA4SidebarWidget` | `12`–`18_battle_hud_*.png` | Snapshot симуляции, миникарта, очередь | Пять стилевых профилей, бюджет поля боя 65–72% | `FRA4HUDSnapshotView` |
| `URA4ShowcaseWidget` (28 экранов) | вся серия | Механика показа | Полный перенос содержимого в новый мир | — |

## 2. Состояние по этапам

| Этап | Статус | Проверено |
|---|---|---|
| 0. Аудит | **Готово** | Документ обновлён по факту |
| 1. Общая UI-система | **Готово** | Палитра пяти направлений — единственный источник: `FRA4FactionDataRegistry`. Showcase, mission flow и главное меню больше не держат локальных копий цветов |
| 2. Сквозной путь выбора | **Частично** | Данные и композиция готовы, сборка и тесты зелёные; **скриншотов из редактора нет** |
| 3. Кампания | **Частично** | Экраны собираются, глава Евразийского пакта соответствует референсу; кампании США, Китая, Японии и Ирана содержат по 1–4 миссии вместо 12 |
| 4. Сетевое лобби | **Частично** | Поля блока, страны, доктрины и готовности есть в модели и тестах; визуальная проверка не проводилась |
| 5. Боевой HUD | **Частично** | Бюджет поля боя 65–72% закреплён автотестом `RA4.UI.HUD.Layout.BattlefieldKeepsItsShareOfTheScreen` |
| 6. Масштабирование и управление | **Не сделано** | Сайдбар масштабируется по высоте вьюпорта (0.82–1.45), остальные разрешения не проверялись |
| 7. Проверка | **Частично** | Сборка и автотесты — да; запуск игры и скриншоты — нет |

## 3. Проверки, которые реально выполнялись

| Проверка | Результат |
|---|---|
| `UnrealBuildTool RedAlert4Editor Mac Development` | `Result: Succeeded`, 0 ошибок |
| `Automation RunTests RA4.UI` | 26 succeeded, 0 failed |
| `build/hb/RA4Tests` | 673 passed, 0 failed |

Предупреждения: 3 штуки, все `FSlateFontInfo is deprecated` (UE 5.6 API),
существовали до этих изменений.

## 4. Обязательные визуальные правила и как они закреплены

| Правило | Где закреплено |
|---|---|
| Евразийский пакт — фиолетовый, никогда не красный | `GetBlocPrimaryColor/GetBlocAccentColor`; mission flow переведён со `MissionRed` на `MissionAccent` |
| Scarlet — только горизонт и тревога | `GetHorizonScarletColor()`; используется в заставке, тревожных экранах и рамке главного меню |
| Независимые державы — не союз | `FRA4BlocInfo::bIsCategoryOnly`; заголовок «СТРАНЫ КАТЕГОРИИ», отсутствие общей рамки, собственный акцент у каждой страны |
| Не одинаковые вертикальные карточки | `LayoutWeight`, `PanelDensity`, `FrameRail` у блока и `LayoutWeight` у страны |
| Хронолегион — только Legacy | `ERA4FactionTheme::Chronolegion` помечен «(Legacy)», названия в UI получают суффикс `(Legacy)` |
| Нет старых названий в UI | Сплошная проверка `Source/RA4UI` по СССР / Альянс / Хронолегион / Тесла / Политрук / RED ALERT — совпадений нет |

## 5. Что осталось сделать вручную в Unreal Editor

C++ этого не решает — `.uasset` и `.umap` текстом не редактируются.

1. Переименовать текстуры с устаревшими именами и обновить ссылки:
   - `T_RA4_USSR_CommandCenter`, `T_RA4_USSR_MainMenuChrome`, `T_RA4_USSR_MissionMap`,
     `T_RA4_USSR_CampaignCommander`, `T_RA4_USSR_LoadingKyiv`,
     `T_RA4_PanelGradient_USSR`, `T_RA4_Allies_CampaignCommander`.
2. Заменить содержимое `T_RA4_Logo` на логотип Scarlet Horizon.
3. Подготовить сценические слои по новым референсам (командир, театр, карта)
   как отдельные элементы, **не** как полноэкранные фоны.
4. Сделать скриншоты реализованных экранов и сравнить с референсами.

## 6. Известные ограничения

- Ни один экран не проверялся визуально: редактор в этой сессии запускался
  только в headless-режиме для автотестов.
- Нумерация референсов в `RA4UIScreenCatalog` (13–24) осталась от старого
  набора `SCREENSHOTS`, новый набор нумеруется 01–19. Сопоставление требует
  отдельного ADR.
- Технические имена `SovietCampaign`, `AlliesHud`, `ChronoHud`,
  `ERA4UIScreenVariant::SovietBattle` остались в перечислениях. Игроку они не
  видны; переименование меняет публичный интерфейс и требует ADR.
- Кампании США, Китая, Японии и Ирана наполнены не полностью.
- Раскладки для 2560×1440, 3840×2160, 1920×1200 и ультраширокого экрана не
  проверялись; поддержка контроллера не проверялась.
