# RA3 Mechanic Parity Matrix

Полная сверка механик Red Alert 3 (base game + Uprising) с текущим состоянием
симуляционного ядра Scarlet-Horizon. Статусы:

- **DONE** — реализовано в симуляции, покрыто тестами.
- **PARTIAL** — каркас есть, части не хватает.
- **ROADMAP** — отсутствует; зафиксировано как следующий этап.

Последнее обновление: после пакета "RA3 tactical batch" (статус-эффекты,
транспорты/мультиганнер, захват инженером, техздания, стены).

---

## 1. Базовая экономика и строительство

| Механика RA3 | Статус | Где |
|---|---|---|
| Рудные поля, сбор харвестерами | DONE | `SystemHarvesters` |
| Регенерация руды (`bRegrows`) | DONE | `SystemHarvesters`, тест `Economy.RegrowingField...` |
| Богатая руда (×2 цена за единицу) | DONE | `resource.rich_ore_field` + `CargoDef` |
| Оплата по узлу-источнику | DONE | `HarvesterComp::CargoDef` |
| Раскидывание харвестеров по полям | DONE | `FindNearestResourceNode` load tally |
| Энергосистема с уровнями дефицита | DONE | ADR-0013, PowerTier |
| Поточная оплата построек (flow payment) | DONE | ADR-0012 |
| Продажа зданий с возвратом | DONE | SellBuilding, 50% |
| Платный ремонт зданий | DONE | SystemRepair |
| Очередь стройки + размещение по радиусу | DONE | PlaceBuilding/build radius |
| MCV развёртывание / сворачивание | DONE | Deploy, MCVDeployed/Undeployed |
| **Нефтяные вышки (нейтральные tech-здания) с пассивным доходом** | **DONE** | `bIsTechBuilding`, `SystemTechIncome`, инженер |
| Захват нейтральных/вражеских зданий инженером | **DONE** | `CaptureBuilding`, канал 100 тиков, инженер расходуется |
| Апгрейды (покупаемые глобальные улучшения) | ROADMAP | нужен UpgradeDef + модификаторы игрока |

## 2. Боевые системы

| Механика RA3 | Статус | Где |
|---|---|---|
| Матрица урона (warhead × armor) | DONE | DamageMatrix из библии |
| Ветерианство (Veteran/Elite/Heroic) | DONE | kill-value, бонусы урона/HP, реген, события |
| Давление пехоты колёсами/гусеницами (crush) | DONE | SystemMovement crush pass |
| Гарнизон гражданских зданий | DONE | GarrisonUrbanCombat |
| Анти-гарнизонное оружие (огонь/крио/токсин выкуривают) | DONE | ApplyAntiGarrisonAttack |
| Вторичные способности юнитов (F-key) | DONE (каркас) | ToggleSecondaryAbility, баффы скорости/брони/отключения оружия |
| **Статус-эффекты: стан (EMP)** | **DONE** | `StatusComp::StunTicks`, `WeaponDef::StunTicksOnHit` |
| **Заморозка крио + двойной урон** | **DONE** | `FreezeTicks`, множитель ×2 в ApplyDamage |
| **Уменьшение (shrink ray), двойной урон** | **DONE** | `ShrinkTicks` |
| **Заражение терродроном (DoT)** | **DONE** | `InfectionTicks`, 1 hp/тик; лечение сбрасывает (когда появится ремонт юнитов) |
| **Неуязвимость (Iron Curtain / time belt)** | **DONE** | `InvulnerableTicks`, проверяется во всех путях урона |
| Разбить замороженного (shatter) | PARTIAL | покрыт двойным уроном; отдельного «осколочного» события нет |
| Мгновенное раздавливание уменьшенной техники | ROADMAP | нужна правка crush-pass |
| Транспорт-пассажиры (Bullfrog/Twinblade/Riptide) | **DONE (каркас)** | Board/Unload команды, `TransportComp`/`PassengerOf`, пассажиры погибают с транспортом |
| Multigunner: оружие от пассажира | **DONE (каркас)** | `ResolveFireWeapon`, `bMultigunner`; контентных носителей пока нет |
| Выстрел пехоты «пушкой» (man-cannon fling) | ROADMAP | баллистика выброса |
| Авиация: боезапас + возврат на базу (RTB) | **DONE** | CombatComp Ammo, FindNearestRearmPoint, save v12; тест AircraftOps.* |
| Подводные лодки (submerge/invisible state) | ROADMAP | stealth-состояние |
| Дезактивация оружия (Hydrofoil jammer) | ROADMAP | статус «weapon disabled» |
| Лазерный целеуказатель Guardian (бафф урона союзникам) | ROADMAP | аура-модификатор |
| Щит Athena / Force Shield | ROADMAP | щитовой HP-слой |
| Дискретный реверс движения танков | ROADMAP | движение задним ходом |

## 3. Протоколы и супероружие

| Механика RA3 | Статус | Где |
|---|---|---|
| Дерево протоколов, ветки/тиры, очки | DONE | ProtocolRuntime |
| Кулдауны сил | DONE | PlayerProtocolState.Cooldowns |
| Орбитальный удар (обломки) | DONE | OrbitalStrike + ExoticSuperweaponPhysics |
| Орбитальный десант (TroopDrop) | **DONE** | ProtocolRuntime, тест Protocols.TroopDrop* |
| Салваж-бонус за убийства (SalvageBounty) | **DONE** | ProcessSimEvents + AddCredits |
| Фазовое поле ChronoLegion (массовая неуязвимость своим) | **DONE** | PhaseField → ApplyStatusInRadius |
| ЭМП-импульс по области (EmpPulse) | **DONE** | PhaseField-семейство, стан врагам |
| Камикадзе-эскадрилья (KamikazeSquadron) | **DONE** | N ракет с falloff, детерминированный RNG |
| Магнитный сателлит (притягивание техники) | DONE | MagneticDraw |
| Крио-заморозка области | DONE | CryoFreeze |
| Бомба замедленного действия | DONE | TimeBomb |
| Аварийный ремонт дронами | DONE | EmergencyRepairDrone |
| Разведка области (surveillance sweep) | DONE | ReconSurge |
| Пассивные протоколы (модификаторы) | DONE | Passive kind |
| Vacuum Imploder / Proton Collider / Psionic Decimator | DONE (каркас) | SuperweaponRechargeTicks на зданиях |
| Paradrop / Orbital Drop десант | ROADMAP | спавн группы в точке |
| Cash Bounty (% от убийств) | ROADMAP | хук в kill-credit |
| Iron Curtain как сила (массовый бафф) | PARTIAL | эффект есть (`InvulnerableTicks`), каст силы — нет |
| Хроносфера (телепорт группы) | ROADMAP | TeleportEntity есть, группового каста нет |
| Balloon Bombs / Final Squadron | ROADMAP | дрейфующие/камикадзе снаряды |
| Rage (ускорение строя своих) | ROADMAP | глобальный таймер-бафф |

## 4. Составы сторон (units roster)

Каркас data-driven (`EntityDef`): контент добавляется без кода.
Проверить покрытие библией: медведь/пёс-разведчики, Terror Drone, Bullfrog,
Twinblade, Sickle, Stingray, Mortar Cycle, Peacekeeper, Javelin, Guardian,
Athena, Mirage (маскировка — ROADMAP), Cryocopter (shrink — готов через статус),
Century Bomber, Hydrofoil, Assault Destroyer, Aircraft Carrier, Imperial Warrior
(banzai), Tank Buster, Shinobi, Tengu/Jet Tengu (трансформации — ROADMAP),
Tsunami, Striker VX, King Oni (bull rush), Wave-Force (заряд), Rocket Angel,
Yuriko/Natasha/Tanya (герои), Kirov, Akula, Shogun, Nagara, Seawing.

Трансформации Empire (Tengu ↔ Jet Tengu, Sea-Wing) — ROADMAP:
нужна смена Def на лету с сохранением здоровья/ветеранства.

## 5. Прочее

| Механика | Статус |
|---|---|
| Fog of war + радар | DONE |
| Belief/recon (искажённая разведка) | DONE |
| Мораль/подавление | DONE |
| Фракционные ресурсы | DONE |
| Прямое управление юнитом | DONE |
| Кооп-пинги | DONE |
| Rollback/lockstep сеть | DONE |
| Replay/save | DONE |
| Скримиш-AI с личностями | DONE |
| Стены (фортификации) | **DONE (контент)**: building.sov.wall / building.all.wall |
| Мины | ROADMAP |
| Нейтральные развед-посты (reveal) | PARTIAL: техздание-каркас есть, reveal-ауры нет |
