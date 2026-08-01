#06. Next exploration passes
## P0 - architectural core
### R-001. Simulation loop and state ownership
Explore:
- update order in Generals/Zero Hour;- object lifecycle;
- random streams;
- state hashing/CRC;
- logic/client update separation.
Exit:
- `SIMULATION_INVARIANTS.md`;
- sequence diagram of the frame;
- list of nondeterminism hazards for Unreal.
### R-002. Command pipeline

Explore:
- input translation;
- game messages;
- command validation;
- deterministic ordering;
- AI commands vs player commands.

Exit:
- `COMMAND_PROTOCOL_SPEC.md`;
- versioned command envelope;
- validation matrix.

### R-003. Network and replay
Explore:
- FrameData/NetCommand flow;
- input delay/barriers;
- content/version checks;
- mismatch detection;
- replay format and seeking.

Exit:
- comparative ADR for lockstep/server/hybrid;- desync diagnostics design;
- replay test plan.

### R-004. Entity/component model

Explore:
- GameObject inheritance;
- module lifecycle;
- body/AI/client behavior separation;
- ownership and event flow.
Exit:
- RA4 component boundaries;
- prohibited dependencies;
- lifecycle state machine.

## P1 — gameplay systems

### R-005. Weapons, damage and armor
- targeting filters;
- weapon slots/modes;
- turret control;
- projectiles;
- damage categories;
- modifiers/status effects;
- death modules.

### R-006. Movement, pathfinding and formations
- locomotor profiles;
- AIPathfind;
- partition/spatial queries;
- terrain domains;
- group movement;
- dynamic blockers;
- amphibious/naval edge cases.

### R-007. Economy, production and tech graph
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

### R-009. Selection, commands and control bar
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

The goal is our own Unreal implementation matrix, not an HLSL port.
## P3 - validation and production readiness
### R-012. Content compiler

- independent RA4 schema;
- validators;
- dependency graph;
- hot reload/editor tooling;
- cooked manifest;
- migration system.

### R-013. Performance model

- CPU budgets for simulation/navigation/AI;- memory per entity;
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

## Definition of Done for each study
Each item is considered closed only with availability:
1. list of primary sources;
2. confirmed observations separate from assumptions;
3. independent RA4 specification;4. diagram/data-flow;
5. edge cases;
6. tests/acceptance criteria;
7. legal provenance;
8. ADR with an accepted or deferred decision.