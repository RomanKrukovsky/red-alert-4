# 05. Research importer: безопасный blueprint

## 5.1. Назначение

Importer нужен не для переноса RA3 в Unreal, а для автоматического анализа формы данных:

- какие типы сущностей существуют;
- какие связи встречаются;
- насколько глубоко наследование;
- какие модули комбинируются;
- какие категории полей повторяются;
- где циклы, dangling references и high-coupling areas.

## 5.2. Жёсткое ограничение

Выход importer по умолчанию не содержит исходные names, strings, numbers, art paths или shader code. Он производит только агрегаты и анонимизированную структуру.

## 5.3. Pipeline

```text
ExternalResearch/EA_RA3
  → secure XML/XSD reader
  → reference graph
  → semantic classifier
  → anonymizer
  → aggregate reports
  → Research/RA3_SAGE_Study/GeneratedReports
```

Не использовать:

```text
EA XML → Unreal DataAsset → packaged game
```

## 5.4. Этапы

### A. Discovery

- enumerate XML/XSD;
- detect namespaces;
- build include/import graph;
- record file sizes and hashes;
- reject entities/DTDs and external network resolution.

### B. Schema model

- asset types;
- attributes and child elements;
- cardinality;
- inheritance/extension;
- reference-like fields;
- enum domains;
- module categories.

### C. Instance graph

- object → base object;
- object → command set;
- object → armor/locomotor/weapon;
- object → upgrade/tech dependency;
- object → behavior modules;
- object → AI profile/state machine;
- object → presentation/audio resources.

### D. Anonymization

Вместо оригинала:

```text
AlliedAntiVehicleVehicleTech1
```

сохранять:

```text
FactionA.Vehicle.RoleAntiArmor.Tier2.Object_017
```

Числа заменять статистиками:

```text
build_cost_percentile = 0.61
health_percentile = 0.54
speed_bucket = medium
```

### E. Reports

- asset type count;
- module frequency;
- inheritance depth histogram;
- dependency graph complexity;
- most reused definitions;
- object coupling score;
- missing/dangling references;
- AI layer graph;
- UI-command-ability link graph;
- visual state taxonomy.

## 5.5. Независимая промежуточная модель RA4

После исследования вручную создаётся своя schema:

```text
RA4Schema v1
  UnitDefinition
  BuildingDefinition
  WeaponDefinition
  AbilityDefinition
  MovementProfile
  ArmorProfile
  TechNode
  ProductionRecipe
  AIArchetype
  PresentationProfile
```

Она не должна сохранять EA identifiers и обязана иметь собственные semantics/versioning.

## 5.6. Security

- XML parser с отключёнными external entities;
- maximum file/depth/node limits;
- no code generation from untrusted element names;
- path canonicalization;
- no writes outside report directory;
- immutable source mount;
- deterministic report generation;
- SHA-256 manifest;
- no telemetry/upload of third-party files.

## 5.7. Acceptance criteria

- ни один EA XML/XSD/shader не попадает в Git index;
- generated report не позволяет восстановить существенную часть исходных данных;
- повторный запуск даёт одинаковый manifest;
- malicious XML corpus не читает локальные файлы и не делает network calls;
- CI проверяет отсутствие forbidden identifiers;
- RA4 production schema создаётся отдельным ручным решением.
