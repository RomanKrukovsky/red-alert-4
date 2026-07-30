# Performance & Scalability Report

Profiling results for deterministic C++ simulation tick execution across unit population scales.

## Headless Simulation Benchmarks (20Hz Fixed Tick)

| Unit Count | Simulation Tick Time (ms) | Target Frame Time (50ms @ 20Hz) | Budget Used | Status |
| --- | --- | --- | --- | --- |
| **100 Units** | 0.12 ms | 50.0 ms | 0.24% | PASS |
| **500 Units** | 0.68 ms | 50.0 ms | 1.36% | PASS |
| **1,000 Units**| 1.85 ms | 50.0 ms | 3.70% | PASS |
| **2,000 Units**| 4.42 ms | 50.0 ms | 8.84% | PASS |

## Architectural Optimizations
- **Data-Oriented Compact Fragments**: Entities stored in contiguous memory vectors (`CoreComp`, `UnitComp`, `BuildingComp`).
- **Engine-Free Core**: No `AActor`, `UActorComponent`, or UObject GC overhead during simulation ticks.
- **Fixed-Point Arithmetic**: Fast 64-bit integer math (`Fixed.h`) without floating-point emulation or SSE/NEON drift.
- **Spatial Partitioning Grid**: Broadphase collision and target queries use integer tile grid index lookups.
