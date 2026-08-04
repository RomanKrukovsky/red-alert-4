# Opus Audit — Test Quality Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Test Suite Overview

| Metric | Value |
|--------|-------|
| Total RA4_TEST macros | 297 |
| Test files | 21 |
| All compiled into RA4Tests | Yes (confirmed in CMakeLists.txt) |
| All pass on clean build | Yes (ctest: 4/4 suites pass) |
| Build from clean state | Yes (cmake configure + build from /tmp) |

---

## Test Count by File

| File | Tests | Category |
|------|-------|----------|
| TestInput.cpp | 66 | Input, Selection, Orders, HitTest, ControlScheme |
| TestAI.cpp | 46 | AI strategy, doctrine, economy, combat, scouting |
| TestSimulation.cpp | 35 | Content, Commands, Economy, Combat, Movement, Victory |
| TestPresentation.cpp | 23 | HUD snapshot, Art mapping |
| TestMissionRuntime.cpp | 21 | Campaign mission objectives |
| TestNavigation.cpp | 17 | NavGrid, FlowField, MNavRouter, Reservation, Formation |
| TestBibleImport.cpp | 15 | JSON bible parsing, faction loading |
| TestNetwork.cpp | 13 | Lockstep protocol, frame assembly, desync |
| TestCore.cpp | 14 | Fixed math, ByteStream, Command serialization, Checksum, IDs |
| TestVerticalSlice.cpp | 8 | Full match, determinism, replay |
| Test_RA3PipelineAndCommandBus.cpp | 6 | Pipeline integration |
| TestFactionResources.cpp | 6 | Faction-specific economy |
| TestFogOfWar.cpp | 5 | Fog grid, reveal, SimWorld integration |
| TestBibleContent.cpp | 4 | Content database validation |
| TestProvingGround.cpp | 4 | Stress/proving ground |
| TestVeterancy.cpp | 4 | Veterancy system |
| TestUI.cpp | 4 | UI data provider |
| TestCampaign.cpp | 2 | Campaign framework |
| Test_HUDIntegrationAndGameState.cpp | 2 | HUD integration |
| TestSaveSystem.cpp | 1 | Save/load |

---

## Test Quality Classification

### HIGH QUALITY (REGRESSION — would catch real bugs)

These tests exercise real behavior and would fail if the system broke:

1. `VerticalSlice.FullMatchFromBaseBuildingToVictory` — full match with assertions on winner, ticks, credits, tanks
2. `VerticalSlice.IdenticalInputsProduceIdenticalStateEveryTick` — tick-by-tick checksum comparison
3. `VerticalSlice.DifferentSeedsDoNotProduceIdenticalState` — proves RNG feeds simulation
4. `Replay.PlaybackReproducesEveryCheckpointChecksum` — rebuilds world from header, replays frames, verifies checksums
5. `Replay.RejectsFilesFromADifferentContentBuild` — content hash validation
6. `Replay.RejectsCorruptFiles` — garbage/truncation detection
7. `Lockstep.TwoPeersStayInSyncAcrossAFullMatch` — two independent SimWorlds stay bit-identical
8. `Lockstep.DesyncIsCaughtOnTheTickItHappens` — corruption detected on exact tick
9. `Lockstep.FrameAssemblyFollowsSlotOrderNotArrivalOrder` — proves deterministic assembly
10. `Commands.RejectsOrdersOnUnitsYouDoNotOwn` — ownership validation
11. `Commands.RejectsStaleEntityHandles` — generation check
12. `Commands.RejectsProductionWithoutPrerequisites` — tech tree enforcement
13. `Commands.RejectsProductionYouCannotAfford` — credit check
14. `Commands.ThrottlesCommandFloods` — rate limiting
15. `Economy.HarvesterCompletesTheFullGatherLoop` — full economic cycle
16. `Economy.ResourceFieldsAreFinite` — resource depletion
17. `Economy.MultiHarvesterTenCyclesAndRefineryQueue` — multi-harvester queuing
18. `Combat.ArmourClassesDecideTheOutcome` — damage matrix
19. `Combat.SplashDamageHitsEverythingInTheBlast` — area damage
20. `Movement.RoutesAroundTerrainWallThroughItsOnlyGap` — pathfinding through bottleneck
21. `Navigation.BridgeDestroyInvalidatesPath` — topology revision
22. `FogOfWar.SimWorldIntegration` — fog updates with simulation
23. `Camera.PanIsFrameRateIndependent` — frame-rate independence
24. `Content.ValidationCatchesAuthoringMistakes` — validates error detection
25. `Content.HashChangesWithBalanceEdits` — hash sensitivity

### MEDIUM QUALITY (HAPPY PATH — confirms working, may not catch subtle regressions)

~150 tests that confirm the happy path works but may not catch edge-case regressions:
- All movement tests (units reach destination, queued waypoints)
- All camera tests (pan, zoom, edge scroll)
- All selection tests (click, marquee, control groups)
- Most navigation tests
- Most fog of war tests

### LOW QUALITY (TRIVIAL or MOCKED)

~30 tests that confirm function existence or test against mocks:
- `Lifecycle.SimWorldRestartRestoresCleanState` — just restarts and checks phase
- Some presentation tests that only verify struct field values
- `UI` tests that verify data provider existence

---

## Critical Test Gaps

1. **No 500/1000/2000 entity stress tests** — claimed in Gemini docs but not in code
2. **No malformed command tests** — `ServerSubmitFrame_Validate` checks payload size but no test exercises it
3. **No packet loss / jitter simulation** — LockstepSession tests use direct method calls, not simulated network
4. **No save/load roundtrip with complex state** — only 1 test in TestSaveSystem.cpp
5. **No campaign mission gameplay test** — TestMissionRuntime tests the framework, not actual mission flow
6. **No performance benchmark tests** — no tick-time measurement in test suite
7. **No concurrent access tests** — single-threaded only (correct for deterministic sim, but no thread-safety verification for the network layer)
8. **No UI rendering tests** — UI tests verify data, not visual output

---

## Test Infrastructure Notes

- Custom test framework (`RA4_TEST`, `RA4_EXPECT`, `RA4_REQUIRE`) — lightweight, no external dependency
- `TestHelpers.h` provides shared fixtures (`BuildDefaultContent`, `MakeTestSetup`, `FindFirstOfType`)
- Test content IDs in `TestHelpers.h::Ids` namespace are intentionally duplicated from `DefaultContent.cpp` — a name change must break tests
- The `SliceCommander` in TestVerticalSlice.cpp is a state-machine-driven test commander, not hardcoded tick numbers — this is good design
