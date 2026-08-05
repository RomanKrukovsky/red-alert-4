# Perception Warfare — Design Direction Evaluation (`PERCEPTION_WARFARE_DIRECTION.md`)

**Status**: Documentation package COMPLETE. Implementation gated on independent review (NEXT_ACTIONS P-1).
**Foundation gate**: PASSED, verified by running the suite on 2026-08-05 — `main` builds the headless
target clean and reports 479 passing / 0 failing across four suites. It was NOT satisfied via
`remediation/foundation-fixes`, which does not compile (5 errors in `TestAI.cpp`: `TileCoord::IsValid`
and `Ids::AllRefinery` do not exist on that branch). `main` had already superseded it by 12 commits;
that branch is a candidate for archival.
**Date**: 2026-08-05
**Decision authority**: Executive Producer / Game Director review of the 20-idea concept brief.

---

## 1. Selected Core Concept

> RTS, в которой информация может быть ложной, приказы не всегда доходят, противник запоминает игрока, а последствия каждого сражения сохраняются.

This is **Variant A ("Самая умная RTS") + selected elements of Variant C ("Война, которая всё помнит")** from the concept brief. Variant B (temporal mechanics) is *deferred, not rejected* — see §4.

Rationale: Variants A and C decompose into five systems that are all *deterministic sim-state extensions* of architecture we already have (fog grid, entity/component model, HTN AI, replay pipeline, telemetry). Variant B requires parallel world-state — an order of magnitude more architectural risk — and already has a beachhead in ADR-0019 (Chrono temporal debt) that should mature first.

## 2. The Five Foundation Systems (ADRs written)

| # | System | ADR | Sequencing | Why this order |
|---|---|---|---|---|
| 1 | Knowledge Map (belief state, intel decay) | ADR-0021 | **First** | Everything else reads or writes belief state; also structurally enforces AI zero-cheat |
| 2 | Command Network (order propagation, autonomy) | ADR-0022 | Second | Independent of 1, but UX depends on Knowledge Map UI language |
| 3 | Deception System (signatures, decoys) | ADR-0023 | Third | Hard dependency on 1; pure data + belief writes once 1 exists |
| 4 | Battlefield Memory (terrain state, wrecks) | ADR-0024 | Parallel to 2–3 | No dependency on 1; touches nav + save formats instead |
| 5 | Adaptive Opponent (player modeling) | ADR-0025 | **Last** | Needs replay analyzer tooling; benefits from 1–4 existing as doctrine levers |

**Implementation status**: system 1 has an implementation decision record (ADR-0026, "Unreliable
Intelligence Layer") authored by a parallel work stream, whose M0 skeleton exists as
`Source/RA4Intel/` with 14 `Intel.*` tests. Its claim of a clean build and 331 passing tests was
independently re-verified by running the suite and is true. ADR-0021 remains the design-intent
document and is now Superseded in part; ADR-0026 is the authority on what is actually implemented, and
ADR-0021 carries the full rejection log (P-2 complete).

**However**, independent review found the M0 read surface violates two of ADR-0021's own K-invariants:
a ground-truth flag (`bPhantom`) sits inside the struct handed to UI callers, and belief state is
publicly mutable. Both were confirmed in the headers. They are now INVARIANTS 10 and 9 respectively,
recorded as violated, and gate M1 as tasks I-B1/I-B2. The lesson is structural: K1–K3 lived only in ADR
prose, and prose does not stop an implementation. Systems 2-5 have no code.

Each ADR contains its own verification plan. Of the three original preconditions:
1. ~~foundation green~~ — **DONE** via `main` (479/479, verified by run; see Status above);
2. ~~performance budgets~~ — **DONE**, `PERFORMANCE_BUDGETS.md` section 4, with provisional figures
   marked `(p)` that must be replaced by measurements (NEXT_ACTIONS P-7);
3. independent architecture review — **DONE 2026-08-05**, after three attempts lost to gateway
   timeouts, split across two reviewers who authored none of the documents. Outcome: ADR-0022..0025 all
   **APPROVE-WITH-CHANGES** (none ready for Accepted; required changes tracked as P-8..P-12), and the
   ADR-0021/ADR-0026 pair returned **divergences partly hidden** with two BLOCKERs re-verified directly
   in the M0 headers (tracked as I-B1/I-B2). Implementation of systems 2-5 remains unauthorized until
   their required changes land.

## 3. Disposition of the Remaining 15 Ideas

**Absorbed into the five ADRs (no separate system needed):**
- Идея 4 «информационный туман» → is ADR-0021 itself.
- Идея 3 «дезинформация» → ADR-0023 v1 scope (3 tools); UI-фальсификации (fake alerts, icon substitution) explicitly deferred inside ADR-0023.
- Идея 8 «наследство юнитов» → ADR-0024 WreckEntity.
- Идея 7 «поле боя помнит» → ADR-0024 TerrainStateLayer + TheaterState.
- Идея 1 «враг учится» / цифровой двойник → ADR-0025.
- Идея 15 «база перестраивается по правилам» → partially ADR-0022 autonomy doctrines; full rule-editor UI deferred (post-v1 tooling).

**Deferred — good ideas, wrong time (register entries, no ADRs yet):**
- Идея 5 «хронодолг» — extend ADR-0019 after economy telemetry (ADR-0020) validates the debt math in playtests.
- Идея 6 «две версии карты» — after ADR-0024 proves layered map state; re-evaluate as *layer states of one map*, exactly as the brief suggests.
- Идея 11 «доктрины вместо апгрейдов» — after ADR-0025, reusing DoctrineBias vocabulary for the *player* side.
- Идея 18 «ИИ-директор событий» — after ADR-0025 analyzer exists (same signals).
- Идея 20 «документальный фильм после матча» — after replay analyzer (ADR-0025 tooling) exists; high marketing value, low sim risk.
- Идея 9/10 «характер армии / память командиров» — after Battlefield Memory; needs narrative design pass first (frustration risk).
- Идея 16 «база притворяется мёртвой» — AI content built on ADR-0022 distributed command graph; skirmish option, not default.
- Идея 17 «законы карты» — content-layer feature; needs map-editor stream maturity.

**Rejected for v1 (recorded to prevent scope creep):**
- Идея 12 «конструктор техники» и Идея 13 «конструктор супероружия» — combinatorial balance surface incompatible with shipping 4 asymmetric factions; revisit post-release as expansion candidates.
- Идея 19 «сохранения как сюжет» — clever, but conflicts with accessibility rules and QA determinism of campaign missions; a *single* scripted nod to it may live in Chrono campaign as narrative content, not as a system.

## 4. Product Positioning Impact

The four factions map cleanly onto the perception-warfare axis, replacing "faction = different units" with "faction = different relationship to information":

- **СССР** — massed force, resilient analog command (degrades gracefully when jammed);
- **Альянс** — sensor/network superiority, best Knowledge Map fidelity, fragile to infrastructure loss;
- **Восточная коалиция** — deception and information attack specialists (ADR-0023 flagship);
- **Хронолегион** — keeps temporal identity via ADR-0019 debt economy now; parallel-timeline mechanics later (Variant B deferral).

`PRODUCT_VISION.md` §4 gains a fifth solved genre problem: **"Scouting is a solved checkbox in modern RTS"** → solved via belief-state intel with confidence and decay. Formal edits to PRODUCT_VISION/GDD are **not** made in this change to avoid conflicting with the active remediation branch — they are queued in §5.

## 5. Follow-up Work — Status

| # | Item | Status |
| :--- | :--- | :--- |
| 1 | Independent review of ADR-0021..0026 by a non-author agent (project rule 12) | **DONE** — two reviewers, findings recorded in ADR-0026 and ADR-0021; follow-ups are I-B1..I-B5 and P-8..P-12 |
| 2 | `PERFORMANCE_BUDGETS.md` budgets for intel, CommandGraph, TerrainStateLayer | **DONE** — section 4; provisional numbers marked `(p)`, measurement tracked as P-7 |
| 3 | `GAME_DESIGN_DOCUMENT.md` sections for intel, command infrastructure, salvage | **DONE** — sections 8-11 |
| 4 | `PRODUCT_VISION.md` positioning per §4 | **DONE** — genre problems 5-7 and the differentiation rationale |
| 5 | `RISK_REGISTER.md` frustration and process risks | **DONE** — RISK-11..19, including accessibility of uncertainty presentation |
| 6 | `Docs/Agent/NEXT_ACTIONS.md` sequencing | **DONE** — tasks P-0..P-7 |
| 7 | ADR-0022 open question: DirectControl vs propagation | **RESOLVED** — a possessed unit bypasses the command graph; recorded in ADR-0022 and GDD section 9 |

Discovered and fixed while completing the above, not part of the original plan:

| # | Item | Status |
| :--- | :--- | :--- |
| 8 | `INVARIANTS.md` contradicted the code on three counts: 60Hz/16.66ms tick (actual: 20 Hz / 50 ms, `kTicksPerSecond`), `FixedPoint.h` (actual: `RA4Core/Fixed.h`), checksum every 10 ticks (actual: 20, `kChecksumIntervalTicks`) | **FIXED** — v3.1, corrected against `SimConfig.h` |
| 9 | Tick-derived figures in ADR-0021/0022 were computed at 60Hz | **FIXED** — recalculated at 20 Hz |
| 10 | ADR-0026 was untracked in a shared working copy and at risk of loss through stash churn | **FIXED** — preserved on this branch |
| 11 | ADR-0001 still claims 60Hz; other docs may too | **OUTSTANDING** — tracked as NEXT_ACTIONS P-4 |
| 12 | Duplicate ADR numbering across `Docs/ADRs/` and `Docs/Architecture/ADR/`, with `ADR-0011` used twice | **OUTSTANDING** — tracked as NEXT_ACTIONS P-3 |

## 6. What Was NOT Done

- **No code, content or test changes by this work stream.** The only code in this direction is
  ADR-0026's M0, written by a different session.
- **ADR statuses (updated 2026-08-05)**: ADR-0022..0025 are **Accepted** — independent review
  passed, all required changes applied, product owner sign-off recorded in each Status line.
  ADR-0021 is Superseded in part by ADR-0026 (which carries the rejection log). ADR-0026's M0 is
  Accepted and its two invariant violations are FIXED (commit e557128, re-verified independently:
  391/391 tests, leak-detector test present). Acceptance authorizes *scheduling*; sequencing in §2
  still governs what may start when.
- **No implementation is authorized** for systems 2-5 (Command Network, Deception, Battlefield Memory,
  Adaptive Opponent).
- **Provisional budgets are not measurements.** Every `(p)` figure in PERFORMANCE_BUDGETS.md section 4
  is an engineering estimate and must not be cited as evidence of performance.
- **No UI language exists yet** for confidence presentation; UI_UX_BIBLE work is a precondition for
  building any intel UI (RISK-11, RISK-19, NEXT_ACTIONS P-5).

## 7. Working-Copy Note

This package was authored while the main working copy was in continuous use by other sessions —
during the work its branch changed four times (`remediation/foundation-fixes` →
`feat/archipelago-skirmish-map` → `feat/intel-unreliable` → `feature/kimi-skirmish-production`) and
the documents were at one point swept into a stash by an external tool. They were recovered, and a
parallel session has since landed ADR-0021..0026 on `main` directly.

`docs/perception-warfare-adrs` is therefore rebased on `main` and contains only what is *not* yet
there: GDD sections 8-11, PRODUCT_VISION problems 5-7, RISK-11..19,
PERFORMANCE_BUDGETS.md section 4, the tick-rate corrections across nine documents, the recovered
ADR-0019/0020, and the resolution of the ADR-0022 DirectControl question. It touches only `Docs/` and
can be merged independently of any code branch.

Two ADRs were recovered from `remediation/foundation-fixes` in the process. That branch does not
compile (5 errors in `TestAI.cpp`: `TileCoord::IsValid` and `Ids::AllRefinery` do not exist on it) and
`main` had already superseded it by 12 commits, but it was the sole home of ADR-0019 and ADR-0020.
Archiving it without extracting them would have silently broken every reference from this package.
