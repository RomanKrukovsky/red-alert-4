# Сборка интерфейса Red Alert 4 в Unreal Editor

Этот документ — точная карта визуальной сборки для редактора Unreal Engine 5.6. Она основана на 24 файлах из `SCREENSHOTS`. Файлы `.uasset` намеренно не создаются текстом: это бинарные объекты Unreal, которые должен создать редактор.

## Общий корень каждого экрана

Каждый `WBP_RA4_*` наследуется от указанного C++-класса, а корнем использует `SafeZone → Overlay`. Основная композиция строится в `ScaleBox` с `Scale To Fit`, контрольное разрешение — 1920×1080. Боковые HUD-панели привязываются к краям, центральные панели — к центру, нижняя командная панель — к нижнему краю. Не используйте `CanvasPanel` с фиксированными координатами как единственный контейнер.

Связывайте текст и состояние только с `URA4UIScreenViewModel` через MVVM. Кнопки вызывают `NavigateToScreen`, `NavigateBack` или методы ViewModel. Не обращайтесь к `SimWorld`, актёрам или компонентам юнитов из виджетов.

## Темы

| Data Asset | Цвет рамки | Второй цвет | Экранные фоны |
| --- | --- | --- | --- |
| `DA_UITheme_USSR` | `#D92525` | `#160607` | `1, 2, 4, 8–13, 19–21` |
| `DA_UITheme_Allies` | `#3D9BE9` | `#0A1726` | `5, 11, 14, 22, 23` |
| `DA_UITheme_Eastern` | `#D7B14E` | `#123025` | `6, 15, 18` |
| `DA_UITheme_Chrono` | `#A95CFF` | `#160B25` | `7, 16, 24` |

Для всех тем создайте один базовый материал панели `M_UI_AngularPanel`: чёрная полупрозрачная заливка, двухпиксельная внешняя рамка, срезанные углы, параметр `AccentColor` и слабое внешнее свечение. Экземпляры `MI_UI_*` получают цвет из `DA_UITheme_*`.

## Widget Blueprints по референсам

| Референс | Widget Blueprint | C++-класс | Ключевые настоящие элементы |
| --- | --- | --- | --- |
| 1 | `WBP_RA4_Splash` | `URA4SplashWidget` | логотип, фоновая иллюстрация, подсказка продолжения |
| 2 | `WBP_RA4_MainMenu` | `URA4MainMenuWidget` | вертикальное меню, карточка профиля, новости, статус сети |
| 3 | `WBP_RA4_FactionSelect` | `URA4FactionSelectWidget` | четыре карточки фракций, описание, кнопка продолжения |
| 4 | `WBP_RA4_Campaign_USSR` | `URA4SovietCampaignWidget` | портрет маршала, цели, выбор операции |
| 5 и 11 | `WBP_RA4_Campaign_Allies` | `URA4AlliesCampaignWidget` | портрет адмирала, цели, вкладки кампании |
| 6 | `WBP_RA4_Campaign_Eastern` | `URA4EasternCampaignWidget` | портрет генерала, данные операции |
| 7 | `WBP_RA4_Campaign_Chrono` | `URA4ChronoCampaignWidget` | временные маркеры, данные операции |
| 8 | `WBP_RA4_MissionMap_USSR` | `URA4MissionMapWidget` | карта, узлы миссий, сложность, старт операции |
| 9 | `WBP_RA4_Briefing_USSR` | `URA4BriefingWidget` | сводка, задачи, параметры операции |
| 10 | `WBP_RA4_VideoComms` | `URA4VideoCommsWidget` | два видеоканала, таймкод, диалог |
| 12 и 19 | `WBP_RA4_Loading_USSR` | `URA4LoadingWidget` | прогресс, подсказка, статус инициализации |
| 13 | `WBP_RA4_HUD_USSR` | `URA4SovietHUDWidget` | ресурсы, мини-карта, очередь, карточки юнитов |
| 14 | `WBP_RA4_HUD_Allies` | `URA4AlliesHUDWidget` | ресурсы, мини-карта, авиационная очередь |
| 15 | `WBP_RA4_HUD_Eastern` | `URA4EasternHUDWidget` | ресурсы, мини-карта, производственная очередь |
| 16 | `WBP_RA4_HUD_Chrono` | `URA4ChronoHUDWidget` | ресурсы, мини-карта, хроно-индикаторы |
| 17 | `WBP_RA4_MultiplayerLobby` | `URA4LobbyWidget` | список игроков, чат, карта, слоты, готовность |
| 18 | `WBP_RA4_Campaign_EasternDetail` | `URA4EasternCampaignWidget` | расширенная карточка Восточной коалиции |
| 20 | `WBP_RA4_HUD_USSR_Battle` | `URA4SovietHUDWidget` | боевые цели, выделение, производство |
| 21 | `WBP_RA4_HUD_USSR_Alert` | `URA4SovietHUDWidget` | тревога, красные предупреждения, список целей |
| 22 | `WBP_RA4_HUD_Allies_Naval` | `URA4AlliesHUDWidget` | морская тактика, эскадра, мини-карта |
| 23 | `WBP_RA4_HUD_Allies_Air` | `URA4AlliesHUDWidget` | авиакрыло, способности, очередь |
| 24 | `WBP_RA4_HUD_ChronoSuperweapon` | `URA4ChronoHUDWidget` | заряд супероружия, таймер, цель |

Дополнительные экраны без отдельного референса: `WBP_RA4_Pause` (`URA4PauseWidget`), `WBP_RA4_Victory` (`URA4MatchResultWidget`), `WBP_RA4_Encyclopedia` (`URA4EncyclopediaWidget`), `WBP_RA4_TechTree` (`URA4TechTreeWidget`), `WBP_RA4_Mods` (`URA4ModsWidget`), `WBP_RA4_Settings` (`URA4SettingsWidget`). Они используют те же панель, тему, типографику и навигацию.

## Интерактивность и состояния

Создайте `WBP_RA4_Button` от `URA4ButtonBase` и четыре `CommonButtonStyle` (`CBS_RA4_USSR`, `CBS_RA4_Allies`, `CBS_RA4_Eastern`, `CBS_RA4_Chrono`). Для каждого состояния задайте отдельные brush/цвета: `Normal` — тёмная панель и тонкая рамка, `Hovered` — яркая рамка и лёгкое свечение, `Pressed` — тёмная заливка и смещение на 2 px, `Selected` — плотная акцентная рамка, `Disabled` — 35% непрозрачности без свечения.

Модальные окна добавляются в верхний `CommonActivatableWidgetStack`; закрытие вызывается через `NavigateBack`. Для вкладок используйте `CommonTabListWidgetBase`, для очередей — `ListView` с `FRA4ProductionQueueItem`, для лобби — `ListView` с `FRA4LobbyPlayerSlot`.

## Ввод, локализация и проверка

Создайте `IA_UI_Back`, `IA_UI_Confirm`, `IA_UI_Pause` в Enhanced Input и добавьте их в CommonUI action data. Исходный язык — `ru`; все пользовательские строки должны быть `FText`/`LOCTEXT` либо строки String Table `ST_RA4_UI_RU`.

В редакторе проверьте каждый экран в `1280×720`, `1920×1080`, `2560×1440`, `3840×2160` и `3440×1440`. На 21:9 центральная композиция остаётся в 16:9 `ScaleBox`, а свободные боковые зоны заполняет фон; HUD остаётся привязан к безопасным краям.
