# CommandBus Review

Verdict: CHANGES_REQUIRED

## Important findings

1. `CommandBus::DispatchTick` теперь молча глотает команды после окончания матча и не создаёт обязательные `CommandRejected` events.
   - Файл: [Source/RA4Simulation/Private/CommandBus.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Simulation/Private/CommandBus.cpp:37)
   - Строки: `40-42`
   - Почему это проблема:
     - Сейчас есть ранний `return 0`, если `World.GetPhase() != MatchPhase::Running`.
     - Но контракт симуляции другой: `SimWorld::ApplyCommand()` на non-running world обязан вернуть `MatchOver` и эмитить `SimEventType::CommandRejected`.
     - Доказательство в [Source/RA4Simulation/Private/SimWorld.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Simulation/Private/SimWorld.cpp:696): `Reject(...)` эмитит `CommandRejected` на `700-711`, а `Phase != MatchPhase::Running` уходит в `Reject(CommandReject::MatchOver)` на `714-716`.
   - Риск:
     - клиент/лог/диагностика больше не увидят причину, почему поздняя команда не сработала;
     - это ломает правило “не дропать команду молча”.

2. Тесты не прикрывают самый важный регресс по событийному контракту после `MatchOver`.
   - Файл: [Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp:108)
   - Почему это проблема:
     - тест `DispatchTickReturnsZeroWhenMatchIsAlreadyOver` проверяет только `Accepted == 0`, кредиты и очередь;
     - он не проверяет, что для каждой команды появился `CommandRejected` с причиной `MatchOver`.
   - Следствие:
     - текущая баговая реализация проходит тест, хотя теряет важный сигнал для сети/UI/реплея.

## Minor findings

1. В отчёте автора зафиксировано слишком сильное утверждение про phase guard как про “safe fix”, хотя он меняет внешний контракт по rejected events.
   - Файл: [.codex-work/networking-replay-commandbus.md](/Users/romanmolodyko/Documents/red-alert-4/.codex-work/networking-replay-commandbus.md:70)
   - Строки: `70-72`, `141-145`
   - Комментарий:
     - residual risk описан только как риск подсчёта accepted count;
     - пропущен более серьёзный риск: исчезновение `CommandRejected(MatchOver)` при non-running world.

2. Нет теста, который фиксирует поведение при повторном `DispatchTick` для того же буфера до `ClearUpToTick()`.
   - Файл: [Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp:49)
   - Комментарий:
     - двойное pre-apply исправлено для одного вызова `DispatchTick`;
     - но тестами не зафиксировано, должен ли второй `DispatchTick` на том же tick быть запрещён, безопасен или требовать явного `ClearUpToTick()`.
     - Если контракт “caller обязан чистить frame сам”, это надо явно закрепить тестом или комментарием.

## Mandatory tests

1. `DispatchTick` на завершённом матче должен эмитить `CommandRejected` с `MatchOver` для каждой команды в frame.
2. Mixed-frame test:
   - пример: первая команда валидна и завершает матч / меняет фазу;
   - следующая команда в том же frame должна дать `CommandRejected(MatchOver)`;
   - `DispatchTick` должен вернуть точное число accepted commands.
3. Поведение при повторном `DispatchTick` одного и того же buffered tick:
   - либо явно проверяем, что caller обязан сделать `ClearUpToTick()`,
   - либо защищаемся от повторного применения и тестируем это.

