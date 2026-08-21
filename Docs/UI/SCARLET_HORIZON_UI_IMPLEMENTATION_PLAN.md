# План реализации пользовательского интерфейса Scarlet Horizon (C++ / UMG / Slate)

## 1. Результаты аудита текущего состояния

### 1.1 Среда и стек проекта
- **Проект:** `/Users/romanmolodyko/Documents/Scarlet-Horizon/RedAlert4.uproject`
- **Версия движка:** Unreal Engine 5.8 (Mac ARM64)
- **Основной модуль UI:** `Source/RA4UI` (C++, UMG, Slate, CommonUI)
- **Связанные модули:** `Source/RA4Presentation`, `Source/RA4Input`, `Source/RA4Content`, `Source/RA4Simulation`, `Source/RA4Campaign`, `Source/RedAlert4`
- **Сборка и тесты:**
  - Headless/CI: `cmake --build Tools/HeadlessBuild/build && ctest --test-dir Tools/HeadlessBuild/build --output-on-failure`
  - Unreal Editor UBT: `"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/DotNet/10.0/mac-arm64/dotnet" "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll" RedAlert4Editor Mac Development -Project="/Users/romanmolodyko/Documents/Scarlet-Horizon/RedAlert4.uproject"`

### 1.2 Матрица экранов и компонентов

| Существующий экран / класс | Новый референс | Что сохраняется | Что меняется и переносится | Необходимые данные |
|---|---|---|---|---|
| `URA4SplashScreenWidget` | `01_title_screen.png` | Контракт роутинга, анимация появления | Логотип Scarlet Horizon, дальний панорамный горизонт 2035-2045, слоган «Альтернативная современность», минималистичный футер | `ERA4UIScreenId::Splash` |
| `URA4MainMenuScreenWidget` | `02_main_menu.png` | Модель командного центра, кнопки перехода, статус профиля | Левое угловое меню с шевронами, панорамный командный зал с голографической картой мира, карточки «Сводка фронтов», «Текущая операция», «Состояние сети» | `URA4MainMenuViewModel` |
| `URA4CampaignSelectWidget` | `05_block_overview_cinematic_vivid.png`, `06_eurasian_country_russia_vivid.png` .. `10_independent_countries_vivid.png` | Навигация и выбор, анимация входа | Полный трёхступенчатый путь `Блок/Категория → Страна → Доктрина`, поддержка 5 направлений (Евразия, Атлантика, Восток, Пакт, Независимые), карточки доктрин, статы | `FRA4BlocData`, `FRA4CountryData`, `FRA4DoctrineData` |
| `URA4CampaignScreenWidget` / `URA4MissionFlowWidgets` | `03_campaign_eurasian_russia.png` .. `06_campaign_pacific_japan.png`, `19_campaign_independent_iran.png`, `07_campaign_map_eurasian.png` .. `10_mission_loading_eurasian.png` | Общая структура миссий и брифингов | Персонажи-командиры, прогресс глав, интерактивная тактическая карта операций со связями, защищённый канал связи двух командиров, экран загрузки с миссионной иллюстрацией и задачами | `FRA4CampaignChapter`, `FRA4MissionNode`, `FRA4BriefingData` |
| `URA4LobbyScreenWidget` | `11_multiplayer_lobby.png` | Виртуализированный список игроков, чат, проверка готовности | Колонки: Игрок, Команда, Блок/Категория, Страна, Доктрина, Готовность; пресеты карты «Архипелаг Тифон», параметры лобби | `FRA4LobbyPlayerView` с полями Bloc, Country, Doctrine |
| `URA4HUDWidget` / `RA4FactionHUDWidget` / `RA4SidebarWidget` | `12_battle_hud_eurasian_ground.png` .. `18_battle_hud_pacific_base.png` | Интеграция со snapshot симуляции, панель ресурсов, миникарта | Фракционные профили HUD (Евразия - фиолетовый/металл/РЭБ; Атлантика - кобальт/авиация/флот; Восток - изумруд/золото/дроны; Пакт - бирюза/робототехника; Иран - асимметричный удар), круговая и секторная индикация готовности, проценты на иконках производства | `FRA4HUDSnapshotView`, `URA4UITheme` |
| `URA4SkirmishSetupWidget` | `05_block_overview_cinematic_vivid.png` | Валидация конфликтов цветов/слотов, выбор правил | Выбор блока, страны и доктрины для каждого слота вместо устаревших 4 фракций | `FRA4SkirmishSlotConfig` |

---

## 2. Этапы выполнения

- **Этап 0. Аудит** (Выполнен, зафиксирован план)
- **Этап 1. Общая UI-система стилей и данных (`RA4UITheme`, `RA4FactionData`, палитры)**
  - Обновление перечислений и структур данных: 5 направлений (Евразийский пакт, Атлантический альянс, Восточная коалиция, Тихоокеанский пакт, Независимые державы).
  - Единый репозиторий данных стран и доктрин (`FRA4CountryData`, `FRA4DoctrineData`).
  - Фирменные палитры, шрифты, стили кнопок, рамок и индикаторов.
- **Этап 2. Первый сквозной путь: `Заставка → Главное меню → Выбор Блока → Выбор Страны → Выбор Доктрины`**
  - Реализация `URA4SplashScreenWidget` (логотип Scarlet Horizon, горизонт).
  - Реализация `URA4MainMenuScreenWidget` (командный центр, карточки фронтов).
  - Реализация интерактивного экрана выбора с поддержкой хлебных крошек `01 Блок → 02 Страна → 03 Доктрина`.
  - Поддержка России, США, Китая, Японии, Ирана и расширяемость для остальных стран.
- **Этап 3. Кампания: Экраны фракционных кампаний, карта операций, брифинг, видеосвязь, загрузка**
  - Поддержка кампаний Евразии (Россия), Атлантики (США), Востока (Китай), Тихоокеанского пакта (Япония) и независимого Ирана.
  - Экран ветвящейся тактической карты операций (`07_campaign_map_eurasian.png`).
  - Экран брифинга с целями и разведданными (`08_operation_briefing_eurasian.png`).
  - Экран защищённого канала связи (`09_secure_channel_eurasian_atlantic.png`).
  - Экран загрузки миссии (`10_mission_loading_eurasian.png`).
- **Этап 4. Сетевое лобби (`URA4LobbyScreenWidget`)**
  - 8-пользовательское лобби с разделением на Игрок, Команда, Блок/Категория, Страна, Доктрина, Готовность (`11_multiplayer_lobby.png`).
- **Этап 5. Боевой HUD и профили фракций**
  - Поддержка 5 визуальных стилей HUD с сохранением 68-72% площади под поле боя.
  - Отображение процентов прогресса и круговой индикации прямо на карточках производства.
- **Этап 6. Адаптивность, разрешения и навигация**
  - Безопасные отступы, поддержка 16:9, 16:10, 21:9, контроллер, клавиатура, мышь.
- **Этап 7. Сборка, верификация тестов и запуск игры.**
