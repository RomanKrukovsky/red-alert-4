# SCARLET HORIZON — Промпты для генерации арта

Промт-шаблоны для генеративных нейросетей: концепт-арты, 3D-модели, здания, супероружие, доктринные скины, национальные маркировки, портреты и UI-иконки. Все юниты и здания Пакта и Альянса.

Источник канона: `SCARLET_HORION_Factions_Modern_World.md` (разделы 6–7).
Палитра: `Assets/RA4UI/Generated/ScarletHorizonRemaster/README.md:9` — Пакт = фиолетовый (НЕ красный), Альянс = холодная синяя сталь.

## Файлы блоков

- `01_Style_common.md` — общий блок STYLE (2D-концепты).
- `02_Units_2D_concepts.md` — 39 юнитов, концепт-арт.
- `03_Units_3D_models.md` — 3D game-ready модели.
- `04_Buildings.md` — 36 зданий и обороны.
- `05_Superweapons.md` — 11 ключевых кадров супероружия.
- `06_Doctrine_skins.md` — 4-ранговые скины ветеранов.
- `07_National_accents.md` — национальные маркировки.
- `08_Portraits.md` — портреты командиров и EVA.
- `09_UI_icons.md` — UI-иконки 256×256.

## Как использовать

1. Скопируй блок `STYLE` (из `01`) + нужный список юнитов целиком в генератор.
2. Для единообразия держи `STYLE` и `PALETTE` неизменными во всех запросах; меняй только список юнитов.
3. Для разных выходов (2D-концепт / 3D-модель / иконка / портрет) выбери соответствующий `STYLE`-блок.
4. Stable ID в подписи изображения — для прямой интеграции с движком без переименования.

## Целевая структура папок

```
ArtSource/Concepts/{Pact,Alliance}/   (2D-концепты по Stable ID)
ArtSource/Models/{Pact,Alliance}/     (3D game-ready)
ArtSource/Icons/{Pact,Alliance}/      (UI 256×256)
ArtSource/Portraits/{Pact,Alliance}/   (командиры/EVA)
ArtSource/Doctrines/{Pact,Alliance}/   (4-ранговые скины)
ArtSource/DetailSheets/                (национальные маркировки)
ArtSource/Superweapons/                (ключи супероружия)
ArtSource/Buildings/{Pact,Alliance}/   (здания)
```

Имена файлов = Stable IDs из библии, движок подхватывает ассеты без переименования.

## Источники канона

- `SCARLET_HORION_Factions_Modern_World.md` — раздел 6 (Евразийский пакт — Россия), раздел 7 (Атлантический альянс).
- `Assets/RA4UI/Generated/ScarletHorizonRemaster/README.md:9` — палитра фракций (Пакт = фиолетовый, не красный).
- `CLAUDE.md` — инварианты проекта, запрет на нелицензированные ассеты и копирование реальных мировых логотипов/дизайна.