# 06. Следующие исследовательские проходы

## P0 — архитектурное ядро

### R-001. Simulation loop и state ownership

Изучить:

- порядок update в Generals/Zero Hour;
- object lifecycle;
- random streams;
- state hashing/CRC;
- разделение logic/client update.

Выход:

- `SIMULATION_INVARIANTS.md`;
- sequence diagram кадра;
- список nondeterminism hazards для Unreal.

### R-002. Command pipeline

Изучить:

- input translation;
- game messages;
- command validation;
- deterministic ordering;
- AI commands vs player commands.

Выход:

- `COMMAND_PROTOCOL_SPEC.md`;
- versioned command envelope;
- validation matrix.

### R-003. Network и replay

Изучить:

- FrameData/NetCommand flow;
- input delay/barriers;
- content/version checks;
- mismatch detection;
- replay format and seeking.

Выход:

- сравнительный ADR по lockstep/server/hybrid;
- desync diagnostics design;
- replay test plan.

### R-004. Entity/component model

Изучить:

- GameObject inheritance;
- module lifecycle;
- body/AI/client behavior separation;
- ownership и event flow.

Выход:

- RA4 component boundaries;
- prohibited dependencies;
- lifecycle state machine.

## P1 — gameplay systems

### R-005. Weapons, damage и armor

- targeting filters;
- weapon slots/modes;
- turret control;
- projectiles;
- damage categories;
- modifiers/status effects;
- death modules.

### R-006. Movement, pathfinding и formations

- locomotor profiles;
- AIPathfind;
- partition/spatial queries;
- terrain domains;
- group movement;
- dynamic blockers;
- amphibious/naval edge cases.

### R-007. Economy, production и tech graph

- accounts/resources;
- build assistant;
- queues and factories;
- dependencies/upgrades;
- cancel/refund;
- power/capacity;
- production placement.

### R-008. AI

- opening moves;
- personalities;
- strategic states;
- army definitions;
- target heuristics;
- micro managers;
- difficulty modifiers without cheating.

## P2 — player-facing systems

### R-009. Selection, commands и control bar

- selection translator;
- command availability;
- hotkeys;
- contextual cursor;
- multi-selection aggregation;
- production UI binding.

### R-010. Campaign scripting

- script conditions/actions;
- shared mission libraries;
- restrictions;
- victory relevance;
- save/load compatibility;
- deterministic cutscene/gameplay boundaries.

### R-011. Visual feature taxonomy

- damage states;
- status overlays;
- water;
- distortion;
- particles;
- outlines;
- post FX;
- faction readability.

Цель — собственная Unreal implementation matrix, а не портирование HLSL.

## P3 — validation и production readiness

### R-012. Content compiler

- independent RA4 schema;
- validators;
- dependency graph;
- hot reload/editor tooling;
- cooked manifest;
- migration system.

### R-013. Performance model

- CPU budgets на simulation/navigation/AI;
- memory per entity;
- network bytes per command;
- replay size;
- server cost;
- 1k/5k/10k entities stress profiles.

### R-014. Compliance automation

- provenance database;
- forbidden-content scanner;
- similarity review workflow;
- release gate;
- third-party notices.

## Definition of Done для каждого исследования

Каждый пункт считается закрытым только при наличии:

1. списка первичных источников;
2. подтверждённых наблюдений отдельно от предположений;
3. самостоятельной RA4-спецификации;
4. diagram/data-flow;
5. edge cases;
6. tests/acceptance criteria;
7. юридического provenance;
8. ADR с принятым либо отложенным решением.
