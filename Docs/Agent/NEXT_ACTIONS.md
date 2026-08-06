# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 12.1
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
| "393/393 C++ unit tests pass" | **TRUE, and understated.** Re-measured 2026-08-05 after the AI hierarchy landed: **583 pass, 0 fail** — RA4Tests 376, RA4InputTests 66, RA4PresentationTests 23, RA4AITests 118. `ctest` reports 4/4 suites green in 15.11 s. Headless target builds clean with no errors. (Earlier figure in this table was 479, measured before the AI work.) |
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

- **Deterministic simulation core.** 583 headless tests green, including
  determinism, replay, fog of war, pathfinding, save/load, campaign runtime,
  network lockstep, and the AI hierarchy.
- **AI hierarchy (landed 2026-08-05).** Opponent modelling, four strategic
  directors (economy/scouting/defence/offence), pre-engagement battle
  forecasting, threat and value maps, `Expert` difficulty and four extended
  personalities (Rush / Turtle / AirSuperiority / Guerrilla). All are actually
  consulted by `AICommander` — verified by mutation testing, i.e. deliberately
  unwiring each system and confirming a named test fails, then passes on revert.
  The vertical-slice determinism checksum (`3d34b67647f82b75`) is unchanged by
  the AI additions, so none of it introduced non-determinism.
- **No AI cheating.** Difficulty scales reaction speed, observation cadence,
  memory length and patience only. There is no income multiplier for any tier
  and no fog bypass: battle forecasts are built from fog-limited memory, so an
  unscouted objective yields a deliberately low-confidence estimate. Guarded by
  `NoDifficultyTierIsHandedFreeIncome`, `FogOfWarStrictCompliance` and
  `NoCheatResources`.
- **Skirmish map `RA4_Skirmish_Production`.** Rebuilt as a tropical
  archipelago by `Tools/Editor/make_archipelago_map.py`. Geometry verified
  programmatically (40 actors checked against sampled terrain and the
  waterline, 0 problems) and visually via editor screen capture.

## What is unverified or known broken

- **No packaged game build.** Nothing has been produced that a player could run.
- **`ra4-ui` prototype calls an LLM inside the match loop.**
  `ra4-ui/src/adminConsoleService.ts` posts game state to `openrouter.ai` and
  parses the reply into gameplay commands. That is non-deterministic, so it
  cannot ship in any build supporting lockstep or replay — see
  `Docs/Architecture/ADR/ADR-0018-no-llm-in-runtime-command-path.md`. The file
  also hardcodes an API key. Not fixed here: it belongs to the web prototype,
  not the C++ core.
- **AI depth is unproven at scale.** The hierarchy is wired and unit-tested, but
  it has not been run through a large AI-vs-AI league, so profile balance and
  long-match behaviour are unmeasured. The blueprint's self-play league,
  role/bidding system and deception behaviours are not implemented.
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
| **V-1** | Produce a packaged build that launches — **root cause now identified, still NOT done** | A `RedAlert4.app` and a 445 MB monolithic `Binaries/Mac/RedAlert4` exist, so the *build* half is done. The *package* half is not: launching it fails before the engine initialises. Evidence, both attempts recorded: `LogProjectManager: Error: Failed to open descriptor file ../../RedAlert4.uproject` then `Could not find a valid project file, the engine will exit now`, followed by an ICU assertion. Passing an explicit project path does not help — the binary resolves paths relative to itself and expects a **staged layout** (`<Staged>/RedAlert4/RedAlert4.uproject` beside cooked content). `Saved/Cooked/Mac/` contains only `ue.projectstore`, i.e. **nothing has been cooked**. So this needs a real UAT run (`RunUAT BuildCookRun -project=... -platform=Mac -clientconfig=Development -cook -stage -pak`), not another launch attempt of the existing binary. Blocked-adjacent: cooking pulls in the third-party packs, so RISK-21 (unrecorded licences) should be resolved before a distributable package is produced. |
| **V-2** | Re-audit milestones 2 and 4-11 | For each, either supply evidence from a real run or downgrade it to unproven. Do not accept a document as evidence. |
| **V-3** | Verify the map interactively | Open the archipelago in the editor, click the viewport to force a lit realtime pass, and confirm lighting, water, and foliage read correctly. |
| **V-4** | ~~Repath CityPark material references~~ **DONE — verified by running 2026-08-06** | Fixed by two `CoreRedirects` substring entries in `Config/DefaultEngine.ini` (QuantumCharacter and CityPark), not by touching assets. Evidence: `UnrealEditor-Cmd RedAlert4.uproject /Game/Maps/RA4_Skirmish_Production -run=ResavePackages -verify -unattended -abslog=/tmp/v4verify.log`. **The acceptance criterion as written ("level load produces no LoadErrors") is NOT met and cannot be met by this fix**: the run ends `Failure - 208 error(s), 360 warning(s)` and exits 1. All 208 are attributable to one unrelated third-party pack — 152 `LogBlueprint`/95 `LogScript` compile errors, every one under `Content/ThirdParty/FactoryEnvironment/`, plus one blend-space asset there with an empty sample. **Zero errors mention CityPark or any `RA4_Skirmish` map**, and there are zero material/texture/mesh load failures, which is the claim V-4 actually needed. Note the commandlet walks all 13,435 packages, not just the map, so it surfaces pack rot that never loads in a match. Remaining CityPark mentions are two benign warnings: the documented `MatchSubstring` deprecation notice and `TConvex` physics warnings from an unused showcase map inside the pack. Follow-up split out as **V-6** (FactoryEnvironment compile errors). |
| **V-5** | Replace blockout map art | Requires licensed tropical foliage, cliff meshes, and beach textures. Blocked on asset acquisition; needs a decision from the project owner. |
| **V-6** | ~~Triage `Content/ThirdParty/FactoryEnvironment` compile errors~~ **DONE — verified by running 2026-08-06** | The pack was removed (commit `7ffacbd`). Verified by re-running the same command that surfaced the problem: `UnrealEditor-Cmd RedAlert4.uproject /Game/Maps/RA4_Skirmish_Production -run=ResavePackages -verify -unattended -abslog=/tmp/v6full.log` now ends `Failure - 4 error(s), 23 warning(s)`, down from **208 errors**. All four remaining errors are environmental or engine-owned, none from project content: the `GameFeatureData` asset-manager rule (one error logged twice), `HttpListener unable to bind to 127.0.0.1:8000` (a port another local process holds), and a `FortniteMain` custom-version warning-as-error while resaving an **Engine** tutorial asset. **The exit code is still 1**, so RISK-22 stands: it cannot gate CI until these four are addressed — but they are now a short, named list instead of pack rot. Method note: an earlier attempt reported 0 errors from a log whose process had been killed by a shell timeout; the figure above comes from a run that recorded `COMMANDLET_EXIT=1` and wrote a complete 7.2 MB log. |

## Unreliable Intelligence layer (ADR-0026, branch `feat/intel-unreliable`)

M0 (skeleton) is done and verified: `RA4Intel` module, JSON config + validator,
empty phase pipeline wired into `SimWorld` after fog of war, save v3 with v2
migration, checksum coverage, 14 `Intel.*` tests (suite: 331 passed / 0 failed;
UE 5.8 editor target: `Result: Succeeded`). Feature ships disabled by default.

This M0 claim was independently re-verified on 2026-08-05 by a second session
that built the headless target and ran the suite itself: clean build, 331
passed / 0 failed, 14 `Intel.*` tests present. It is TRUE — a notable exception
in a repository with a history of fabricated status claims.

| Task ID | Task | Acceptance criteria |
| :--- | :--- | :--- |
| **I-M1** | ~~Truthful pipeline (Observation→Report→Track, zero distortion/delay)~~ — **DONE 2026-08-06** | Phases Observation/ReportEmission/Aggregation/TrackUpdate implemented (truthful, zero-delay). PS mirrors visible enemies exactly (`TruthfulPipelineMirrorsVisibleEnemy`), moving contacts update one track via the association table (`TrackFollowsMovingContactWithoutDuplicates`), lost contacts freeze at last-known-position and go stale (`LostContactFreezesAsLastKnownPositionAndGoesStale`), association tables serialized+hashed (save keeps single track after resume). Suite 411/0; UE editor build Succeeded. |
| **I-M2** | ~~Distortion stages 1–5 + unit tests + two-maps overlay~~ — **DONE 2026-08-06** | Stages 1–5 as pure functions, each with a disable flag pinned by `Recon.EveryStageHonoursItsDisableFlag`; χ² on the confusion matrix; fear monotonicity; symmetric incompetence noise; radar anonymous contacts (owner decision D6); morale model with ally-death and superiority-dread terms; budgets measured at the 2000-entity baseline (P-7). Two-maps overlay (`691cfbd`) **visually verified in a running match** (`?Recon=1?ShowTruth=1` → log `RA4 recon enabled (showTruth=1)`, 38 actors synced): belief boxes, ground-truth crosses, error-radius circles and `HV VEH x1 100% 0s` labels all render — evidence in `SCREENSHOTS/recon_two_maps_overlay.png`. I-B5 review obligation discharged by `Recon.SwappedTuningProducesADifferentBeliefTimeline`: equal-hash settings reproduce the belief timeline bit-for-bit, a changed tunable changes it, so the replay header's `ReconSettingsHash` gate is load-bearing rather than decorative. Suite 431 passed / 0 failed; UE 5.8 editor target Succeeded. |
| **I-M3** | ~~Chain of command: hops, delays, aggregation, `bContested`, blackout freeze~~ — **DONE 2026-08-06, independently reviewed, review findings fixed** | Nodes are the player's own command buildings (owner decision 9-в); latency ladder 1 hop / 2 hops / courier / silent; grouped aggregation with corroboration and `bContested`; belief ageing with monotonic error growth and deterministic GC. Review verdict was REQUEST CHANGES: **4 blockers** (three fields hashed but never serialized → guaranteed post-load desync; plus a V-F fog regression this stream introduced), **5 majors** (stale association generation, ratcheting contested interval, track-count-dependent decay rate, blackout untested — and writing the test found the emission veto was unreachable), **6 minors**. All fixed in `0883b7d` + `84e4474`, each mutation-verified. Suite 453 passed / 0 failed; ctest 4/4; UE 5.8 editor target Succeeded. Commits `c2296f5`, `8aa4464`, `85319b3`, `0883b7d`, `84e4474`. |
| **I-M3-perf** | ~~Spatial index for belief queries (review M2)~~ — **DONE 2026-08-06** | Coarse uniform grid (8 tiles/cell) for `GetTracksInRegion`, plus a track→entity reverse index so releasing a track costs O(its own associations) instead of O(every entity slot) — the case blackout creates by design. Both are derived state: rebuilt on load, never serialized, never hashed. Measured at the 2000-entity baseline and the 5000 stress case: recon per-tick 4 µs → **0 µs**, stress test 84 s → 21 s. Two mutation-verified tests, including a direct audit of the reverse index against the forward table (two ordering bugs in the rebuild were caught this way, neither of which would have crashed or mismatched a checksum). Suite 466/0. Commit `34fce2d`. |
| **I-M4** | ~~Fabrication + self-report bias + guaranteed phantom refutation~~ — **DONE 2026-08-06** | Stage 6 gated on the PRODUCT of broken morale, exhaustion and being in contact, so a phantom means something rather than being noise; phantoms are anonymous, get an association-free track, and their truth stays in the private side table. §4.5 guarantee has both paths, each mutation-verified: a friendly unit on the reported tile seeing nothing clears the ghost, and the deadline clears it regardless. Stage 7 (self-report bias) is information-only per owner decision D3. Four real bugs found by writing the tests: distortion skipped players with no observations (making fabrication unreachable in exactly its own use case), dread read `TicksUnderFire` which is smallest when shelling is heaviest, refutation asked the observation log instead of the fog (so a scout in an empty clearing could never disprove anything), and the empty-check used the merge radius so any phantom born in a firefight was unrefutable. Suite 464/0; UE editor Succeeded. Commit `587f4d6`. |
| **I-M5** | ~~Profiling vs budgets + post-match report~~ — **DONE 2026-08-06** | Profiling half was measured under P-7 at the agreed 2,000-entity baseline and then improved by M3-perf (recon per-tick 4 µs → **0 µs** at the 5,000 stress figure; TrackUpdate median 0 µs against a 500 µs hard max; ~16 KB belief per player). Post-match report: every belief-shaping report is logged with what it claimed, the ground truth beside it, the filing post, the hop count and arrival delay, and the observer's morale/fatigue/clarity. `ExplainTrack` names the specific failure — OVERCOUNTED, MISIDENTIFIED, or "the reporting unit invented this contact" — instead of restating the belief. The log is an OUTPUT (excluded from checksum and saves), which is what lets it safely hold ground truth without risking a desync or an INVARIANT 10 leak; proved by running two worlds and clearing one's log every tick. Bounded to a 4,096-entry ring, and a track older than the log says so rather than fabricating a history. Console: `recon.DumpTracks`, `recon.LogChain <idx>`, both ECVF_Cheat and sharing `ExplainTrack` with the UI. Suite 478/0; ctest 4/4; UE editor Succeeded. Commit `548bcab`. |
| **I-M6** | ~~AI commander plays from belief~~ — **STRUCTURALLY DONE 2026-08-06; balance delta NOT measured** | `SimWorldView::UpdateMemory` branches once: recon off scans entities exactly as before (so disabling the layer leaves other systems debugging against the usual opponent), recon on rebuilds enemy memory from `PerceivedWorld`. Memory is replaced not merged (merging retains knowledge the staff map dropped, including refuted contacts); `EnemyMemory::Entity` is left invalid on purpose, since a track has no EntityId and any attempt to look truth back up must fail loudly; confidence comes from the track rather than a second decay clock. Zero-cheat is now structural. Four tests, mutation-verified — the phantom test plants a ghost in a scene with no real enemy, so anything the AI knows came from belief alone. Suite 482/0; ctest 4/4; UE editor Succeeded. Commit `d8892bf`. **OPEN**: the before/after AI strength delta the acceptance criterion asks for needs an AI-vs-AI league; the commander reads less than it used to and will play worse by an unmeasured amount. |

**Budget note — RESOLVED 2026-08-05**: the entity baseline is unified at **2,000**
(product owner decision, recorded in PERFORMANCE_BUDGETS §4.4). I-M5 measures at
2,000; a 5,000-entity run is an informational stress metric only.

### Perception-warfare stream — design and process (ADR-0021..0026)

The wider direction this layer belongs to is documented on
`docs/perception-warfare-adrs`: ADR-0021 (Knowledge Map — design intent behind
ADR-0026), ADR-0022 (Command Network), ADR-0023 (Deception), ADR-0024
(Battlefield Memory), ADR-0025 (Adaptive Opponent), plus
`PERCEPTION_WARFARE_DIRECTION.md`, GDD sections 8-11, PRODUCT_VISION problems
5-7, RISK-11..19, and PERFORMANCE_BUDGETS.md section 4. ADR-0026's intel layer
is system 1 of five; systems 2-5 have no code and none is authorized yet.

| Task ID | Task | Acceptance criteria |
| :--- | :--- | :--- |
| **P-1** | ~~Independent review of ADR-0021..0026~~ — **DONE 2026-08-05** | Two independent reviewers (authors of none of the ADRs) reviewed in two halves after three earlier attempts died on gateway timeouts. Verdicts: ADR-0021 vs ADR-0026 — *divergences partly hidden*, 2 BLOCKERs re-verified in code (see I-B1/I-B2 below); ADR-0022..0025 — all four APPROVE-WITH-CHANGES, none ready for Accepted. Findings recorded in ADR-0026 ("Independent review findings"), ADR-0021 (rejection log) and as the tasks below. |
| **P-2** | ~~Reconcile ADR-0021 against ADR-0026~~ — **DONE 2026-08-05** | ADR-0021 now carries a full rejection log: every divergence classified as accepted-change, rejected-by-omission, or deferred, with reasons. ADR-0021 marked *Superseded in part*; ADR-0026 is authoritative on implemented behaviour. K1–K3 promoted from ADR-0021 prose into INVARIANTS.md as invariants 9–11, with their current violation status stated. |
| **P-3** | ~~Consolidate duplicate ADR numbering~~ — **MOSTLY DONE; one rename OUTSTANDING** | Done: all 23 files in the legacy `Docs/ADR/` and `Docs/ADRs/` directories now carry SUPERSEDED banners (ADR-0011 decided this and it had never been applied — no file had any marker); `Docs/integration/templates/ADR_INDEX.md` is quarantined with a banner because it reserved numbers 0012–0021 that are *all* already occupied in the canonical series, so any agent following it would have created ten collisions; ADR-0011's own errata are recorded in it. Recovered `ADR-0027-faction-economy-extension-points.md` (authored as ADR-0018 on the dead branch, colliding with `main`'s ADR-0018 "No LLM In The Runtime Command Path"). **Open**: `ADR-0011-DirectControl.md` and `ADR-0011-adr-series-consolidation.md` still share the number ADR-0011 in the canonical directory. Renumber the DirectControl one to ADR-0028 and leave a redirect stub; check inbound references first (`grep -rn "ADR-0011" Docs/ Source/`). Next free canonical number after that is ADR-0029. |
| **P-4** | ~~Audit 60Hz claims~~ — **DONE for docs; economy rebalance OUTSTANDING** | Corrected: INVARIANTS, GDD, PRODUCT_VISION, ARCHITECTURE, CONTENT_ARCHITECTURE, NETWORK_ARCHITECTURE, HIERARCHICAL_AI_ARCHITECTURE, DATA_FLOW, TECHNICAL_DESIGN_DOCUMENT, PERFORMANCE_BUDGETS; errata on ADR-0001, ADR-0016, ADR-0019. Remaining 60Hz strings sit inside errata or historical Milestone/Audit reports, which are records of past claims and must not be rewritten. **Economy part RESOLVED 2026-08-05** (product owner decision, recorded in ADR-0016): StandardOre regenerates at 2 credits/s, RichOre at 3 credits/s, delay 1,200 ticks (60 s), via a new integer `RegenIntervalTicks` mechanism (+1 credit every N ticks) replacing the unusable `RegenPerTick` at 20 Hz. Remaining: confirm no code hardcodes a 60-tick assumption, and implement the `RegenIntervalTicks` rename with JSON validator coverage. |
| **P-5** | ~~UI_UX_BIBLE uncertainty language~~ — **DONE 2026-08-05** | `Docs/Production/UI_UX_BIBLE.md` created (the CLAUDE.md-mandated file did not exist at all). Section 1 is normative: three-independent-channel redundancy rule (at most one colour/opacity), five canonical confidence tiers with shape+text encoding, permanent-numeric accessibility mode, source-disagreement surfacing, high-contrast intel mode, two-stage order acknowledgement for ADR-0022, teaching obligations, and eight hard prohibitions. Note: §1.4 source attribution depends on I-B3 — if source types are not restored, the bible must be amended. Sections 2-5 are owned stubs for the UI/UX stream and gate the UI milestone. |
| **P-6** | ~~Visibility-query call-site inventory~~ — **DONE 2026-08-06** | `Docs/Architecture/VISIBILITY_CALLSITE_INVENTORY.md`: every non-test objective-state reader classified (OWN / FOG-GATED / OMNISCIENT-BY-DESIGN / LEAK). Headline findings: the AI is already funneled through ONE belief-relevant site (`AIWorldView::UpdateMemory`) — I-M6 is a one-funnel replacement, not a 15-site migration; two real presentation leaks found and queued (V-A: actor sync spawns visible actors for fogged enemies — player-facing fog hole today; V-B: cursor picking iterates fogged entities). Static leak detector `Recon.ObjectiveStateFunnelInventory` pins the classified file list — an unclassified new reader fails the suite (it caught `MissionRuntime.cpp` missed by the manual sweep on its first run). Runtime per-read detector deferred to I-M6 with SimWorld hooks. |
| **P-7** | ~~Measure the provisional budgets~~ — **DONE for §4.1 (2026-08-06); §4.2/4.3 remain (p) by necessity** | `ProvingGround.ReconBudgetsAt2000Entities4Players` measures at the agreed 2,000/4p baseline and GATES on the §4.1 hard maxima permanently in CI. Measured: recon pipeline ~0 µs median / 5 µs peak per tick (hard max 1,500 µs — ~300x headroom); recon checksum share 5 µs (hard max 600); PerceivedWorld ~18 KB/player at baseline occupancy. The 5,000-entity stress run exists as informational-only per §4.4. Two honest caveats recorded in the budget doc: per-phase µs quantization makes "0" mean "<7 µs", and M3/M4 phases were empty at measurement — REMEASURE then (gates stay valid). Found and fixed en route: `PhaseStats` existed since M0 but was never written — every phase timer read zero until instrumented. §4.2/4.3 stay (p): CommandGraph and TerrainStateLayer have no code, measuring nothing would be fiction. |

### Intel layer — invariant violations found in review (must precede I-M1)

These are code changes, not documentation. Both were re-verified in the headers, not taken on trust.

| Task ID | Task | Acceptance criteria |
| :--- | :--- | :--- |
| **I-B1** | ~~Remove ground truth from the belief read surface~~ — **DONE 2026-08-05** | `bPhantom` moved out of `PerceivedTrack` into `PerceivedWorld::PhantomFlags` (private side table, serialized+hashed in slot order, save format v2). Leak detector: `Intel.PhantomTruthLivesOutsideTheReadSurface` static_asserts the read-surface layout. Suite 391/0; UE editor build Succeeded. |
| **I-B2** | ~~Make belief write access structural~~ — **DONE 2026-08-05** | `GetPerceivedWorldMutable` deleted; every `PerceivedWorld` writer (Initialize/Reset/Allocate/Release/GetTrackMutable/SetLastObservedTick/Deserialize/phantom accessors) is private with `friend IntelSystem` + `PerceivedWorldTestAccess` for the deterministic tests. |
| **I-B3** | ~~Decide the fate of per-source-type decay~~ — **DECIDED 2026-08-05** | Architecture decision recorded in ADR-0021's rejection log: `SourceType` (uint8 enum: visual/radar/thermal/report/inference) is restored to `Observation`, and `DominantSourceType` to `PerceivedTrack` — required by UI attribution (UI_UX_BIBLE §1.4), ADR-0023 sensor axes, and contested readouts. Decay remains a single curve (sensor quality is already modeled at observation time via `Clarity` + confusion matrix); `TrackTuning.DecayMultiplierPerSourcePerMille[]` (identity default) makes per-type decay data-tunable without code change. **Implementation belongs to I-M1** — the enum is part of the truthful pipeline's vocabulary; serializer, checksum and the read-surface leak test must all cover the new fields. |
| **I-B4** | ~~Give amortized decay a home~~ — **DONE 2026-08-05** | `TrackTuning.TracksPerTickBudget` added (default 512 → full sweep of the 4,096-track cap every 8 ticks / 0.4 s, far inside the −2%/s decay timescale); `PerceivedWorld.DecayCursor` added as real sim state — serialized (format v3), checksummed, reset-cleared — because sweep position determines which tick each track decays, so a divergent cursor is a delayed-fuse desync. Validator rejects budget ∉ (0, MaxTracksPerPlayer]; shipped intel_settings.json carries the key. Tests: `Intel.DecayCursorIsSimStateNotScratch`, `Intel.ValidatorRejectsBadTracksPerTickBudget`. Decay *math* remains M2, as designed. |
| **I-B5** | ~~Replay-reconstructible belief test~~ — **DONE 2026-08-05/06, twice, complementary** | (a) `Recon.BeliefIsReplayReconstructible` — two in-process SimWorlds on one recorded command stream agree on full state checksum all 120 ticks (in-process determinism). (b) `Recon.BeliefIsReconstructibleFromReplayAlone` — reconstruction from the serialized replay FILE: format v2 header carries `bReconEnabled` + `ReconSettingsHash` (`ReconSettings::ComputeSettingsHash()`, all gameplay fields, canonical order); rebuild uses an independently constructed equal-hash settings object; per-tick belief checksum of player 0 matches the live run; structures arrive through the replayed stream so the timeline is non-vacuous (review MAJOR addressed). `VerifyReplay` refuses ruleset mismatches in four tested modes; contradictory header (hash without flag) refused at deserialize; `Recon.PerceivedWorldRefusesForeignVersion` pins the version gate (closes I-B4 review MINOR). Independent review: APPROVE-WITH-CHANGES, all findings addressed or anchored to I-M2 (tuning-swap obligation). Ported across the RA4Intel→RA4Recon rename. INVARIANT 11 closed. |

### ADR-0022..0025 — required before Accepted (from review)

| Task ID | Task | Acceptance criteria |
| :--- | :--- | :--- |
| **P-8** | ~~Fix ADR-0022's latency budget and close its two deferred decisions~~ — **APPLIED 2026-08-05** | ADR-0022: end-to-end latency budget (2 ticks input delay + 2 ticks propagation = 4 total), full order-supersede specification keyed by (player, group), recompute tick phase + ascending-EntityId traversal order. Awaiting acceptance decision on the four ADRs. |
| **P-9** | ~~Correct ADR-0023's scope claim and draw its boundary with ADR-0026~~ — **APPLIED 2026-08-05** | ADR-0023: scope estimate corrected (sensor resolution is an intel-phase rework, not pure data), ADR-0022 dependency added, phantoms defined belief-only (no nav/reservation/cap footprint), fabrication boundary with ADR-0026 delimited. Awaiting acceptance decision on the four ADRs. |
| **P-10** | ~~Give ADR-0024 a nav-invalidation contract~~ — **APPLIED 2026-08-05** | ADR-0024: per-tick batched cost mutations with ascending-index recompute order charged to the pathfinding budget; per-region rolling hash for checksum and delta saves. Awaiting acceptance decision on the four ADRs. |
| **P-11** | ~~Specify DoctrineBias serialization and its relationship to existing AI types~~ — **APPLIED 2026-08-05** | ADR-0025: canonical Fixed-only encoding with ascending key order, dropped-entry accounting and version-skew refusal; section 0 states extension of OpponentModel/AIDoctrine/AIStrategy. Awaiting acceptance decision on the four ADRs. |
| **P-12** | ~~Add the unaddressed design risks to the ADR Consequences sections~~ — **APPLIED 2026-08-05** | all four ADRs carry the reviewer-flagged risks in Consequences: spectator readability, snowball coupling, wreck-spam griefing, salvage asymmetry, road collapse, profile poisoning, newcomer guard (zero bias below 8 matches). Awaiting acceptance decision on the four ADRs. |

### Presentation fog leaks (from P-6 inventory — fixable today, independent of recon)

| Task ID | Task | Acceptance criteria |
| :--- | :--- | :--- |
| **V-A** | ~~Hide fogged enemy actors in `RA4SimWorldSubsystem` actor sync~~ — **CODE DONE 2026-08-06; in-editor visual check STILL REQUIRED** | Actor sync now calls `SimWorld::IsEntityVisibleTo(local, index)` every sync and applies `SetActorHiddenInGame` (hide, not destroy — avoids actor churn at fog edges; no-op for own units and fogless matches). Helper moved from private to the public read section of SimWorld.h with a rationale comment. UE 5.8 editor target: `Result: Succeeded`. Headless: `FogOfWar.EntityVisibilityGateAnswersPerViewer` pins the per-viewer contract. **NOT claiming visual verification**: nobody has launched the editor and looked; per CLAUDE.md that check is still owed before this row reads fully done. Known debt: local player is the constant 0 throughout this subsystem — real multiplayer seats must replace it everywhere at once. |
| **V-B** | ~~Gate cursor picking by visibility~~ — **DONE 2026-08-06** | `BuildPickCandidates` skips entities failing `IsEntityVisibleTo(Selection.GetLocalPlayer(), …)`: the cursor, hover tooltip and click-pick can no longer land on fogged enemies. UE editor target builds clean; suite 430/430. Same seat caveat as V-A does not apply here — picking already read the seat from `Selection.GetLocalPlayer()`. |

Sequencing: P-1 and P-2 are **done**. I-B1 and I-B2 are the highest-priority
items in this file — they are live invariant violations in shipped-but-disabled
code, and both gate I-M1. I-B3/I-B4 gate I-M2 (distortion needs a decay model
that exists). I-B5 gates I-M1. P-8..P-12 gate moving ADR-0022..0025 to Accepted
and can proceed in parallel with the I-B work, since they touch only documents.
P-3, P-4 and P-5 remain parallel-safe. P-6 gates I-M6. P-7 gates I-M2.

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
