# QA / Debug / Perf Audit

Дата аудита: 2026-07-30

## Scope

- Репозиторий: `/Users/romanmolodyko/Documents/red-alert-4`
- Режим: только диагностика, без правок исходников
- Проверено:
  - реальные headless-команды и актуальные clean build'ы
  - CI workflow
  - sanitizer setup
  - determinism / replay coverage
  - `AI.WoundedUnitRetreatsToBase`
  - Unreal automation / functional coverage
  - stress / benchmark coverage
  - слабые content tests и устаревшие отчеты

## Важное про инструкции

- Корневой `AGENTS.md` в самом репозитории не найден. Поиск `find .. -name AGENTS.md -maxdepth 3` вернул только соседние проекты.
- Использованы инструкции из промпта и маршрут `systematic-debugging`.

## Что реально запускалось

### Clean release build

```bash
cmake -S Tools/HeadlessBuild -B build/qa-headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/qa-headless --parallel
ctest --test-dir build/qa-headless --output-on-failure
./build/qa-headless/RA4Tests --filter=WoundedUnitRetreatsToBase
./build/qa-headless/RA4Tests --list | wc -l
```

Факт:

- `ctest` в clean release build: `4/4` passed
- `RA4Tests --list`: `208` тестов
- `AI.WoundedUnitRetreatsToBase`: PASS

### Clean ASan/UBSan build

```bash
cmake -S Tools/HeadlessBuild -B build/qa-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build/qa-asan --parallel
./build/qa-asan/RA4Tests --filter=WoundedUnitRetreatsToBase
./build/qa-asan/RA4Tests --list | rg 'AI\.WoundedUnitRetreatsToBase'
```

Факт:

- тест присутствует и в clean sanitized build
- `AI.WoundedUnitRetreatsToBase`: PASS под ASan/UBSan

### Доп. runtime probe по retreat

Одноразовый внешний probe без правок репозитория показал:

- раненный юнит стартует в `(5000,5000)`
- первый retreat command выходит на 10-м decision tick
- цель retreat-команды: `(2300,2300)` — позиция своей базы
- юнит реально начинает двигаться назад, а не только пишет лог

Это значит: сам behavior сейчас работает, но покрытие тестом слабое.

## Главные findings

### 1. CI determinism gate сейчас сломан логически

Файлы:

- `.github/workflows/core.yml:30-40`
- `Source/RA4Tests/Private/TestMain.cpp:13-21`

Детали:

- CI вызывает `./build/RA4Tests --gtest_filter=VerticalSlice.FullMatch` и Windows-вариант с тем же флагом.
- Headless runner не понимает `--gtest_filter`; он поддерживает только `--list` и `--filter=`. Это видно в `Source/RA4Tests/Private/TestMain.cpp:13-21`.
- Из-за этого запускается не один vertical-slice тест, а весь бинарник.
- Дальше workflow ищет строку `checksum=` через `grep/findstr`, но текущий тест печатает строку вида `final checksum ...`, а не `checksum=`. Поэтому шаг падает в fallback `checksum=match_completed` из `.github/workflows/core.yml:33` и `.github/workflows/core.yml:40`.
- Итог: job `compare-checksums` чаще всего сравнивает одинаковый fallback, а не реальный checksum.

Риск:

- высокий
- cross-platform determinism gate может быть зелёным даже если checksum extraction фактически не работал

Минимальная рекомендация:

- заменить `--gtest_filter` на `--filter=VerticalSlice.FullMatch`
- извлекать реальный checksum по текущему формату вывода или печатать отдельную стабильную строку только для CI
- добавить проверку, что artifact не равен fallback-строке

### 2. `AI.WoundedUnitRetreatsToBase` не воспроизводит баг; это слабый acceptance test

Файлы:

- `Source/RA4Tests/Private/TestAI.cpp:582-608`
- `Source/RA4AI/Private/AICommander.cpp:737-774`
- `Source/RA4AI/Private/AICommander.cpp:909-968`

Подтвержденное:

- clean release и clean ASan/UBSan build проходят этот тест
- `AICommander::CommandArmy()` реально создает `Move` на базу для раненого armed unit
- одноразовый probe показал, что юнит не только пишет decision log, но и начинает движение к базе

Root cause текущей путаницы:

- тест проверяет только наличие записи в `DecisionLog` с reason containing `wounded`
- тест не проверяет:
  - что команда ушла именно к своей базе
  - что у конкретного раненного юнита появилась нужная очередь приказов
  - что он реально уменьшил расстояние до базы
  - что он не получил противоречащий приказ в том же decision tick
  - что retreat завершается или хотя бы продолжается после нескольких тиков

Отдельный риск:

- `AICommander::Tick()` сначала делает `CommandArmy()`, потом может добавить еще команды стратегии (`Source/RA4AI/Private/AICommander.cpp:950-963`). Тест этого не контролирует.

Риск:

- средний
- тест зеленый, но не защищает от регрессии поведения

Минимальная рекомендация:

- заменить лог-ориентированную проверку на state-based:
  - проверить `Orders(TankId)->Count > 0`
  - проверить destination около своей базы
  - проверить, что distance-to-base уменьшается после N ticks
  - failure path: если у юнита уже есть order queue, retreat не должен бесконечно спамиться

### 3. Stress/perf покрытие есть только по названию, но слабое по сути

Файлы:

- `Source/RA4Tests/Private/TestProvingGround.cpp:16-93`
- `Docs/content/PERFORMANCE_REPORT.md:5-18`

Детали:

- `HeadlessStressScenario500Entities` спавнит 500 юнитов, но двигает только `HandlesP0[0]` и `HandlesP1[0]`.
- Остальные 498 сущностей просто стоят.
- Тест не меряет wall-clock, не сравнивает budget, не проверяет массовое pathfinding contention, не проверяет multi-order storm, не проверяет массовый combat.
- `Docs/content/PERFORMANCE_REPORT.md:7-12` содержит цифры до 2000 юнитов, но в текущем test code нет воспроизводимого benchmark harness, который эти цифры генерирует.

Риск:

- высокий для perf-уверенности
- доки создают ощущение покрытия, которого в коде сейчас нет

Минимальная рекомендация:

- добавить отдельные headless perf/stress tests:
  - 300-500 юнитов с одним rally point
  - 300-500 юнитов с разными destination
  - 500+ юнитов в массовом бою
  - perf probe с wall-clock и верхним budget
  - soak test на 10k-100k ticks с checksum snapshot intervals

### 4. Unreal automation / functional coverage фактически отсутствует

Файлы:

- `Source/RA4Tests/Private/TestFramework.h:3-5`
- `Source/RedAlert4/Private/RA4SimWorldSubsystem.cpp:115-125`

Подтвержденное:

- комментарий в `TestFramework.h` ссылается на `RA4Tests/Private/AutomationBridge.cpp`, но такого файла в `Source/RA4Tests` сейчас нет
- поиск по `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, `BEGIN_DEFINE_SPEC`, `FunctionalTest`, `AutomationBridge` в `Source/` ничего реального не дал
- `URA4SimWorldSubsystem::TickSimulation()` только собирает `PendingCommands` и тикает `SimWorld`; вызовов `AICommander` в UE-слое не найдено

Что это значит:

- headless suite полезна, но она не доказывает UE integration path
- нет automation coverage на:
  - загрузку карты / subsystems
  - UI snapshot wiring
  - player controller → command enqueue path
  - in-editor AI behavior

Риск:

- высокий на integration edge

Минимальная рекомендация:

- добавить хотя бы smoke automation:
  - boot map
  - subsystem creation
  - one command from player controller до `SimWorld`
  - one AI-enabled skirmish smoke, если AI должен жить в UE runtime

### 5. Content coverage частично сильная, но рядом много ложной уверенности

Файлы:

- `Source/RA4Tests/Private/TestBibleContent.cpp:57-96`
- `Source/RA4Tests/Private/TestBibleContent.cpp:115-136`
- `Source/RA4Tests/Private/TestBibleImport.cpp:220-234`
- `Source/RA4Tests/Private/TestBibleImport.cpp:236-269`
- `Docs/content/TEST_REPORT.md:7-10`
- `Docs/content/CURRENT_IMPLEMENTATION_AUDIT.md:7`

Детали:

- хорошие части:
  - normalized JSON реально грузится
  - 78 unit IDs реально проверяются
  - несколько matrix / veterancy / faction resource checks есть
- слабые части:
  - `Verify78UniqueUnitsInManifest` по сути ищет строки в JSON и магическое число `78`, а не валидирует структуру/семантику
  - `VerifyVoiceManifestContains624Events` считает строки и принимает `>= 397`, хотя название обещает 624
  - `BuildingsHavePowerValues` только печатает `[NO POWER]`, но не падает на bad building; проверяет лишь `BuildingCount >= 35`
  - `IdempotentReloadDoesNotDuplicate` не проверяет идемпотентность строго; допускает `>= 78`
- документация уже отстала:
  - `Docs/content/TEST_REPORT.md:7-10` пишет `240` total
  - clean build сейчас показывает `208`
  - `Docs/content/CURRENT_IMPLEMENTATION_AUDIT.md:7` пишет `182+ tests pass`

Риск:

- средний
- реальный quality gate слабее, чем выглядит по отчетам

Минимальная рекомендация:

- сделать тесты fail-fast по точным значениям
- заменить string-search на structured assertions по parsed content
- синхронизировать markdown-отчеты с реальным clean build

## Determinism / replay coverage: что реально есть

Сильные стороны:

- replay module присутствует и реально тестируется:
  - `Source/RA4Tests/Private/TestVerticalSlice.cpp` содержит replay serialization / playback / checksum tests
- `AI.IsDeterministic` есть:
  - `Source/RA4Tests/Private/TestAI.cpp:465-493`
- clean release suite: `208/208`
- clean sanitized targeted run на wounded retreat проходит

Пробелы:

- CI checksum extraction bug ломает ценность cross-platform verify
- нет длительного soak/replay drift test на большие tick budgets
- нет dedicated desync report validation на first divergent subsystem dump

## Sanitizers

Файлы:

- `.github/workflows/core.yml:85-98`

Что хорошо:

- ASan+UBSan реально настроены в CI
- clean local sanitized build собрался и targeted AI test прошел

Что слабо:

- в workflow sanitizer job гоняет только `./build-san/RA4Tests`
- нет отдельного `ctest` по дополнительным бинарям `RA4InputTests`, `RA4PresentationTests`, `RA4AITests`
- нет TSan
- нет sanitizer run для UE integration path

Рекомендация:

- хотя бы добавить `ctest --test-dir build-san --output-on-failure`
- явно прогонять все зарегистрированные headless binaries

## Рекомендуемые новые тесты

### Для `AI.WoundedUnitRetreatsToBase`

1. `AI.WoundedUnitRetreatOrderTargetsOwnYard`
   - проверить точную цель приказа, а не просто лог
2. `AI.WoundedUnitActuallyReducesDistanceToBase`
   - после 20-50 ticks distance до базы должен уменьшаться
3. `AI.WoundedUnitWithExistingOrdersDoesNotSpamRetreatEachDecisionTick`
   - failure path на уже занятую очередь
4. `AI.NonCombatOrHarvesterUnitDoesNotUseCombatRetreat`
   - integration edge по фильтрам unit type

### Для CI determinism

1. `VerticalSlice.CIStableChecksumLine`
   - отдельный machine-readable output
2. `Replay.LongSoakChecksumSnapshots`
   - 10k+ ticks, несколько checkpoint intervals
3. `Replay.CrossBuildChecksumParity`
   - release vs ASan/UBSan artifact compare без fallback

### Для stress/perf

1. `Navigation.ThreeHundredUnitsOneRallyStress`
2. `Navigation.ThreeHundredUnitsDistinctDestinationsPerf`
3. `Combat.FiveHundredUnitsMassBattleBudget`
4. `Simulation.Soak100kTicksNoChecksumDrift`

### Для content

1. strict exact count на voice manifest
2. strict exact idempotent reload count
3. every building must have valid power semantics with failing assertion
4. parsed JSON schema assertions instead of substring search

## Итог

### Confirmed

- текущий baseline clean build: `208/208`
- `AI.WoundedUnitRetreatsToBase` сейчас **не падает** и behavior **реально работает**
- главный реальный дефект сейчас не в самом retreat, а в test/CI confidence:
  - сломан checksum extraction в CI
  - слабый retreat acceptance test
  - слабый stress/perf harness
  - отсутствует Unreal automation coverage

### Residual risk

- высокий для CI determinism signal
- высокий для UE integration path
- средний для AI regression detection
- средний для content confidence

