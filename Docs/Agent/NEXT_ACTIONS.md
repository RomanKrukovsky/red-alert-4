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
| **V-1** | Produce a packaged build that launches | A binary that starts, loads `RA4_Skirmish_Production`, and reaches a playable state without a crash. Record the exact command and its output. |
| **V-2** | Re-audit milestones 2 and 4-11 | For each, either supply evidence from a real run or downgrade it to unproven. Do not accept a document as evidence. |
| **V-3** | Verify the map interactively | Open the archipelago in the editor, click the viewport to force a lit realtime pass, and confirm lighting, water, and foliage read correctly. |
| **V-4** | Repath CityPark material references | Level load produces no `LoadErrors`. |
| **V-5** | Replace blockout map art | Requires licensed tropical foliage, cliff meshes, and beach textures. Blocked on asset acquisition; needs a decision from the project owner. |

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
| **I-M2** | Distortion stages 1–5 + unit tests each + two-map debug overlay | χ² on confusion matrix; fear monotonicity; per-stage disable flags honoured. **Plus (I-B5 review obligation)**: extend `Recon.BeliefIsReconstructibleFromReplayAlone` with a tuning-swap case — replaying under settings with a different hash must yield a different belief timeline, or the header hash gate is decorative. |
| **I-M3** | Chain of command: hops, delays, aggregation, `bContested`, blackout freeze | Blackout keeps last-known data, error radius grows monotonically |
| **I-M4** | Fabrication + self-report bias + guaranteed phantom refutation | Phantom always cleared within `MaxPhantomLifetimeTicks` by clean observation |
| **I-M5** | Profiling vs budgets (≤0.8 ms/tick @ **2,000** entities — baseline unified with PERFORMANCE_BUDGETS §4.4 by product owner decision 2026-08-05; a 5,000-entity run is recorded as a stress metric, never a gate), post-match report | Numbers from real runs recorded in PERFORMANCE_BUDGETS.md |
| **I-M6** | AI commander plays from belief (`GetIntel()`), not GT scans | Zero-cheat structural; AI strength delta measured before/after |

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
| **P-7** | Measure the provisional budgets | Replace every `(p)` figure in PERFORMANCE_BUDGETS.md section 4 with a measured number at an agreed entity baseline (see the budget note above). One renegotiation with evidence is permitted, then the numbers freeze. |

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
| **V-A** | Hide fogged enemy actors in `RA4SimWorldSubsystem` actor sync | Enemy entities on tiles that are neither CurrentlyVisible nor RadarDetected for the local player have hidden actors (`IsEntityVisibleTo` exists at SimWorld.h:200 — use it). Player-facing fog hole TODAY: fogged enemies render on screen. Requires an in-editor visual check, not only unit tests. |
| **V-B** | Gate cursor picking by visibility | `RA4PlayerController.cpp:677` skips entities failing `IsEntityVisibleTo(LocalPlayer, …)`; tooltip/cursor can no longer reveal fogged presence. |

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
