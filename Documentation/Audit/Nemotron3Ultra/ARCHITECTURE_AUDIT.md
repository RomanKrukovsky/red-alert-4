# RedAlert4 — Architecture Audit

**Audit Date:** 2026-07-30  
**Scope:** All C++ modules, build system, module dependencies, determinism guarantees

---

## Module Dependency Graph (Actual vs Intended)

### Intended Layering (from `RedAlert4.uproject` + `Build.cs` files)
```
Unreal Engine (CoreUObject, Engine, etc.)
    │
    ├── RA4Editor (Editor-only)
    │
    ├── RA4UI (Runtime, depends on CommonUI, MVVM, EnhancedInput)
    │
    ├── RA4Network (Runtime, depends on CoreUObject)
    │
    ├── RA4Campaign (Runtime)
    │
    ├── RA4AI (Runtime)
    │
    ├── RA4Presentation (Runtime)
    │
    ├── RA4Input (Runtime, depends on EnhancedInput)
    │
    ├── RA4FogOfWar (Runtime)
    │
    ├── RA4Navigation (Runtime)
    │
    ├── RA4Combat (Runtime)
    │
    ├── RA4Replay (Runtime)
    │
    ├── RA4Simulation (Runtime, **NO UNREAL DEPS**)
    │
    ├── RA4Content (Runtime, **NO UNREAL DEPS**)
    │
    └── RA4Core (Runtime, **NO UNREAL DEPS**)
```

### Actual Dependencies (from `Build.cs` + header includes)

| Module | Declared Deps | Actual Unreal Deps in Headers | Violations |
|--------|---------------|-------------------------------|------------|
| RA4Core | (none) | **NONE** ✅ | — |
| RA4Content | RA4Core | **NONE** ✅ | — |
| RA4Simulation | RA4Core, RA4Content, RA4Navigation | **NONE** ✅ | — |
| RA4Navigation | RA4Core | **NONE** ✅ | — |
| RA4Replay | RA4Core, RA4Simulation | **NONE** ✅ | — |
| RA4Combat | RA4Core | **NONE** ✅ | — |
| RA4FogOfWar | RA4Core | **NONE** ✅ | — |
| RA4AI | RA4Core, RA4Simulation, RA4Navigation | **NONE** ✅ | — |
| RA4Network | CoreUObject | `WorldSubsystem` only ✅ | — |
| RA4Input | EnhancedInput | `EnhancedInputComponent` ✅ | — |
| RA4Presentation | Engine, CoreUObject | `Niagara`, `AnimInstance` — **in presentation only** ✅ | — |
| RA4UI | CommonUI, MVVM, EnhancedInput | `UUserWidget`, `Mvvm` — **in UI only** ✅ | — |
| RA4Editor | UnrealEd, RA4Simulation, RA4Content | `Commandlet` — **editor only** ✅ | — |
| RedAlert4 (main) | Engine, CoreUObject | GameMode, PlayerController, CameraPawn — **game glue only** ✅ | — |

**✅ No layering violations found.** The simulation core (RA4Core, RA4Content, RA4Simulation, RA4Navigation, RA4Replay, RA4Combat, RA4FogOfWar) has **zero Unreal Engine header dependencies**. This is a genuine achievement.

---

## Circular Dependency Check

**None detected.** Module dependency graph is a clean DAG. Verified by:
- CMake `target_link_libraries` order builds correctly
- No header includes flow upward from simulation to presentation
- `RA4Simulation` includes `RA4Navigation` headers but not vice versa

---

## Determinism Guarantees — Architecture Level

### ✅ Guaranteed by Design
| Mechanism | Location | Verification |
|-----------|----------|--------------|
| Fixed-point math (48.16) | `RA4Core/Fixed.h` | `__int128` widening + portable fallback |
| Generation-handled EntityIds | `RA4Core/Ids.h:13-31` | Slot recycle cannot retarget stale orders |
| Ordered `std::map` for CommandBus frames | `CommandBus.h:45` | Deterministic iteration vs `unordered_map` |
| Fixed tick system order | `SimWorld.h:145-158` | 14 systems, no dynamic registration |
| RNG per-simulation, seeded | `SimWorld.h:137`, `Random.h` | `Xoshiro256++`, reproducible |
| State checksum excludes caches | `SimWorld.h:128` | Events, flow field cache excluded |
| Replay format versioned | `Replay.h:22` | `kReplayFormatVersion=1`, magic `0x34414952` |

### ⚠️ Risks to Determinism
| Risk | Location | Severity | Mitigation |
|------|----------|----------|------------|
| `ContentDatabase` uses `unordered_map` for lookup indices | `ContentDatabase.h:58-62` | **HIGH** | `ComputeContentHash()` iterates `unordered_map` — order differs between libstdc++/libc++. **Content hash will diverge across platforms.** |
| `ToDoubleUnsafe()` exists in `Fixed` | `Fixed.h:110` | MEDIUM | Named "Unsafe", only for logging. Audit all call sites. |
| `NavigationGrid` full rebuild on building placement | `SimWorld.cpp:399-414` | MEDIUM | O(WH) per placement. Use dirty-rect incremental update. |
| `FlowFieldCache` LRU eviction uses `AccessSerial` | `SimWorld.cpp:526-536` | LOW | Deterministic if serial counter is per-tick. Verified. |
| `std::sort` in `RefreshPlayerTech` | `SimWorld.cpp:612` | LOW | Sorts `ContentId` (uint32) — stable across platforms. |

### ❌ Content Hash Non-Determinism — **Critical Blocker for Cross-Platform Lockstep**
```cpp
// ContentDatabase.h:58-62
std::unordered_map<uint32_t, size_t> EntityIndex;
std::unordered_map<uint32_t, size_t> WeaponIndex;
// ...
uint64_t ComputeContentHash() const {
    // Iterates unordered_map → ORDER UNDEFINED → HASH DIVERGES
}
```
**Fix required:** Replace with `std::map` or sort keys before hashing. This breaks cross-platform multiplayer and replay verification on non-identical standard libraries.

---

## Technical Debt Inventory

### Critical (Blocks shipping or causes silent correctness bugs

| ID | Location | Issue | Effort |
|----|----------|-------|--------|
| ARCH-001 | `ContentDatabase.h:58-62` | `unordered_map` iteration order non-deterministic → content hash diverges | S (1 day) |
| ARCH-002 | `DefaultContent.cpp` | Only 2/4 factions, 10/78 units, damage matrix 7/64 entries | L (weeks — content) |
| ARCH-003 | `BibleContentLoader.cpp` | Expects `RA4_Bible_Normalized.json` — **file missing from repo** | M (1 week pipeline) |
| ARCH-004 | `SimWorld.cpp:399` | Full navigation grid rebuild on every building placement | M (2 days) |

### Major (Degrades maintainability/performance)

| ID | Location | Issue | Effort |
|----|----------|-------|--------|
| ARCH-005 | `SimWorld.cpp:551-549` | Flow field cache capped at 64 entries, LRU eviction | S (tuning) |
| ARCH-006 | `CommandBus.h:45` | `std::map<TickIndex, CommandFrame>` — O(log N) per frame, acceptable but could be ring buffer | S |
| ARCH-007 | `SimWorld.h:236` | `kMaxCommandsPerPlayerPerTick = 64` hardcoded — should be config | S |
| ARCH-008 | `SimConfig.h` | `kMaxEntities` not visible — entity budget opaque | S |
| ARCH-009 | `DefaultContent.cpp:45` | FactionSetup struct duplicates data for Soviet/Alliance — not data-driven | M (refactor to Data Assets) |

### Minor (Code quality)

| ID | Location | Issue |
|----|----------|-------|
| ARCH-010 | `Fixed.h:142-143` | `FxSin`/`FxCos` declared `RA4CORE_API` but no `.cpp` visible — check linkage |
| ARCH-011 | `Ids.h:59-68` | `HashName` FNV-1a — good, but `constexpr` only works for literals |
| ARCH-012 | `Command.h:71-87` | `Serialize`/`Deserialize` manual — consider generated serialization |

---

## Build System Assessment

### CMake Configuration (`build/CMakeCache.txt`)
- **Generator:** Ninja
- **Build types:** Debug, Development, Shipping (inferred)
- **Sanitizers:** ASan builds exist (`build/asan/`, `build/hb-asan/`)
- **Tests:** CTest enabled (`build/Testing/`), `RA4Tests` executable registered

### Unreal Build Tool Integration
- Modules compiled as **static libraries** (`libRA4Core.a`, etc.) via CMake
- **Not using UBT** — this is a custom CMake build that mimics UBT module structure
- **Risk:** Unreal plugins (GameplayAbilities, CommonUI, MVVM, EnhancedInput) are declared in `.uproject` but **not linked in CMake**. Editor build will fail without them.

### Missing Build Verification
| Check | Status |
|-------|--------|
| Editor builds (`RedAlert4Editor`) | ❌ Not tested |
| Shipping build (optimizations, no asserts) | ❌ Not tested |
| Plugin linkage (GameplayAbilities, CommonUI, MVVM) | ❌ Not in CMake |
| Cooking / packaging | ❌ Not tested |
| iOS / Android / Linux targets | ❌ Not configured |

---

## Security & Supply Chain

| Check | Result |
|-------|--------|
| Hardcoded secrets in code | ✅ None found |
| Third-party deps | Minimal: only Unreal Engine + STL |
| `Random.h` seed source | Deterministic (explicit seed) — **not crypto** |
| Serialization bounds checking | `ByteReader::HasError()` checked in `CommandFrame::Deserialize` ✅ |
| Command rate limiting | 64 cmds/player/tick enforced ✅ |

---

## Recommendations (Architecture)

1. **IMMEDIATE:** Fix `ContentDatabase` content hash — replace `unordered_map` with `std::map` or sorted vector before any cross-platform testing.
2. **IMMEDIATE:** Add `RA4_Bible_Normalized.json` to repo or document pipeline to generate it from markdown.
3. **SHORT TERM:** Migrate `DefaultContent.cpp` → Data Assets (Primary Data Assets per unit/building). The `FactionSetup` pattern proves data-driven design is intended.
4. **SHORT TERM:** Incremental navigation grid updates (dirty rects) — 500+ entities will stall on full rebuild.
5. **MEDIUM:** Adopt UBT for Unreal modules, keep CMake only for headless simulation libs. Hybrid build is fragile.
6. **MEDIUM:** Add `ContentDatabase::Validate()` call in `SimWorld::Initialize()` — catch authoring errors at match start.
7. **LONG TERM:** ECS archetype migration (current SoA is fixed-schema). Current design supports ~20 components; adding faction-unique components will require array growth.

---

*End of Architecture Audit*