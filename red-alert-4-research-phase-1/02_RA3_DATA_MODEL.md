# 02. Что реально даёт модель данных RA3

## 2.1. Верхнеуровневое устройство

RA3 XML разбит не только по фракциям. В официальном дереве присутствуют:

- faction units и structures;
- `BaseObjects`;
- `GlobalData`;
- `Includes` и `Templates`;
- `SkirmishAI`;
- `Multiplayer`;
- `Maps`, `MapSpecific`, `Terrain`;
- `UI`, `UIInGame`, `Shell`;
- `Sounds`, `Cinematics`, `IntelDB`, `WorldBuilder`.

Следствие: data layer современной C&C — это не каталог статов, а декларативный граф ресурсов и поведений.

## 2.2. Анатомия одного боевого юнита

На примере официального XML наземной противотанковой машины видны отдельные блоки:

1. **Identity и наследование**
   - уникальный asset id;
   - `inheritFrom` базового транспортного объекта;
   - сторона, роль редактора, портрет, кнопка, локализованные строки.

2. **Производство и экономика**
   - build time;
   - стоимость по account/resource;
   - обязательный upgrade/tech dependency.

3. **Классификация**
   - набор признаков наподобие selectable, vehicle, can attack, tier;
   - weapon category;
   - subgroup priority.

4. **Выживаемость и движение**
   - armor definition;
   - damage FX profile;
   - locomotor template, condition и speed;
   - body/max health;
   - collision geometry.

5. **Представление**
   - model condition states;
   - bones для muzzle, recoil, launch и turret;
   - damage textures и particles;
   - animation states;
   - faction coloring и tracks.

6. **Runtime-модули**
   - weapon-set update;
   - turret settings и target chooser data;
   - physics;
   - death modules;
   - toggle state и special powers;
   - status upgrades.

7. **AI и client-only behavior**
   - unit AI state machine;
   - auto-acquire settings;
   - target chooser policy;
   - отдельные клиентские звуковые реакции.

## 2.3. Что из этого должно появиться в Unreal

Не следует делать монолитный `URA4UnitDataAsset` на сотни полей. Рекомендуемая композиция:

```text
URA4UnitDefinition
  IdentityRef
  ClassificationRef
  EconomyRef
  TechRequirementsRef
  MovementProfileRef
  DurabilityProfileRef
  WeaponLoadoutRef
  AbilitySetRef
  AIProfileRef
  PresentationProfileRef
  AudioProfileRef
  CollisionProfileRef
```

Предлагаемые типы:

```text
URA4UnitDefinition                основной immutable descriptor
URA4EconomyDefinition             стоимость, время, refund, queue rules
URA4TechRequirementSet            AND/OR-граф требований
URA4MovementProfile               domain, speed, acceleration, turn, avoidance
URA4ArmorProfile                  категории входящего урона и modifiers
URA4WeaponDefinition              fire model, target filter, projectile/effect
URA4WeaponLoadout                 slots, modes, turrets, switching conditions
URA4AbilityDefinition             activation, target mode, costs, cooldown
URA4AIProfile                     role, targeting, micro policy, strategic hints
URA4PresentationProfile           mesh/anim/FX state mapping
URA4AudioProfile                  semantic events без привязки к EA-аудио
```

## 2.4. Gameplay Tags

RA3 `KindOf`, object/model/status conditions и категории нельзя копировать как готовый словарь, но сама идея полезна.

Пример независимой таксономии:

```text
Unit.Domain.Ground
Unit.Class.Vehicle
Unit.Role.AntiArmor
Unit.Tier.2
Unit.Capability.Attack
Unit.Capability.Capture
Unit.State.Moving
Unit.State.Damaged
Unit.State.Cloaked
Weapon.Target.Ground
Weapon.Damage.Kinetic
Production.Factory.Vehicle
```

Теги должны быть семантическими. Не следует превращать их в свалку произвольных булевых флагов.

## 2.5. Технологический граф

RA3 связывает объект с upgrade dependencies. В RA4 нужен нормальный граф, поддерживающий:

- `AllOf`;
- `AnyOf`;
- `NoneOf`;
- required structure;
- required research;
- faction doctrine;
- player rank;
- mutually exclusive branch;
- temporary unlock;
- campaign override.

Граф должен валидироваться на циклы и недостижимые узлы ещё при cook/CI.

## 2.6. AI-структура

В RA3 skirmish AI отдельно хранятся:

- opening moves;
- personalities;
- states;
- micro manager library;
- target heuristic library;
- army definitions по фракциям;
- общая AI data/configuration.

Для RA4 это даёт пятислойную модель:

```text
StrategicDirector
  → EconomyPlanner
  → ProductionPlanner
  → ArmyComposer
  → TacticalGroupController
  → UnitMicroController
```

Target scoring должен быть отдельным сервисом, а не зашитым условием в каждом юните.

## 2.7. Визуальные состояния

Набор RA3 shaders показывает feature taxonomy: faction materials, damaged/frozen states, particles, distortion, lightning, lasers, water/ocean, rain, outlines, bloom и lookup-table post-processing.

Для RA4 полезно составить собственную матрицу визуальных функций, но создавать материалы заново в Unreal:

```text
Surface faction tint
Damage stages
Status overlay
Selection/outline
Construction progress
Cloak/reveal
Shield/ invulnerability
Freeze/burn/electric
Water interaction
Projectile trail/beam
Area preview
Post-process battlefield feedback
```
