# ArtSource — pipeline генерации арта

Канон v3 (Scarlet Horizon) для фракций **Евразийский пакт** и **Атлантический альянс**. Старые блокаут-ассеты v2 (Soviet/Alliance/Coalition/Chronolegion) остаются в `RA4/` как legacy-референс.

## Структура

```
ArtSource/
├── AssetManifest_v3.csv      # Production-трекинг всех 70 ассетов v3
├── Prompts/                  # Промпт-блоки для генеративных сетей
│   ├── README.md             # Индекс + как использовать
│   ├── 01_Style_common.md    # Общий блок STYLE (2D)
│   ├── 02_Units_2D_concepts.md   # 39 юнитов
│   ├── 03_Units_3D_models.md     # 3D game-ready вариант
│   ├── 04_Buildings.md           # 36 зданий
│   ├── 05_Superweapons.md        # 11 ключей супероружия
│   ├── 06_Doctrine_skins.md      # 4-ранговые скины ветеранов
│   ├── 07_National_accents.md    # Национальные маркировки
│   ├── 08_Portraits.md           # Портреты командиров/EVA
│   └── 09_UI_icons.md            # UI-иконки 256×256
├── Concepts/{Pact,Alliance}/     # 2D-концепты (по Stable ID)
├── Models/{Pact,Alliance}/       # 3D game-ready (.fbx/.blend)
├── Icons/{Pact,Alliance}/        # UI-иконки 256×256 (.png)
├── Portraits/{Pact,Alliance}/    # Портреты командиров/EVA
├── Doctrines/{Pact,Alliance}/    # 4-ранговые скины
├── Buildings/{Pact,Alliance}/    # Здания (отдельно от юнитов)
├── DetailSheets/                 # Национальные маркировки
├── Superweapons/                 # Ключевые кадры супероружия
└── RA4/                          # Существующие блокаут-ассеты v2 (legacy)
```

## Канон и источники истины

- **Библия фракций**: `SCARLET_HORION_Factions_Modern_World.md` разделы 6 (Пакт — Россия) и 7 (Альянс).
- **Палитра**: `Assets/RA4UI/Generated/ScarletHorizonRemaster/README.md:9` — Пакт = фиолетовый (НЕ красный), Альянс = холодная синяя сталь.
- **Инварианты проекта**: `CLAUDE.md` — запрет на нелицензированные ассеты, копирование реальных мировых логотипов/дизайна, чужой IP Electronic Arts / Command & Conquer.

## Именование (Stable ID)

Имя файла = Stable ID из библии. Движок подхватывает ассеты без переименования.

| Фракция | Префикс | Пример |
| --- | --- | --- |
| Пакт — Россия | `RU_` | `RU_GranitMBT.png`, `SM_Pact_RU_GranitMBT.fbx` |
| Альянс — общий пул | `ATL_` | `ATL_SentinelRifleman.png`, `SM_Alliance_ATL_SentinelRifleman.fbx` |
| Альянс — США | `US_` | `US_OracleArtillery.png`, `SM_Alliance_US_OracleArtillery.fbx` |
| Альянс — Великобритания | `GB_` | `GB_LongwatchSniper.png`, `SM_Alliance_GB_LongwatchSniper.fbx` |
| Альянс — Франция | `FR_` | `FR_KestrelScout.png`, `SM_Alliance_FR_KestrelScout.fbx` |
| Альянс — Германия | `DE_` | `DE_BulwarkMBT.png`, `SM_Alliance_DE_BulwarkMBT.fbx` |

## Путь импорта в Unreal

Из `AssetManifest_v3.csv` колонка `FilePath`:

- Пакт: `Content/RA4/Art/Pact/{Buildings,Units}/SM_Pact_<StableID>.fbx`
- Альянс: `Content/RA4/Art/Alliance/{Buildings,Units}/SM_Alliance_<StableID>.fbx`

## Пайплайн работы

1. **Генерация**: открыть нужный блок из `Prompts/`, скопировать `STYLE` + список юнитов в генератор.
2. **Импорт**: сохранить результат в соответствующую папку (`Concepts/`, `Models/`, `Icons/` и т.д.) с именем = Stable ID.
3. **Трекинг**: при завершении ассета сменить `ExportStatus` в `AssetManifest_v3.csv` с `PENDING` на `REVIEW` → `APPROVED`.
4. **Импорт в UE**: разместить `.fbx` по пути из колонки `FilePath`, иконки — в `Content/RA4/UI/Icons/` (симметрично `ArtSource/Icons/`).

## Состояние

- Промпты: **готово** (9 блоков + README).
- Манифест: **готово** (70 ассетов, все `PENDING`).
- Папки: **созданы** (пустые, ждут генерацию).
- Ассеты: **0/70 сгенерировано**.

## Связанные документы

- `AssetAcquisitionPlan.md` — план приобретения сторонних ассетов (если требуется).
- `AssetRequirements.csv` — старый v2-требования (11 строк, legacy).
- `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` — исторический v2 источник.
- `Docs/AgentHandoffs/art.md` — handoff для арт-агента (v2 блокаут).