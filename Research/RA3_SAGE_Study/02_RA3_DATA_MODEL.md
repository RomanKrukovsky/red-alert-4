# 02. What the RA3 data model really provides
## 2.1. Top-level device
RA3 XML is not only divided into factions. The official tree contains:
- faction units and structures;- `BaseObjects`;
- `GlobalData`;
- `Includes` and `Templates`;- `SkirmishAI`;
- `Multiplayer`;
- `Maps`, `MapSpecific`, `Terrain`;
- `UI`, `UIInGame`, `Shell`;
- `Sounds`, `Cinematics`, `IntelDB`, `WorldBuilder`.

Corollary: the data layer of modern C&C is not a catalog of stats, but a declarative graph of resources and behaviors.
## 2.2. Anatomy of one combat unit
Using the example of the official XML of a ground anti-tank vehicle, individual blocks are visible:
1. **Identity and inheritance**
   - unique asset id;
   - `inheritFrom` of the underlying transport object;
   - side, editor role, portrait, button, localized strings.
2. **Production and Economy**   - build time;
- cost by account/resource;
   - mandatory upgrade/tech dependency.
3. **Classification**
   - a set of features like selectable, vehicle, can attack, tier;   - weapon category;
   - subgroup priority.

4. **Survivability and Move**   - armor definition;
   - damage FX profile;
- locomotor template, condition and speed;   - body/max health;
   - collision geometry.

5. **Performance**   - model condition states;
- bones for muzzle, recoil, launch and turret;
   - damage textures and particles;   - animation states;
- faction coloring and tracks.
6. **Runtime modules**   - weapon-set update;
- turret settings and target chooser data;   - physics;
   - death modules;
- toggle state and special powers;   - status upgrades.

7. **AI and client-only behavior**   - unit AI state machine;
   - auto-acquire settings;
   - target chooser policy;
- individual client sound reactions.
## 2.3. Which of these should appear in Unreal?
You should not make a monolithic `URA4UnitDataAsset` for hundreds of fields. Recommended composition:
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

Suggested types:
```text
URA4UnitDefinition main immutable descriptor
URA4EconomyDefinition cost, time, refund, queue rules
URA4TechRequirementSet AND/OR requirements graphURA4MovementProfile               domain, speed, acceleration, turn, avoidance
URA4ArmorProfile incoming damage categories and modifiersURA4WeaponDefinition              fire model, target filter, projectile/effect
URA4WeaponLoadout                 slots, modes, turrets, switching conditions
URA4AbilityDefinition             activation, target mode, costs, cooldown
URA4AIProfile                     role, targeting, micro policy, strategic hints
URA4PresentationProfile           mesh/anim/FX state mapping
URA4AudioProfile semantic events without binding to EA audio```

## 2.4. Gameplay Tags

RA3 `KindOf`, object/model/status conditions and categories cannot be copied as a ready-made dictionary, but the idea itself is useful.
Example of an independent taxonomy:
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

Tags must be semantic. They should not be turned into a dump of arbitrary boolean flags.
## 2.5. Technology graph
RA3 associates the object with upgrade dependencies. RA4 needs a normal graph that supports:
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

The graph must be validated for cycles and unreachable nodes even with cook/CI.
## 2.6. AI structure
RA3 skirmish AI stores separately:
- opening moves;
- personalities;
- states;
- micro manager library;
- target heuristic library;
- army definitions by faction;
- general AI data/configuration.
for RA4 this gives a five-layer model:
```text
StrategicDirector
  → EconomyPlanner
  → ProductionPlanner
  → ArmyComposer
  → TacticalGroupController
  → UnitMicroController
```

Target scoring should be a separate service, not a hard-wired condition in each unit.
## 2.7. Visual states
The RA3 shaders set shows feature taxonomy: faction materials, damaged/frozen states, particles, distortion, lightning, lasers, water/ocean, rain, outlines, bloom and lookup-table post-processing.
for RA4 it is useful to create your own matrix of visual features, but create the materials again in Unreal:
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