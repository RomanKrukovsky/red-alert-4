# Kimi Work State

## Baseline (итерация 1)
- Дата: 2026-08-01/02
- Ветка: feature/kimi-skirmish-production (создана из main @ 5f6c977)
- Проект: RedAlert4.uproject, Unreal Engine 5.8
- Unreal MCP: недоступен в этой сессии (нет mcp-инструментов) → использовать Unreal Python / Editor Utility / Editor
- Определённый Python-скрипт для Editor см. в Saved/Reports (будет создаваться по мере надобности)

### Screenshots
- Папка: `SCREENSHOTS/` (а также `RA4_Skirmish.umap` модифицирован в main до ветки — не трогаем)
- Найдено 24 PNG: `1.png` … `24.png`
- Разрешение всех: 1672×941 (~16:9)
- Назначение и элементы — НЕ проанализированы, следующая итерация

### Карты проекта
- Content/Maps/RA4_Skirmish.umap
- RA4_Skirmish_Canyon.umap, RA4_Skirmish_Hills.umap, RA4_Skirmish_Production.umap,
  RA4_Skirmish_VisualIntegration.umap, RA4_ArtLab.umap

### Модули Source/
RA4AI, RA4Campaign, RA4Combat, RA4Content, RA4Core, RA4Editor, RA4FogOfWar,
RA4Input, RA4Navigation, RA4Network, RA4Presentation, RA4Replay, RA4Simulation,
RA4Tests, RA4UI, RAAI. Targets: RedAlert4, Editor, Server.

### Git baseline
- main HEAD: 5f6c977 «feat(code, ui, audio): add camera controls, cheat console, update localizations…»
- Принесённые незакоммиченные изменения в main (перенесены в ветку без изменений):
  `M Content/Maps/RA4_Skirmish.umap`, удаления в build/Compliance corpus, untracked `ra4-ui/`, `KIMI.md` — оставлены как есть, не коммитим.

## Итерации
| # | Задача | Статус | Коммит |
|---|--------|--------|--------|
| 1 | Безопасная подготовка: ветка, WorkState, инвентарь baseline | COMPLETE | (этот коммит) |

| 2 | Инвентаризация screenshots | PARTIAL | (см. ниже) |
| 3 | Задача B: инвентаризация UI-кода и UI-assets | COMPLETE | (этот коммит) |

### Итерация 3 — детали (задача B, ≤8 мин)
- Создан `Saved/Reports/UICodeInventory.json` — валидный JSON, покрывает RA4UI, Content/RA4UI WBP, ra4-ui/.
- **ra4-ui/** — это отдельный git-репозиторий, React-прототип UI (Vite + React 19 + TS), НЕ частью UE-проекта, не старая реализация. Содержит скриншоты 1–24 и экраны StartScreen/MainMenu/CampaignSelect/FactionBriefing/InGameHUD/SkirmishScreen. Ничего не удалено, не скопировано; использовать только как визуальный референс.
- Рантайм-точки создания виджетов: `RA4PlayerController.cpp` BeginPlay (Sidebar, ViewportSubsystem->AddWidget), ShowMatchResult (AddToViewport(100)) + menu-экраны с прямым CreateWidget/AddToViewport (CommandCentre, CampaignSelect, Showcase, SkirmishSetup). Роутер URA4UIRouterSubsystem существует, но обходится — зафиксировано как дефект.
- Существующие экраны: глав. меню, skirmish setup, HUD (9 WBP-вариантов + C++), правая панель (C++ Sidebar с радаром), миникарта (дублируется WBP/C++), выбранный объект (WBP_SelectionPanel), очередь (WBP_Production*), пауза (только WBP_RA4_Pause), победа/поражение (один MatchResultOverlay + WBP_RA4_Victory).
- UI/виджеты не изменены, новые виджеты не созданы.

### Итерация 2 — детали
- Проверено существование всех 24 PNG, зафиксированы размеры (1672×941), пути, размеры файлов → `Saved/Reports/ScreenshotUIInventory.json` (валидный JSON, 24 записи).
- БЛОКЕР: текущая модель не принимает изображения на вход (image input отключён; Read отказывается читать PNG, emitImage отклонён). Субагенты explore тоже без ReadMediaFile. Визуальный контент 1–16.png НЕ проанализирован; 17–24.png — только непроверенная эвристика (не использовать как факт).
- Файл инвентаря честно помечен `analysis_status: pending_vision`. screen/elements/visual = null.
- Отмечено: в проекте есть незакоммиченный `ra4-ui/` (untracked) — возможно, там уже есть UI-наработки; стоит осмотреть в итерации 3b.

## Следующая итерация (5)
Итерация 5: сверить `DA_RA4_ArtMappings` и фактический runtime-path на тему «юнит -> mesh», особенно для SimWorldSubsystem → EntityActor. Проверить, есть ли runtime-выводы в `DescribeVisualState()` и нет ли расхождений по `ContentMeshRegistry`, `ArtMapping` и `DA_RA4_ArtMappings` после коммита `0535000`. Затем точечно починить найденные mismatch'и (не переписывая UI и не ломая текущий deterministic sim contract).

CONTINUE_COMMAND: «Прочитай Docs/KimiWorkState.md и продолжи с итерации 5: проверить и поправить оставшиеся расхождения art-mapping для юнитов/зданий в RA4, не изменять UI»
