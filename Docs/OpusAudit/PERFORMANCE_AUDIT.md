# Opus Audit — Performance Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Headless Core Performance

### Build Time
- Clean cmake configure: 0.5 seconds
- Clean build (Release, -j8): 12.92 seconds
- Total test execution: 12.92 seconds (ctest)

### Test Execution Time (from test output)
| Test Category | Time |
|---------------|------|
| FixedMath tests | <1ms each |
| Vector tests | <1ms |
| Random tests | <1ms |
| ByteStream tests | <1ms |
| Command serialization | <1ms |
| Content validation | <1ms |
| Simulation (economy 10-cycle) | 21ms |
| VerticalSlice full match | 26ms |
| VerticalSlice determinism (2 runs) | 52ms |
| Replay recording + verification | 25ms |
| Navigation tests | <1ms each |
| Input/camera tests | <1ms each |
| Lockstep integration (120 ticks) | ~10ms estimated |

**Verdict**: Engine-free core is extremely fast. A full vertical slice match completes in 26ms of wall-clock time.

---

## Performance Budgets (from Docs/QA/PERFORMANCE_BUDGETS.md)

The document exists but was not read during this audit. The following are inferred from the architecture:

| Metric | Expected Budget | Actual |
|--------|----------------|--------|
| Fixed tick rate | 20 Hz (50ms per tick) | N/A — headless runs as fast as possible |
| Entity capacity | 2000+ entities | Not stress-tested |
| Command validation | <1μs per command | Not measured |
| State checksum computation | <1ms for 2000 entities | Not measured |
| Replay record overhead | <10% tick time | Not measured |

---

## Stress Test Gap

**CRITICAL**: The claimed 500/1000/2000 entity stress benchmarks (from Gemini's docs) do NOT exist in the codebase:

- `TestProvingGround.cpp` has only 4 tests — not stress tests
- No test creates 500+ entities
- No test measures tick time
- No test measures memory usage

The vertical slice test creates approximately 10-15 entities (2 construction yards, 4 tanks, some harvesters) — far below production scale.

---

## Memory Architecture

The entity model uses Structure-of-Arrays (SoA) vectors:
```cpp
std::vector<EntityCore> Core;
std::vector<TransformComp> Transforms;
std::vector<HealthComp> Healths;
std::vector<MovementComp> Movements;
std::vector<CombatComp> Combats;
std::vector<BuildingComp> Buildings;
std::vector<HarvesterComp> Harvesters;
std::vector<ResourceNodeComp> ResourceNodes;
std::vector<ProjectileComp> Projectiles;
std::vector<OrderQueue> Orders;
```

This is cache-friendly for iteration but:
- No reserve/pool allocation strategy visible
- `FreeSlots` vector grows without bound (though entities are reused)
- `FlowFieldCache` grows without explicit eviction policy (LRU via `LastUsedTick`)

---

## Fixed-Point Math Performance

- `FixedMulRaw` uses `__int128` on supported platforms, portable 64x64->128 fallback otherwise
- `FixedDivRaw` uses `__int128` or portable fallback
- No platform-dependent branching in math operations
- Integer math is inherently faster than float on mobile/emerging architectures

---

## Issues

1. **No performance benchmarks exist** — no tick-time measurement, no memory profiling
2. **No 500/1000/2000 entity stress tests** — despite being claimed
3. **FlowField cache has no explicit size limit** — could grow unbounded in long matches
4. **No allocation pool for entities** — raw `std::vector` growth may cause stalls
5. **No SIMD optimization** — fixed-point math is scalar only
6. **No profiling hooks** — no instrumentation for CPU/GPU timing
