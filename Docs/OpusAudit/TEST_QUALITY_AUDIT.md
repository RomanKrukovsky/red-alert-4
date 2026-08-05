# Opus Audit — Test Quality Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Overview

- Total RA4_TEST macros: 297
- Test files: 21 (all compiled into RA4Tests)
- All pass: Yes (4 test suites: core, input, presentation, ai)
- Gemini claimed 245 tests — actual count is 297

## High Quality (Regression)

These tests would catch real bugs:

1. VerticalSlice.FullMatchFromBaseBuildingToVictory
2. VerticalSlice.IdenticalInputsProduceIdenticalStateEveryTick
3. VerticalSlice.DifferentSeedsDoNotProduceIdenticalState
4. Replay.PlaybackReproducesEveryCheckpointChecksum
5. Replay.RejectsFilesFromADifferentContentBuild
6. Replay.RejectsCorruptFiles
7. Lockstep.TwoPeersStayInSyncAcrossAFullMatch
8. Lockstep.DesyncIsCaughtOnTheTickItHappens
9. Lockstep.FrameAssemblyFollowsSlotOrderNotArrivalOrder
10. Commands.RejectsOrdersOnUnitsYouDoNotOwn
11. Commands.RejectsStaleEntityHandles
12. Commands.RejectsProductionWithoutPrerequisites
13. Commands.RejectsProductionYouCannotAfford
14. Commands.ThrottlesCommandFloods
15. Economy.HarvesterCompletesTheFullGatherLoop
16. Economy.ResourceFieldsAreFinite
17. Economy.MultiHarvesterTenCyclesAndRefineryQueue
18. Combat.ArmourClassesDecideTheOutcome
19. Combat.SplashDamageHitsEverythingInTheBlast
20. Movement.RoutesAroundTerrainWallThroughItsOnlyGap
21. Navigation.BridgeDestroyInvalidatesPath
22. FogOfWar.SimWorldIntegration
23. Camera.PanIsFrameRateIndependent
24. Content.ValidationCatchesAuthoringMistakes
25. Content.HashChangesWithBalanceEdits

## Medium Quality (Happy Path)

~150 tests confirm happy path but may miss edge cases:
- Movement reach/waypoint/stop tests
- Camera pan/zoom/edge scroll tests
- Selection click/marquee/shift/ctrl tests
- Navigation FlowField/MNavRouter/Reservation tests
- FogOfWar grid/reveal tests

## Low Quality (Trivial)

~30 tests confirm function existence:
- Lifecycle.SimWorldRestartRestoresCleanState
- Some presentation struct field tests
- UI data provider existence tests

## Critical Gaps

1. **No 500/1000/2000 entity stress tests** — claimed but not implemented
2. **No malformed command tests** — ServerSubmitFrame_Validate exists but untested
3. **No packet loss/jitter simulation** — tests bypass real network
4. **No save/load roundtrip with complex state** — 1 test only
5. **No campaign mission gameplay test** — framework tests only
6. **No performance benchmark tests** — no tick-time measurement
7. **No UI rendering tests** — UI tests verify data, not visuals

## Test Infrastructure

- Custom framework: RA4_TEST, RA4_EXPECT, RA4_REQUIRE
- TestHelpers.h provides BuildDefaultContent, MakeTestSetup, FindFirstOfType
- Content IDs in TestHelpers.h::Ids intentionally duplicated from DefaultContent.cpp
- SliceCommander is state-machine-driven (good design — not hardcoded tick numbers)
