#05. Research importer: secure blueprint
## 5.1. Purpose
Importer is needed not for transferring RA3 to Unreal, but for automatic analysis of the data form:
- what types of entities exist;
- what connections are found;
- how deep is the inheritance;
- which modules are combined;
- what categories of fields are repeated;
- where are the cycles, dangling references and high-coupling areas.
## 5.2. Hard limit
The default importer output does not contain the original names, strings, numbers, art paths or shader code. It produces only aggregates and an anonymized structure.
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

Do not use:
```text
EA XML → Unreal DataAsset → packaged game
```

## 5.4. Stages
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

Instead of the original:
```text
AlliedAntiVehicleVehicleTech1
```

save:
```text
FactionA.Vehicle.RoleAntiArmor.Tier2.Object_017
```

Replace numbers with statistics:
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

## 5.5. Independent intermediate model RA4
After research, your own schema is manually created:
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

It should not save EA identifiers and must have its own semantics/versioning.
## 5.6. Security

- XML ​​parser with external entities disabled;- maximum file/depth/node limits;
- no code generation from untrusted element names;
- path canonicalization;
- no writes outside report directory;
- immutable source mount;
- deterministic report generation;
- SHA-256 manifest;
- no telemetry/upload of third-party files.

## 5.7. Acceptance criteria

- not a single EA XML/XSD/shader is included in the Git index;
- generated report does not allow you to restore a significant part of the original data;
- repeated launch gives the same manifest;
- malicious XML corpus does not read local files and does not make network calls;
- CI checks for the absence of forbidden identifiers;
- RA4 production schema is created by a separate manual solution.