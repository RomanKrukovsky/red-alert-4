# RedAlert4 — Архитектурный аудит

**Дата аудита:** 2026-07-30  
**Область:** Все C++ модули, система сборки, зависимости модулей, гарантии детерминизма

---

## Граф зависимостей модулей (Фактический vs Запланированный)

### Запланированная слоистая архитектура (из `RedAlert4.uproject` + `Build.cs`)
```
Unreal Engine (CoreUObject, Engine, etc.)
    │
    ├── RA4Editor (Editor-only)
    │
    ├── RA4UI (Runtime, зависит от CommonUI, MVVM, EnhancedInput)
    │
    ├── RA4Network (Runtime, зависит от CoreUObject)
    │
    ├── RA4Campaign (Runtime)
    │
    ├── RA4AI (Runtime)
    │
    ├── RA4Presentation (Runtime)
    │
    ├── RA4Input (Runtime, зависит от EnhancedInput)
    │
    ├── RA4FogOfWar (Runtime)
    │
    ├── RA4Navigation (Runtime)
    │
    ├── RA4Combat (Runtime)
    │
    ├── RA4Replay (Runtime)
    │
    ├── RA4Simulation (Runtime, **НЕТ UNREAL DEPS**)
    │
    ├── RA4Content (Runtime, **НЕТ UNREAL DEPS**)
    │
    └── RA4Core (Runtime, **НЕТ UNREAL DEPS**)
```

### Фактические зависимости (из `Build.cs` + include заголовков)

| Модуль | Декларируемые депы | Фактические Unreal депы в заголовках | Нарушения |
|--------|-------------------|--------------------------------------|-----------|
| RA4Core | (нет) | **НЕТ** ✅ | — |
| RA4Content | RA4Core | **НЕТ** ✅ | — |
| RA4Simulation | RA4Core, RA4Content, RA4Navigation | **НЕТ** ✅ | — |
| RA4Navigation | RA4Core | **НЕТ** ✅ | — |
| RA4Replay | RA4Core, RA4Simulation | **НЕТ** ✅ | — |
| RA4Combat | RA4Core | **НЕТ** ✅ | — |
| RA4FogOfWar | RA4Core | **НЕТ** ✅ | — |
| RA4AI | RA4Core, RA4Simulation, RA4Navigation | **НЕТ** ✅ | — |
| RA4Network | CoreUObject | Только `WorldSubsystem` ✅ | — |
| RA4Input | EnhancedInput | `EnhancedInputComponent` ✅ | — |
| RA4Presentation | Engine, CoreUObject | `Niagara`, `AnimInstance` — **только в презентации** ✅ | — |
| RA4UI | CommonUI, MVVM, EnhancedInput | `UUserWidget`, `Mvvm` — **только в UI** ✅ | — |
| RA4Editor | UnrealEd, RA4Simulation, RA4Content | `Commandlet` — **только editor** ✅ | — |
| RedAlert4 (main) | Engine, CoreUObject | GameMode, PlayerController, CameraPawn — **клей игры** ✅ | — |

**✅ Нарушений слоистой архитектуры не найдено.** Ядро симуляции (RA4Core, RA4Content, RA4Simulation, RA4Navigation, RA4Replay, RA4Combat, RA4FogOfWar) имеет **ноль Unreal Engine заголовочных зависимостей**. Это настоящее достижение.

---

## Проверка на циклические зависимости

**Не обнаружено.** Граф зависимостей модулей — чистый DAG. Проверено:
- CMake `target_link_libraries` порядок собирается корректно
- Никакие хедеры не текут вверх от симуляции к презентации
- `RA4Simulation` инклюдит `RA4Navigation` заголовки, но не наоборот

---

## Гарантии детерминизма — Уровень архитектуры

### ✅ Гарантировано по дизайну
| Механизм | Местоположение | Верификация |
|----------|----------------|-------------|
| Фиксированная точка (48.16) | `RA4Core/Fixed.h` | `__int128` widening + переносимый фоллбэк |
| Generation-handled EntityIds | `RA4Core/Ids.h:13-31` | Переиспользование слота не может перенацелить устаревшие приказы |
| Упорядоченный `std::map` для кадров CommandBus | `CommandBus.h:45` | Детерминированная итерация vs `unordered_map` |
| Фиксированный порядок систем тика | `SimWorld.h:145-158` | 14 систем, никакой динамической регистрации |
| RNG на симуляцию, с сидом | `SimWorld.h:137`, `Random.h` | `Xoshiro256++`, воспроизводимый |
| Контрольная сумма исключает кэши | `SimWorld.h:128` | События, кэш flow field исключены |
| Версионированный формат реплея | `Replay.h:22` | `kReplayFormatVersion=1`, magic `0x34414952` |

### ⚠️ Риски для детерминизма
| Риск | Местоположение | Серьёзность | Митигация |
|------|----------------|-------------|-----------|
| `ContentDatabase` использует `unordered_map` для lookup индексов | `ContentDatabase.h:58-62` | **ВЫСОКАЯ** | `ComputeContentHash()` итерирует `unordered_map` — порядок отличается между libstdc++/libc++. **Хеш контента расходится кроссплатформенно.** |
| `ToDoubleUnsafe()` существует в `Fixed` | `Fixed.h:110` | СРЕДНЯЯ | Назван "Unsafe", только для логирования. Проаудировать все call sites. |
| `NavigationGrid` полная перестройка при каждой постройке | `SimWorld.cpp:399-414` | СРЕДНЯЯ | O(WH) на постройку. Использовать dirty-rect инкрементальное обновление. |
| `FlowFieldCache` LRU eviction использует `AccessSerial` | `SimWorld.cpp:526-536` | НИЗКАЯ | Детерминировано если счётчик на тик. Проверено. |
| `std::sort` в `RefreshPlayerTech` | `SimWorld.cpp:612` | НИЗКАЯ | Сортирует `ContentId` (uint32) — стабильно кроссплатформенно. |

### ❌ Недетерминизм хеша контента — **Критический блокер для кроссплатформенного локстепа**
```cpp
// ContentDatabase.h:58-62
std::unordered_map<uint32_t, size_t> EntityIndex;
std::unordered_map<uint32_t, size_t> WeaponIndex;
// ...
uint64_t ComputeContentHash() const {
    // Итерирует unordered_map → ПОРЯДОК НЕОПРЕДЕЛЁН → ХЕШ РАСХОДИТСЯ
}
```
**Необходимо исправление:** Заменить на `std::map` или сортировать ключи перед хешированием. Это ломает кроссплатформенный мультиплеер и верификацию реплеев на неидентичных стандартных библиотеках.

---

## Инвентарь технического долга

### Критический (Блокирует шиппинг или вызывает скрытые баги корректности)

| ID | Местоположение | Проблема | Усилия |
|----|----------------|----------|--------|
| ARCH-001 | `ContentDatabase.h:58-62` | `unordered_map` порядок итерации недетерминирован → хеш контента расходится | S (1 день) |
| ARCH-002 | `DefaultContent.cpp` | Только 2/4 фракции, 10/78 юнитов, матрица урона 7/64 записи | L (недели — контент) |
| ARCH-003 | `BibleContentLoader.cpp` | Ожидает `RA4_Bible_Normalized.json` — **файл отсутствует в репозитории** | M (1 неделя пайплайн) |
| ARCH-004 | `SimWorld.cpp:399` | Полная перестройка навигационного грида при каждой постройке | M (2 дня) |

### Мажорный (Деградирует поддерживаемость/производительность)

| ID | Местоположение | Проблема | Усилия |
|----|----------------|----------|--------|
| ARCH-005 | `SimWorld.cpp:551-549` | Кэш flow field ограничен 64 записями, LRU вытеснение | S (тюнинг) |
| ARCH-006 | `CommandBus.h:45` | `std::map<TickIndex, CommandFrame>` — O(log N) на кадр, допустимо но могло быть ring buffer | S |
| ARCH-007 | `SimWorld.h:236` | `kMaxCommandsPerPlayerPerTick = 64` захардкожено — должно быть конфигом | S |
| ARCH-008 | `SimConfig.h` | `kMaxEntities` невидим — бюджет энтити непрозрачен | S |
| ARCH-009 | `DefaultContent.cpp:45` | Структура FactionSetup дублирует данные для Советского/Альянса — не data-driven | M (рефактор в Data Assets) |

### Минорный (Качество кода)

| ID | Местоположение | Проблема |
|----|----------------|----------|
| ARCH-010 | `Fixed.h:142-143` | `FxSin`/`FxCos` объявлены `RA4CORE_API` но `.cpp` не виден — проверить линковку |
| ARCH-011 | `Ids.h:59-68` | `HashName` FNV-1a — хорошо, но `constexpr` работает только для литералов |
| ARCH-012 | `Command.h:71-87` | `Serialize`/`Deserialize` ручные — рассмотреть генерацию сериализации |

---

## Оценка системы сборки

### CMake конфигурация (`build/CMakeCache.txt`)
- **Генератор:** Ninja
- **Типы сборки:** Debug, Development, Shipping (вывод)
- **Санитайзеры:** ASan сборки существуют (`build/asan/`, `build/hb-asan/`)
- **Тесты:** CTest включён (`build/Testing/`), `RA4Tests` исполняемый зарегистрирован

### Интеграция с Unreal Build Tool
- Модули компилируются как **статические библиотеки** (`libRA4Core.a` и т.д.) через CMake
- **UBT не используется** — это кастомная CMake сборка, имитирующая структуру UBT модулей
- **Риск:** Unreal плагины (GameplayAbilities, CommonUI, MVVM, EnhancedInput) объявлены в `.uproject` но **не линкуются в CMake**. Editor сборка упадёт без них.

### Отсутствующая верификация сборки
| Проверка | Статус |
|----------|--------|
| Editor сборки (`RedAlert4Editor`) | ❌ Не тестировалось |
| Shipping сборка (оптимизации, без ассертов) | ❌ Не тестировалось |
| Линковка плагинов (GameplayAbilities, CommonUI, MVVM) | ❌ Нет в CMake |
| Cooking / паковка | ❌ Не тестировалось |
| iOS / Android / Linux таргеты | ❌ Не настроены |

---

## Безопасность и Supply Chain

| Проверка | Результат |
|----------|-----------|
| Хардкодные секреты в коде | ✅ Не найдено |
| Сторонние зависимости | Минимальны: только Unreal Engine + STL |
| Источник сида `Random.h` | Детерминированный (явный сид) — **не криптографический** |
| Проверка границ сериализации | `ByteReader::HasError()` проверяется в `CommandFrame::Deserialize` ✅ |
| Rate limiting команд | 64 команды/игрок/тик принудительно ✅ |

---

## Рекомендации (Архитектура)

1. **НЕМЕДЛЕННО:** Исправить хеш контента `ContentDatabase` — заменить `unordered_map` на `std::map` или отсортированный вектор до любого кроссплатформенного тестирования.
2. **НЕМЕДЛЕННО:** Добавить `RA4_Bible_Normalized.json` в репозиторий или задокументировать пайплайн его генерации из маркдауна.
3. **КРАТКОСРОЧНО:** Мигрировать `DefaultContent.cpp` → Data Assets (Primary Data Assets на юнит/здание). Паттерн `FactionSetup` доказывает, что data-driven дизайн заложен.
4. **КРАТКОСРОЧНО:** Инкрементальные обновления навигационного грида (dirty rects) — 500+ энтити застанет на полной перестройке.
5. **СРЕДНЕСРОЧНО:** Перейти на UBT для Unreal модулей, оставить CMake только для headless симуляционных либ. Гибридная сборка хрупка.
6. **СРЕДНЕСРОЧНО:** Добавить вызов `ContentDatabase::Validate()` в `SimWorld::Initialize()` — ловить авторские ошибки на старте матча.
7. **ДОЛГОСРОЧНО:** Миграция на ECS архетYPES (текущий SoA фиксированной схемы). Текущий дизайн поддерживает ~20 компонентов; добавление фракционно-уникальных компонентов потребует роста массивов.

---

*Конец архитектурного аудита*