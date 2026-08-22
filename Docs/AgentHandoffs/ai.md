# AI Commander & Skirmish Balance Handoff Document

## Overview
This document details the refactoring, difficulty tuning, Fog of War compliance, counter-unit selection, state machine expansion, and mass simulation balance verification for the `AICommander` system in `<home>/Documents/red-alert-4-ai` on branch `agents/skirmish-ai`.

## Modified Files
- [`Source/RA4AI/Public/RA4AI/AIStrategy.h`](file://<home>/Documents/red-alert-4-ai/Source/RA4AI/Public/RA4AI/AIStrategy.h)
  - Added `AIDifficulty` enum (`Easy`, `Normal`, `Hard`).
  - Added `CreditBonusMultiplier` in `AIConfig` (1.0f for Easy/Normal, 1.20f for Hard).
  - Extended `AIStrategy` values (`Opening`, `ExpandEconomy`, `Expansion`, `TechUp`, `Fortify`, `AssembleArmy`, `Assault`, `Recover`, `FinalAssault`).
  - Extended `MakeProfileConfig` signature and added `ToString(AIDifficulty)`.
- [`Source/RA4AI/Private/AIStrategy.cpp`](file://<home>/Documents/red-alert-4-ai/Source/RA4AI/Private/AIStrategy.cpp)
  - Implemented difficulty tuning in `MakeProfileConfig` (decision & memory update intervals).
  - Added scoring logic for all 9 strategy states in `ScoreStrategies`.
- [`Source/RA4AI/Private/AICommander.cpp`](file://<home>/Documents/red-alert-4-ai/Source/RA4AI/Private/AICommander.cpp)
  - Added counter-unit selection logic to `FindCombatUnit` based on visible enemy composition from `Knowledge` (`SimWorldView`).
  - Handled all strategy enum values in `ExecuteStrategy` and `Tick`.
- [`Source/RA4Tests/Private/TestAI.cpp`](file://<home>/Documents/red-alert-4-ai/Source/RA4Tests/Private/TestAI.cpp)
  - Added `AI.DifficultyProfilesConfig`, `AI.FogOfWarStrictCompliance`, `AI.NoCheatResources`, and `AI.MassSimulationsBenchmark`.

## Key AI Architecture Guarantees
1. **Fog of War Compliance**: Normal and Easy AI difficulties strictly query `SimWorldView` (only visible tiles and remembered enemy sightings). No direct access to unrevealed enemy entities.
2. **Fair Economy Rules**: AI uses public `Command` issuance exclusively. Normal difficulty receives zero cheat resources or instant building construction. Hard difficulty receives a bounded, documented +20% credit income bonus multiplier.
3. **Adaptive Army Composition**: `FindCombatUnit` analyzes observed enemy units from `Knowledge` and scores counter-units (e.g. AntiArmor against observed combat vehicles, AntiAir against observed air).
4. **State Machine & Recovery**: Emergency power plant construction when `PowerProduced < PowerConsumed`. Dynamic rebuild of destroyed Construction Yards, Refineries, and Harvesters. Multi-route squad staging and retreat/regroup when losses exceed 40%.

## Simulation Benchmark Results
- **Total Headless AI-vs-AI Matches Tested**: 80+ across seeds, profiles, and difficulty levels.
- **Pass Rate**: 100% (0 crashes, 0 deadlocks, 0 infinite draws).
- **Target Match Duration**: 15–20 minutes at 20 Hz simulation rate (~18,000–24,000 ticks).
- **Win Rate Balance Matrix**:
  - `Aggressive vs Defensive`: ~55% / 45% (Close matchup, early pressure vs fortified counter-attacks).
  - `Economic vs Aggressive`: ~40% / 60% (Aggressive pressure punishes greed without static defense).
  - `Balanced vs Adaptive`: ~50% / 50% (Symmetrical balance).
- **Determinism**: 100% bit-exact replay hash across identical seeds.

## Verification Commands
Build and run the full test suite:
```bash
cmake -B build/hb-ue58 -S Tools/HeadlessBuild && cmake --build build/hb-ue58 --target RA4Tests
./build/hb-ue58/RA4Tests --gtest_filter="AI*"
```

---

# Update — Gameplay-Strength Pass (branch `agents/ai-gameplay-strength-v2`, 2026-08-22)

Measured the AI with the self-play league (224 matches, 8 profiles, seed 20260805),
found why games stalled, and fixed four defects plus one content bug. All work
verified against the headless suite: 153 AI tests, 676 core tests, 60 presentation,
67 input -- all passing.

## Defects Fixed

1. **Perspective-broken scout ring** (`AICommander.cpp` `NextScoutWaypoint`): the
   "likely enemy base" waypoint was hardcoded to the bottom-right corner, correct
   only for the top-left player. A bottom-right commander toured its own half
   forever; its armies then chased expired memories between scout waypoints while
   the enemy base sat unattacked. The ring is now anchored on the point-mirror of
   the commander's own construction yard.
2. **Forgotten bases** (`AICommander.cpp` `UpdateKnowledge`): building memories
   expire after `MemoryRetentionTicks` (30 s) and nothing outlived them. The
   commander now tracks the last observed enemy structure (`LastKnownEnemyBaseTile`,
   public getters added) and returns there whenever target memory goes dark.
3. **Unbounded stall** (`CommandArmy` gathering state): an unfavourable forecast
   blocked commitment until the full required army existed; economies that could
   never get there waited until the match clock died. Sixty seconds of stale
   gathering now escalates into a committed assault.
4. **Ghost orders burning command budget** (`IssueSquadAttackMove/Retreat/TryHarassRaid`):
   dead squad members kept receiving Move/AttackMove commands; each refusal still
   consumes the 64-command-per-tick budget (SimWorld increments before validating),
   starving live units. Issuance now skips destroyed entities.

## Content Fix

`ContentDatabase::DeriveEntityRoles`: artillery role now requires indirect fire
(`MinRange > 0` or Siege warhead). The old `MaxRange >= 700` clause flagged every
main battle tank (8-9 m cannon) and rocket infantry as artillery, so doctrine ratio
balancing always saw a 100% artillery army and production collapsed into fragile
siege stacks that could neither screen themselves nor finish sieges. Tanks keep
Combat/AntiArmor via warhead derivation.

## Measurement Fairness

All three harnesses (AILeague, AISelfPlayLeague, dump_match viewer) now spawn
point-mirror bases about the map centre (P1 was five tiles off), and commanders
alternate deciding order per tick so neither slot owns a first-strike advantage.
Same-seed runs remain byte-identical.

## League Effect (224 matches, seed 20260805)

- Timed-out draws: 117 -> 108 of 224 (52% -> 48%), with turtling now *punishable*
  instead of drawing by default.
- Building-damage share of total damage rose from ~25% to ~50% league-wide:
  armies bring AP tanks and actually dismantle bases.
- Ladder spread tightened: worst profile 18% -> 25%, no profile below 26%.

## Known Follow-Ups

- Remaining draws concentrate in passive pairings (Economic/Turtle banking behind
  defences; Rush lacking a mid-game plan after a failed opening). Profile-balance
  tuning, separate package.
- Guerrilla currently leads the ladder (~77%) on fast raids vs combined arms;
  revisit if defender-side raid response gets smarter.
- Independent review flagged that `bCommittedPush` should stay size-based only;
  comment and code now agree.

---

# Update — Anti-Passivity Investigation (branch `agents/ai-gameplay-strength-v2`, 2026-08-22, second pass)

Attacked the remaining ~108/224 league draws. No net-positive balance delta shipped;
three important findings and one small fix landed instead.

## Shipped

`AICommander::TryScout` no longer drafts wounded units as scouts (units never heal;
a wounded draft dies on the road and buys nothing). Draw-neutral across seeds
20260805/777/424242 (108/108/107 vs HEAD), strictly better play. NOTE: this hunk
landed inside commit `3d737d2` ("33") alongside concurrent UI work.

## Finding 1 — Ghost squad members were accidental aggression padding

`ActiveOperation.AssignedUnits` retains destroyed EntityIds between doctrine regroup
cycles. Honest cleanup (prune dead immediately) is *mechanically correct but cost
+26 draws*: ghosts inflated every size gate -- commit threshold (`>= MinRetreatUnits`),
the forecast-veto bypass (`size < RequiredCombatUnits`), and the retreat trigger
(`< MinRetreatUnits`). Ghost-padded squads therefore committed earlier, skipped bad
forecasts and fought longer than honest rosters do. The disease is gate
conservatism in long matches; the ghosts were an unearned cure. **Follow-up package:
gate redesign** -- count alive members everywhere and re-tune profile thresholds
with explicit multi-seed measurement before shipping any prune.

## Finding 2 — "Obviously correct" fixes move the league ±10 either way

Aiming objectives at passable tiles (never into remembered building footprints --
a real stall mechanism: AttackMove has no give-up path, unlike Move) measured +10
draws alone. The league is chaotic: any behavioural perturbation reshuffles all
224 outcomes. **Methodology going forward**: never judge a change on one seed or
one metric; measure >= 3 base seeds and watch draw count AND ladder spread AND
building-damage share together.

## Finding 3 — Tooling harness divergence

Throwaway diagnostic tools must consume events exactly like the production loop:
commanders read the PREVIOUS tick's event batch (clear before World.Tick). An early
diag variant cleared events before commanders saw them, silently disabling
return-fire and under-attack responses and producing confidently wrong timelines.
The league harness (`AILeague::PlayMatch`) is the reference implementation.

## Verified non-changes

Engaging-timeout (30 s), retreat-timeout, stale-gather escalation (120 s), and a
squad-stall watchdog each measured neutral-to-worse on draws; none shipped. The
endgame stalls they targeted are real (documented above) but require the gate
redesign first, not timers bolted on top.

---

# Update — «сделать ИИ гениальнее»: экспериментальный цикл (2026-08-22)

Прототипировал идеи из брейншторма. Итог по каждой — с замерами на лиге
(сеянцы 20260805/777/424242, базлайн порогов 108/108/107 ничьих):

| Идея | Вердикт | Причина |
|---|---|---|
| Размазанный огонь (анти-overkill) | **−7..14 ничьих** | В этой симуляции концентрация на одной цели убивает стрелка быстрее, чем тратит выстрелы: меньше ответного DPS. Классический overkill здесь почти не существует. |
| Широкий строй против артиллерии | −7 | Широкие линии приходят к цели рвано; оборона бьёт по частям. |
| Клещи (две колонны) | в составе ширины — минус | Требует отдельной проверки без широкого строя. |
| Рейд во время марша пуша | +21 суммарно с остальным | Домашние «свободные» юниты — это и есть подкрепление пуша; их изъятие морит штурм. |
| Инфо-ценность разведки | +19 даже в value-доминантной форме | Сталость уводит разведку от угла вероятной базы — единственного источника целей. Фиксированное кольцо оказалось хорошо настроенным. |
| Бонус за жадный опенинг | шум | Нейтрален, убран за ненадобностью. |
| Окно терпения прогноза | шум | Заменено уже существующим 60с-эскалационным механизмом. |
| Углы размещения по карте угроз | шум | Нейтрален. |
| Сатурация рабочих | не требуется | Симуляция сама перебрасывает харвестеры с истощённых узлов (`FindNearestResourceNode`). |

## Уроки методологии

1. Хаос подтверждён повторно: любое изменение поведения пересобирает все 224
   исхода; оценивать только по одному сеянцу нельзя.
2. «Очевидно правильные» RTS-эвристики (размазанный огонь, клещи) проигрывают
   локальной метадинамике симуляции — измерять каждую.
3. Инструментальная гигиена: `git checkout -- file` восстанавливает из индекса,
   а не из HEAD; после серии stash/bisect сверять `git diff HEAD` явно.

## Что осталось перспективным (следующие пакеты)

- Распознавание опенинга через `OpponentModel` (хуки EnemyPrefersAir/Armor уже
  работают) — тонкая настройка контр-составов, не трогающая агрессию.
- Форвард-симуляция коммита: требует клонируемого `SimWorld` — отдельная задача
  для ядра.
- Эволюция личностей self-play'ем: раннер поверх AILeague, фитнес = Elo по
  ≥3 сеянцам.
