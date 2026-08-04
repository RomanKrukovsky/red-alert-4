# RA4 — Unreal Integration Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`
**Engine:** UE 5.8.1 (`++UE5+Release-5.8`, CL 56057345), macOS arm64

Supersedes the previous version, whose central finding (NoesisGUI blocks the UBT build) is
false at this commit, and which described a CI file (`ci.yml`) and a default map
(`M_Skirmish_Desert`) that do not exist.

## 1. The editor target builds — verified, not assumed

```
$ "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
    RedAlert4Editor Mac Development -Project=".../RedAlert4.uproject"
...
[23/23] Link [Apple] libUnrealEditor-RedAlert4.dylib
Result: Succeeded
Total execution time: 14.66 seconds
```

All 15 module dylibs are produced in `Binaries/Mac/`, including
`libUnrealEditor-RA4UI.dylib` (1.5 MB). The previous `BLOCKED_PLUGIN_MISSING` verdict for
`RA4UI` is stale: Noesis was abandoned, `RA4UI` now builds against UMG/CommonUI/MVVM, and
no `RA4NoesisHUDViewModel.*` file exists in `Source/RA4UI`.

## 2. Engine version — resolve the ambiguity

| Source | Version |
| --- | --- |
| `RedAlert4.uproject` `EngineAssociation` | **5.8** |
| Installed and used for this build | **5.8.1** |
| CLAUDE.md project brief | **5.6** |
| `/Users/Shared/Epic Games/UE_5.6` | present but **broken** — no `Engine/Build/Build.version`, no `Engine/Binaries/Mac`, only `Binaries/`, `Intermediate/`, `Plugins/`, `docs` |

The project is really on 5.8 and 5.8 works. CLAUDE.md's "5.6" is out of date and should be
corrected, or the decision to move to 5.8 recorded in an ADR. The half-installed 5.6 tree
should be removed to prevent accidental use.

## 3. Plugins — all resolve, three need provenance

Every plugin in `RedAlert4.uproject` resolves inside `UE_5.8/Engine/Plugins`:

| Plugin | Status |
| --- | --- |
| `GameplayAbilities`, `CommonUI`, `ModelViewViewModel`, `EnhancedInput`, `FunctionalTestingEditor`, `PythonScriptPlugin`, `EditorScriptingUtilities` | stock Epic — **OK** |
| `ModelContextProtocol`, `AllToolsets`, `ToolsetRegistry` | resolve **on this machine**, but are not stock UE 5.8 plugin names |

The last three are the risk. They were found under the engine installation, not under a
project-local `Plugins/` folder (which does not exist). If they are not part of a clean Epic
5.8 distribution, then this project silently requires a modified engine and will fail to
open on any other machine and in CI. This must be resolved before onboarding anyone or
enabling CI. See ASSET_AND_LICENSE_AUDIT §6.

## 4. Targets

Three targets, all present and correctly typed:

| File | `TargetType` |
| --- | --- |
| `RedAlert4Editor.Target.cs` | `Editor` |
| `RedAlert4.Target.cs` | `Game` |
| `RedAlert4Server.Target.cs` | `Server` |

Only the Editor target was built in this audit. Game and Server targets are **unverified**.

## 5. Module registration — one module is missing

`RedAlert4.uproject` declares 15 modules with sensible loading phases (`RA4Core` at
`EarliestPossible`; `RA4Content`/`RA4Simulation`/`RA4Replay` at `PreDefault`; the rest at
`Default`; `RA4Editor` as `Editor`).

`RA4Tests` is **not** among them and has no `RA4Tests.Build.cs`:

```
$ grep -c "RA4Tests" RedAlert4.uproject
0
$ ls Source/RA4Tests/*.Build.cs
(no such file)
```

7 436 lines of tests are therefore invisible to UBT and can only run through CMake. Nothing
in Unreal — actors, UMG, rendering, asset loading — is covered by any automated test. This
is the highest-leverage fix available in the project.

## 6. Configuration

`Config/` contains `DefaultEngine.ini`, `DefaultGame.ini`, `DefaultInput.ini`,
`DefaultUserInterface.ini`, plus `Config/Audio/` and `Config/Localization/`.

Verified settings:

```ini
; DefaultEngine.ini
EditorStartupMap=/Game/Maps/RA4_Skirmish_Production
GameDefaultMap=/Game/Maps/RA4_Skirmish_Production
GlobalDefaultGameMode=/Script/RedAlert4.RA4SkirmishGameMode

; DefaultGame.ini
ProjectName=Red Alert 4
ProjectVersion=0.1.0
Description=Internal working title. No Electronic Arts licence; no Command & Conquer content.
Culture=ru
BuildConfiguration=PPBC_Shipping
UsePakFile=True
+DirectoriesToAlwaysStageAsUFS=(Path="RA4UI/Fonts")
```

Two observations:

- `RA4_Skirmish_Production.umap` exists, so the default map is valid. The previously
  documented `M_Skirmish_Desert` does not exist anywhere and was fabricated.
- `ProjectVersion=0.1.0` is the only honest version number in the repository. It directly
  contradicts `Docs/Milestones/GOLD_MASTER_MANIFEST.md`, which declares
  `v1.0.0-gold-master`. Trust the ini.

## 7. Simulation ↔ Unreal bridge

`URA4SimWorldSubsystem` (`Source/RedAlert4/Private/RA4SimWorldSubsystem.cpp`, 1 240 lines) is
the integration seam and is well constructed:

- **Fixed timestep, frame-rate independent.** `Tick()` accumulates `TimeSinceLastSimTick`
  and runs `while (TimeSinceLastSimTick >= SimTickDelta) { … TickSimulation(); … }`
  (`:347-380`). This satisfies the invariant that render rate must not change simulation
  outcome.
- **Network-gated advance.** `if (!Network->CanAdvanceToTick(...)) break;` (`:369`) — the
  sim stalls rather than diverging when a lockstep frame has not landed.
- **Presentation is a projection, not a source of truth.** `SyncPresentation()` (`:765`)
  diffs sim entities against an `EntityActors` map, spawning `ARA4EntityActor` (`:829`) and
  removing dead entries (`:796`, `:972`). No write-back into sim state was observed.
- **Commands flow one way.** `Network->SendCommandToServer(Command, tick)` (`:396`).

This is the correct shape and matches ADR-0002 / ADR-0009.

## 8. Content and maps

8 project maps (`RA4_Skirmish{,_Production,_Hills,_Canyon,_VisualIntegration}`, `RA4_ArtLab`,
2 art showcases) plus 11 third-party demo maps under `Content/ThirdParty/`. The third-party
demo maps (`Overview`, `Showcase`, `Demonstration`, `TechArt`) are vendor sample levels and
should be excluded from packaging — they are pure bloat in a shipped build and part of the
14.2 GB problem described in ASSET_AND_LICENSE_AUDIT §2.

`.gitignore` ignores `*.uasset` and `*.umap`, yet many are tracked (they were force-added
before the rule, so the rule does not apply to them). The result is a confusing state where
some binary content is versioned and new content silently is not. This needs a deliberate
decision — likely Git LFS — before more art lands.

## 9. Automation scripts

`Tools/` holds ~30 Python scripts across `Art/`, `Audio/`, `Editor/`, `ContentImport/`,
`VoiceGeneration/`, `MatchViewer/`, `RuntimeFixes/`. They are genuine working tooling
(model generation/import/validation, EVA voice generation, map construction, screenshot
capture). Two concerns:

- `Tools/ContentImport/fetch_ra3_xmls.py` and `fetch_all_ea_xmls.py` download EA GPLv3
  material — see ASSET_AND_LICENSE_AUDIT §1.
- None of the Editor Python scripts are invoked by CI, so their continued correctness is
  unmonitored.

## 10. Packaged build

Never produced. No `.app`, `.exe`, `.pak` or staged directory exists anywhere in the
repository or in `Saved/`. `BuildCookRun` is not scripted. Packaging settings are present
and plausible (`UsePakFile=True`, `PPBC_Shipping`, fonts staged as UFS) but untested.

Any document asserting packaged-build readiness — including
`Docs/Audit/INDEPENDENT_RELEASE_REVIEW.md` item 7 and `GOLD_MASTER_MANIFEST.md` — is
asserting something that has never been attempted.

## 11. Summary

| Item | Status |
| --- | --- |
| Editor target builds | **VERIFIED** |
| Game / Server targets | **UNVERIFIED** |
| Plugins resolve | **VERIFIED** (3 need provenance) |
| Sim↔Unreal bridge correctness | **VERIFIED by inspection** |
| Fixed-timestep invariant | **VERIFIED by inspection** |
| Engine-side automated tests | **IMPOSSIBLE — `RA4Tests` not a UBT module** |
| Packaged build | **NEVER ATTEMPTED** |
| Editor runtime behaviour | **UNVERIFIED — never launched** |
