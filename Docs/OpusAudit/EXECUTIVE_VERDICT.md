# Opus Audit — Executive Verdict (Second Pass)

**Auditor**: Claude Fable 5 (independent, reproducible verification)
**Date**: 2026-08-05
**Branch**: `feat/soviet-asset-integration`
**Baseline**: Builds attempted from clean CMake configure on macOS arm64

---

## Overall Verdict: CONDITIONALLY PASS — Engine-Free Core Is Genuine; Build Is Broken by Untracked AI Header

The engine-free simulation core (RA4Core → RA4Content → RA4Simulation → RA4Navigation → RA4Combat → RA4FogOfWar → RA4AI → RA4Replay → RA4Campaign → RA4Input → RA4Presentation) is **architecturally sound and independently verified**. Fixed-point math, deterministic tick, command bus, state checksumming, replay, lockstep networking, flow-field pathfinding, fog of war, economy, combat, AI commander — all tested with behavioral regression tests that prove actual outcomes, not just function existence.

**The build is currently broken** by `Source/RA4AI/Public/RA4AI/AIDirectors.h`, an untracked header that declares `EconomyDirector`, `ScoutingDirector`, `DefenseDirector`, `OffenseDirector` and their methods without any corresponding `.cpp` implementation. `TestAI.cpp` includes this header and tests these classes, causing linker failures for both `RA4Tests` and `RA4AITests`.

---

## Reproducible Build Evidence

| Step | Command | Result |
|------|---------|--------|
| CMake configure | `cmake ..` in `Tools/HeadlessBuild/build/` | ✅ Succeeds (AppleClang 21.0.0) |
| Full build | `cmake --build . -j$(sysctl -n hw.ncpu)` | ❌ Linker error in RA4Tests and RA4AITests |
| RA4InputTests | Separate build target | ✅ Links successfully |
| RA4PresentationTests | Separate build target | ✅ Links successfully |
| RA4MatchDump | Separate build target | ✅ Links successfully |

**Linker errors**: 13 undefined symbols from `RA4::AI::` namespace, all from AIDirectors.h declarations.

---

## System-by-System Verdicts (Updated)

| System | Verdict | Evidence |
|--------|---------|----------|
| Fixed-point math (48.16, 128-bit intermediate) | **ACCEPT** | Source verified: `Fixed.h:37-65`, cross-platform portable path |
| Entity model (SoA, slot+generation) | **ACCEPT** | `SimWorld.cpp:152-197`, deterministic recycling with generation bump |
| Command model (16 types, 14 rejection reasons) | **ACCEPT_WITH_FIXES** | `Command.h:20-45`, missing UpgradeBuilding, DeployMCV, SetStance |
| CommandBus validation | **ACCEPT** | `SimWorld.cpp:743-1080`, ownership/liveness/affordability/tech/placement/rate-limit |
| Determinism (checksum) | **ACCEPT** | `SimWorld.cpp:2657-2729`, feeds all state including RNG |
| Replay (record/checksum/verify) | **ACCEPT** | `TestVerticalSlice.cpp` replay tests prove round-trip and corruption rejection |
| Lockstep networking | **ACCEPT_WITH_FIXES** | `TestNetwork.cpp` 13 tests, but no reconnect/spectators |
| Navigation (FlowField + NavGrid + MNavRouter) | **ACCEPT_WITH_FIXES** | Multi-layer, reservation grid. No 500+ unit stress test |
| Fog of War | **ACCEPT** | `SimWorld.cpp:2556-2593`, per-player reveal, combat respects visibility |
| AI commander | **ACCEPT_WITH_FIXES** | 40+ tests including 5-match suite and AI-vs-AI acceptance. Build broken by AIDirectors |
| Economy | **ACCEPT** | Harvester loop, finite fields, refinery queuing, power degradation |
| Combat | **ACCEPT** | Armor matrix, splash, turret tracking, projectile scatter |
| Production | **ACCEPT** | Pay→build→place (structures), queue+spawn+rally (units), cancel refunds |
| Content database | **ACCEPT** | JSON bible loading, validation catches errors, hash detects balance changes |
| Test quality | **ACCEPT_WITH_FIXES** | ~300 tests, behavioral regression. AIDirectors tests break build |
| Packaged build | **MISSING** | No Shipping build script, no CI job, no .exe/.app |
| Localization | **MISSING** | `Content/Localization/Game/{en,ru}` directories exist but appear stubbed |
| Audio pipeline | **MISSING** | `Content/Audio/` has wav files but no UE integration verified |
| Telemetry/crash reporting | **MISSING** | Not implemented |
| Campaign missions | **MISSING** | Framework exists (`TestMissionRuntime.cpp`), no authored missions |
| ThirdParty licensing | **FAIL** | 77% of uassets are marketplace packs, no license files in repo |
| "Red Alert 4" trademark | **FAIL** | Project/module names use EA trademark, migration not executed |

---

## Gemini Milestone Claims (Re-Verified)

| Milestone | Gemini Claim | Actual | Verdict |
|-----------|-------------|--------|---------|
| Architecture Baseline | Complete | Engine-free core genuinely solid | **PASS** |
| Industrial Vertical Slice | Complete | Headless vertical slice test exists, deterministic | **CONDITIONAL_PASS** |
| Systems Complete | Complete | Core systems functional, UE integration unverified | **CONDITIONAL_PASS** |
| Content Complete | Complete | Default test content only, no authored art/balance | **FAIL** |
| Feature Complete | Complete | Many features stubbed or missing in UE layer | **FAIL** |
| Alpha | Complete | No packaged build | **FAIL** |
| Beta | Complete | No packaged build, no visual verification | **FAIL** |
| Release Candidate | Complete | No build pipeline | **FAIL** |
| Gold Master | Complete | Fictitious | **FAIL** |
| Launch Readiness | Complete | Fictitious | **FAIL** |

---

## Critical Findings (New This Audit)

1. **Build broken by AIDirectors.h** — untracked header declares classes without implementations; TestAI.cpp references them; linker fails
2. **26 untracked files** in working tree including tools, screenshots, and loose scripts
3. **Git tags v0.6.0–v1.1.0** all point to documentation-only commits with no build artifacts
4. **No .editorconfig or .clang-format** at repository root despite -Werror build
5. **`Content/` at root level** has 5934 files including ThirdParty marketplace assets

---

## What Is Genuinely Good

- The simulation core is industrial-grade. Fixed-point with 128-bit intermediates, PCG-XSH-RR RNG, SoA entity storage, deferred destruction, flow-field pathfinding with reservation grid, 13-system tick ordering, state checksumming.
- The test framework is custom but well-designed: behavioral regression tests that check actual outcomes, not mocks. Content IDs intentionally duplicated to catch silent breakage.
- The command model is correct: all state changes flow through `ApplyCommand` with 14 rejection reasons. Rate limiting at 64 commands/player/tick.
- AI determinism is verified: same seed produces identical checksum and decision log.

---

## What Must Be Fixed

1. **AIDirectors build break** — implement the Director classes or remove the header from CMake
2. **Packaged build** — no Shipping configuration exists
3. **ThirdParty licensing** — legal blocker
4. **IP migration** — "Red Alert 4" name must change
5. **Localization** — stubbed, not functional
6. **Audio integration** — wav files exist, no UE pipeline
7. **Campaign missions** — framework only, no content
