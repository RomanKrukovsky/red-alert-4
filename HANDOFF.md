# RA4 — Project Handoff

---

You are the Lead C++ / Unreal Engine 5.6 Developer for the RTS project "Red Alert 4" (internal working title, Clean-Room profile — original names, zero EA content).
Project: `/Users/romanmolodyko/Documents/red-alert-4`.

## Core Architectural Rule

The simulation is 100% deterministic and **completely engine-independent**. Modules `RA4Core`, `RA4Content`, `RA4Simulation`, `RA4Navigation`, `RA4Input`, `RA4Presentation`, and `RA4Replay` compile with standard Clang in ~2 seconds and are covered by unit tests. Only `RedAlert4` and `RA4UI` depend on Unreal Engine / UMG.

No `float` types are used in the simulation — fixed-point math 48.16 (`RA4Core/Fixed.h`) and integer CORDIC are used instead of `libm`. State mutates **exclusively** through `SimWorld::ApplyCommand`, which validates ownership, cost, tech prerequisites, placement, target, and rate limits.

## Verification Workflow

```bash
cd /Users/romanmolodyko/Documents/red-alert-4

# Headless Core test suite (seconds)
cmake -S Tools/HeadlessBuild -B build/hb && cmake --build build/hb -j8
./build/hb/RA4Tests                    # 245 passed, 0 failed (verified 2026-08-04)
```

## Implemented & Verified Systems

| Subsystem | State |
| --- | --- |
| Deterministic simulation, 20 Hz tick rate, fixed system order | Working |
| Commands + full server validation + rate limits | Working |
| Economy: Harvester mining, credit accumulation, energy degradation | Working |
| Production: Queues, payments, cancellations with refunds, rally points | Working |
| Combat: Armor/warhead matrix, projectiles, splash damage, turret rotation | Working |
| Navigation: NavGrid, Portals, FlowField, ReservationGrid, MNavRouter, Formations | Working |
| Input: Camera, box selection, contextual right-click, control groups | Working |
| HUD Data (`RA4Presentation/HudSnapshot`) | Working |
| Replays: Recording, playback, checksum verification | Working |
| **Map `/Game/Maps/RA4_Skirmish_Production`** | **Created and verified** |

Determinism confirmed: identical checksum across builds with `-O3` and `-O0 + ASan + UBSan`.

## Task 1 — AI Commander Profiles

`Source/RA4AI/` contains AI commanders (`AICommander`, `AIStrategy`, `AIDoctrine`) implementing economic, defensive, aggressive, and adaptive strategies.

- Engine-free module depending on `RA4Simulation` read-only.
- Generates `std::vector<Command>` validated by server rules.
- Covered by unit tests in `build/hb/RA4Tests`.

## Task 2 — Unreal Presentation Layer (PIE)

Open the project in Unreal Editor, press Play (PIE) and verify:
- WASD camera navigation and edge scrolling, mouse wheel zoom.
- Left-click unit selection, drag-box group selection.
- Right-click ground for movement, right-click enemy for attack.
- Sidebar MVVM HUD with production queues and resource display.

## Design Bible

`RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` contains full specs for 4 factions, economy, ~78 units, damage matrix, and voice lines.

## Documentation Reference

`Docs/Architecture.md`, `Docs/Roadmap.md`, `Docs/ADR/0001..0011`, `Docs/Skirmish_Production_Readiness_Report.md`.
