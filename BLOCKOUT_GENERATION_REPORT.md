# RA4 — Отчёт о генерации комплекта blockout-моделей (v2.0 Production Blockout)

**Дата:** 2026-07-29  
**Статус:** Полный комплект из 140 оригинальных blockout-моделей успешно сгенерирован, экспортирован в FBX и привязан к реестру ассетов.  
**Соблюдение правил Naming Reset:** 100% (использованы только новые Stable ID из библии v2.0).

---

## 1. Общий обзор выполненной работы

Создан полный функциональный комплект **140 3D blockout-моделей** для 4 фракций:
- **СССР (`Soviet`)**: 16 зданий + 19 юнитов/техники = 35 объектов
- **Альянс (`Alliance`)**: 16 зданий + 19 юнитов/техники = 35 объектов
- **Восточная коалиция (`Coalition`)**: 16 зданий + 19 юнитов/техники = 35 объектов
- **Хронолегион (`Chronolegion`)**: 16 зданий + 19 юнитов/техники = 35 объектов

Все объекты выполнены в точном масштабе Unreal Engine 5 (1 единица = 1 см), с опорными точками (Pivot) по центру основания на уровне земли `(0,0,0)`, правильными габаритами Footprint (в сетке клеток 200x200 см) и сокетами для башен, орудий, спавна, эффектов и входов.

---

## 2. Фракционная визуальная идентичность

### 2.1. СССР (`Soviet`)
- **Стиль:** Тяжёлая советско-российская индустриальная школа. Броневые плиты с винтами, монументальные формы, реакторные трубы, гусеничные траки, прямоугольные силуэты.
- **Цветовая палитра:** Тёмно-серый базовый металл (`#4D5966`) с яркими ало-красными маркерами (`#D62828`).

### 2.2. Альянс (`Alliance`)
- **Стиль:** Модульная американско-натовская военная техника. Острые углы скоса брони, композитные материалы, призматические излучатели, модульные навесы.
- **Цветовая палитра:** Светлый сине-серый корпус (`#5C6B73`) с кобальтово-синими элементами (`#0077B6`).

### 2.3. Восточная коалиция (`Coalition`)
- **Стиль:** Синтез китайской, японской и индийской инженерных школ. Восьмиугольные и пагодообразные элементы, многоногие шагоходы, приподнятые носы кораблей.
- **Цветовая палитра:** Бронзово-сланцевый базовый металл (`#3D4A41`) с нефритово-зелеными маркерами (`#2A9D8F`).

### 2.4. Хронолегион (`Chronolegion`)
- **Стиль:** Экспериментальные темпоральные конструкции. Парящие кольца стазиса, квантовые кристаллы, асимметричные призматические корпуса.
- **Цветовая палитра:** Тёмно-фиолетовый сланцевый корпус (`#3A1240`) с маджентово-голубыми свечениями (`#7209B7` / `#4CC9F0`).

---

## 3. Сводный реестр сгенерированных объектов

### 3.1. СССР (`Soviet`)
| Stable ID | Класс | Размеры (ШхДхВ, см) | Footprint | Подвижные части / Сокеты | Путь к FBX |
| --- | --- | --- | --- | --- | --- |
| `SU_MCV_MobileYard` | Здание | 800x700x500 | 4x4 | Корпус, Антенны / `SOCKET_Spawn`, `SOCKET_Entrance` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MCV_MobileYard_Blockout.fbx` |
| `SU_ConYard` | Здание | 800x800x600 | 4x4 | Штабной замок, Вышка / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ConYard_Blockout.fbx` |
| `SU_PowerPlant` | Здание | 600x600x500 | 3x3 | Градирни, ТЭЦ / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_PowerPlant_Blockout.fbx` |
| `SU_Refinery` | Здание | 800x800x450 | 4x4 | Загрузочная рампа, Силос / `SOCKET_Entrance` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Refinery_Blockout.fbx` |
| `SU_Barracks` | Здание | 600x600x400 | 3x3 | Ворота, Тир / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Barracks_Blockout.fbx` |
| `SU_WarFactory` | Здание | 800x800x500 | 4x4 | Ангар, Кран / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_WarFactory_Blockout.fbx` |
| `SU_Airfield` | Здание | 800x800x450 | 4x4 | ВПП, Вышка / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Airfield_Blockout.fbx` |
| `SU_NavalYard` | Здание | 1000x1000x500 | 5x5 | Док, Пирс / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_NavalYard_Blockout.fbx` |
| `SU_Radar` | Здание | 400x400x700 | 2x2 | Радарная тарелка / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Radar_Blockout.fbx` |
| `SU_TechCenter` | Здание | 600x600x550 | 3x3 | Энергокупол / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_TechCenter_Blockout.fbx` |
| `SU_GunTurret` | Здание | 400x400x300 | 2x2 | Башня, Ствол / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GunTurret_Blockout.fbx` |
| `SU_AATurret` | Здание | 400x400x350 | 2x2 | Ракетный блок / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_AATurret_Blockout.fbx` |
| `SU_TeslaTower` | Здание | 400x400x550 | 2x2 | Катушка Перун / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_TeslaTower_Blockout.fbx` |
| `SU_Bunker` | Здание | 400x400x250 | 2x2 | Амбразура / `SOCKET_Entrance` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Bunker_Blockout.fbx` |
| `SU_SuperweaponDome` | Здание | 1000x1000x850 | 5x5 | Купол генератора / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SuperweaponDome_Blockout.fbx` |
| `SU_SuperweaponSilo` | Здание | 1000x1000x950 | 5x5 | Шахта ракеты / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SuperweaponSilo_Blockout.fbx` |
| `SU_RubezhRifleman` | Пехота | 60x60x180 | 1x1 | Торс, АК / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RubezhRifleman_Blockout.fbx` |
| `SU_ZapalGrenadier` | Пехота | 70x70x185 | 1x1 | Рюкзак, Гранатомёт / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZapalGrenadier_Blockout.fbx` |
| `SU_ZaslonAATeam` | Пехота | 65x65x180 | 1x1 | ПЗРК / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZaslonAATeam_Blockout.fbx` |
| `SU_MasterEngineer` | Пехота | 60x60x175 | 1x1 | Рюкзак сапёра / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MasterEngineer_Blockout.fbx` |
| `SU_RazryadTrooper` | Пехота | 80x80x195 | 1x1 | Тесла-костюм / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RazryadTrooper_Blockout.fbx` |
| `SU_VektorOfficer` | Пехота | 60x60x180 | 1x1 | Радиостанция / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_VektorOfficer_Blockout.fbx` |
| `SU_BogatyrOreCarrier` | Сборщик | 600x360x280 | 1x1 | Ковш, Колёса / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BogatyrOreCarrier_Blockout.fbx` |
| `SU_RysScout` | Лёгкая техника | 380x220x180 | 1x1 | Колёса, Пулемёт / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RysScout_Blockout.fbx` |
| `SU_GranitMBT` | Танк | 550x330x220 | 1x1 | Башня, Ствол, Гусеницы / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GranitMBT_Blockout.fbx` |
| `SU_ZarevoMLRS` | Артиллерия | 600x320x250 | 1x1 | Пакет ракет, Кабина / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZarevoMLRS_Blockout.fbx` |
| `SU_GromoboyRam` | Спецтехника | 580x340x240 | 1x1 | Электротаран / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GromoboyRam_Blockout.fbx` |
| `SU_VoevodaHeavyTank` | Тяжёлый танк | 720x460x350 | 1x1 | Две башни, Стволы / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_VoevodaHeavyTank_Blockout.fbx` |
| `SU_KrechetInterceptor` | Авиация | 650x500x180 | 1x1 | Крылья, Сопла / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_KrechetInterceptor_Blockout.fbx` |
| `SU_KorshunGunship` | Авиация | 700x550x250 | 1x1 | Винт, Турель / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_KorshunGunship_Blockout.fbx` |
| `SU_GromadaAirship` | Авиация | 1300x600x450 | 1x1 | Корпус дирижабля, Бомболюк / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GromadaAirship_Blockout.fbx` |
| `SU_BuranPatrolBoat` | Флот | 1000x350x280 | 1x1 | Корпус, Башня / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BuranPatrolBoat_Blockout.fbx` |
| `SU_MorokSubmarine` | Флот | 1400x350x320 | 1x1 | Рубка, Торпедный аппарат / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MorokSubmarine_Blockout.fbx` |
| `SU_SvyatogorCruiser` | Флот | 2000x600x500 | 1x1 | Рубка, Ракетные шахты / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SvyatogorCruiser_Blockout.fbx` |
| `SU_Hero_Morozova` | Герой | 70x70x185 | 1x1 | Винтовка, Костюм / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Hero_Morozova_Blockout.fbx` |

*(Таблицы для `Alliance`, `Coalition` и `Chronolegion` выполнены аналогичным образом и зафиксированы в CSV-манифесте `Content/RA4/Art/Blockout/Blockout_Manifest.csv`)*.

---

## 4. Качество и проверка готовых FBX-ассетов
1. **Pivot & Transforms:** Все ассеты имеют единичный масштаб (Scale = 1.0, 1.0, 1.0) и обнулённое вращение.
2. **Габариты UE5:**
   - Пехота: ~180 см в высоту, ширина 60–80 см.
   - Танки: ~500–700 см в длину, 320–460 см в ширину, 220–350 см в высоту.
   - Здания: от 400x400 см (2x2) до 1000x1000 см (5x5).
3. **Коллизии:** Настройки простых хитбоксов и меш-коллизий сохранены в манифесте.

---

## 5. Заключение

Сгенерированные ассеты полностью заменяют встроенные примитивы `Cube` и готовы к использованию в подсистеме `RA4SimWorldSubsystem` и тестовых картах.
