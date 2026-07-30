# Strategic AI Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the fixed-priority computer opponent with a deterministic Utility AI that selects and explains economy, technology, defence, army, assault and recovery strategies.

**Architecture:** `AICommander` remains the engine-free command producer. A new pure `AIStrategy` unit owns integer scoring and hysteresis, while `AICommander` builds a deterministic world assessment, maps the selected strategy to existing production and army actions, and records bounded diagnostics.

**Tech Stack:** C++17 headless core, Unreal Engine 5.6 module rules, CMake, custom `RA4_TEST` harness, fixed-tick deterministic simulation.

## Global Constraints

- The authoritative simulation and AI remain free of Unreal types.
- AI state changes enter `SimWorld` only through ordinary `Command` values.
- Strategy scores are integers in the inclusive range 0–1000.
- Decision order and tie-breaking are stable; wall time, floating point and unordered iteration must not affect results.
- Marketplace plugins, HTN, fog filtering, scouting, influence maps, squads, StateTree, MassEntity and Learning Agents are out of scope.
- Preserve unrelated changes already present in the working tree.
- Keep includes at the top of each module.

---

## File map

- Create `Source/RA4AI/Public/RA4AI/AIStrategy.h`: strategy enum, assessment values, score result and pure selection API.
- Create `Source/RA4AI/Private/AIStrategy.cpp`: integer utility formulas, stable winner selection and hysteresis.
- Modify `Source/RA4AI/Public/RA4AI/AICommander.h`: use strategy types, add active-strategy state and richer diagnostics.
- Modify `Source/RA4AI/Private/AICommander.cpp`: build the assessment, remember recent damage and dispatch actions by selected strategy.
- Modify `Source/RA4Tests/Private/TestAI.cpp`: pure strategy tests, diagnostics tests and end-to-end regression coverage.
- Modify `Tools/HeadlessBuild/CMakeLists.txt`: compile `AIStrategy.cpp` into the headless AI library.
- Modify `Docs/Roadmap.md`: report the verified AI milestone and test evidence.

---

### Task 1: Restore the AI baseline

**Files:**
- Modify: `Source/RA4AI/Private/AICommander.cpp`
- Test: `Source/RA4Tests/Private/TestAI.cpp`

**Interfaces:**
- Consumes: existing `RA4::AI::AICommander`.
- Produces: a cleanly compiling `RA4AITests` baseline with unchanged behaviour.

- [ ] **Step 1: Reproduce the baseline failure**

Run:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless --target RA4AITests -j8
```

Expected: compilation fails because the local `IsConstructionYard` helper is unused.

- [ ] **Step 2: Remove only the unused helper**

Delete this line from `Source/RA4AI/Private/AICommander.cpp`:

```cpp
bool IsConstructionYard(const EntityDef& D) { return D.Kind == EntityKind::Building && D.Building.bIsConstructionYard; }
```

- [ ] **Step 3: Verify current AI behaviour**

Run:

```bash
cmake --build build/headless --target RA4AITests -j8
./build/headless/RA4AITests --filter=AI
```

Expected: all existing `AI` tests pass. If an existing behaviour test fails, fix that baseline defect before beginning Task 2 and do not weaken the assertion.

- [ ] **Step 4: Commit the baseline fix**

```bash
git add Source/RA4AI/Private/AICommander.cpp
git commit -m "fix(ai): restore focused test build"
```

---

### Task 2: Add pure utility scoring

**Files:**
- Create: `Source/RA4AI/Public/RA4AI/AIStrategy.h`
- Create: `Source/RA4AI/Private/AIStrategy.cpp`
- Modify: `Source/RA4AI/Public/RA4AI/AICommander.h`
- Modify: `Source/RA4AI/Private/AICommander.cpp`
- Modify: `Source/RA4Tests/Private/TestAI.cpp`
- Modify: `Tools/HeadlessBuild/CMakeLists.txt`

**Interfaces:**
- Consumes: `AIProfile` and `AIConfig` currently declared in `AICommander.h`.
- Produces:
  - `enum class AIStrategy : uint8_t`
  - `struct AIWorldAssessment`
  - `struct AIStrategyScore`
  - `std::vector<AIStrategyScore> ScoreStrategies(const AIWorldAssessment&, const AIConfig&)`
  - `AIStrategy SelectStrategy(const std::vector<AIStrategyScore>&, AIStrategy, bool, const AIConfig&)`
  - `int32_t RequiredCreditReserve(AIStrategy, const AIConfig&)`
  - `const char* ToString(AIStrategy)`

- [ ] **Step 1: Write failing pure scoring tests**

Add these cases to `Source/RA4Tests/Private/TestAI.cpp`:

```cpp
RA4_TEST(AI, UtilitySelectsEconomyForNewBase)
{
    AIWorldAssessment A;
    A.Credits = 10000;
    A.PowerProduced = 100;
    A.bHasConstructionYard = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::ExpandEconomy);
}

RA4_TEST(AI, UtilitySelectsFortifyUnderAttack)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;
    A.ProductionBuildings = 2;
    A.bUnderAttack = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::Fortify);
}

RA4_TEST(AI, UtilitySelectsAssaultWithReadyArmy)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;
    A.ProductionBuildings = 2;
    A.ArmedUnits = 8;
    A.bHasEnemyTarget = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::Assault);
}

RA4_TEST(AI, UtilitySelectsTechWhenEconomyIsReady)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::TechUp);
}

RA4_TEST(AI, UtilitySelectsArmyWhenProductionIsReady)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;
    A.ProductionBuildings = 2;
    A.Defences = 2;
    A.ArmedUnits = 1;
    A.bHasEnemyTarget = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::AssembleArmy);
}

RA4_TEST(AI, UtilitySelectsRecoveryAfterEconomyLoss)
{
    AIWorldAssessment A;
    A.bHasConstructionYard = true;
    A.TotalHarvested = 2000;
    A.Refineries = 0;
    A.Harvesters = 1;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::Recover);
}
```

- [ ] **Step 2: Run the tests and confirm the new API is missing**

Run:

```bash
cmake --build build/headless --target RA4AITests -j8
```

Expected: compilation fails because `AIWorldAssessment`, `AIStrategyScore`,
`ScoreStrategies` and `FindWinningStrategy` are not defined.

- [ ] **Step 3: Define strategy and assessment types**

Create `Source/RA4AI/Public/RA4AI/AIStrategy.h` with these public values:

```cpp
#pragma once

#include <cstdint>
#include <vector>

namespace RA4::AI
{

struct AIConfig;

enum class AIStrategy : uint8_t
{
    ExpandEconomy = 0,
    TechUp,
    Fortify,
    AssembleArmy,
    Assault,
    Recover,
};

struct AIWorldAssessment
{
    int32_t Credits = 0;
    int32_t PowerProduced = 0;
    int32_t PowerConsumed = 0;
    int32_t TotalHarvested = 0;
    int32_t PowerPlants = 0;
    int32_t Refineries = 0;
    int32_t Harvesters = 0;
    int32_t ProductionBuildings = 0;
    int32_t Defences = 0;
    int32_t ArmedUnits = 0;
    bool bHasConstructionYard = false;
    bool bHasEnemyTarget = false;
    bool bUnderAttack = false;
    bool bAssaultActive = false;
};

struct AIStrategyScore
{
    AIStrategy Strategy = AIStrategy::ExpandEconomy;
    int32_t Score = 0;
    const char* Reason = "";
};

std::vector<AIStrategyScore> ScoreStrategies(const AIWorldAssessment& Assessment,
                                             const AIConfig& Config);
AIStrategy FindWinningStrategy(const std::vector<AIStrategyScore>& Scores);
AIStrategy SelectStrategy(const std::vector<AIStrategyScore>& Scores,
                          AIStrategy CurrentStrategy,
                          bool bHasCurrentStrategy,
                          const AIConfig& Config);
int32_t FindStrategyScore(const std::vector<AIStrategyScore>& Scores, AIStrategy Strategy);
int32_t RequiredCreditReserve(AIStrategy Strategy, const AIConfig& Config);
const char* ToString(AIStrategy Strategy);

} // namespace RA4::AI
```

Move `AIProfile` and `AIConfig` from `AICommander.h` into `AIStrategy.h`. Extend
`AIConfig` with:

```cpp
int32_t StrategySwitchMargin = 100;
int32_t EmergencyStrategyScore = 900;
int32_t UnderAttackMemoryTicks = 100;
int32_t EconomyWeight = 100;
int32_t TechWeight = 100;
int32_t DefenceWeight = 100;
int32_t ArmyWeight = 100;
int32_t AssaultWeight = 100;
int32_t RecoveryWeight = 100;
```

Include `RA4AI/AIStrategy.h` from `AICommander.h`.

- [ ] **Step 4: Implement fixed integer formulas**

Create `Source/RA4AI/Private/AIStrategy.cpp`. Use a local `ClampScore` helper and
return one entry for every strategy in enum order. Implement these base rules:

```cpp
ExpandEconomy = max(
    PowerProduced <= PowerConsumed ? 900 : 0,
    Refineries == 0 ? 850 : 0,
    Harvesters < TargetHarvesters ? 600 + 50 * (TargetHarvesters - Harvesters) : 0);

TechUp = Refineries > 0 && Harvesters >= min(TargetHarvesters, 2)
    ? (ProductionBuildings == 0 ? 700 : 250)
    : 0;

Fortify = bUnderAttack ? 1000
    : (Defences < TargetDefences ? 300 + 50 * (TargetDefences - Defences) : 0);

AssembleArmy = ProductionBuildings > 0 && ArmedUnits < AttackArmySize
    ? 550 + 25 * (AttackArmySize - ArmedUnits)
    : 100;

Assault = bHasEnemyTarget && ArmedUnits >= AttackArmySize
    ? 750 + 20 * (ArmedUnits - AttackArmySize)
    : (bAssaultActive && ArmedUnits >= MinimumAttackSize ? 700 : 0);

Recover = TotalHarvested > 0 && (Refineries == 0 || PowerPlants == 0)
    ? 950
    : 0;
```

Multiply each base score by its profile weight, divide by 100, then clamp to
0–1000. `FindWinningStrategy` must retain the first enum-ordered entry on ties.
Every `ToString(AIStrategy)` case returns a stable English diagnostic name; the
default path returns `"Invalid"`. `RequiredCreditReserve` returns zero for
`AIStrategy::Recover` and `Config.CreditReserve` for every other strategy.

- [ ] **Step 5: Configure profile weights**

Extend `MakeProfileConfig`:

```cpp
Aggressive: AssaultWeight=125, ArmyWeight=115, EconomyWeight=85,
            DefenceWeight=70, StrategySwitchMargin=60.
Defensive:  DefenceWeight=130, ArmyWeight=110, AssaultWeight=80,
            StrategySwitchMargin=120.
Economic:   EconomyWeight=130, TechWeight=110, AssaultWeight=80,
            RecoveryWeight=120, StrategySwitchMargin=120.
Balanced:   all weights remain 100.
```

- [ ] **Step 6: Add the new source to the headless target**

Add this entry to `add_library(RA4AI STATIC ...)` in
`Tools/HeadlessBuild/CMakeLists.txt`:

```cmake
${RA4_SOURCE_ROOT}/RA4AI/Private/AIStrategy.cpp
```

- [ ] **Step 7: Run focused tests**

Run:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless --target RA4AITests -j8
./build/headless/RA4AITests --filter=Utility
```

Expected: all six new utility tests pass.

- [ ] **Step 8: Commit utility scoring**

```bash
git add Source/RA4AI/Public/RA4AI/AIStrategy.h \
  Source/RA4AI/Public/RA4AI/AICommander.h \
  Source/RA4AI/Private/AIStrategy.cpp \
  Source/RA4AI/Private/AICommander.cpp \
  Source/RA4Tests/Private/TestAI.cpp \
  Tools/HeadlessBuild/CMakeLists.txt
git commit -m "feat(ai): add deterministic strategy scoring"
```

---

### Task 3: Add hysteresis and emergency switching

**Files:**
- Modify: `Source/RA4AI/Private/AIStrategy.cpp`
- Modify: `Source/RA4Tests/Private/TestAI.cpp`

**Interfaces:**
- Consumes: `SelectStrategy`, `FindStrategyScore` and the types from Task 2.
- Produces: stable strategy switching with emergency defence and recovery overrides.

- [ ] **Step 1: Write failing hysteresis tests**

Add:

```cpp
RA4_TEST(AI, StrategyHysteresisKeepsCurrentChoiceNearATie)
{
    AIConfig Config;
    Config.StrategySwitchMargin = 100;
    const std::vector<AIStrategyScore> Scores = {
        {AIStrategy::ExpandEconomy, 600, "economy"},
        {AIStrategy::TechUp, 650, "tech"},
    };

    RA4_EXPECT(SelectStrategy(Scores, AIStrategy::ExpandEconomy, true, Config) ==
               AIStrategy::ExpandEconomy);
}

RA4_TEST(AI, EmergencyStrategyOverridesHysteresis)
{
    AIConfig Config;
    Config.StrategySwitchMargin = 200;
    Config.EmergencyStrategyScore = 900;
    const std::vector<AIStrategyScore> Scores = {
        {AIStrategy::AssembleArmy, 850, "army"},
        {AIStrategy::Fortify, 1000, "under attack"},
    };

    RA4_EXPECT(SelectStrategy(Scores, AIStrategy::AssembleArmy, true, Config) ==
               AIStrategy::Fortify);
}
```

- [ ] **Step 2: Verify the tests fail**

Run:

```bash
cmake --build build/headless --target RA4AITests -j8
./build/headless/RA4AITests --filter=Strategy
```

Expected: at least the near-tie test fails before hysteresis is implemented.

- [ ] **Step 3: Implement selection**

In `SelectStrategy`:

1. Return `FindWinningStrategy(Scores)` when there is no active strategy.
2. Return the winner immediately when it is `Fortify` or `Recover` and its score is
   at least `EmergencyStrategyScore`.
3. Keep the current strategy when `winnerScore < currentScore +
   StrategySwitchMargin`.
4. Otherwise return the winner.

If `Scores` is empty, return `CurrentStrategy`.

- [ ] **Step 4: Run all utility and strategy tests**

Run:

```bash
cmake --build build/headless --target RA4AITests -j8
./build/headless/RA4AITests --filter=AI
```

Expected: the focused AI suite passes.

- [ ] **Step 5: Commit hysteresis**

```bash
git add Source/RA4AI/Private/AIStrategy.cpp Source/RA4Tests/Private/TestAI.cpp
git commit -m "feat(ai): stabilize strategy switching"
```

---

### Task 4: Drive `AICommander` through Utility AI

**Files:**
- Modify: `Source/RA4AI/Public/RA4AI/AICommander.h`
- Modify: `Source/RA4AI/Private/AICommander.cpp`
- Modify: `Source/RA4Tests/Private/TestAI.cpp`

**Interfaces:**
- Consumes: `ScoreStrategies`, `SelectStrategy`, `FindStrategyScore` from Tasks 2–3.
- Produces:
  - `AIWorldAssessment AICommander::BuildAssessment(const SimWorld&) const`
  - `bool AICommander::ExecuteStrategy(AIStrategy, const SimWorld&, std::vector<Command>&)`
  - `AIStrategy AICommander::GetActiveStrategy() const`
  - `void AICommander::LogIdleStrategyDecision(TickIndex)`
  - enriched `AIDecision` entries.

- [ ] **Step 1: Write failing commander integration tests**

Add:

```cpp
RA4_TEST(AI, CommanderReportsSelectedStrategyAndScore)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(5));

    const std::vector<AIDecision>& Log = M.Commanders[0].GetDecisionLog();
    RA4_REQUIRE(!Log.empty());
    RA4_EXPECT(Log.back().Strategy == M.Commanders[0].GetActiveStrategy());
    RA4_EXPECT(Log.back().StrategyScore >= 0);
    RA4_EXPECT(Log.back().StrategyScore <= 1000);
    RA4_EXPECT(!Log.back().Reason.empty());
}

RA4_TEST(AI, AggressiveProfileValuesAssaultMoreThanDefensive)
{
    AIWorldAssessment A;
    A.Refineries = 1;
    A.Harvesters = 2;
    A.ProductionBuildings = 2;
    A.ArmedUnits = 6;
    A.bHasEnemyTarget = true;

    const auto AggressiveScores = ScoreStrategies(A, MakeProfileConfig(AIProfile::Aggressive));
    const auto DefensiveScores = ScoreStrategies(A, MakeProfileConfig(AIProfile::Defensive));

    RA4_EXPECT(FindStrategyScore(AggressiveScores, AIStrategy::Assault) >
               FindStrategyScore(DefensiveScores, AIStrategy::Assault));
}
```

- [ ] **Step 2: Verify the new diagnostics API is missing**

Run:

```bash
cmake --build build/headless --target RA4AITests -j8
```

Expected: compilation fails because `AIDecision::Strategy`,
`AIDecision::StrategyScore` and `AICommander::GetActiveStrategy` are missing.

- [ ] **Step 3: Add commander strategy state**

Add to `AICommander`:

```cpp
AIStrategy GetActiveStrategy() const { return ActiveStrategy; }

AIWorldAssessment BuildAssessment(const SimWorld& World) const;
bool ExecuteStrategy(AIStrategy Strategy, const SimWorld& World,
                     std::vector<Command>& Out);

AIStrategy ActiveStrategy = AIStrategy::ExpandEconomy;
bool bHasActiveStrategy = false;
TickIndex LastUnderAttackTick = 0;
bool bHasSeenAttack = false;
AIStrategy PreviousStrategyForDecision = AIStrategy::ExpandEconomy;
int32_t ActiveStrategyScore = 0;
```

Extend `AIDecision`:

```cpp
AIStrategy Strategy = AIStrategy::ExpandEconomy;
int32_t StrategyScore = 0;
AIStrategy PreviousStrategy = AIStrategy::ExpandEconomy;
```

Reset `bHasActiveStrategy`, `LastUnderAttackTick` and `bHasSeenAttack` in
`AICommander::Reset`.

- [ ] **Step 4: Remember damage on every tick**

Before the decision cadence early return in `AICommander::Tick`, inspect current
events. When a `DamageApplied` event targets an owned entity:

```cpp
LastUnderAttackTick = World.GetTick();
bHasSeenAttack = true;
```

`BuildAssessment` sets `bUnderAttack` when:

```cpp
bHasSeenAttack &&
World.GetTick() - LastUnderAttackTick <= TickIndex(Config.UnderAttackMemoryTicks)
```

This prevents the ten-tick decision interval from missing a one-tick damage event.

- [ ] **Step 5: Build one deterministic assessment**

Implement `BuildAssessment` with one index-ordered pass over
`World.GetAllCores()`. Count owned role flags using `ContentDatabase`, count
production buildings with `ProducesCategory`, and set `bHasEnemyTarget` from the
first live enemy building or unit. Copy credits, power and `TotalHarvested` from
`PlayerState`. Set `bAssaultActive` from `bAttacking`.

- [ ] **Step 6: Map strategies to existing actions**

Implement `ExecuteStrategy`:

```cpp
switch (Strategy)
{
    case AIStrategy::ExpandEconomy:
    case AIStrategy::Recover:
        return TryBuildEconomy(World, Out);
    case AIStrategy::TechUp:
        return TryBuildTech(World, Out);
    case AIStrategy::Fortify:
        return TryBuildDefence(World, Out) || TryTrainArmy(World, Out);
    case AIStrategy::AssembleArmy:
        return TryTrainArmy(World, Out);
    case AIStrategy::Assault:
        CommandArmy(World, Out);
        return TryTrainArmy(World, Out);
}
return false;
```

`CommandArmy` remains the only group-order producer and is called only for the
`Assault` strategy. It continues an active assault while the army remains above
`MinimumAttackSize`.

- [ ] **Step 7: Replace the fixed priority chain**

Score the strategy before issuing actions. Store `PreviousStrategyForDecision` and
`ActiveStrategyScore` so every existing action-level `Log` call can copy them into
`AIDecision`. Keep finished structure placement as the mandatory first build
action, then execute the selected strategy:

```cpp
const AIWorldAssessment Assessment = BuildAssessment(World);
const std::vector<AIStrategyScore> Scores = ScoreStrategies(Assessment, Config);
const AIStrategy Previous = ActiveStrategy;
ActiveStrategy = SelectStrategy(Scores, ActiveStrategy, bHasActiveStrategy, Config);
bHasActiveStrategy = true;
PreviousStrategyForDecision = Previous;
ActiveStrategyScore = FindStrategyScore(Scores, ActiveStrategy);

const size_t CommandCountBefore = OutCommands.size();
if (!TryPlaceFinishedStructure(World, OutCommands) &&
    FindOwnConstructionYard(World).IsValid())
{
    ExecuteStrategy(ActiveStrategy, World, OutCommands);
}
if (OutCommands.size() == CommandCountBefore)
{
    LogIdleStrategyDecision(World.GetTick());
}
```

Extend the existing `Log` implementation so each emitted action records
`ActiveStrategy`, `ActiveStrategyScore` and `PreviousStrategyForDecision`.
`LogIdleStrategyDecision` appends a `CommandType::None` entry with reason
`"strategy selected; no valid action"`. This records every selected strategy
without deleting the useful action-specific reasons.

- [ ] **Step 8: Enforce the credit reserve**

In `QueueProduction`, calculate:

```cpp
const int32_t RequiredReserve = RequiredCreditReserve(ActiveStrategy, Config);
if (World.GetPlayer(Player).Credits <
    Def->Production.Cost + RequiredReserve)
{
    return false;
}
```

Add this pure policy test:

```cpp
RA4_TEST(AI, RecoveryMaySpendTheCreditReserve)
{
    AIConfig Config;
    Config.CreditReserve = 400;

    RA4_EXPECT_EQ(RequiredCreditReserve(AIStrategy::AssembleArmy, Config), 400);
    RA4_EXPECT_EQ(RequiredCreditReserve(AIStrategy::ExpandEconomy, Config), 400);
    RA4_EXPECT_EQ(RequiredCreditReserve(AIStrategy::Recover, Config), 0);
}
```

The existing `AI.DoesNotBankruptItselfOnOneDecisionTick` remains the observable
commander-level regression test.

- [ ] **Step 9: Run focused integration tests**

Run:

```bash
cmake --build build/headless --target RA4AITests -j8
./build/headless/RA4AITests --filter=AI
```

Expected: all pure and commander tests pass. The existing economy, build order,
combat and determinism tests must remain green without weakened assertions.

- [ ] **Step 10: Commit commander integration**

```bash
git add Source/RA4AI/Public/RA4AI/AICommander.h \
  Source/RA4AI/Private/AICommander.cpp \
  Source/RA4Tests/Private/TestAI.cpp
git commit -m "feat(ai): select actions through utility strategies"
```

---

### Task 5: Verify matches, determinism and sanitizers

**Files:**
- Modify: `Source/RA4Tests/Private/TestAI.cpp`
- Modify: `Docs/Roadmap.md`

**Interfaces:**
- Consumes: completed Utility AI commander.
- Produces: verified end-to-end behaviour and honest project status.

- [ ] **Step 1: Strengthen decision-sequence determinism**

Extend `AI.IsDeterministic` after the existing checksum checks:

```cpp
const auto& LogA = A.Commanders[0].GetDecisionLog();
const auto& LogB = B.Commanders[0].GetDecisionLog();
RA4_REQUIRE(LogA.size() == LogB.size());
for (size_t I = 0; I < LogA.size(); ++I)
{
    RA4_EXPECT(LogA[I].Tick == LogB[I].Tick);
    RA4_EXPECT(LogA[I].Strategy == LogB[I].Strategy);
    RA4_EXPECT_EQ(LogA[I].StrategyScore, LogB[I].StrategyScore);
    RA4_EXPECT(LogA[I].Command == LogB[I].Command);
    RA4_EXPECT(LogA[I].Reason == LogB[I].Reason);
}
```

- [ ] **Step 2: Run the focused AI match suite**

Run:

```bash
cmake --build build/headless --target RA4AITests -j8
./build/headless/RA4AITests --filter=AI
```

Expected: zero failures; both commanders build, harvest and fight.

- [ ] **Step 3: Run the full release suite**

Run:

```bash
cmake --build build/headless -j8
./build/headless/RA4Tests
```

Expected: zero failures across every suite.

- [ ] **Step 4: Run the sanitizer suite**

Run:

```bash
cmake -S Tools/HeadlessBuild -B build/asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan -j8
./build/asan/RA4Tests
```

Expected: zero test failures and no AddressSanitizer or UndefinedBehaviorSanitizer
report.

- [ ] **Step 5: Update the roadmap with measured evidence**

In `Docs/Roadmap.md`:

- add the deterministic Utility AI, four profiles, hysteresis, diagnostics and
  AI-versus-AI coverage to `Verified working`;
- remove “AI director and its profiles” from `Not started`;
- keep scouting, tactical groups, fog-aware knowledge, HTN and influence maps in
  future sequencing;
- replace the test count only with the exact count printed by the verified full
  suite.

- [ ] **Step 6: Check the final diff**

Run:

```bash
git diff --check
git status --short
git diff -- Source/RA4AI Source/RA4Tests/Private/TestAI.cpp \
  Tools/HeadlessBuild/CMakeLists.txt Docs/Roadmap.md
```

Expected: no whitespace errors and no unrelated files included in the AI change.

- [ ] **Step 7: Commit verification and documentation**

```bash
git add Source/RA4Tests/Private/TestAI.cpp Docs/Roadmap.md
git commit -m "test(ai): verify deterministic utility matches"
```

---

## Final acceptance

Run:

```bash
./build/headless/RA4AITests --filter=AI
./build/headless/RA4Tests
./build/asan/RA4Tests
```

The work is complete only when all three commands exit successfully, the decision
log explains every selected strategy, and `git diff --check` reports no errors.
