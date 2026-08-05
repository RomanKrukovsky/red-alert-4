# Perception Warfare — Design Direction Evaluation (`PERCEPTION_WARFARE_DIRECTION.md`)

**Status**: Direction approved for documentation; implementation blocked until `remediation/foundation-fixes` is green.
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

Each ADR contains its own verification plan; none may start implementation until:
1. `remediation/foundation-fixes` is merged and the full test suite is green (verified by run, not by docs);
2. performance budgets for each system are added to `Docs/QA/PERFORMANCE_BUDGETS.md`;
3. an independent architecture review of ADR-0021 (it modifies the fog/AI contract).

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

## 5. Queued Follow-up Work (do not start before remediation is green)

1. Independent review of ADR-0021..0025 by an agent that did not author them (project rule 12).
2. `PERFORMANCE_BUDGETS.md`: tick/memory budgets for KnowledgeMap, CommandGraph, TerrainStateLayer.
3. `GAME_DESIGN_DOCUMENT.md`: new sections — Intel & Confidence, Command Infrastructure, Salvage.
4. `PRODUCT_VISION.md`: positioning update per §4.
5. `RISK_REGISTER.md`: add frustration-risk entries (deceived players, delayed orders) with mitigation owners.
6. `Docs/Agent/NEXT_ACTIONS.md`: sequence entry after current remediation items.
7. Resolve open question in ADR-0022: DirectControl (F-key possession) vs command propagation.

## 6. What Was NOT Done

- No code, no content, no test changes.
- No edits to shared living documents (GDD, PRODUCT_VISION, PROJECT_STATE, NEXT_ACTIONS) — another work stream is active on this working copy; edits are queued instead (§5).
- No commit yet: working copy is shared with an active branch (`feat/archipelago-skirmish-map`) and the machine is near disk exhaustion; these five ADRs + this document must be committed on a dedicated `docs/perception-warfare-adrs` branch once the working copy is free.
