# CommandBus Audit

Дата: 2026-07-30

## Scope

- Изменял только:
  - `Source/RA4Simulation/Private/CommandBus.cpp`
  - `Source/RA4Simulation/Public/RA4Simulation/CommandBus.h`
  - `Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp`
- CI не трогал
- Чужие правки не откатывал
- Работа сделана поверх `HEAD d0d0b85`

## Root Cause

Подтвержденный root cause был в двойном execution path:

- `CommandBus::DispatchTick` вручную вызывал `World.ApplyCommand(Cmd)` для каждого command:
  - `Source/RA4Simulation/Private/CommandBus.cpp:37-49` в старой версии
- затем тот же frame передавался в `World.Tick(&Frame)`:
  - `Source/RA4Simulation/Private/CommandBus.cpp:52-53` в старой версии
- а `SimWorld::SystemApplyCommands` внутри `Tick` снова вызывал `ApplyCommand(Cmd)`:
  - `Source/RA4Simulation/Private/SimWorld.cpp:1185-1198`

Для неидемпотентных команд это давало двойной side effect. Самый простой валидный пример:

- `StartProduction` списывает кредиты и пушит item в production queue:
  - `Source/RA4Simulation/Private/SimWorld.cpp:815-885`

Итог до фикса:

- один `DispatchTick` с валидным `StartProduction`:
  - списывал `1600` вместо `800`
  - клал `2` item в queue вместо `1`

## Red Evidence

После добавления regression test:

```bash
cmake --build build/qa-headless --parallel --target RA4Tests
./build/qa-headless/RA4Tests --filter=DispatchTickAppliesAProductionFrameExactlyOnce
```

Полученный red:

```text
[ FAIL ] CommandBus.DispatchTickAppliesAProductionFrameExactlyOnce     0 ms
         Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp:89: expected F.World.GetPlayer(0).Credits == BeforeCredits - 800 (got 8400 vs 9200)
         Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp:90: expected int32_t(YardState->Queue.size()) == 1 (got 2 vs 1)
```

Это и было прямым доказательством двойного применения frame.

## Fix

Сделан минимальный фикс в одном месте:

- `Source/RA4Simulation/Private/CommandBus.cpp:37-62`

Что изменено:

1. `DispatchTick` больше не вызывает `World.ApplyCommand` вручную.
2. Единственный путь применения frame теперь: `World.Tick(&Frame)`.
3. Return contract сохранён полезным:
   - метод возвращает количество команд, принятых валидацией
   - считается как `frame.Commands.size() - rejected_count`
   - reject считается по новым `CommandRejected` events, появившимся за этот dispatch
4. Добавлен safe guard:
   - если `World.GetPhase() != MatchPhase::Running`, метод сразу возвращает `0`
   - это закрывает крайний случай, где `Tick` рано выходит и не генерирует reject events

Комментарий в header обновлён:

- `Source/RA4Simulation/Public/RA4Simulation/CommandBus.h:34-36`

## Tests Added

Новые regression tests:

- `CommandBus.DispatchTickAppliesAProductionFrameExactlyOnce`
  - проверяет, что credits списываются один раз и queue size равен 1
- `CommandBus.DispatchTickReturnsZeroForRejectedCommands`
  - проверяет новый/сохранённый return contract на invalid command
- `CommandBus.DispatchTickReturnsZeroWhenMatchIsAlreadyOver`
  - проверяет крайний случай non-running world

Тесты находятся в:

- `Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp:69-130`

## Green Evidence

Focused run после фикса:

```bash
cmake --build build/qa-headless --parallel --target RA4Tests
./build/qa-headless/RA4Tests --filter=CommandBus.DispatchTick
```

Результат:

```text
[ PASS ] CommandBus.DispatchTickAppliesAProductionFrameExactlyOnce     0 ms
[ PASS ] CommandBus.DispatchTickReturnsZeroForRejectedCommands         0 ms
[ PASS ] CommandBus.DispatchTickReturnsZeroWhenMatchIsAlreadyOver      0 ms

3 passed, 0 failed, 1 ms total
```

Полный headless прогон:

```bash
ctest --test-dir build/qa-headless --output-on-failure
```

Результат:

```text
100% tests passed, 0 tests failed out of 4
Total Test time (real) =   3.32 sec
```

## Final State

Исправленные файлы:

- `Source/RA4Simulation/Private/CommandBus.cpp`
- `Source/RA4Simulation/Public/RA4Simulation/CommandBus.h`
- `Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp`

Ключевые строки после фикса:

- `Source/RA4Simulation/Private/CommandBus.cpp:37-62`
- `Source/RA4Simulation/Public/RA4Simulation/CommandBus.h:34-36`
- `Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp:69-130`

## Residual Risk

Низкий, но есть одно допущение:

- accepted count в `DispatchTick` считается через delta новых `CommandRejected` events
- это корректно для текущего `SimWorld`, потому что reject-события создаются в `ApplyCommand`
- если позже появятся другие источники `CommandRejected` вне command validation path, этот подсчёт надо будет пересмотреть
