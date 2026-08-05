# Opus Audit — Gameplay Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Vertical Slice

| Test | Result | Evidence |
|------|--------|----------|
| FullMatchFromBaseBuildingToVictory | PASS | 2816 ticks, 140.8s sim, 6000 credits, checksum 4e6d9e69576c002b |
| IdenticalInputsProduceIdenticalStateEveryTick | PASS | Tick-by-tick checksum comparison across 2 runs |
| DifferentSeedsDoNotProduceIdenticalState | PASS | Proves RNG feeds simulation |

## Economy

| System | Test | Status |
|--------|------|--------|
| Harvester gather loop | Economy.HarvesterCompletesTheFullGatherLoop | PASS |
| Finite resource fields | Economy.ResourceFieldsAreFinite | PASS |
| Multi-harvester + refinery queue | Economy.MultiHarvesterTenCyclesAndRefineryQueue | PASS |
| Power shortage slows production | Power.ShortageSlowsProductionInsteadOfStoppingIt | PASS |

## Combat

| System | Test | Status |
|--------|------|--------|
| Armor class outcome | Combat.ArmourClassesDecideTheOutcome | PASS |
| Hold fire outside range | Combat.UnitsHoldFireOutsideWeaponRange | PASS |
| Attack closes distance | Combat.AttackOrderClosesTheDistance | PASS |
| Defensive auto-engage | Combat.DefensiveStructuresEngageOnTheirOwn | PASS |
| Splash damage | Combat.SplashDamageHitsEverythingInTheBlast | PASS |

## Pathfinding

| System | Test | Status |
|--------|------|--------|
| Reach destination | Movement.UnitsReachTheirDestination | PASS |
| Queued waypoints | Movement.QueuedWaypointsAreFollowedInOrder | PASS |
| Impassable terrain | Movement.ImpassableTerrainBlocksGroundUnits | PASS |
| Route through gap | Movement.RoutesAroundTerrainWallThroughItsOnlyGap | PASS |
| FlowField correct routing | Navigation.FlowFieldRoutesThroughOnlyGapWithoutDiagonalCornerCutting | PASS |
| Formation following | Navigation.FormationMembersFollowLeaderSlot | PASS |
| Bridge destruction | Navigation.BridgeDestroyInvalidatesPath | PASS |

## Input / Selection (66 tests — all PASS)

Click, marquee, shift-add, ctrl-toggle, double-click type select, control groups, right-click move/attack, force attack, attack-move, harvester harvest order, placement mode, rally point, cursor hints.

## Fog of War (5 tests — all PASS)

Grid init, circular reveal, clear visibility, SimWorld integration, unit movement updates.

## Camera (7 tests — all PASS)

Keyboard pan, diagonal speed, map clamping, zoom limits, edge scroll focus/drag gating, frame-rate independence.

## AI (46 tests — all PASS)

Strategy selection, doctrine loading, economy decisions, combat targeting, scouting, harassment, build order, army group management.

## Missing Gameplay

1. Tutorial — not implemented
2. Campaign missions — framework only, no authored content
3. Superweapon abilities — not in tests
4. Naval units — no water map tests
5. Air units — no flight model tests
6. Transport/load-unload — not tested
7. Sell/repair buildings — commands exist, no tests
