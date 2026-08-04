# Opus Audit — Gameplay Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Vertical Slice (End-to-End Match)

### Test: `VerticalSlice.FullMatchFromBaseBuildingToVictory`
**Status**: PASS

The scripted `SliceCommander` plays a deterministic 2-player match:
1. Builds power plant → refinery → war factory
2. Produces 4 heavy tanks
3. Sends assault on enemy construction yard
4. Match ends with player 0 victory

**Measured**: 2816 ticks (140.8s simulated), harvested 6000 credits, final checksum `4e6d9e69576c002b`

### Test: `VerticalSlice.IdenticalInputsProduceIdenticalStateEveryTick`
**Status**: PASS — tick-by-tick checksum comparison across two identical runs

### Test: `VerticalSlice.DifferentSeedsDoNotProduceIdenticalState`
**Status**: PASS — proves RNG is actually feeding the simulation

---

## Economy

| System | Test | Status |
|--------|------|--------|
| Harvester full gather loop | `Economy.HarvesterCompletesTheFullGatherLoop` | PASS |
| Finite resource fields | `Economy.ResourceFieldsAreFinite` | PASS |
| Multi-harvester + refinery queue | `Economy.MultiHarvesterTenCyclesAndRefineryQueue` | PASS |
| Power shortage slows production | `Power.ShortageSlowsProductionInsteadOfStoppingIt` | PASS |

**Verdict**: Economy is functional. No balance tuning data exists.

---

## Construction / Building

| System | Test | Status |
|--------|------|--------|
| Build radius and clear ground | `Placement.RequiresBuildRadiusAndClearGround` | PASS |
| Rejects water and cliffs | `Placement.RejectsWaterAndCliffs` | PASS |
| Structure payment and placement | `Production.StructureIsPaidQueuedThenPlaced` | PASS |
| Cancellation refund | `Production.CancellingRefundsCredits` | PASS |
| Rally point | `Production.UnitLeavesTheFactoryAndObeysTheRallyPoint` | PASS |
| Under-construction targetability | `Construction.UnderConstructionTargetabilityAndCancellation` | PASS |

**Verdict**: Building construction is functional.

---

## Combat

| System | Test | Status |
|--------|------|--------|
| Armor class determines outcome | `Combat.ArmourClassesDecideTheOutcome` | PASS |
| Hold fire outside weapon range | `Combat.UnitsHoldFireOutsideWeaponRange` | PASS |
| Attack order closes distance | `Combat.AttackOrderClosesTheDistance` | PASS |
| Defensive structures auto-engage | `Combat.DefensiveStructuresEngageOnTheirOwn` | PASS |
| Splash damage | `Combat.SplashDamageHitsEverythingInTheBlast` | PASS |

**Verdict**: Core combat is functional. No ammo system, no veterancy-in-combat integration test found.

---

## Movement / Pathfinding

| System | Test | Status |
|--------|------|--------|
| Units reach destination | `Movement.UnitsReachTheirDestination` | PASS |
| Queued waypoints | `Movement.QueuedWaypointsAreFollowedInOrder` | PASS |
| Stop clears order queue | `Movement.StopClearsTheWholeOrderQueue` | PASS |
| Impassable terrain blocks | `Movement.ImpassableTerrainBlocksGroundUnits` | PASS |
| Routes through terrain gap | `Movement.RoutesAroundTerrainWallThroughItsOnlyGap` | PASS |
| FlowField correct routing | `Navigation.FlowFieldRoutesThroughOnlyGapWithoutDiagonalCornerCutting` | PASS |
| FlowField layer clearance | `Navigation.FlowFieldRespectsLayerAndClearanceRequirements` | PASS |
| Formation leader following | `Navigation.FormationMembersFollowLeaderSlot` | PASS |
| Bridge destruction invalidates | `Navigation.BridgeDestroyInvalidatesPath` | PASS |

**Verdict**: Pathfinding is functional. No stress test with 500+ units.

---

## Input / Selection

| System | Test | Status |
|--------|------|--------|
| Click selects own units | `Selection.ClickPrefersOwnUnitsOverEverythingElse` | PASS |
| Shift+click additive | `Selection.ShiftAddsAndCtrlToggles` | PASS |
| Marquee selection | `Selection.MarqueeTakesOwnUnitsAndIgnoresEnemiesAndBuildings` | PASS |
| Double-click same type | `Selection.DoubleClickSelectsOnlyTheSameUnitType` | PASS |
| Control groups | `Selection.ControlGroupsAssignRecallAndForgetTheDead` | PASS |
| Selection cap | `Selection.SelectionIsCappedSoOrdersFitTheCommandBudget` | PASS |
| Right-click move | `Orders.RightClickOnGroundIsAMoveForEverySelectedUnit` | PASS |
| Right-click enemy attacks | `Orders.RightClickOnAnEnemyAttacksWithArmedUnitsAndMovesTheRest` | PASS |
| Force attack (Ctrl) | `Orders.ForceAttackTargetsAnAllyWhenCtrlIsHeld` | PASS |
| Attack-move | `Orders.AttackMoveModeProducesAttackMove` | PASS |
| Classic scheme | All `ClassicScheme.*` tests | PASS |

**Verdict**: Input handling is comprehensive and well-tested.

---

## Fog of War

| System | Test | Status |
|--------|------|--------|
| Grid initialization | `FogOfWar.GridInitialization` | PASS |
| Circular reveal | `FogOfWar.RevealCircularArea` | PASS |
| Clear current visibility | `FogOfWar.ClearCurrentVisibility` | PASS |
| SimWorld integration | `FogOfWar.SimWorldIntegration` | PASS |
| Unit movement updates fog | `FogOfWar.UnitMovementUpdatesFog` | PASS |

**Verdict**: Fog of War is functional.

---

## Camera

| System | Test | Status |
|--------|------|--------|
| Keyboard pan | `Camera.KeyboardPanMovesTheFocusAndSettles` | PASS |
| Diagonal pan speed | `Camera.DiagonalPanIsNotFasterThanCardinalPan` | PASS |
| Focus clamped to map | `Camera.FocusIsClampedToTheMap` | PASS |
| Zoom limits | `Camera.ZoomStaysWithinItsLimits` | PASS |
| Edge scroll focus-gated | `Camera.EdgeScrollOnlyRunsWhenTheWindowIsFocused` | PASS |
| Edge scroll suppressed while dragging | `Camera.EdgeScrollIsSuppressedWhileDragging` | PASS |
| Frame-rate independent pan | `Camera.PanIsFrameRateIndependent` | PASS |

**Verdict**: Camera is well-implemented and thoroughly tested.

---

## AI

| System | Test | Status |
|--------|------|--------|
| 46 AI tests | `TestAI.cpp` | All PASS |
| AI generates valid commands | AI produces CommandFrame through CommandBus | Verified in code |
| Strategy selection | AIStrategy enum with scoring | Verified in code |
| Doctrine per faction | AIDoctrineRegistry | Verified in code |
| HTN planning scaffolding | HTNPlan, HTNTask, HTNWorldState headers exist | Verified in code |

**Verdict**: AI framework is functional. Self-play league and advanced tactics are headers only (no implementation found in compiled sources).

---

## Save / Load

| System | Test | Status |
|--------|------|--------|
| 1 save system test | `TestSaveSystem.cpp` | PASS |

**Verdict**: Minimal save test exists. No migration testing, no stress testing with large states.

---

## Replay

| System | Test | Status |
|--------|------|--------|
| Record and verify | `Replay.RecordedMatchReplaysToTheSameResult` | PASS |
| Checkpoint verification | `Replay.PlaybackReproducesEveryCheckpointChecksum` | PASS |
| Content hash validation | `Replay.RejectsFilesFromADifferentContentBuild` | PASS |
| Corruption rejection | `Replay.RejectsCorruptFiles` | PASS |
| File roundtrip | `Replay.SurvivesAFileRoundTrip` | PASS |

**Verdict**: Replay system is well-tested and functional.

---

## Missing Gameplay Features (not implemented or not verified)

1. **Tutorial** — No tutorial system found
2. **Campaign missions** — Framework exists but no authored missions
3. **Superweapon abilities** — Not in test suite
4. **Tech tree progression** — Prerequisites tested but no full tech tree gameplay
5. **Naval units** — No water map or naval unit tests
6. **Air units** — No flight model or air unit tests
7. **Transport** — No transport/load-unload mechanics tested
8. **Veterancy in combat** — Veterancy tests exist but not integrated into combat flow
9. **Sell buildings** — Command type exists but no sell test
10. **Repair buildings** — Command type exists but no repair test
