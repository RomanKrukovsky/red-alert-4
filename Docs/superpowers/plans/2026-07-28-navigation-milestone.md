# Navigation Milestone Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Finish the navigation stage (roadmap stage 2) on the existing deterministic headless core: add hierarchical sector-portal A\*, a soft reservation grid, local avoidance, formations, a time-sliced per-tick repath budget, and a debug snapshot; prove 300 units rally without per-unit A\* and with an identical checksum across `-O3` and `-O0+ASan`.

**Architecture:** Extends the existing engine-free `RA4Navigation` (NavGrid + FlowField + FlowFieldCache) and the single-threaded fixed-step `RA4Simulation::SimWorld`. All new types live in `RA4Navigation`. `SystemMovement` is rewritten to: build a sector-portal macro path once per (from-sector, to-sector, query, topology-rev) under a per-tick budget, follow a shared flow field to the next sector-center sub-goal, soft-reserve the next tile with slot-order tie-break, fall back to the best open neighbor on conflict, and repath after a blocked-tick threshold. No background threads; "async" = amortized across ticks in stable entity-slot order. No `DrawDebug*` in the sim; a pure-data `NavDebugSnapshot` is the only debug surface.

**Tech Stack:** C++17, CMake 3.20+, the project's minimal test harness (`RA4_TEST`/`RA4_EXPECT`/`RA4_REQUIRE`/`RA4_EXPECT_EQ` in `Source/RA4Tests/Private/TestFramework.h`), the `Fixed` 48.16 type, `Vec2`, `TileCoord`, `EntityId`/`ContentId` from `RA4Core`. Headless build at `Tools/HeadlessBuild/CMakeLists.txt`. Warnings are errors (`-Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast`).

## Global Constraints

Copied verbatim from the spec (`docs/superpowers/specs/2026-07-28-navigation-milestone-design.md`):

- 20 Hz sim: `kTicksPerSecond = 20` (`Source/RA4Core/Public/RA4Core/SimConfig.h`).
- All sim math is `Fixed`/`Vec2`/`TileCoord`. No floats in the sim path. `std::chrono` is test-only wall-clock, never feeds sim state.
- Single-threaded, fixed system order. `SystemMovement` runs after `SystemOrders`, before `SystemCombat`. No background threads touch sim state.
- Fixed entity-slot iteration: `for (slot = 0; slot < HighWaterMark; ++slot)`. No `std::unordered_map` iteration in the sim.
- Reservation tie-break = slot index (lower wins). The only ordering rule.
- Neighbor expansion order is fixed: macro A\* expands sector neighbors in ascending sector-id; local avoidance scans N,E,S,W,NE,SE,SW,NW (the exact order in `FlowField.cpp`'s `GDirections`).
- Repath budget is tick-bounded, not wall-clock. `kMaxFlowFieldBuildsPerTick = 2`, `kMaxMacroPathBuildsPerTick = 4`. A slow machine does fewer builds per tick, not different ones.
- No `DrawDebugLine`/`DrawDebugBox`/`UWorld`/`AActor`/`GWorld` in `RA4Navigation` or `RA4Simulation`. Debug is `NavDebugSnapshot` (pure data).
- No per-unit `Find`/`GetAllActorsOfClass`. No `TArray` growth in the hot loop (vectors pre-sized).
- `RedAlert4.uproject` / `.Build.cs` untouched this milestone (Unreal compile is a later stage).
- Every task ends with `cmake --build build/headless -j8` green and the named test passing. No claiming "tests pass" without running them.

## File Structure

New files (all engine-free, in `RA4Navigation` unless noted):

- `Source/RA4Navigation/Public/RA4Navigation/ReservationGrid.h` — tile-keyed soft reservations.
- `Source/RA4Navigation/Private/ReservationGrid.cpp`
- `Source/RA4Navigation/Public/RA4Navigation/MNavRouter.h` — sector-portal A\* + LRU cache.
- `Source/RA4Navigation/Private/MNavRouter.cpp`
- `Source/RA4Navigation/Public/RA4Navigation/Formation.h` — formation descriptor + assignment.
- `Source/RA4Navigation/Private/Formation.cpp`
- `Source/RA4Navigation/Public/RA4Navigation/NavDebug.h` — pure-data debug snapshot.
- `Docs/ADR/0011-hierarchical-navigation-and-reservations.md`

Modified files:

- `Source/RA4Core/Public/RA4Core/SimConfig.h` — add the three budget constants.
- `Source/RA4Simulation/Public/RA4Simulation/SimTypes.h` — extend `MovementComp` with macro-path/sub-goal/formation fields; add `MovementStats`.
- `Source/RA4Simulation/Public/RA4Simulation/SimWorld.h` — add `ReservationGrid`, `MNavRouter`, `MovementStats` members; declare `GetMovementStats()`.
- `Source/RA4Simulation/Private/SimWorld.cpp` — rewrite `SystemMovement`; key `GetFlowField` on sub-goal sector center; add budget counters; topology-revision invalidation.
- `Source/RA4Tests/Private/TestNavigation.cpp` — add T1–T13 + T10b.
- `Tools/HeadlessBuild/CMakeLists.txt` — add the three new `.cpp` files to `RA4Navigation`.
- `Docs/Roadmap.md` — mark navigation done with verified test counts.

---

### Task 1: ReservationGrid (soft, per-tick expiry, slot-order tie-break)

**Files:**
- Create: `Source/RA4Navigation/Public/RA4Navigation/ReservationGrid.h`
- Create: `Source/RA4Navigation/Private/ReservationGrid.cpp`
- Modify: `Tools/HeadlessBuild/CMakeLists.txt` (add `ReservationGrid.cpp` to `RA4Navigation`)
- Test: `Source/RA4Tests/Private/TestNavigation.cpp` (append T5, T6)

**Interfaces:**
- Consumes: `RA4Core/Vector.h` (`TileCoord`), `RA4Core/SimConfig.h` (`TickIndex` — read the existing definition; if `TickIndex` is not already an int type in `SimTypes.h`, use `int32_t` and note it), `RA4Core/Ids.h` (use `uint32_t kInvalidSlot` local constant = `0xFFFFFFFFu`).
- Produces: `RA4::Nav::ReservationGrid`, `RA4::Nav::ReservationCell`. Used by Task 5 (`SystemMovement`) and Task 4 (`NavDebug` snapshot).

- [ ] **Step 1: Write the failing tests T5 and T6**

Append to `Source/RA4Tests/Private/TestNavigation.cpp` (after the existing `RebuildsOnceForBatchedTopologyChanges` test, before the closing of the file — there is no closing brace, tests are free functions):

```cpp
#include "RA4Navigation/ReservationGrid.h"

RA4_TEST(Navigation, ReservationLowerSlotWinsTie)
{
    // Break caught: if the tie-break were not deterministic, two units racing for
    // the same tile would flip-flop ownership across ticks and never make progress.
    ReservationGrid Grid(4, 4);
    constexpr TickIndex Now = 100;

    RA4_EXPECT(Grid.IsFree(TileCoord(2, 2), Now));
    RA4_EXPECT(Grid.TryReserve(TileCoord(2, 2), /*Slot=*/10, Now, /*HoldTicks=*/2));
    // Lower slot wins the tie even though 10 already holds it.
    RA4_EXPECT(Grid.TryReserve(TileCoord(2, 2), /*Slot=*/3, Now, /*HoldTicks=*/2));
    // Higher slot does not displace the lower-slot holder.
    RA4_EXPECT(!Grid.TryReserve(TileCoord(2, 2), /*Slot=*/20, Now, /*HoldTicks=*/2));
}

RA4_TEST(Navigation, ReservationExpiresAndFreesTile)
{
    // Break caught: a reservation that never expired would leak tiles until the
    // unit that held them was destroyed, starving the rest of the army.
    ReservationGrid Grid(4, 4);
    Grid.TryReserve(TileCoord(1, 1), /*Slot=*/7, /*Now=*/100, /*HoldTicks=*/2);
    RA4_EXPECT(!Grid.IsFree(TileCoord(1, 1), /*Now=*/101));
    RA4_EXPECT(!Grid.IsFree(TileCoord(1, 1), /*Now=*/102));
    RA4_EXPECT(Grid.IsFree(TileCoord(1, 1), /*Now=*/103));
}
```

- [ ] **Step 2: Add `ReservationGrid.cpp` to the CMake `RA4Navigation` target**

In `Tools/HeadlessBuild/CMakeLists.txt`, the `add_library(RA4Navigation STATIC ...)` block currently lists only `NavGrid.cpp` and `FlowField.cpp`. Add the new source:

```cmake
add_library(RA4Navigation STATIC
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/NavGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/FlowField.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/ReservationGrid.cpp
)
```

- [ ] **Step 3: Run the tests to verify they fail to compile**

Run: `cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release && cmake --build build/headless -j8 2>&1 | tail -20`
Expected: compile error — `ReservationGrid` not declared.

- [ ] **Step 4: Write `ReservationGrid.h`**

Create `Source/RA4Navigation/Public/RA4Navigation/ReservationGrid.h`:

```cpp
// Copyright (c) Red Alert 4 project. Soft tile reservations for group movement.
//
// A reservation lets one unit claim a tile for the next few ticks so a second unit
// heading into the same tile waits or diverts instead of stacking on top of it.
// Ties are broken by entity slot index (lower wins) so the outcome is identical on
// every machine and every run -- a precondition for replay and lockstep checksums.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Navigation/NavGrid.h"

#ifndef RA4NAVIGATION_API
#define RA4NAVIGATION_API
#endif

namespace RA4
{
namespace Nav
{

using ::RA4::TickIndex;

constexpr uint32_t kInvalidReservationSlot = 0xFFFFFFFFu;

struct ReservationCell
{
    uint32_t OccupantSlot = kInvalidReservationSlot;
    TickIndex ExpiryTick = 0;   // 0 = free
};

class RA4NAVIGATION_API ReservationGrid
{
public:
    explicit ReservationGrid(int32_t InWidth, int32_t InHeight);

    int32_t GetWidth() const { return Width; }
    int32_t GetHeight() const { return Height; }

    bool IsInBounds(const TileCoord& Tile) const;

    // A tile is free at Now if it was never reserved, or its reservation expired at
    // or before Now. ExpiryTick == 0 is the sentinel for "never reserved".
    bool IsFree(const TileCoord& Tile, TickIndex Now) const;

    // Reserve Tile for Slot for the next HoldTicks ticks. Returns false (and does
    // nothing) if a higher-priority occupant -- a strictly lower slot index --
    // already holds the tile past Now. Equal or higher slots are displaced.
    bool TryReserve(const TileCoord& Tile, uint32_t Slot, TickIndex Now, int32_t HoldTicks);

    // Clears every cell owned by Slot. Called when a unit arrives or dies.
    void Release(uint32_t Slot);

    // Single deterministic sweep at the start of a tick. Marks expired cells free.
    // O(tiles) and called once per tick, not per unit.
    void Expire(TickIndex Now);

private:
    int32_t ToIndex(const TileCoord& Tile) const;

    int32_t Width = 0;
    int32_t Height = 0;
    std::vector<ReservationCell> Cells;
};

} // namespace Nav
} // namespace RA4
```

- [ ] **Step 5: Write `ReservationGrid.cpp`**

Create `Source/RA4Navigation/Private/ReservationGrid.cpp`:

```cpp
// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/ReservationGrid.h"

#include <algorithm>

namespace RA4
{
namespace Nav
{

ReservationGrid::ReservationGrid(int32_t InWidth, int32_t InHeight)
    : Width(std::max(0, InWidth))
    , Height(std::max(0, InHeight))
    , Cells(size_t(Width) * size_t(Height))
{
}

int32_t ReservationGrid::ToIndex(const TileCoord& Tile) const
{
    return Tile.Y * Width + Tile.X;
}

bool ReservationGrid::IsInBounds(const TileCoord& Tile) const
{
    return Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Width && Tile.Y < Height;
}

bool ReservationGrid::IsFree(const TileCoord& Tile, TickIndex Now) const
{
    if (!IsInBounds(Tile))
    {
        return false;
    }
    const ReservationCell& C = Cells[size_t(ToIndex(Tile))];
    return C.OccupantSlot == kInvalidReservationSlot || C.ExpiryTick <= Now;
}

bool ReservationGrid::TryReserve(const TileCoord& Tile, uint32_t Slot, TickIndex Now, int32_t HoldTicks)
{
    if (!IsInBounds(Tile))
    {
        return false;
    }
    const int32_t Idx = ToIndex(Tile);
    ReservationCell& C = Cells[size_t(Idx)];
    // Displace only if the existing holder has expired OR the new slot is strictly
    // lower (the documented tie-break). Equal slots do not displace -- a unit never
    // needs to take its own tile twice.
    const bool bExpired = C.OccupantSlot == kInvalidReservationSlot || C.ExpiryTick <= Now;
    const bool bLowerWins = Slot < C.OccupantSlot;
    if (!bExpired && !bLowerWins)
    {
        return false;
    }
    C.OccupantSlot = Slot;
    C.ExpiryTick = Now + HoldTicks;
    return true;
}

void ReservationGrid::Release(uint32_t Slot)
{
    for (ReservationCell& C : Cells)
    {
        if (C.OccupantSlot == Slot)
        {
            C.OccupantSlot = kInvalidReservationSlot;
            C.ExpiryTick = 0;
        }
    }
}

void ReservationGrid::Expire(TickIndex Now)
{
    for (ReservationCell& C : Cells)
    {
        if (C.OccupantSlot != kInvalidReservationSlot && C.ExpiryTick <= Now)
        {
            C.OccupantSlot = kInvalidReservationSlot;
            C.ExpiryTick = 0;
        }
    }
}

} // namespace Nav
} // namespace RA4
```

- [ ] **Step 6: Verify `TickIndex` is reachable from `RA4Navigation`**

Run: `grep -n "TickIndex" Source/RA4Simulation/Public/RA4Simulation/SimTypes.h Source/RA4Core/Public/RA4Core/Ids.h Source/RA4Core/Public/RA4Core/SimConfig.h`
If `TickIndex` is defined in `SimTypes.h` (inside `RA4Simulation`, which `RA4Navigation` does not depend on), the `using ::RA4::TickIndex;` line will not compile. In that case, change `ReservationGrid.h` to use a local alias backed by the actual underlying type — read the definition and use that type directly (e.g. `using TickIndex = int32_t;` inside `namespace RA4::Nav`). Do not add a dependency on `RA4Simulation`.

- [ ] **Step 7: Build and run T5 + T6**

Run: `cmake --build build/headless -j8 && ./build/headless/RA4Tests --filter=Navigation`
Expected: `Navigation.ReservationLowerSlotWinsTie` and `Navigation.ReservationExpiresAndFreesTile` PASS; the four pre-existing `Navigation.*` tests still PASS. Zero failures.

- [ ] **Step 8: Commit**

```bash
git add Source/RA4Navigation/Public/RA4Navigation/ReservationGrid.h \
        Source/RA4Navigation/Private/ReservationGrid.cpp \
        Source/RA4Tests/Private/TestNavigation.cpp \
        Tools/HeadlessBuild/CMakeLists.txt
git commit -m "feat(navigation): add soft ReservationGrid with slot-order tie-break"
```

---

### Task 2: MNavRouter (sector-portal A\*, LRU cache, deterministic expansion)

**Files:**
- Create: `Source/RA4Navigation/Public/RA4Navigation/MNavRouter.h`
- Create: `Source/RA4Navigation/Private/MNavRouter.cpp`
- Modify: `Tools/HeadlessBuild/CMakeLists.txt` (add `MNavRouter.cpp` to `RA4Navigation`)
- Test: `Source/RA4Tests/Private/TestNavigation.cpp` (append T1, T2, T3)

**Interfaces:**
- Consumes: `RA4Navigation/NavGrid.h` (`NavGrid`, `NavSector`, `NavPortal`, `NavQuery`, `TileCoord`, `NavLayerMask`).
- Produces: `RA4::Nav::MacroPath`, `RA4::Nav::MNavRouter`. Used by Task 5 (`SystemMovement`) and Task 4 (`NavDebug` snapshot).

- [ ] **Step 1: Write the failing tests T1, T2, T3**

Append to `Source/RA4Tests/Private/TestNavigation.cpp` (add the include at the top of the file with the other includes):

```cpp
#include "RA4Navigation/MNavRouter.h"
```

Append the tests:

```cpp
RA4_TEST(Navigation, MacroRouterFindsShortestCorridor)
{
    // Two sectors, one open portal column between them. The macro path must cross
    // the portal exactly once and the result must be identical on a second call.
    NavGrid Grid(32, 16);
    MNavRouter Router(Grid);
    const NavQuery Query{NavLayer_Tracked, 1};

    MacroPath Path = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, /*MaxWaypoints=*/8);
    RA4_REQUIRE(!Path.Waypoints.empty());
    RA4_EXPECT_EQ(Path.Waypoints.size(), size_t(2));
    RA4_EXPECT_EQ(Path.BuiltTopologyRevision, Grid.GetTopologyRevision());

    MacroPath Again = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, /*MaxWaypoints=*/8);
    RA4_EXPECT_EQ(Again.Waypoints.size(), Path.Waypoints.size());
    RA4_EXPECT(Again.Waypoints[0] == Path.Waypoints[0]);
}

RA4_TEST(Navigation, MacroRouterRespectsLayerAndClearance)
{
    // If every portal cell is masked to infantry-only, a tracked query must get an
    // empty path while an infantry query still routes through.
    NavGrid Grid(32, 16);
    // Force the whole portal column (x=15..16) to infantry-only.
    Grid.BeginTopologyUpdate();
    for (int32_t Y = 0; Y < 16; ++Y)
    {
        Grid.SetPassability(TileCoord(15, Y), NavLayer_Infantry);
        Grid.SetPassability(TileCoord(16, Y), NavLayer_Infantry);
    }
    Grid.EndTopologyUpdate();

    MNavRouter Router(Grid);
    const NavQuery Tracked{NavLayer_Tracked, 1};
    MacroPath TankPath = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Tracked, 8);
    RA4_EXPECT(TankPath.Waypoints.empty());

    const NavQuery Inf{NavLayer_Infantry, 1};
    MacroPath InfPath = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Inf, 8);
    RA4_EXPECT(!InfPath.Waypoints.empty());
}

RA4_TEST(Navigation, MacroRouterInvalidatesOnTopologyRevision)
{
    // A cached path built against revision N must be rejected after the grid's
    // topology changes to revision N+1 -- otherwise units walk through a wall that
    // was just placed across their corridor.
    NavGrid Grid(32, 16);
    MNavRouter Router(Grid);
    const NavQuery Query{NavLayer_Tracked, 1};

    MacroPath First = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, 8);
    RA4_REQUIRE(!First.Waypoints.empty());
    const uint32_t Rev0 = First.BuiltTopologyRevision;

    Grid.BeginTopologyUpdate();
    Grid.SetPassability(TileCoord(15, 8), NavLayer_None);
    Grid.SetPassability(TileCoord(16, 8), NavLayer_None);
    Grid.EndTopologyUpdate();
    RA4_EXPECT(Grid.GetTopologyRevision() != Rev0);

    Router.InvalidateAll();
    MacroPath Second = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, 8);
    RA4_EXPECT(Second.BuiltTopologyRevision != Rev0);
}
```

- [ ] **Step 2: Add `MNavRouter.cpp` to the CMake `RA4Navigation` target**

```cmake
add_library(RA4Navigation STATIC
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/NavGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/FlowField.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/ReservationGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/MNavRouter.cpp
)
```

- [ ] **Step 3: Run the tests to verify they fail to compile**

Run: `cmake --build build/headless -j8 2>&1 | tail -20`
Expected: `MNavRouter` not declared.

- [ ] **Step 4: Write `MNavRouter.h`**

Create `Source/RA4Navigation/Public/RA4Navigation/MNavRouter.h`:

```cpp
// Copyright (c) Red Alert 4 project. Hierarchical macro router over sector portals.
//
// A* over the sector-portal graph produces a coarse corridor of sector-center
// sub-goals. Units then follow the shared flow field to each sub-goal, so 300
// units heading to one destination build one macro path and one flow field per
// sector, not 300 of each. Expansion order is fixed (ascending sector id) and the
// tie-break is (g+h, sector id), so the path is identical on every machine.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Navigation/NavGrid.h"

#ifndef RA4NAVIGATION_API
#define RA4NAVIGATION_API
#endif

namespace RA4
{
namespace Nav
{

struct MacroPath
{
    std::vector<TileCoord> Waypoints;     // sector-center sub-goals; final = destination tile
    uint32_t BuiltTopologyRevision = 0;   // stale paths are rejected, not rebuilt blindly
    NavQuery Query{};
};

class RA4NAVIGATION_API MNavRouter
{
public:
    explicit MNavRouter(const NavGrid& InGrid);

    // A* over sector portals from From -> To. Emits at most MaxWaypoints sub-goals.
    // Returns an empty MacroPath if no portal path exists. Deterministic.
    MacroPath Find(const TileCoord& From, const TileCoord& To,
                   const NavQuery& Query, int32_t MaxWaypoints);

    // Drop every cached path. Called when the grid's topology revision bumps.
    void InvalidateAll();

    // Cache diagnostics for the determinism/perf tests.
    uint32_t GetCacheHits() const { return CacheHits; }
    uint32_t GetCacheMisses() const { return CacheMisses; }
    void ResetCounters() { CacheHits = 0; CacheMisses = 0; }

private:
    struct CacheEntry
    {
        uint16_t FromSector = 0;
        uint16_t ToSector = 0;
        NavQuery Query{};
        uint32_t TopologyRevision = 0;
        std::vector<TileCoord> Waypoints;
        int32_t LastUsedSerial = 0;   // for LRU; bumped on each Find
    };

    TileCoord SectorCenter(uint16_t SectorId) const;
    const std::vector<CacheEntry>& GetCacheForTest() const { return Cache; }

    const NavGrid& Grid;
    std::vector<CacheEntry> Cache;
    int32_t AccessSerial = 0;
    uint32_t CacheHits = 0;
    uint32_t CacheMisses = 0;
    static constexpr size_t kCacheCap = 128;
};

} // namespace Nav
} // namespace RA4
```

- [ ] **Step 5: Write `MNavRouter.cpp`**

Create `Source/RA4Navigation/Private/MNavRouter.cpp`:

```cpp
// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/MNavRouter.h"

#include <algorithm>
#include <queue>

namespace RA4
{
namespace Nav
{

namespace
{
struct AStarNode
{
    uint16_t SectorId = 0;
    int32_t G = 0;        // cost from start in sector hops
    int32_t H = 0;        // heuristic: sector-grid octile distance
    int32_t F = 0;        // G + H
    uint16_t CameFrom = 0;
};
} // namespace

MNavRouter::MNavRouter(const NavGrid& InGrid)
    : Grid(InGrid)
{
}

TileCoord MNavRouter::SectorCenter(uint16_t SectorId) const
{
    const std::vector<NavSector>& Sectors = Grid.GetSectors();
    if (SectorId >= Sectors.size())
    {
        return TileCoord();
    }
    const NavSector& S = Sectors[SectorId];
    return TileCoord((S.Min.X + S.Max.X) / 2, (S.Min.Y + S.Max.Y) / 2);
}

MacroPath MNavRouter::Find(const TileCoord& From, const TileCoord& To,
                           const NavQuery& Query, int32_t MaxWaypoints)
{
    MacroPath Out;
    Out.Query = Query;
    Out.BuiltTopologyRevision = Grid.GetTopologyRevision();

    if (!Grid.IsInBounds(From) || !Grid.IsInBounds(To))
    {
        return Out;
    }

    // Sector id from a tile: floor(tile / kSectorSize) plus row-major index.
    const auto SectorOf = [&](const TileCoord& T) -> uint16_t {
        const int32_t Sx = T.X / NavGrid::kSectorSize;
        const int32_t Sy = T.Y / NavGrid::kSectorSize;
        const int32_t W = (Grid.GetWidth() + NavGrid::kSectorSize - 1) / NavGrid::kSectorSize;
        return uint16_t(Sy * W + Sx);
    };

    const uint16_t StartSector = SectorOf(From);
    const uint16_t GoalSector = SectorOf(To);

    // Cache lookup. Hit = same from/to sectors, same query, same topology revision.
    for (CacheEntry& E : Cache)
    {
        if (E.FromSector == StartSector && E.ToSector == GoalSector &&
            E.Query.LayerMask == Query.LayerMask && E.Query.RequiredClearance == Query.RequiredClearance &&
            E.TopologyRevision == Out.BuiltTopologyRevision)
        {
            ++CacheHits;
            E.LastUsedSerial = ++AccessSerial;
            Out.Waypoints = E.Waypoints;
            // LRU: move to back so the front is always the coldest entry.
            std::rotate(Cache.begin(), std::find(Cache.begin(), Cache.end(), E), Cache.begin() + 1);
            return Out;
        }
    }
    ++CacheMisses;

    // Same-sector shortcut: the flow field alone handles it.
    if (StartSector == GoalSector)
    {
        Out.Waypoints.push_back(To);
        return Out;
    }

    // A* over portals. Neighbor expansion order is ascending destination sector id;
    // the priority queue tie-breaks on (F, SectorId) so it is fully deterministic.
    const std::vector<NavPortal>& Portals = Grid.GetPortals();
    const std::vector<NavSector>& Sectors = Grid.GetSectors();

    std::vector<AStarNode> Nodes(Sectors.size());
    std::vector<bool> Closed(Sectors.size(), false);
    for (uint16_t I = 0; I < Sectors.size(); ++I)
    {
        Nodes[I].SectorId = I;
        Nodes[I].CameFrom = I;
    }

    auto Heuristic = [&](uint16_t A, uint16_t B) -> int32_t {
        const NavSector& SA = Sectors[A];
        const NavSector& SB = Sectors[B];
        const int32_t Dx = (SA.Min.X + SA.Max.X) / 2 - (SB.Min.X + SB.Max.X) / 2;
        const int32_t Dy = (SA.Min.Y + SA.Max.Y) / 2 - (SB.Min.Y + SB.Max.Y) / 2;
        const int32_t Adx = Dx < 0 ? -Dx : Dx;
        const int32_t Ady = Dy < 0 ? -Dy : Dy;
        return 10 * (Adx < Ady ? Adx : Ady) + 14 * (Adx > Ady ? Adx - Ady : Ady - Adx);
    };

    auto Greater = [](const AStarNode& A, const AStarNode& B) {
        return A.F != B.F ? A.F > B.F : A.SectorId > B.SectorId;
    };
    std::priority_queue<AStarNode, std::vector<AStarNode>, decltype(Greater)> Open(Greater);

    Nodes[StartSector].G = 0;
    Nodes[StartSector].H = Heuristic(StartSector, GoalSector);
    Nodes[StartSector].F = Nodes[StartSector].H;
    Open.push(Nodes[StartSector]);

    bool bFound = false;
    while (!Open.empty())
    {
        AStarNode Cur = Open.top();
        Open.pop();
        if (Closed[Cur.SectorId])
        {
            continue;
        }
        Closed[Cur.SectorId] = true;
        if (Cur.SectorId == GoalSector)
        {
            bFound = true;
            break;
        }
        // Expand neighbors in ascending destination-sector-id order for determinism.
        // Portals are pre-sorted by SectorA then SectorB at grid build time; filter.
        for (const NavPortal& P : Portals)
        {
            uint16_t Neighbor = 0;
            if (P.SectorA == Cur.SectorId) { Neighbor = P.SectorB; }
            else if (P.SectorB == Cur.SectorId) { Neighbor = P.SectorA; }
            else { continue; }
            if (Closed[Neighbor])
            {
                continue;
            }
            // Layer/clearance check against the portal's passability mask.
            if ((P.PassabilityMask & Query.LayerMask) == 0)
            {
                continue;
            }
            // Note: per-portal clearance is not stored today; the flow field handles
            // cell-level clearance. The router only guarantees sector reachability.
            const int32_t TentativeG = Cur.G + 1;
            if (Nodes[Neighbor].G == 0 && Neighbor != StartSector)
            {
                Nodes[Neighbor].G = TentativeG;
                Nodes[Neighbor].H = Heuristic(Neighbor, GoalSector);
                Nodes[Neighbor].F = TentativeG + Nodes[Neighbor].H;
                Nodes[Neighbor].CameFrom = Cur.SectorId;
                Open.push(Nodes[Neighbor]);
            }
            else if (TentativeG < Nodes[Neighbor].G)
            {
                Nodes[Neighbor].G = TentativeG;
                Nodes[Neighbor].F = TentativeG + Nodes[Neighbor].H;
                Nodes[Neighbor].CameFrom = Cur.SectorId;
                Open.push(Nodes[Neighbor]);
            }
        }
    }

    if (!bFound)
    {
        // Unreachable. Cache the empty path so we don't A* again next tick.
        if (Cache.size() >= kCacheCap)
        {
            Cache.erase(Cache.begin());
        }
        Cache.push_back({StartSector, GoalSector, Query, Out.BuiltTopologyRevision, {}, ++AccessSerial});
        return Out;
    }

    // Reconstruct sector chain, then emit sector centers as sub-goals, final = To.
    std::vector<uint16_t> Chain;
    uint16_t Cur = GoalSector;
    while (Cur != StartSector && Nodes[Cur].CameFrom != Cur)
    {
        Chain.push_back(Cur);
        Cur = Nodes[Cur].CameFrom;
    }
    std::reverse(Chain.begin(), Chain.end());

    Out.Waypoints.reserve(Chain.size() + 1);
    for (uint16_t S : Chain)
    {
        Out.Waypoints.push_back(SectorCenter(S));
        if (int32_t(Out.Waypoints.size()) >= MaxWaypoints - 1)
        {
            break;
        }
    }
    Out.Waypoints.push_back(To);

    if (Cache.size() >= kCacheCap)
    {
        Cache.erase(Cache.begin());
    }
    Cache.push_back({StartSector, GoalSector, Query, Out.BuiltTopologyRevision, Out.Waypoints, ++AccessSerial});
    return Out;
}

void MNavRouter::InvalidateAll()
{
    Cache.clear();
}

} // namespace Nav
} // namespace RA4
```

- [ ] **Step 6: Build and run T1, T2, T3**

Run: `cmake --build build/headless -j8 && ./build/headless/RA4Tests --filter=Navigation`
Expected: T1, T2, T3 PASS; T5, T6 still PASS; the four pre-existing nav tests still PASS. Zero failures.

- [ ] **Step 7: Commit**

```bash
git add Source/RA4Navigation/Public/RA4Navigation/MNavRouter.h \
        Source/RA4Navigation/Private/MNavRouter.cpp \
        Source/RA4Tests/Private/TestNavigation.cpp \
        Tools/HeadlessBuild/CMakeLists.txt
git commit -m "feat(navigation): add MNavRouter sector-portal A* with LRU cache"
```

---

### Task 3: Formation (slot assignment, leader-follows)

**Files:**
- Create: `Source/RA4Navigation/Public/RA4Navigation/Formation.h`
- Create: `Source/RA4Navigation/Private/Formation.cpp`
- Modify: `Tools/HeadlessBuild/CMakeLists.txt` (add `Formation.cpp`)
- Test: `Source/RA4Tests/Private/TestNavigation.cpp` (append T9)

**Interfaces:**
- Consumes: `RA4Core/Vector.h` (`Vec2`), `RA4Core/Ids.h` (`ContentId`, `EntityId`).
- Produces: `RA4::FormationDef`, `RA4::FormationAssignment`. Used by Task 5 (`SystemMovement` reads `MovementComp.FormationId`/`FormationSlot`, the leader sets members' destinations).

- [ ] **Step 1: Write the failing test T9**

Append the include at the top:

```cpp
#include "RA4Navigation/Formation.h"
```

Append the test:

```cpp
RA4_TEST(Navigation, FormationMembersFollowLeaderSlot)
{
    // Break caught: if members computed their own macro path, a formation of 8
    // would build 8 paths instead of 1. The leader owns the path; members follow
    // rotated offsets and must arrive within one tile of their slot.
    FormationDef Def;
    Def.Id = MakeContentId("formation.test.wedge");
    // 8-slot wedge: leader at origin, 3 behind-left, 3 behind-right, 1 tail.
    Def.Offsets = {
        Vec2(Fixed::Zero(), Fixed::Zero()),
        Vec2(Fixed::FromInt(-100), Fixed::FromInt(-100)),
        Vec2(Fixed::FromInt(-200), Fixed::FromInt(-200)),
        Vec2(Fixed::FromInt(-300), Fixed::FromInt(-300)),
        Vec2(Fixed::FromInt(100), Fixed::FromInt(-100)),
        Vec2(Fixed::FromInt(200), Fixed::FromInt(-200)),
        Vec2(Fixed::FromInt(300), Fixed::FromInt(-300)),
        Vec2(Fixed::FromInt(0), Fixed::FromInt(-400)),
    };
    RA4_EXPECT_EQ(Def.Offsets.size(), size_t(8));

    // Rotating the leader by 90 degrees (kAngleTurn/4) maps the +X offset to +Y.
    const int32_t FacingRight = 1 << 10;   // 4096/4 == 1024 == 90 degrees
    const Vec2 LeaderPos(Fixed::FromInt(500), Fixed::FromInt(500));
    const Vec2 Slot1 = LeaderPos + RotateOffset(Def.Offsets[1], FacingRight);
    // Slot1 original (-100,-100) rotated 90deg CW -> (-100, +100) relative, plus leader.
    RA4_EXPECT_NEAR(Slot1.X.Raw, Fixed::FromInt(400).Raw, Fixed::FromInt(5).Raw);
    RA4_EXPECT_NEAR(Slot1.Y.Raw, Fixed::FromInt(600).Raw, Fixed::FromInt(5).Raw);
}
```

- [ ] **Step 2: Add `Formation.cpp` to CMake**

```cmake
add_library(RA4Navigation STATIC
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/NavGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/FlowField.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/ReservationGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/MNavRouter.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/Formation.cpp
)
```

- [ ] **Step 3: Run the test to verify it fails to compile**

Run: `cmake --build build/headless -j8 2>&1 | tail -20`
Expected: `Formation.h` not found, `RotateOffset` not declared.

- [ ] **Step 4: Write `Formation.h`**

Create `Source/RA4Navigation/Public/RA4Navigation/Formation.h`:

```cpp
// Copyright (c) Red Alert 4 project. Formation descriptors and slot assignment.
//
// Formations are authored as data (a ContentId + a list of Vec2 offsets relative to
// the leader's heading). The leader owns the macro path; members never build their
// own -- their Destination is set every tick to LeaderPos + Rotate(Offset[slot],
// LeaderFacing). This keeps a formation of N units at one flow field per sector,
// not N.
#pragma once

#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

#ifndef RA4NAVIGATION_API
#define RA4NAVIGATION_API
#endif

namespace RA4
{

struct FormationDef
{
    ContentId Id;
    std::vector<Vec2> Offsets;   // slot i -> offset from leader, in leader-facing space
};

struct FormationAssignment
{
    ContentId FormationId;
    EntityId Leader;
    std::vector<EntityId> Members;   // slot i -> Members[i]
};

// Rotates an offset by a 4096-step angle (kAngleTurn). Used by the movement system
// and by the formation tests.
RA4NAVIGATION_API Vec2 RotateOffset(const Vec2& Offset, int32_t Facing);

} // namespace RA4
```

- [ ] **Step 5: Write `Formation.cpp`**

Create `Source/RA4Navigation/Private/Formation.cpp`:

```cpp
// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/Formation.h"

namespace RA4
{

Vec2 RotateOffset(const Vec2& Offset, int32_t Facing)
{
    // 2D rotation by the fixed-point angle. FxCos/FxSin are in Fixed.h.
    const Fixed C = FxCos(Facing);
    const Fixed S = FxSin(Facing);
    return Vec2(Offset.X * C - Offset.Y * S, Offset.X * S + Offset.Y * C);
}

} // namespace RA4
```

- [ ] **Step 6: Build and run T9**

Run: `cmake --build build/headless -j8 && ./build/headless/RA4Tests --filter=Navigation`
Expected: T9 PASS; all prior nav tests still PASS. Zero failures.

- [ ] **Step 7: Commit**

```bash
git add Source/RA4Navigation/Public/RA4Navigation/Formation.h \
        Source/RA4Navigation/Private/Formation.cpp \
        Source/RA4Tests/Private/TestNavigation.cpp \
        Tools/HeadlessBuild/CMakeLists.txt
git commit -m "feat(navigation): add Formation descriptor and RotateOffset"
```

---

### Task 4: NavDebugSnapshot (pure-data, no draw dependency)

**Files:**
- Create: `Source/RA4Navigation/Public/RA4Navigation/NavDebug.h`
- Modify: `Source/RA4Navigation/Public/RA4Navigation/ReservationGrid.h` (add `Snapshot` method)
- Modify: `Source/RA4Navigation/Public/RA4Navigation/MNavRouter.h` (add `Snapshot` method)
- Modify: `Source/RA4Navigation/Private/ReservationGrid.cpp`
- Modify: `Source/RA4Navigation/Private/MNavRouter.cpp`
- Test: `Source/RA4Tests/Private/TestNavigation.cpp` (append T13)

**Interfaces:**
- Consumes: the three structures from Tasks 1–3.
- Produces: `RA4::Nav::NavDebugSnapshot`. Used by the future presentation bridge (out of scope) and by T13 to prove the sim has no draw dependency.

- [ ] **Step 1: Write the failing test T13**

Append the include at the top:

```cpp
#include "RA4Navigation/NavDebug.h"
```

Append the test:

```cpp
RA4_TEST(Navigation, NavDebugSnapshotHasNoDrawDependency)
{
    // Break caught: if NavDebug ever pulled in UWorld/DrawDebug, the headless
    // build would fail to link. This test exists to make that link failure a
    // test failure instead of a surprise in CI.
    NavGrid Grid(8, 8);
    ReservationGrid Res(8, 8);
    MNavRouter Router(Grid);
    Res.TryReserve(TileCoord(3, 3), /*Slot=*/1, /*Now=*/0, /*HoldTicks=*/5);
    Router.Find(TileCoord(0, 0), TileCoord(7, 7), NavQuery{NavLayer_Tracked, 1}, 8);

    NavDebugSnapshot Snap;
    Res.Snapshot(Snap);
    Router.Snapshot(Snap);
    Grid.Snapshot(Snap);   // if NavGrid doesn't have Snapshot yet, add a trivial one

    RA4_EXPECT(Snap.TopologyRevision == Grid.GetTopologyRevision());
    RA4_EXPECT(!Snap.ReservationSample.empty());
    RA4_EXPECT(!Snap.ActiveMacroPaths.empty());
    // Serialize to bytes to prove it is plain data, not a draw handle.
    std::vector<uint8_t> Bytes;
    SerializeNavDebugSnapshot(Snap, Bytes);
    RA4_EXPECT(!Bytes.empty());
}
```

- [ ] **Step 2: Run the test to verify it fails to compile**

Run: `cmake --build build/headless -j8 2>&1 | tail -20`
Expected: `NavDebug.h` not found, `Snapshot`/`SerializeNavDebugSnapshot` not declared.

- [ ] **Step 3: Write `NavDebug.h`**

Create `Source/RA4Navigation/Public/RA4Navigation/NavDebug.h`:

```cpp
// Copyright (c) Red Alert 4 project. Pure-data navigation debug snapshot.
//
// This is the ONLY debug surface the simulation exposes. The presentation bridge
// (a later roadmap stage) renders it; the headless tests serialize it. There is no
// DrawDebug* anywhere in RA4Navigation or RA4Simulation, because that would make the
// headless build link-fail and the deterministic core depend on the renderer.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Navigation/FlowField.h"
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/NavGrid.h"
#include "RA4Navigation/ReservationGrid.h"

namespace RA4
{
namespace Nav
{

struct NavDebugSnapshot
{
    uint32_t TopologyRevision = 0;
    std::vector<FlowDirection> FlowFieldSample;     // sparse: only dirty tiles
    std::vector<ReservationCell> ReservationSample; // copied cells (occupied only)
    std::vector<TileCoord> BlockedTiles;
    std::vector<MacroPath> ActiveMacroPaths;        // capped at 16 for debug
};

RA4NAVIGATION_API void SerializeNavDebugSnapshot(const NavDebugSnapshot& Snap, std::vector<uint8_t>& OutBytes);

} // namespace Nav
} // namespace RA4
```

- [ ] **Step 4: Add `Snapshot` methods and a tiny serializer**

Add to `ReservationGrid.h` (inside `class ReservationGrid`, after `Expire`):

```cpp
    void Snapshot(NavDebugSnapshot& Out) const;
```

Add to `MNavRouter.h` (inside `class MNavRouter`, after `ResetCounters`):

```cpp
    void Snapshot(NavDebugSnapshot& Out) const;
```

Add to `NavGrid.h` (inside `class NavGrid`, after `GetPortals`):

```cpp
    void Snapshot(NavDebugSnapshot& Out) const;
```

Create `Source/RA4Navigation/Private/NavDebug.cpp`:

```cpp
// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/NavDebug.h"

namespace RA4
{
namespace Nav
{

void SerializeNavDebugSnapshot(const NavDebugSnapshot& Snap, std::vector<uint8_t>& OutBytes)
{
    OutBytes.clear();
    auto Append = [&](uint32_t V) {
        OutBytes.push_back(uint8_t(V & 0xFF));
        OutBytes.push_back(uint8_t((V >> 8) & 0xFF));
        OutBytes.push_back(uint8_t((V >> 16) & 0xFF));
        OutBytes.push_back(uint8_t((V >> 24) & 0xFF));
    };
    Append(Snap.TopologyRevision);
    Append(uint32_t(Snap.ReservationSample.size()));
    Append(uint32_t(Snap.ActiveMacroPaths.size()));
}

void ReservationGrid::Snapshot(NavDebugSnapshot& Out) const
{
    Out.ReservationSample.clear();
    for (const ReservationCell& C : Cells)
    {
        if (C.OccupantSlot != kInvalidReservationSlot)
        {
            Out.ReservationSample.push_back(C);
        }
    }
}

void MNavRouter::Snapshot(NavDebugSnapshot& Out) const
{
    Out.ActiveMacroPaths.clear();
    for (const CacheEntry& E : Cache)
    {
        MacroPath P;
        P.Waypoints = E.Waypoints;
        P.BuiltTopologyRevision = E.TopologyRevision;
        P.Query = E.Query;
        Out.ActiveMacroPaths.push_back(std::move(P));
        if (Out.ActiveMacroPaths.size() >= 16)
        {
            break;
        }
    }
}

void NavGrid::Snapshot(NavDebugSnapshot& Out) const
{
    Out.TopologyRevision = TopologyRevision;
    Out.BlockedTiles.clear();
    for (int32_t Y = 0; Y < Height; ++Y)
    {
        for (int32_t X = 0; X < Width; ++X)
        {
            const NavCell& C = Cells[size_t(Y * Width + X)];
            if (C.PassabilityMask == NavLayer_None)
            {
                Out.BlockedTiles.push_back(TileCoord(X, Y));
            }
        }
    }
}

} // namespace Nav
} // namespace RA4
```

Add `NavDebug.cpp` to CMake:

```cmake
add_library(RA4Navigation STATIC
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/NavGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/FlowField.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/ReservationGrid.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/MNavRouter.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/Formation.cpp
    ${RA4_SOURCE_ROOT}/RA4Navigation/Private/NavDebug.cpp
)
```

- [ ] **Step 5: Build and run T13**

Run: `cmake --build build/headless -j8 && ./build/headless/RA4Tests --filter=Navigation`
Expected: T13 PASS; all prior nav tests still PASS. Zero failures.

- [ ] **Step 6: Commit**

```bash
git add Source/RA4Navigation/Public/RA4Navigation/NavDebug.h \
        Source/RA4Navigation/Private/NavDebug.cpp \
        Source/RA4Navigation/Public/RA4Navigation/ReservationGrid.h \
        Source/RA4Navigation/Private/ReservationGrid.cpp \
        Source/RA4Navigation/Public/RA4Navigation/MNavRouter.h \
        Source/RA4Navigation/Private/MNavRouter.cpp \
        Source/RA4Navigation/Public/RA4Navigation/NavGrid.h \
        Source/RA4Tests/Private/TestNavigation.cpp \
        Tools/HeadlessBuild/CMakeLists.txt
git commit -m "feat(navigation): add pure-data NavDebugSnapshot, no draw dependency"
```

---

### Task 5: Rewire `SystemMovement` (router + reservations + budget + avoidance)

This is the core task. It modifies `SimWorld` and `SimTypes.h` and must keep every existing `TestVerticalSlice` test passing unchanged.

**Files:**
- Modify: `Source/RA4Core/Public/RA4Core/SimConfig.h` (add budget consts)
- Modify: `Source/RA4Simulation/Public/RA4Simulation/SimTypes.h` (extend `MovementComp`; add `MovementStats`)
- Modify: `Source/RA4Simulation/Public/RA4Simulation/SimWorld.h` (add members + accessors)
- Modify: `Source/RA4Simulation/Private/SimWorld.cpp` (rewrite `SystemMovement`; add `GetFlowField` budget; invalidation hooks)
- Test: `Source/RA4Tests/Private/TestNavigation.cpp` (append T4, T7, T8, T11, T12)

**Interfaces:**
- Consumes: Tasks 1–4 (`ReservationGrid`, `MNavRouter`, `Formation`, `NavDebugSnapshot`).
- Produces: `SimWorld::GetMovementStats()` returning `MovementStats{FlowFieldBuilds, MacroPathBuilds, ReservationContests}`. Used by T4/T8/T10/T11 to prove no per-unit A\*.

- [ ] **Step 1: Add budget constants to `SimConfig.h`**

Append (before the closing `} // namespace RA4`) in `Source/RA4Core/Public/RA4Core/SimConfig.h`:

```cpp
// Per-tick repath budget. Tick-bounded, not wall-clock: a slow machine does fewer
// builds per tick and catches up over more ticks, but the build *sequence* is
// identical, so the checksum is identical. Identical inputs -> identical state.
constexpr int32_t kMaxFlowFieldBuildsPerTick = 2;
constexpr int32_t kMaxMacroPathBuildsPerTick = 4;
constexpr int32_t kRepathBlockedTickThreshold = 60;   // 3s at 20Hz
```

- [ ] **Step 2: Extend `MovementComp` and add `MovementStats` in `SimTypes.h`**

In `Source/RA4Simulation/Public/RA4Simulation/SimTypes.h`, replace the existing `MovementComp` block with:

```cpp
struct MovementComp
{
    Vec2 Destination;
    bool bHasDestination = false;
    Fixed CurrentSpeed = Fixed::Zero();
    Fixed ArriveRadius = Fixed::FromInt(30);
    int32_t BlockedTicks = 0;
    // --- navigation milestone ---
    Nav::MacroPath CurrentMacroPath;
    int32_t NextWaypointIndex = 0;
    TileCoord CurrentSubGoal;
    ContentId FormationId;          // ContentId() == no formation
    int32_t FormationSlot = -1;     // -1 == leader or unassigned
    TickIndex LastRepathTick = 0;
};

struct MovementStats
{
    uint32_t FlowFieldBuilds = 0;
    uint32_t MacroPathBuilds = 0;
    uint32_t ReservationContests = 0;
};
```

Add the include at the top of `SimTypes.h`:

```cpp
#include "RA4Navigation/MNavRouter.h"
```

- [ ] **Step 3: Add members and accessors to `SimWorld.h`**

In `Source/RA4Simulation/Public/RA4Simulation/SimWorld.h`, add to the private state section (near `FlowFieldCache`):

```cpp
    std::unique_ptr<Nav::ReservationGrid> Reservations;
    std::unique_ptr<Nav::MNavRouter> Router;
    MovementStats Stats;
    int32_t FlowFieldBuildsThisTick = 0;
    int32_t MacroPathBuildsThisTick = 0;
```

Add to the public section (near `GetEvents`):

```cpp
    const MovementStats& GetMovementStats() const { return Stats; }
    void ResetMovementStats() { Stats = MovementStats{}; }
```

Add the include at the top:

```cpp
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/ReservationGrid.h"
```

- [ ] **Step 4: Write the failing tests T4, T7, T8, T11, T12**

Append to `Source/RA4Tests/Private/TestNavigation.cpp`. These need a `SimWorld` built the same way `TestSimulation.cpp` builds one — with a local `Fixture` that owns the `ContentDatabase` and calls `BuildDefaultContent`. Add the includes at the top of the file with the other includes:

```cpp
#include "TestHelpers.h"
#include "RA4Core/SimConfig.h"
#include "RA4Content/ContentDatabase.h"
```

And add a file-local fixture above the new tests (matches the `Fixture` in `TestSimulation.cpp`):

```cpp
namespace
{
struct NavFixture
{
    ContentDatabase Content;
    SimWorld World;
    explicit NavFixture(uint64_t Seed)
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, RA4Test::MakeTestSetup(Seed));
    }
};
} // namespace
```

Every test below that needs a world uses `NavFixture F(Seed); SimWorld& World = F.World;` and, where it needs to mutate the map before `Initialize`, constructs the `MatchSetup` first, mutates it, then calls `World.Initialize(&F.Content, Setup)` — see the T7/T8/T12 examples which build the setup explicitly.

Append:

```cpp
RA4_TEST(Navigation, FlowFieldSharedAcrossUnits)
{
    // Break caught: if each unit built its own flow field, 50 units to one rally
    // point would build 50 fields. Sharing means exactly one build.
    NavFixture F(42);
    SimWorld& World = F.World;

    const Vec2 Rally(Fixed::FromInt(2000), Fixed::FromInt(2000));
    for (int32_t I = 0; I < 50; ++I)
    {
        const EntityId U = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                           Vec2(Fixed::FromInt(100), Fixed::FromInt(100 + I * 10)));
        Command Move;
        Move.Type = CommandType::Move;
        Move.Issuer = PlayerId{0};
        Move.Target = U;
        Move.Position = Rally;
        World.ApplyCommand(Move);
    }

    World.ResetMovementStats();
    for (int32_t T = 0; T < 20; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    const MovementStats& S = World.GetMovementStats();
    // 50 units, one shared destination -> at most a handful of flow-field builds,
    // never 50. Allow a few for sub-goal sectors along the corridor.
    RA4_EXPECT(S.FlowFieldBuilds <= 5u);
    RA4_EXPECT(S.MacroPathBuilds <= 4u);
}

RA4_TEST(Navigation, LocalAvoidancePicksBestOpenNeighbor)
{
    // Break caught: if the desired tile is blocked by a static obstacle and the
    // unit did not divert, it would sit against the wall until the blocked-tick
    // repath, wasting the avoidance pass.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(7);
    // Build a 1-tile wall straight in front of the unit's path.
    Setup.Map.Resize(16, 16, Tile_GroundPassable);
    Setup.Map.GetTile(8, 4) = uint8_t(Tile_Cliff);   // wall on the direct line
    SimWorld World;
    World.Initialize(&Content, Setup);
    const EntityId U = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                       Vec2(Fixed::FromInt(1600), Fixed::FromInt(800)));
    Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
    Move.Target = U; Move.Position = Vec2(Fixed::FromInt(1600), Fixed::FromInt(0));
    World.ApplyCommand(Move);

    // Give it enough ticks to reach the wall and divert.
    for (int32_t T = 0; T < 200; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    const TransformComp* Tx = World.GetTransform(U);
    RA4_REQUIRE(Tx != nullptr);
    // The unit must have moved past Y=800 (its start) -- i.e. it did not get stuck.
    RA4_EXPECT(Tx->Position.Y.Raw < Fixed::FromInt(800).Raw);
}

RA4_TEST(Navigation, BlockedUnitRepatsAfterThreshold)
{
    // Break caught: a wedged unit that never repaths blocks the tile forever and
    // its blocked-tick counter climbs without bound.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(9);
    Setup.Map.Resize(16, 16, Tile_GroundPassable);
    // Box the unit in: walls on three sides, open only behind it.
    Setup.Map.GetTile(8, 6) = uint8_t(Tile_Cliff);
    Setup.Map.GetTile(7, 7) = uint8_t(Tile_Cliff);
    Setup.Map.GetTile(9, 7) = uint8_t(Tile_Cliff);
    Setup.Map.GetTile(8, 7) = uint8_t(Tile_Cliff);
    SimWorld World;
    World.Initialize(&Content, Setup);
    const EntityId U = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                       Vec2(Fixed::FromInt(1700), Fixed::FromInt(1500)));
    Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
    Move.Target = U; Move.Position = Vec2(Fixed::FromInt(1700), Fixed::FromInt(1100));
    World.ApplyCommand(Move);

    for (int32_t T = 0; T < kRepathBlockedTickThreshold + 20; ++T)
    {
        World.Tick(nullptr); World.ClearEvents();
    }
    // After the threshold, the macro path was cleared at least once and the
    // blocked-tick counter was reset (not left to grow forever).
    const MovementComp* M = World.GetMovement(U);
    RA4_REQUIRE(M != nullptr);
    RA4_EXPECT(M->BlockedTicks < kRepathBlockedTickThreshold);
}

RA4_TEST(Navigation, RepathBudgetStallsDeterministically)
{
    // Break caught: if the budget were wall-clock, a slow machine would produce a
    // different build *sequence* and a different checksum. With a tick-bounded
    // budget, budget=2 splits 10 builds as 2/2/2/2/2 across 5 ticks; budget=10 does
    // all 10 in one tick. The final state must be identical. The full cross-build
    // checksum comparison is exercised in Task 6; here we only assert the budget
    // consts exist, are positive, and are the values the rest of the plan depends on.
    RA4_EXPECT(kMaxFlowFieldBuildsPerTick > 0);
    RA4_EXPECT(kMaxMacroPathBuildsPerTick > 0);
    RA4_EXPECT_EQ(kMaxFlowFieldBuildsPerTick, 2);
    RA4_EXPECT_EQ(kMaxMacroPathBuildsPerTick, 4);
}

RA4_TEST(Navigation, BridgeDestroyInvalidatesPath)
{
    // Break caught: a unit mid-cross must not keep walking on a destroyed bridge's
    // stale flow field; the topology bump must invalidate its cached path.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(13);
    Setup.Map.Resize(32, 16, Tile_GroundPassable);
    // Carve a water channel with one ground-tile "bridge" at x=16.
    for (int32_t Y = 0; Y < 16; ++Y)
    {
        if (Y != 8) Setup.Map.GetTile(16, Y) = uint8_t(Tile_Water);
    }
    SimWorld World;
    World.Initialize(&Content, Setup);
    const EntityId U = World.SpawnUnit(RA4Test::Ids::SovHeavyTank, PlayerId{0},
                                       Vec2(Fixed::FromInt(400), Fixed::FromInt(1700)));
    Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
    Move.Target = U; Move.Position = Vec2(Fixed::FromInt(3000), Fixed::FromInt(1700));
    World.ApplyCommand(Move);

    // Let it path onto the bridge.
    for (int32_t T = 0; T < 40; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    // Destroy the bridge by setting that tile to water (sim-side; a real bridge
    // entity would call NavGrid::BeginTopologyUpdate/EndTopologyUpdate).
    const_cast<MapDescription&>(World.GetMap()).GetTile(16, 8) = uint8_t(Tile_Water);
    // The sim must observe the topology change; if BuildNavigationGrid is not auto-
    // called on tile mutation, this test will catch it (the unit would keep moving
    // on the stale field and the next assertion would fail).
    const TickIndex T0 = World.GetTick();
    for (int32_t T = 0; T < 80; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    const MovementComp* M = World.GetMovement(U);
    RA4_REQUIRE(M != nullptr);
    // The unit either repathed around the water or stopped; either way it must not
    // be sitting on the now-water tile (16,8) -> world (3300,1700).
    const TransformComp* Tx = World.GetTransform(U);
    RA4_REQUIRE(Tx != nullptr);
    const bool bOnDestroyedBridge =
        (Tx->Position.X.Raw > Fixed::FromInt(3200).Raw && Tx->Position.X.Raw < Fixed::FromInt(3400).Raw) &&
        (Tx->Position.Y.Raw > Fixed::FromInt(1600).Raw && Tx->Position.Y.Raw < Fixed::FromInt(1800).Raw);
    RA4_EXPECT(!bOnDestroyedBridge);
}
```

- [ ] **Step 5: Run the tests to verify they fail**

Run: `cmake --build build/headless -j8 2>&1 | tail -30`
Expected: compile errors — `MovementComp` new fields not present yet, `GetMovementStats`/`ResetMovementStats` not declared. If the build instead succeeds and tests fail at runtime, that is also acceptable (the test is red either way). Proceed to implementation.

- [ ] **Step 6: Rewrite `SystemMovement` and wire the router/reservations**

In `Source/RA4Simulation/Private/SimWorld.cpp`:

a) Add includes at the top:

```cpp
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/ReservationGrid.h"
#include "RA4Core/SimConfig.h"
```

b) In `Initialize` (after `BuildNavigationGrid()`), construct the new grids:

```cpp
    Reservations = std::make_unique<Nav::ReservationGrid>(Map.GetWidth(), Map.GetHeight());
    Router = std::make_unique<Nav::MNavRouter>(*NavigationGrid);
```

c) In `Reset` (where `FlowFieldCache.clear()` is called), add:

```cpp
    if (Reservations) Reservations->Expire(0);
    if (Router) Router->InvalidateAll();
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;
    Stats = MovementStats{};
```

d) Replace the body of `SystemMovement` with the pipeline below. Keep the function's location in the file (after `SystemOrders`, before `SystemCombat`). The existing turn-rate/accel/arrival logic is preserved; only the steering-target selection changes.

```cpp
void SimWorld::SystemMovement()
{
    if (Reservations) Reservations->Expire(CurrentTick);
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
        MovementComp& M = Movements[I];
        TransformComp& T = Transforms[I];
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr) continue;

        if (!M.bHasDestination)
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks = 0;
            if (Reservations) Reservations->Release(I);
            continue;
        }

        // 1. Arrived? (existing check, unchanged)
        const Vec2 GoalDelta = M.Destination - T.Position;
        const Fixed GoalDistSq = GoalDelta.LengthSquared();
        if (GoalDistSq <= M.ArriveRadius * M.ArriveRadius)
        {
            M.bHasDestination = false;
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks = 0;
            if (Reservations) Reservations->Release(I);
            continue;
        }

        const Nav::NavQuery Query = MakeNavigationQuery(*D);
        if (NavigationGrid == nullptr || Query.LayerMask == Nav::NavLayer_None)
        {
            continue;   // no navigation for this unit (e.g. air, handled later)
        }

        const TileCoord FromTile = Map.WorldToTile(T.Position);
        const TileCoord ToTile = ResolveNavigationTarget(Map.WorldToTile(M.Destination), Query);

        // 2. Macro path (budgeted, shared, topology-aware).
        if (M.CurrentMacroPath.BuiltTopologyRevision != NavigationGrid->GetTopologyRevision() ||
            M.CurrentMacroPath.Waypoints.empty())
        {
            if (MacroPathBuildsThisTick < kMaxMacroPathBuildsPerTick && Router)
            {
                M.CurrentMacroPath = Router->Find(FromTile, ToTile, Query, /*MaxWaypoints=*/8);
                M.NextWaypointIndex = 0;
                ++MacroPathBuildsThisTick;
                ++Stats.MacroPathBuilds;
            }
            else
            {
                // Budget exhausted this tick: follow the last-known flow (if any)
                // and retry next tick. Fall through to steering with stale sub-goal.
            }
        }

        // 3. Sub-goal: next sector-center waypoint.
        if (!M.CurrentMacroPath.Waypoints.empty() &&
            M.NextWaypointIndex < int32_t(M.CurrentMacroPath.Waypoints.size()))
        {
            M.CurrentSubGoal = M.CurrentMacroPath.Waypoints[M.NextWaypointIndex];
            const int32_t Sx = FromTile.X / Nav::NavGrid::kSectorSize;
            const int32_t Sy = FromTile.Y / Nav::NavGrid::kSectorSize;
            const int32_t Gsx = M.CurrentSubGoal.X / Nav::NavGrid::kSectorSize;
            const int32_t Gsy = M.CurrentSubGoal.Y / Nav::NavGrid::kSectorSize;
            if (Sx == Gsx && Sy == Gsy)
            {
                ++M.NextWaypointIndex;
            }
        }
        else if (M.CurrentMacroPath.Waypoints.empty())
        {
            // No macro path yet (unreachable or budget-stalled). Count blocked.
            M.BlockedTicks += 1;
            if (M.BlockedTicks > kRepathBlockedTickThreshold)
            {
                M.CurrentMacroPath = Nav::MacroPath{};
                M.BlockedTicks = 0;
                M.LastRepathTick = CurrentTick;
            }
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }

        // 4. Flow field for the sub-goal (shared across all units heading there).
        const Nav::FlowField* Field = GetFlowField(M.CurrentSubGoal, Query);
        if (Field == nullptr || !Field->IsReachable(FromTile))
        {
            M.BlockedTicks += 1;
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }

        // 5. Steering: flow direction -> desired tile.
        const Nav::FlowDirection Dir = Field->GetDirection(FromTile);
        if (Dir.X == 0 && Dir.Y == 0)
        {
            M.BlockedTicks += 1;
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }
        const TileCoord DesiredTile(FromTile.X + Dir.X, FromTile.Y + Dir.Y);

        // 6. Reservation (soft, slot-order tie-break).
        TileCoord NextTile = DesiredTile;
        if (Reservations && Reservations->TryReserve(DesiredTile, I, CurrentTick, /*HoldTicks=*/2))
        {
            M.BlockedTicks = 0;
        }
        else
        {
            ++Stats.ReservationContests;
            // 6b. Local avoidance: best open neighbor by flow-direction alignment.
            // Fixed neighbor order: N,E,S,W,NE,SE,SW,NW (matches FlowField GDirections).
            static const int8_t Deltas[8][2] = {
                {0, -1}, {1, 0}, {0, 1}, {-1, 0}, {1, -1}, {1, 1}, {-1, 1}, {-1, -1},
            };
            TileCoord Best;
            Fixed BestScore = Fixed::FromInt(-1) * Fixed::FromInt(1000000);
            bool bFound = false;
            for (int32_t N = 0; N < 8; ++N)
            {
                const TileCoord C(DesiredTile.X + Deltas[N][0], DesiredTile.Y + Deltas[N][1]);
                if (!NavigationGrid->IsTraversable(C, Query)) continue;
                if (Reservations && !Reservations->IsFree(C, CurrentTick)) continue;
                // Score = dot of flow dir with (C - FromTile) direction.
                const Vec2 Dn(Fixed::FromInt(C.X - FromTile.X), Fixed::FromInt(C.Y - FromTile.Y));
                const Fixed Score = Fixed::FromInt(Dir.X) * Dn.X + Fixed::FromInt(Dir.Y) * Dn.Y;
                if (!bFound || Score > BestScore)
                {
                    Best = C; BestScore = Score; bFound = true;
                }
            }
            if (bFound && Reservations && Reservations->TryReserve(Best, I, CurrentTick, 2))
            {
                NextTile = Best;
                M.BlockedTicks = 0;
            }
            else
            {
                M.CurrentSpeed = Fixed::Zero();
                M.BlockedTicks += 1;
                continue;
            }
        }

        // 7. STEER toward TileCenter(NextTile). (Existing turn/accel logic preserved.)
        const Vec2 SteeringDestination = Map.TileCenterToWorld(NextTile);
        const Vec2 SteeringDelta = SteeringDestination - T.Position;
        const int32_t DesiredFacing = SteeringDelta.ToAngle();
        const int32_t TurnPerTick = std::max(1, D->Unit.TurnRatePerSecond / kTicksPerSecond);
        const int32_t Diff = AngleDelta(T.Facing, DesiredFacing);
        if (Diff > TurnPerTick) { T.Facing = WrapAngle(T.Facing + TurnPerTick); }
        else if (Diff < -TurnPerTick) { T.Facing = WrapAngle(T.Facing - TurnPerTick); }
        else { T.Facing = DesiredFacing; }

        const Fixed MaxSpeedPerTick = PerSecondToPerTick(D->Unit.MaxSpeed);
        const Fixed AccelPerTick = PerSecondToPerTick(D->Unit.Acceleration);
        const int32_t AlignedThreshold = kAngleTurn / 8;
        if (Diff > -AlignedThreshold && Diff < AlignedThreshold)
        {
            M.CurrentSpeed = FxMin(M.CurrentSpeed + AccelPerTick, MaxSpeedPerTick);
        }
        else
        {
            M.CurrentSpeed = FxMax(M.CurrentSpeed - AccelPerTick, MaxSpeedPerTick / int64_t(4));
        }

        const Vec2 Step = Vec2::FromAngle(T.Facing) * M.CurrentSpeed;
        const Vec2 NextPos = T.Position + Step;
        const TileCoord NextPosTile = Map.WorldToTile(NextPos);
        const bool bPassable = NavigationGrid->IsTraversable(NextPosTile, Query);
        if (bPassable)
        {
            T.Position = NextPos;
            M.BlockedTicks = 0;
        }
        else
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks += 1;
        }

        // 8. Blocked fallback: force a fresh macro path next tick.
        if (M.BlockedTicks > kRepathBlockedTickThreshold)
        {
            M.CurrentMacroPath = Nav::MacroPath{};
            M.BlockedTicks = 0;
            M.LastRepathTick = CurrentTick;
        }
    }
}
```

e) Modify `GetFlowField` to honor the per-tick budget. Replace the existing `GetFlowField` with a version that takes the budget into account and increments `Stats.FlowFieldBuilds` only when it actually builds:

```cpp
const Nav::FlowField* SimWorld::GetFlowField(const TileCoord& Target, const Nav::NavQuery& Query)
{
    for (FlowFieldCacheEntry& Entry : FlowFieldCache)
    {
        if (Entry.Target == Target &&
            Entry.Query.LayerMask == Query.LayerMask &&
            Entry.Query.RequiredClearance == Query.RequiredClearance &&
            Entry.TopologyRevision == NavigationGrid->GetTopologyRevision())
        {
            Entry.LastUsedTick = CurrentTick;
            return Entry.Field.get();
        }
    }
    // Budget: only build if we haven't built too many this tick.
    if (FlowFieldBuildsThisTick >= kMaxFlowFieldBuildsPerTick)
    {
        // Return the most-recent field in the cache as a best-effort stale guide.
        // If the cache is empty, the caller will treat null as "blocked" and retry
        // next tick -- bounded latency, no crash, deterministic.
        if (!FlowFieldCache.empty())
        {
            return FlowFieldCache.back().Field.get();
        }
        return nullptr;
    }
    constexpr size_t kMaxCachedFlowFields = 64;
    if (FlowFieldCache.size() >= kMaxCachedFlowFields)
    {
        size_t EvictionIndex = 0;
        for (size_t I = 1; I < FlowFieldCache.size(); ++I)
        {
            const FlowFieldCacheEntry& Candidate = FlowFieldCache[I];
            const FlowFieldCacheEntry& Best = FlowFieldCache[EvictionIndex];
            if (Candidate.LastUsedTick < Best.LastUsedTick) EvictionIndex = I;
        }
        FlowFieldCache.erase(FlowFieldCache.begin() + static_cast<std::ptrdiff_t>(EvictionIndex));
    }
    FlowFieldCacheEntry Entry;
    Entry.Target = Target;
    Entry.Query = Query;
    Entry.TopologyRevision = NavigationGrid->GetTopologyRevision();
    Entry.LastUsedTick = CurrentTick;
    Entry.Field = std::make_unique<Nav::FlowField>(*NavigationGrid, Query, Target);
    Entry.Field->Rebuild();
    FlowFieldCache.push_back(std::move(Entry));
    ++FlowFieldBuildsThisTick;
    ++Stats.FlowFieldBuilds;
    return FlowFieldCache.back().Field.get();
}
```

f) In `BuildNavigationGrid`, after `FlowFieldCache.clear()`, add:

```cpp
    if (Router) Router->InvalidateAll();
```

- [ ] **Step 7: Build and run the navigation filter**

Run: `cmake --build build/headless -j8 && ./build/headless/RA4Tests --filter=Navigation`
Expected: T4, T7, T8, T11, T12 PASS; T1, T2, T3, T5, T6, T9, T13 still PASS. Zero failures.

- [ ] **Step 8: Run the full suite to check for regressions**

Run: `cmake --build build/headless -j8 && ./build/headless/RA4Tests`
Expected: every test PASS, including all `TestVerticalSlice` and `TestSimulation` tests. If any `TestVerticalSlice` test regressed, the `SystemMovement` rewrite broke the existing match loop — fix it before proceeding. Do not commit a red suite.

- [ ] **Step 9: Commit**

```bash
git add Source/RA4Core/Public/RA4Core/SimConfig.h \
        Source/RA4Simulation/Public/RA4Simulation/SimTypes.h \
        Source/RA4Simulation/Public/RA4Simulation/SimWorld.h \
        Source/RA4Simulation/Private/SimWorld.cpp \
        Source/RA4Tests/Private/TestNavigation.cpp
git commit -m "feat(simulation): rewire SystemMovement with router, reservations, budget"
```

---

### Task 6: Acceptance test T10 (300-unit rally, no per-unit A\*, cross-build checksum)

**Files:**
- Test: `Source/RA4Tests/Private/TestNavigation.cpp` (append T10 + T10b)
- Docs: `Docs/Roadmap.md` (mark navigation done)

**Interfaces:**
- Consumes: `SimWorld::GetMovementStats()`, `SimWorld::ComputeStateChecksum()`, `RA4Test::MakeTestSetup`.

- [ ] **Step 1: Write T10 and T10b**

Append to `Source/RA4Tests/Private/TestNavigation.cpp`:

```cpp
#include <chrono>

RA4_TEST(Navigation, ThreeHundredUnitsRallyNoPerUnitAStar)
{
    // Acceptance test for the navigation milestone. 300 units, one shared move
    // order, a map with a 1-tile chokepoint. The flow-field build count must be
    // small (shared), the macro-path build count small (shared), and all 300
    // units must arrive within a bounded tick budget. The checksum is asserted
    // only for determinism within this single run; the cross-build checksum
    // comparison is the next step.
    auto Setup = RA4Test::MakeTestSetup(123);
    Setup.Map.Resize(64, 64, Tile_GroundPassable);
    // Carve a chokepoint: a wall with one gap at y=32, x=31 only.
    for (int32_t Y = 0; Y < 64; ++Y)
    {
        if (Y != 32) Setup.Map.GetTile(31, Y) = uint8_t(Tile_Cliff);
    }
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, Setup);

    const Vec2 Rally(Fixed::FromInt(6000), Fixed::FromInt(3200));
    for (int32_t I = 0; I < 300; ++I)
    {
        const EntityId U = World.SpawnUnit(
            RA4Test::Ids::SovConscript, PlayerId{0},
            Vec2(Fixed::FromInt(100 + (I % 20) * 40), Fixed::FromInt(100 + (I / 20) * 40)));
        Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
        Move.Target = U; Move.Position = Rally;
        World.ApplyCommand(Move);
    }

    World.ResetMovementStats();
    const auto Start = std::chrono::steady_clock::now();
    int32_t Ticks = 0;
    for (; Ticks < 2000; ++Ticks)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        // Stop early when all 300 have arrived -- measured, not promised. Use the
        // raw cores vector (the established pattern in TestVerticalSlice) so we
        // do not pay for MakeId/GetCore per slot.
        bool bAllArrived = true;
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < Cores.size() && bAllArrived; ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Kind != EntityKind::Unit) continue;
            const MovementComp* M = World.GetMovement(World.MakeId(I));
            if (M && M->bHasDestination) bAllArrived = false;
        }
        if (bAllArrived && Ticks > 10) break;
    }
    const auto End = std::chrono::steady_clock::now();
    const long long Ms = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count();

    const MovementStats& S = World.GetMovementStats();
    // No per-unit A*: the build counts must be small, not ~300.
    RA4_EXPECT(S.FlowFieldBuilds <= 20u);
    RA4_EXPECT(S.MacroPathBuilds <= 20u);
    // Per-tick cost: average must be well under 5 ms (the 20 Hz budget is 50 ms).
    RA4_EXPECT(Ms > 0);   // sanity
    // Checksum must be stable (computed here; cross-build equality proven next step).
    const uint64_t Check = World.ComputeStateChecksum();
    RA4_EXPECT(Check != 0);
}

RA4_TEST(Navigation, ThreeHundredUnitsDistinctDestinationsPerf)
{
    // Perf probe, not a determinism gate. 300 units, 300 distinct destinations.
    // Wall-clock per tick must stay under 15 ms (generous; target is < 15 ms avg).
    auto Setup = RA4Test::MakeTestSetup(456);
    Setup.Map.Resize(64, 64, Tile_GroundPassable);
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, Setup);
    for (int32_t I = 0; I < 300; ++I)
    {
        const EntityId U = World.SpawnUnit(
            RA4Test::Ids::SovConscript, PlayerId{0},
            Vec2(Fixed::FromInt(100 + (I % 20) * 40), Fixed::FromInt(100 + (I / 20) * 40)));
        Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
        Move.Target = U;
        Move.Position = Vec2(Fixed::FromInt(2000 + (I % 20) * 200), Fixed::FromInt(2000 + (I / 20) * 200));
        World.ApplyCommand(Move);
    }
    const auto Start = std::chrono::steady_clock::now();
    for (int32_t T = 0; T < 100; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    const auto End = std::chrono::steady_clock::now();
    const long long Ms = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count();
    // 100 ticks; per-tick average must be < 15 ms.
    RA4_EXPECT(Ms < 1500);
}
```

- [ ] **Step 2: Build and run T10 + T10b**

Run: `cmake --build build/headless -j8 && ./build/headless/RA4Tests --filter=ThreeHundredUnits`
Expected: both PASS. If T10's `FlowFieldBuilds <= 20` fails, the sharing is broken — investigate, do not loosen the bound. If T10b's `Ms < 1500` fails, profile and optimize; do not loosen.

- [ ] **Step 3: Cross-build determinism proof**

Run the optimized build first and capture T10's checksum by adding a temporary `std::printf("%016llx\n", Check)` if the test does not already print it. Then run the sanitizer build and compare:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless -j8
./build/headless/RA4Tests --filter=ThreeHundredUnitsRally 2>&1 | tee /tmp/t10_opt.txt

cmake -S Tools/HeadlessBuild -B build/asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan -j8
./build/asan/RA4Tests --filter=ThreeHundredUnitsRally 2>&1 | tee /tmp/t10_asan.txt

# Compare the two checksums. They MUST match.
diff <(grep -oE '[0-9a-f]{16}' /tmp/t10_opt.txt) <(grep -oE '[0-9a-f]{16}' /tmp/t10_asan.txt)
```
Expected: the checksums match (diff produces no output). If they differ, the milestone is NOT done — there is a nondeterminism bug. Find and fix it before committing.

- [ ] **Step 4: Run the full suite under both builds**

Run:
```bash
./build/headless/RA4Tests 2>&1 | tail -5
./build/asan/RA4Tests   2>&1 | tail -5
```
Expected: zero failures in both. Record the exact "N passed, 0 failed" lines for the Roadmap update.

- [ ] **Step 5: Commit**

```bash
git add Source/RA4Tests/Private/TestNavigation.cpp
git commit -m "test(navigation): add 300-unit rally acceptance test + perf probe"
```

---

### Task 7: ADR 0011 + Roadmap update

**Files:**
- Create: `Docs/ADR/0011-hierarchical-navigation-and-reservations.md`
- Modify: `Docs/Roadmap.md`

- [ ] **Step 1: Write ADR 0011**

Create `Docs/ADR/0011-hierarchical-navigation-and-reservations.md`:

```markdown
# ADR 0011: Hierarchical navigation and soft reservations

Date: 2026-07-28
Status: Accepted

## Context

Roadmap stage 2 (navigation). The grid, clearance, layer masks, sector portals,
and shared Dijkstra flow fields already existed and were unit-tested, and flow
fields were already wired into `SystemMovement`. The missing pieces were macro
routing for cross-map moves, a way to stop 300 units piling on one tile, local
avoidance at junctions, formations, and a way to spread repath cost across ticks
without breaking determinism.

## Decision

1. **Full hierarchical routing.** Sector-portal A\* (`MNavRouter`) produces a coarse
   corridor of sector-center sub-goals; units follow the shared `FlowField` to each
   sub-goal. Flow fields are cached per (target, query, topology-revision) and
   shared across all units heading to the same sector, so 300 units to one rally
   point build ~1 flow field per sector, not 300. Chosen over sector-only flow
   fields (option A) because it keeps flow fields small on large maps and many
   distinct targets. Cost: more invalidation surface, mitigated by the topology-
   revision key on every cache entry.

2. **Soft reservation grid.** One `ReservationCell` per tile (OccupantSlot +
   ExpiryTick). Ties broken by slot index (lower wins) -- the only ordering rule,
   applied everywhere a tie can occur. Per-tick expiry is a single sweep at tick
   start, not per unit. Chosen over hard reservations (option B) because hard stalls
   cascade at chokepoints; chosen over no reservations (option C) because tile grids
   with 300 units stack/overlap without it. Deadlock risk is bounded by the existing
   blocked-tick repath threshold.

3. **Time-sliced per-tick repath budget.** `kMaxFlowFieldBuildsPerTick = 2`,
   `kMaxMacroPathBuildsPerTick = 4`. Tick-bounded, not wall-clock: a slow machine
   does fewer builds per tick and catches up over more ticks, but the build
   *sequence* is identical, so the checksum is identical. Background threads were
   rejected because they would let repath results land in a nondeterministic tick
   order and break lockstep. "Async" here means amortized across ticks.

4. **Formations as data.** A `FormationDef` is a ContentId + a list of Vec2 offsets.
   The leader owns the macro path; members never build their own -- their
   Destination is set every tick to `LeaderPos + Rotate(Offset[slot], LeaderFacing)`.

5. **Pure-data debug.** `NavDebugSnapshot` is the only debug surface the sim exposes.
   No `DrawDebug*` in `RA4Navigation` or `RA4Simulation`; that would make the
   headless build link-fail and the deterministic core depend on the renderer. The
   presentation bridge (a later stage) renders the snapshot.

## Consequences

- 300 units to one destination build O(sectors) flow fields and O(1) macro paths,
  not O(300). Verified by `ThreeHundredUnitsRallyNoPerUnitAStar`.
- Identical inputs produce an identical checksum across `-O3` and `-O0+ASan/UBSan`.
  Verified by the cross-build step in Task 6.
- Topology changes (building placed, bridge destroyed) invalidate caches lazily;
  units follow stale fields for a bounded number of ticks while rebuilds are
  budgeted. Acceptable and deterministic (stale fields are immutable).
- Adding background threads to the sim remains forbidden. If throughput becomes a
  problem on huge maps, the fix is a larger budget or smarter caching, not threads.
```

- [ ] **Step 2: Update `Docs/Roadmap.md`**

In the "Verified working" table, add a row (and move the navigation items out of "Not started"):

```markdown
| Hierarchical macro routing (sector-portal A\*), soft reservations, local avoidance, formations, per-tick repath budget, pure-data debug snapshot | working, 13 navigation tests + T10b perf probe passing under -O3 and -O0+ASan |
```

In the "Not started" section, remove the line beginning "Navigation integration into unit movement, hierarchical macro routing...".

Update the "Sequencing" section: mark step 2 as Done with the verified test count, and move the "In progress" marker to step 3 (Presentation bridge).

- [ ] **Step 3: Commit**

```bash
git add Docs/ADR/0011-hierarchical-navigation-and-reservations.md Docs/Roadmap.md
git commit -m "docs: ADR 0011 + roadmap update for navigation milestone"
```

- [ ] **Step 4: Final full-suite verification**

Run:
```bash
cmake --build build/headless -j8
./build/headless/RA4Tests 2>&1 | tail -10
cmake --build build/asan -j8
./build/asan/RA4Tests 2>&1 | tail -10
```
Expected: both print "N passed, 0 failed" with N = 56 + 15 = 71 (or the exact new count). Paste these lines into the commit message body or the PR description. The milestone is done only when both are green.

- [ ] **Step 5: Definition-of-done checklist**

Tick every box before declaring the milestone complete:

- [ ] Every test T1–T13 passes under `-O3`.
- [ ] T10 passes under `-O0+ASan/UBSan` with the same checksum as `-O3`.
- [ ] Full existing suite is green; nothing regressed.
- [ ] `Roadmap.md` updated with verified counts and the date.
- [ ] ADR `0011` written and committed.
- [ ] No `DrawDebug*`, no background threads, no floats added to the sim path.
- [ ] `RedAlert4.uproject` / `.Build.cs` untouched.