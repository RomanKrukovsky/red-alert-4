# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 12.0
**Actual Project Status**: **PRE-ALPHA.** Deterministic C++ core is healthy and
well tested. There is no runnable game build. The prior "commercial launch
live" claim was false.

---

## Verification performed 2026-08-05

Measured by running commands, not by reading documents. Reproduce with:

```sh
cmake --build Tools/HeadlessBuild/build -j8
ctest --test-dir Tools/HeadlessBuild/build --output-on-failure
```

| Claim in v11.0 of this document | Verification result |
| :--- | :--- |
| "393/393 C++ unit tests pass" | **TRUE, and understated.** 479 pass, 0 fail: RA4Tests 331, RA4InputTests 66, RA4PresentationTests 23, RA4AITests 59. `ctest` reports 4/4 suites green in 14.15 s. Headless target builds clean. |
| "ALL 11 MILESTONE GATES PASSED" | **FALSE.** See below. |
| "COMMERCIAL LAUNCH LIVE (`v1.0.0-launch-ready`)" | **FALSE.** `git show --stat v1.0.0-launch-ready` contains only `Docs/Agent/NEXT_ACTIONS.md` and seven `Docs/Operations/*.md` policy files. No build, binary, or shippable artifact is in that tag. |
| Implied shippable build exists | **FALSE.** `Binaries/Mac/RedAlert4.app` is empty: a single `Contents` directory, 0 B total. Nothing to launch. |
| "LiveOps Post-Launch Queue ACTIVE" (M11-T1/2/3) | **NOT APPLICABLE.** There are no live players, no telemetry, and no incidents, because there is no shipped build. Removed. |

### Why the milestone claims cannot be trusted

Milestones 2 and 4-11 were marked passed on the strength of documents being
authored, not software being verified. Milestone 11 in particular was closed by
creating seven policy `.md` files. Writing an incident-response policy is not
the same as having a game that can incur an incident.

CLAUDE.md is explicit that readiness requires successful compilation,
automated tests, actual execution, behaviour verification, and independent
review. Only the first two are currently satisfied, and only for the headless
core.

---

## What is actually verified working

- **Deterministic simulation core.** 479 headless tests green, including
  determinism, replay, fog of war, pathfinding, save/load, campaign runtime,
  network lockstep, and the AI hierarchy.
- **Skirmish map `RA4_Skirmish_Production`.** Rebuilt as a tropical
  archipelago by `Tools/Editor/make_archipelago_map.py`. Geometry verified
  programmatically (40 actors checked against sampled terrain and the
  waterline, 0 problems) and visually via editor screen capture.

## What is unverified or known broken

- **No packaged game build.** Nothing has been produced that a player could run.
- **Editor viewport renders terrain black over MCP.** Root-caused to
  `ViewportService.set_realtime(True)` not persisting; see note 7 in
  `Tools/Editor/make_archipelago_map.py`. Lighting must be judged
  interactively.
- **Map art is blockout only.** No palm trees, no cliff meshes, no sand or
  beach textures. Ore fields are placeholder tinted cubes. Closing these gaps
  requires licensed assets, per CLAUDE.md's prohibition on unlicensed content.
- **CityPark meshes have broken material references.** Meshes under
  `/Game/ThirdParty/CityPark/...` point at `/Game/CityPark/...`, producing
  `LoadErrors` on level load. Cosmetic, but noisy and should be repathed.
- **Milestones 2 and 4-11 are unaudited.** Each needs re-verification against
  running software before it may be called passed again.

---

## Next actions

| Task ID | Task | Acceptance criteria |
| :--- | :--- | :--- |
| **V-1** | Produce a packaged build that launches | A binary that starts, loads `RA4_Skirmish_Production`, and reaches a playable state without a crash. Record the exact command and its output. |
| **V-2** | Re-audit milestones 2 and 4-11 | For each, either supply evidence from a real run or downgrade it to unproven. Do not accept a document as evidence. |
| **V-3** | Verify the map interactively | Open the archipelago in the editor, click the viewport to force a lit realtime pass, and confirm lighting, water, and foliage read correctly. |
| **V-4** | Repath CityPark material references | Level load produces no `LoadErrors`. |
| **V-5** | Replace blockout map art | Requires licensed tropical foliage, cliff meshes, and beach textures. Blocked on asset acquisition; needs a decision from the project owner. |

---

## Execution Guidelines for Agents

- **Project Status**: Pre-alpha. Core is solid; there is no shippable build.
- **Do not mark a milestone passed because a document describing it exists.**
  CLAUDE.md rule 5 forbids claiming a feature is done because code was
  written; the same applies to prose.
- **State the command you ran and its real output.** Claiming a command ran
  when it did not, or claiming visual verification without launching, is
  explicitly prohibited.
- **Commit Format**: `type(scope): short description`.
