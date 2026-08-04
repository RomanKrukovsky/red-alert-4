# 01 — Форма API для скриптинга миссий

**Дыра, в которую это бьёт:** кампания. 38 миссий RA4 стали данными
(коммит `08fc128`), но данными только в одном измерении: у миссии есть стартовый
расклад, список целей и условия поражения. Всё, что происходит **в середине**
миссии — подкрепления по таймеру, засада при входе в область, «конвой пошёл,
когда ты взял радар», смена союзника на врага — сейчас в RA4 выразить нечем.
Именно это отделяет «миссия с условием победы» от миссии.

---

## 1. Что делает WZ2100 и почему это правильная форма

У WZ2100 вся кампания и все боты написаны поверх игрового API: движок дёргает
события, скрипт спрашивает мир и выдаёт приказы. Три множества:

- **события** — движок → миссия. `eventGameInit`, `eventStartLevel`,
  `eventStructureBuilt`, `eventDroidBuilt`, `eventAttacked`, `eventDestroyed`,
  `eventObjectSeen`, `eventGroupSeen`, `eventGroupLoss`, `eventResearched`,
  `eventDroidIdle`, `eventPlayerLeft`, `eventTransporterLanded`,
  `eventArea<label>` (юнит вошёл в именованную область), `eventMissionTimeout`;
- **запросы** — миссия → мир, без побочных эффектов. `enumDroid`, `enumStruct`,
  `enumArea`, `enumRange`, `enumGroup`, `countStruct`, `countDroid`,
  `playerPower`, `getObject`, `distBetweenTwoPoints`, `getResearch`,
  `droidCanReach`, `structureIdle`;
- **действия** — миссия → мир, с побочными эффектами. `orderDroid`,
  `orderDroidLoc`, `orderDroidObj`, `orderDroidBuild`, `addDroid`,
  `addStructure`, `setPower`, `setAlliance`, `setMissionTime`, `setNoGoArea`,
  `centreView`, `gameOverMessage`, `enableResearch`, `completeResearch`.

Плюс два механизма склейки, без которых первые три бесполезны:

- **метки (labels)** — точка, область, объект или группа получают имя **на
  карте**, а миссия ссылается на имя. `getObject("StartPosition")`,
  `setNoGoArea(lz.x, lz.y, ...)`, `eventAreaRTLZ`. Это единственное, что
  позволяет автору миссии говорить «здесь» и «эти», не зная координат и id;
- **группы** — `newGroup`, `groupAdd`, `groupAddArea`, `groupSize`,
  `eventGroupLoss`. Отряд как единица разговора: «когда этот отряд уполовинили,
  вызови подкрепление».

Их же библиотека `libcampaign.js` показывает, что поверх этого сразу вырастает
второй слой готовых кубиков: `camSetEnemyBases`, `camSetArtifacts`,
`camAreaEvent`, `camSetStandardWinLossConditions`. То есть сырой API — не то,
чем пишут миссии; им пишут **словарь**, которым пишут миссии.

**Что берём:** множества и склейку (метки, группы, стадии). Форму, не код.

---

## 2. Одно решение принять до всего остального: где живёт логика миссии

WZ2100 исполняет скрипты **внутри симуляции, на всех клиентах**. Их же
ChangeLog фиксирует цену:

> *Move research upgrade code to C++ away from Javascript. May fix a late-game
> desync (#1925)*
>
> *Fix firing of triggerEventPlayerLeft causing a desync (#4333)*

То есть логика в скриптовой VM внутри лок-степа — это документированный
источник рассинхронов у них же. Повторять не будем.

**Решение для RA4:** миссионная логика не исполняется как код внутри
симуляции. Есть ровно два допустимых места:

1. **Чистый предикат над `SimWorld`** — читает состояние, ничего не пишет.
   Так уже устроен `MissionRuntime::EvaluateCondition`, и поэтому миссия
   реплей-безопасна: переигровка того же лога команд даёт тот же исход
   (это проверяет `MissionRuntime.ReplayingAMissionProducesTheSameResult`).
2. **Источник команд наравне с игроком** — если миссия что-то *делает*
   (спавн подкрепления, приказ конвою), она делает это, кладя `Command` в тот
   же кадр, что и игроки, на authority. Ровно так уже сделан ИИ
   (`RA4SimWorldSubsystem::TickSimulation`, ветка authority), и по той же
   причине: тогда матч не начинает дополнительно зависеть от того, детерминирован
   ли сам планировщик.

Что из этого следует практически: **триггеры миссии — это данные, а не
скрипт**. Правило вида «условие → действие», сериализуемое, хешируемое,
проверяемое тестом. Скриптовая VM (Lua/JS) может появиться позже как
*авторский* инструмент, который **компилируется в эти данные** перед запуском
матча, но не как то, что крутится в тике.

---

## 3. Минимальный набор для RA4

Критерий отбора один: чтобы миссия «уничтожь базу, но сначала спаси конвой»
писалась без правок C++. Всё, что для этого не нужно, в первую версию не идёт.

### 3.1. Метки — то, чего нет и без чего остальное бессмысленно

Сейчас `MissionSpawn` задаёт `TileCoord` числом, а условие `ReachLocation`
берёт `TargetTile` + `RadiusTiles`. Это работает, пока миссии пишет программист.
Нужен именованный слой:

```
MissionLabel {
    Id;                                  // "lz_north", "convoy_route_end"
    Kind;                                // Point | Area | EntityTag
    TileCoord Tile;  TileCoord Tile2;    // Point: Tile. Area: прямоугольник.
}
```

и `MissionSpawn` получает `std::string Tag` — так спавн становится адресуемым
(«те три грузовика», а не «сущности 14, 15, 16»). Тег, а не id сущности: id
выдаёт симуляция в рантайме, а миссия должна быть написана до неё.

### 3.2. События — минимум

RA4 не нужен их список из 60 штук. Нужны те, на которые вешается стадийность:

| Событие | Зачем в первой версии |
|---|---|
| `MissionStarted` | стартовые приказы, стартовый таймер |
| `ObjectiveCompleted(Id)` | стадия: конвой спасён → раскрыть цель «уничтожь базу» |
| `ObjectiveFailed(Id)` | ветвление на провале второстепенной цели |
| `EntityDestroyed(Tag \| Def, Owner)` | «убили HQ» — и как поражение, и как триггер |
| `EntityBuilt(Def, Owner)` | «построил радар» |
| `AreaEntered(LabelId, Owner)` | засада, зона высадки, точка эвакуации |
| `TickReached(N)` | подкрепления по расписанию |
| `EntityCountCrossed(Owner, Def, N, Dir)` | «отряд уполовинили» |
| `EntitySpotted(Tag, ByOwner)` | вскрытие базы разведкой |

Девять. Всё остальное из списка WZ2100 (`eventDesign*`, `eventMenu*`,
`eventTransporter*`, `eventBeacon`, `eventChat`) — либо про их конструктор
юнитов, либо про их UI, либо про их транспорты. Не наше.

Важно: `EntitySpotted` требует, чтобы симуляция вообще знала, кто кого видит.
Сейчас `SimWorld::AcquireTarget` (`SimWorld.cpp:2180`) ищет цель по
`max(VisionRange, MaxRange)` и **не спрашивает fog grid** — юниты
захватывают цель сквозь туман. Это же одна из предпосылок наводчиков
(см. [03](03_COMBAT_AND_SYSTEMS_MECHANICS.md#1)).

### 3.3. Запросы — минимум

Почти всё уже есть внутри `ObjectiveConditionType`; нужно вынести наружу как
общий предикат, а не только как условие цели:

`CountOf(Owner, Def) → int`, `CountInArea(LabelId, Owner, Def) → int`,
`CreditsOf(Owner) → int`, `IsAlive(Tag) → bool`,
`TicksSinceStart() → TickIndex`, `IsVisibleTo(Tag, Owner) → bool`.

Шесть. Из них пять — переформулировка существующих условий, шестой требует
fog-запроса, которого нет.

### 3.4. Действия — минимум

Все они обязаны выражаться в существующем `CommandType` либо в явно
добавленном «мастерском» наборе. Первая версия:

| Действие | Как исполняется |
|---|---|
| `Spawn(Def, Owner, LabelId, Tag)` | мастерская команда на authority |
| `Order(Tag, Move \| AttackMove \| Attack, LabelId)` | существующий `CommandType` |
| `RevealObjective(Id)` | уже есть: `MissionRuntime::RevealObjective` |
| `CompleteObjective(Id)` / `FailObjective(Id)` | принудительная стадия |
| `SetOwner(Tag, Owner)` | предательство союзника, захват |
| `GrantCredits(Owner, Amount)` | сюжетная подачка |
| `EndMission(Won \| Lost)` | сюжетный финал вне списка целей |

### 3.5. Правило

```
MissionTrigger {
    Id;
    Event;                       // из 3.2, с параметрами
    std::vector<ObjectiveCondition> Guards;   // И-условия, переиспользуем что есть
    std::vector<MissionAction> Actions;       // из 3.4
    bool bOnce = true;           // одноразовый по умолчанию
}
```

`CampaignMissionDef` получает `std::vector<MissionTrigger> Triggers`.
`MissionRuntime::Evaluate` после проверки целей проходит триггеры, и те
действия, которые меняют мир, уходят командами, а не прямой записью в
`SimWorld`. Инвариант «предикаты не пишут в мир» сохраняется буквально:
предикат решает, что сработало; запись делает командная шина.

---

## 4. Проверка: «уничтожь базу, но сначала спаси конвой»

Как это выглядит целиком, без единой строки C++:

```
Labels:
  convoy_start (Point)        convoy_end (Area)      ambush_zone (Area)
  enemy_base   (Area)

Spawns:
  3 × transport  Owner=ally  at convoy_start  Tag="convoy"
  … база противника, Owner=enemy

Objectives:
  obj_convoy   Primary  Active  ->  EntityCountAtLeast(ally, transport, 1)  … и
  obj_base     Primary  Hidden  ->  EntityCountAtMost(enemy, MCV, 0)

FailureConditions:
  EntityCountAtMost(ally, transport, 0)      // конвой вырезали целиком
  EntityCountAtMost(player, <всё>, 0)        // игрока вырезали

Triggers:
  t_start      on MissionStarted
               -> Order("convoy", Move, convoy_end)
  t_ambush     on AreaEntered(ambush_zone, ally)
               -> Spawn(enemy_tank ×4, enemy, ambush_zone, "ambush")
                  Order("ambush", AttackMove, convoy_start)
  t_arrived    on AreaEntered(convoy_end, ally)
               guard CountInArea(convoy_end, ally, transport) >= 2
               -> CompleteObjective(obj_convoy)
                  RevealObjective(obj_base)
                  Spawn(player_tank ×5, player, lz_north, "reinforce")
```

Что здесь сходится с уже существующим RA4:

- скрытая до срока `obj_base` — это `ObjectiveState::Hidden`, который
  `MissionRuntime` уже не оценивает и не считает для победы (тесты
  `HiddenObjectivesDoNotGateVictoryUntilRevealed`,
  `RevealedObjectiveBecomesRequired`);
- «конвой вырезали» как условие поражения, а не как цель — это уже принятое
  разделение `FailureConditions` и `Objectives`;
- «доехали хотя бы двое» — `guard` поверх существующего условия.

Чего не хватает ровно: **меток, тегов на спавнах, девяти событий, одного
fog-запроса и семи действий**. Это и есть объём работы, а не «сделать
скриптовый движок».

---

## 5. Порядок

1. Метки + `Tag` на спавнах. Без этого нельзя написать ни одного триггера.
2. `MissionTrigger` в данных + прогон в `Evaluate`, действия — через командную
   шину. События `MissionStarted`, `TickReached`, `AreaEntered`,
   `EntityDestroyed`, `ObjectiveCompleted` покрывают большинство миссий.
3. Fog-запрос `IsVisibleTo` и событие `EntitySpotted` — вместе с наводчиками
   из [03](03_COMBAT_AND_SYSTEMS_MECHANICS.md), потому что это одна и та же
   недостающая деталь симуляции.
4. Авторский слой поверх — набор готовых конструкторов в духе
   `libcampaign.js`, но на нашей стороне: `Ambush(area, force)`,
   `Escort(tag, from, to)`, `HoldFor(minutes)`. Программист пишет кубики,
   миссии пишутся кубиками.

Пункт 4 — то, ради чего всё остальное. 38 миссий, написанных сырым API,
обойдутся не дешевле 38 миссий на C++.
