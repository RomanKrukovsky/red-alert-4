# Карта референсов и игровых экранов

Базовый холст всех наборов: **1672 × 941**. Экраны авторятся в холсте
`ReferenceSize` и масштабируются `ScaleToFit`; координаты с референса
переносятся множителем `ReferenceSize.X / 1672`.

## Наборы и их роли

| Набор | Кадров | Роль |
|---|---|---|
| `Assets/RA4UI/Generated/ScarletHorizonRemaster` | 19 | **Основная цель.** Композиция и содержание Scarlet Horizon |
| `Assets/RA4UI/Generated/ModernWorldUIReferences` | 12 | **Цель для выбора блока/страны/доктрины** — файлы `05–10_*_vivid`. Файлы `01–04` README помечает как черновики, они не используются |
| `SCREENSHOTS` | 24 | Легаси-источник. Только композиция там, где ремастер намеренно не дублировал. Содержит материалы сторонней серии, рабочий заголовок и советскую символику — в интерфейс не переносится |

## Соответствие: референс → игровой класс

Захват любого экрана: `Docs/UI/Tools/capture.sh <N>`, где `N` — номер в
`ARA4UIShowcaseGameMode::ShowInterface`.

| Референс (цель) | N | Класс | Статус |
|---|---|---|---|
| `01_title_screen` | 1 | `URA4SplashScreenWidget` | Не сверялся |
| `02_main_menu` | 2 | `URA4MainMenuScreenWidget` | Не сверялся |
| `05_block_overview_cinematic_vivid` | 3 | `URA4CampaignSelectWidget` (шаг «Блок») | **Сверен, ср. смещение 6,6 px по горизонтали** |
| `06_eurasian_country_russia_vivid` | 3 | `URA4CampaignSelectWidget` (шаг «Страна») | Не сверялся |
| `07_atlantic_country_usa_vivid` | 3 | тот же виджет, состояние «США» | Не сверялся |
| `08_eastern_country_china_vivid` | 3 | тот же виджет, состояние «Китай» | Не сверялся |
| `09_pacific_country_japan_vivid` | 3 | тот же виджет, состояние «Япония» | Не сверялся |
| `10_independent_countries_vivid` | 3 | тот же виджет, категория «Независимые державы» | Не сверялся |
| `03_campaign_eurasian_russia` | 4 | `URA4CampaignScreenWidget` (Евразийский пакт) | Не сверялся |
| `04_campaign_atlantic_usa` | 5 | `URA4CampaignScreenWidget` (Атлантический альянс) | Не сверялся |
| `05_campaign_eastern_china` | 6 | `URA4CampaignScreenWidget` (Восточная коалиция) | Не сверялся |
| `06_campaign_pacific_japan` | 7 | `URA4CampaignScreenWidget` (Тихоокеанский пакт) | Не сверялся |
| `07_campaign_map_eurasian` | 8 | `URA4MissionMapScreenWidget` | Не сверялся |
| `08_operation_briefing_eurasian` | 9 | `URA4BriefingScreenWidget` | Не сверялся |
| `09_secure_channel_eurasian_atlantic` | 10 | `URA4VideoCommsScreenWidget` | Не сверялся |
| `10_mission_loading_eurasian` | 12 | `URA4LoadingScreenWidget` | Не сверялся |
| `11_multiplayer_lobby` | 17 | `URA4LobbyScreenWidget` | Не сверялся |
| `12_battle_hud_eurasian_ground` | 13 | `URA4FactionHUDWidget` | Не сверялся |
| `13_battle_hud_atlantic_naval` | 22 | `URA4FactionHUDWidget` (AtlanticNaval) | Не сверялся |
| `14_battle_hud_eastern_base` | 15 | `URA4FactionHUDWidget` | Не сверялся |
| `15_battle_hud_pacific_air` | 23 | `URA4FactionHUDWidget` (AtlanticAir → нужен профиль Pacific) | Не сверялся |
| `16_battle_hud_independent_iran` | 17→ нужен отдельный номер | `URA4FactionHUDWidget` (IndependentMissiles, ссылка 17) | Конфликт номеров с лобби |
| `17_battle_hud_eurasian_base` | 21 | `URA4FactionHUDWidget` (EurasianBaseAlert) | Не сверялся |
| `18_battle_hud_pacific_base` | 16 | `URA4FactionHUDWidget` (PacificRobotics) | Не сверялся |
| `19_campaign_independent_iran` | 18 | `URA4CampaignScreenWidget` (нужен режим «Независимая держава») | Не сверялся |

## Известные расхождения нумерации

`ARA4UIShowcaseGameMode` нумерует экраны по легаси-набору `SCREENSHOTS`
(1–24). Набор ремастера нумеруется 01–19, а «vivid»-концепты — 05–10.
Совпадений номеров нет, и ссылка **17** одновременно означает лобби в
диспетчере и боевой HUD Ирана в `ConfigureReference`. Это нужно развести
отдельным ADR до того, как захват станет пакетным по всем экранам.

## Проверочный контур

```bash
Docs/UI/Tools/capture.sh 3                       # снимок игрового UI 1672x941
python3 Docs/UI/Tools/compare.py \
  Assets/RA4UI/Generated/ModernWorldUIReferences/05_block_overview_cinematic_vivid.png \
  Docs/UI/VisualDiff/current/03.png \
  Docs/UI/VisualDiff/out 03_vs_vivid05           # overlay, diff и смещения границ
```

Результаты сравнения складываются в `Docs/UI/VisualDiff` и не
версионируются: это проверочные артефакты, а не часть интерфейса.
