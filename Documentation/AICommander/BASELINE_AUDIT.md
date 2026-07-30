# AI Commander — Baseline Audit

**Date:** 2026-07-30
**Method:** read of the actual source tree and test suite. README, `PROJECT_STATE.md`,
`HANDOFF.md` and prior generation reports were **not** trusted as evidence; every claim
below cites a file and was confirmed by grep/read or by running the tests.

**Baseline commit:** `51ed3be`
**Test state at audit time:** `RA4Tests` 213 passed / 0 failed, `RA4InputTests` 46/0,
`RA4PresentationTests` 21/0, `RA4AITests` 30/0.

---

## 1. What actually exists

### 1.1 Live AI module — `Source/RA4AI` (2251 lines)

Engine-free, listed in `RedAlert4.uproject`, linked by `RedAlert4` and by the headless
test binaries. This is the only AI that runs.

| File | Lines | Live? |
| --- | --- | --- |
| `Private/AICommander.cpp` | 972 | **yes** |
| `Private/AIStrategy.cpp` | 242 | **yes** |
| `Private/AIWorldView.cpp` | 134 | built, **never called by the commander** |
| `Private/TacticalOperation.cpp` | 35 | yes (state tracking only) |
| `Private/HTNPlan.cpp` / `HTNTask.cpp` / `HTNWorldState.cpp` | 175 | **dead** |

Architecture: a flat utility scorer. `ScoreStrategies()` produces a score per
`AIStrategy` (`ExpandEconomy`, `TechUp`, `Fortify`, `AssembleArmy`, `Assault`,
`Recover`); `SelectStrategy()` applies hysteresis (`StrategySwitchMargin`) and an
emergency override; `ExecuteStrategy()` dispatches to one `TryBuild*` / `CommandArmy`
step. One decision every `DecisionIntervalTicks` (default 10 = 0.5 s at 20 Hz).

### 1.2 Determinism infrastructure — sound, keep as-is

- `SimWorld::ComputeStateChecksum()` (`SimWorld.h:128`).
- `Replay` records per-checkpoint checksums (`Replay.h:53-98`).
- `Random` seeded per commander; `AICommander::Initialize(Player, Profile, Seed)`.
- Server-side rate limit `kMaxCommandsPerPlayerPerTick = 64` (`SimWorld.h:237`),
  rejecting with `CommandReject::RateLimited` (`SimWorld.cpp:722`). The AI is bound by
  the same limit as a human — good, and it is the existing anti-spam guarantee.
- `RA4_TEST(AI, IsDeterministic)` already asserts identical outcomes for equal seeds.

**Conclusion:** the determinism contract required by the brief is already satisfied at
the simulation layer. New AI work must not add state outside `SimWorld` that feeds
decisions without being seeded and serialisable.

### 1.3 Fog-of-war knowledge model — written, correct, and **not wired up**

`SimWorldView` (`AIWorldView.h:53`) implements exactly the knowledge model the brief
asks for, and it *is* fog-aware — `UpdateMemory()` consults `World.GetFogGrid()` and
skips entities whose tile is not `CurrentlyVisible` or `RadarDetected`
(`AIWorldView.cpp:40-69`). `EnemyMemory` carries `LastSeenTick` and a decaying
`Confidence`; `DecayMemories()` ages unrefreshed records to a 0.1 floor and drops them
past the retention window. `RA4_TEST(AI, SimWorldViewTracksEnemyMemoryAndDecaysConfidence)`
covers it.

---

## 2. Critical findings

### F-1 (blocker) — the AI cheats: it sees through fog of war

`AICommander` never references `IAIWorldView`, `SimWorldView`, `KnownEnemies` or
`EnemyMemory`. Verified: grep for all five symbols across `AICommander.cpp` returns
nothing.

Instead `AICommander::Tick(const SimWorld& World, ...)` takes the full authoritative
world, and `FindEnemyTarget()` (`AICommander.cpp:297-322`) iterates **every** core in
the world and returns the first living enemy building regardless of visibility.

This directly violates the brief ("AI не видит юниты под fog of war"). The fix is not
to write a knowledge model — one already exists and is tested — but to route the
commander through it and remove the direct-world escape hatch.

### F-2 — three competing AI frameworks in-tree

The brief forbids introducing competing frameworks; the repository already has three.

1. `Source/RA4AI` utility commander — **live**.
2. `Source/RA4AI/**/HTN*` (6 files, 175 lines) — compiled into the module but
   referenced by nothing outside itself. Dead.
3. `Source/RAAI/` — a *separate 19-file Unreal-style module* (`AAIDirector`,
   `FAIHTNPlanner`, gameplay-tag driven). **Not listed in `RedAlert4.uproject`**, not
   referenced by any `.Build.cs`. Never compiled. Dead.

Decision for this work: extend the live utility commander into a three-tier system.
Do not revive either HTN path. Removal of dead code is deferred until the new system is
proven, per the brief's "do not delete the old AI before the new one is demonstrated".

### F-3 — target selection has no threat or value model

`FindEnemyTarget` returns the *first* enemy building in entity-index order — not the
most valuable, most threatening, or most reachable. There is no scoring of refinery vs
power plant vs artillery vs superweapon, no route-danger term, no anti-target-flapping
guard. Everything in the brief's threat/value section is missing.

### F-4 — no Army Groups, no operational layer

`CommandArmy()` (`AICommander.cpp:~818-870`) rescans every owned entity each decision
tick and issues one `AttackMove` per **idle** armed unit toward a single world target.

What is genuinely already right: it only orders units whose `OrderQueue` is empty, so
it does not reset in-flight orders every tick, and wounded units are skipped because a
retreat may already have been emitted. That is the correct instinct and should be
preserved.

What is missing: persistent groups with stable IDs, roles, leaders, target composition,
reinforcement, rally points, zones of responsibility, and reserves. Army composition is
a single scalar (`ArmySize` vs `AttackArmySize`).

### F-5 — no formations on the AI side, but a formation system exists

`RA4Navigation/Formation.h` already provides `FormationDef` (leader-relative offsets),
`FormationAssignment` (leader + slot-ordered members) and `RotateOffset()`, with tests
in `TestNavigation.cpp`. The design note is explicit that members follow
`LeaderPos + Rotate(Offset[slot], LeaderFacing)` so a formation costs one flow field,
not N.

The AI uses none of it. This must be reused, not reimplemented.

### F-6 — role tags do not exist

`Role.Scout`, `Role.Artillery`, `Role.AntiAir`, `Role.AntiArmor`, `Role.Harvester`,
`Role.Engineer` appear nowhere in the project (grep across `.ini`, `.cpp`, `.h`). There
is no `GameplayTagList` in `Config/`.

Content classifies capability today with: `ProductionCategory` (`ContentTypes.h:79`),
`Building.bIsRefinery` (`:158`), `Unit.bIsHarvester` (`:178`), `Unit.bIsBuilder`, and
`Weapon.IsValid()`. The AI branches on these booleans directly.

Consequence for the brief's "don't hardcode unit classes, use role tags": the tag
vocabulary has to be **created** as part of this work. Because the simulation core is
deliberately engine-free, it cannot depend on Unreal's `FGameplayTag`; the role
vocabulary must be an engine-free enum/bitmask on `EntityDef`, with the Unreal
`GameplayTag` layer mapping onto it for data assets.

### F-7 — player-side army control is minimal

`SelectionModel` has 10 control groups, marquee, same-type double-click, priority
sorting and dead-unit pruning — all tested (46 input tests). There are **no** stances,
no formation movement, no attack-move for the player beyond a single armed mode, no
retreat/escort/screen/siege commands, no assisted command, and no group roles.

### F-8 — faction doctrines are content, not behaviour

`RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` defines four factions with
distinct identity, resources and unit rosters. `AIProfile` has only
`Adaptive/Balanced`, `Aggressive`, `Defensive`, `Economic` — numeric weight sets
(`MakeProfileConfig`), identical in behaviour across factions. No doctrine concept.

---

## 3. Performance baseline (measured, not estimated)

`RA4AITests` 30 tests in ~0.65–3.7 s wall depending on build; the full AI-vs-AI match
test (`TwoCommandersPlayAMatchToCompletion`) and the 5-scenario suite run headless in
that budget. `RA4MatchDump` plays a complete 3525-tick match (176 simulated seconds) in
well under a second.

No profiling of AI cost per tick at scale exists. `ProvingGround.HeadlessStressScenario500Entities`
covers 500 entities for the simulation, **not** for AI decision cost. There is no
measurement at 1000 or 3000 entities, and no per-layer timing. Establishing those
budgets is part of this work, not something to be claimed from the existing suite.

---

## 4. Consequences for the plan

1. **Fix the cheat first.** F-1 is a correctness blocker and every later behaviour
   claim is meaningless while the AI reads the whole map. Route the commander through
   `IAIWorldView` and make the direct `SimWorld` path impossible to use for target
   acquisition.
2. **Build the role vocabulary before the tactical layer** (F-6), engine-free, or every
   later system hardcodes unit classes.
3. **Reuse `RA4Navigation` formations** (F-5) and the existing rate limit (1.2).
4. **Preserve the idle-only ordering discipline** already present in `CommandArmy`
   (F-4) — it is the existing defence against command spam.
5. **Do not touch determinism guarantees**: new AI state must be seeded, ordered and
   serialisable, and must not enter `ComputeStateChecksum` unless it is authoritative
   simulation state.

---

## 5. Known risk outside the code

Throughout this session the working tree was being modified concurrently by another
process (GitHub Desktop / a parallel editing session); `main` was reset from `51ed3be`
back to `de90753` at one point and several in-progress edits were overwritten. Parallel
multi-agent writes into this repository are unsafe until that concurrency is stopped.
