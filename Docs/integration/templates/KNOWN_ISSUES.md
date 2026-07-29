# KNOWN_ISSUES

Date: 2026-07-28. Only issues observed by running something are listed.

## K-1 — Five simulation tests fail (high)

```
Movement.UnitsReachTheirDestination
Movement.QueuedWaypointsAreFollowedInOrder
Economy.HarvesterCompletesTheFullGatherLoop
VerticalSlice.FullMatchFromBaseBuildingToVictory   (harvested == 0)
Navigation.LocalAvoidancePicksBestOpenNeighbor
```

Full suite: 98 passed, 5 failed. Reproduce in isolation via `--filter=`.

Single root symptom: a unit reports arrival (`bHasDestination == false`) while still
more than 120 world units from its goal, so harvesters never dock at the refinery and
the vertical-slice match harvests 0 credits. Consistent with the flow-field final
approach terminating at a tile boundary instead of the destination point.

Origin: uncommitted navigation-integration work in the working tree, not committed
code. Verified independent of `RA4Input` (`grep -rl RA4Input Source/RA4Simulation
Source/RA4Navigation Source/RA4Content Source/RA4Core` → 0 files).

## K-2 — A second session is editing this working tree live (high, process issue)

83 files are uncommitted. During the previous session this caused, in one sitting:
a broken `TestNavigation.cpp` mid-edit (unclosed brace, code outside a function), a
`-Werror` format-string break that blocked all compilation, and a held UnrealBuildTool
mutex that prevented verification.

Consequence for this task: **branch operations are unsafe.** Creating
`integration/mass-gas` and the other branches requires a checkout that would carry 83
uncommitted files across, or lose them. Not performed. Unblocking requires the other
session to commit or stash, or to move to a separate worktree.

## K-3 — No map exists (high)

`find Content -name "*.umap"` → 0. Nothing can be played, profiled in Unreal Insights,
cooked meaningfully or used as a dedicated-server test bed. This blocks the vertical
technical circuit independently of the templates.

## K-4 — Dead plugin dependencies (medium)

GameplayAbilities, GameplayTags, GameplayTasks and EnhancedInput are declared
dependencies of `RedAlert4` with **zero** referencing files. They inflate link surface
and misrepresent the project's capabilities.

## K-5 — Server target configured asymmetrically (medium)

`RedAlert4Server.Target.cs` lists only `RedAlert4` in `ExtraModuleNames`; Game and
Editor targets list all fifteen `RA4*` modules. The server target has never been built.

## K-6 — Competing GameModes and HUDs (medium)

See `FEATURE_OWNERSHIP_MATRIX.md`. The UI *showcase* GameMode is the global default,
not the playable one.

## K-7 — `RA4Navigation` declares unused Engine dependencies (low)

`CoreUObject` and `Engine` are private dependencies that the module does not use — it
compiles in the engine-free CMake harness. Because `RA4Simulation` depends on
`RA4Navigation`, this weakens the engine-free guarantee on paper.

## K-8 — Eight modules are empty stubs (medium)

`RA4Units`, `RA4Buildings`, `RA4Economy`, `RA4Combat`, `RA4Audio`, `RA4Modding`,
`RA4SaveSystem`, `RA4Diagnostics` are 30 lines of module boilerplate each. Their
functionality currently lives inside `RA4Simulation`. The module list therefore
overstates what exists.
