# RA4 — AI Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`
**Suite:** `RA4AITests` — **46 passed, 0 failed** (4.88 s), re-run and confirmed

Supersedes the previous version, which quoted specific numbers ("483 decisions vs 123",
"100 concurrent AI matches without thread stalls or memory leaks") that the tests do not
report, and listed test names (`AI.NoCheatResources` as proving starting credits) with
invented detail.

## 1. Verdict

**This is the healthiest subsystem in the project.** The AI is a deterministic utility
commander that is structurally incapable of cheating, and its test suite is the only one in
the repository that meaningfully tests *behaviour* rather than plumbing. The claims made
about it are, unusually for this repo, close to true.

## 2. The no-cheating invariant is enforced by the type system

```cpp
// Source/RA4AI/Public/RA4AI/AICommander.h:62
void Tick(const SimWorld& World, std::vector<Command>& OutCommands);
```

The world arrives `const` and the only output is a `std::vector<Command>` — the same
`Command` type a human player produces, merged into the same `CommandFrame`. There is no
pointer, no friend declaration and no non-const accessor by which the AI could mutate
simulation state. The header's own comment states this and the signature backs it up.

`RA4AI.Build.cs` depends on `RA4Core`, `RA4Content`, `RA4Simulation`, `RA4FogOfWar` — no
Engine, no UObject. The AI is fully headless and self-playable, which is why
`AI.FiveSkirmishScenariosFinishWithAWinner` can run five complete matches in 1.03 s.

Fair-play is additionally covered by dedicated tests:

- `AI.NoCheatResources`
- `AI.FogOfWarStrictCompliance`
- `AIKnowledge.EnemiesOutsideVisionAreNotObserved`
- `AIKnowledge.MemoryIndexesTheFogGridInTilesNotWorldUnits`

The last one is a good sign of genuine engineering: it pins the *unit system* of the memory
index, a classic source of subtle desync.

## 3. Architecture as built

Utility-scored strategy selection with hysteresis, driving tactical operations and army
groups:

| Layer | Files | Evidence |
| --- | --- | --- |
| Strategy selection | `AIStrategy.cpp`, `AIDoctrine.cpp` | 6 `AI.UtilitySelects*` tests |
| Commander loop | `AICommander.cpp` | `AI.DecisionLogExplainsWhatItDid` |
| World model | `AIWorldView.cpp` | `AI.SimWorldViewTracksEnemyMemoryAndDecaysConfidence` |
| Tactical ops | `TacticalOperation.cpp` | `AI.TacticalOperationLifecycleStateTransitions` |
| Army groups | `ArmyGroup.cpp` | `AI.ArmyGroupManagerLifecycle`, `AI.SquadsGatherBeforeAdvancing` |
| Debug | `AIDebugOverlay.cpp` | `AI.AIDebugOverlaySnapshotCreation` |

Strategies confirmed by test name: Economy, Tech, Army, Assault, Fortify, Recovery. Both
hysteresis (`AI.StrategyHysteresisKeepsCurrentChoiceNearATie`) and its emergency bypass
(`AI.EmergencyStrategyOverridesHysteresis`) are tested — that pairing is easy to get wrong
and someone thought about it.

Faction doctrines exist and are differentiated:
`AI.FactionDoctrinesSovietAndAlliance`, `AI.DoctrineRaisesSovietHarvesterTarget`,
`AI.DoctrineLoadsLazilyOnFirstTick`.

## 4. Finding: the HTN planner is dead code contradicting its own ADR

`Docs/Architecture/ADR/ADR-0008-htn-utility-ai-commander.md` specifies HTN planning. The
implementation exists but is inert:

```
Source/RA4AI/Private/HTNPlan.cpp        22 lines
Source/RA4AI/Private/HTNTask.cpp        82
Source/RA4AI/Private/HTNWorldState.cpp  71
Source/RA4AI/Public/RA4AI/HTN*.h       313
                                  total 488 lines
```

Two independent confirmations that it is unused:

1. A repo-wide search for `HTNPlan|HTNTask|HTNWorldState` returns **only the HTN files
   themselves** — nothing in `AICommander.cpp` or `AIStrategy.cpp` references them.
2. `Tools/HeadlessBuild/CMakeLists.txt:116-124` lists the `RA4AI` sources explicitly and
   omits all three HTN `.cpp` files. They are not compiled into the tested library at all.

So the AI that runs is pure utility scoring; HTN is aspirational scaffolding. Either wire it
in or delete it and supersede ADR-0008 — leaving it makes the architecture documentation
actively misleading.

## 5. Difficulty profiles — real, but flatter than documented

`AI.DifficultyProfilesConfig`, `AI.ProfilesProduceDifferentConfigurations`,
`AI.ExposesAdaptiveProfile` and `AI.HardDifficultyIssuesMoreDecisionsThanEasy` all pass, so
profiles genuinely differ and Hard genuinely acts more often.

The previous audit's table (Easy 60 / Medium 30 / Hard 10 ticks; "483 vs 123 decisions")
presents specific numbers that no test output contains — `AI.HardDifficultyIssuesMoreDecisionsThanEasy`
asserts an ordering, not those magnitudes. Treat the table as unverified until read from
`AIDoctrine.cpp`.

Behavioural profiles (`AI.AggressiveProfileValuesAssaultMoreThanDefensive`,
`AI.EconomicProfileOutEarnsAggressive`) are separate from difficulty and are tested.

## 6. Determinism

`AI.IsDeterministic` and `AI.SquadAssignmentIsDeterministic` both pass. `AI.MassSimulationsBenchmark`
runs in 2.1 s — it is a throughput benchmark; the previous claim that it proves "no thread
stalls or memory leaks" is not something that test can establish (no sanitizer is active in
the default build; ASan/UBSan exist only in the CI job that never runs).

## 7. Gaps

| Gap | Impact |
| --- | --- |
| HTN dead code vs ADR-0008 | Documentation misleads future work |
| No AI test for harvester replacement | The economy death-spiral case is unguarded (see GAMEPLAY_AUDIT §2.1) |
| No naval or air behaviour tests | CLAUDE.md requires ground/air/naval; only ground is evidenced |
| No superweapon/ability usage tests | 5 files mention superweapon; AI never exercises it |
| No multi-AI free-for-all beyond 1v1 scenarios | 2v2/3v3/4v4 claimed in gold-master doc, untested |
| Difficulty tuning values unverified | Table in prior audit is unsourced |

## 8. Recommendation

Do not restructure this module. Resolve the HTN/ADR contradiction, and add behavioural tests
for air/naval and superweapon usage when those systems exist. The AI is ahead of the content
it has to command.
