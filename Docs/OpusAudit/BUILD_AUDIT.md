# Opus Audit — Build Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Build Environment

| Item | Value |
|------|-------|
| OS | macOS (Darwin 25.3.0, arm64) |
| CMake | 4.1.1 |
| Compiler | Apple Clang 21.0.0 (clang-2100.0.123.102) |
| UE install | `/Users/Shared/Epic Games/UE_5.6` and `UE_5.8` |

---

## Headless Build (Engine-Free Core)

### Configuration
```
cmake -S Tools/HeadlessBuild -B build/hb -DCMAKE_BUILD_TYPE=Release
```

### Result
- **Status**: PASS — clean build, zero errors
- **Duration**: 12.92 seconds (parallel -j8)
- **Warning**: `ld: warning: ignoring duplicate libraries: 'libRA4FogOfWar.a'` — harmless duplicate link

### Libraries compiled (13 static libraries)
RA4Core, RA4Content, RA4FogOfWar, RA4Navigation, RA4Combat, RA4Simulation, RA4AI, RA4Replay, RA4Input, RA4Presentation, RA4Campaign, RA4UI (headers only), plus UnrealStub

### Test executables
| Executable | Tests | Duration |
|-----------|-------|----------|
| RA4Tests (core) | 127 | 7.64s |
| RA4InputTests | Subset | 0.40s |
| RA4PresentationTests | Subset | 0.36s |
| RA4AITests | Subset | 4.53s |
| **Total (ctest)** | **All pass** | **12.92s** |

### Cross-Platform Determinism
CI extracts `VerticalSlice.FullMatch` checksum on Ubuntu, macOS, and Windows, then compares in a separate job. This is the correct approach for cross-platform determinism verification.

---

## Unreal Engine Build

### Status: NOT VERIFIED

- UE 5.6 and UE 5.8 are installed but the audit was conducted headless
- No `UnrealBuildTool` or `UBT` commands were run
- `Source/RedAlert4/RedAlert4.Build.cs` exists but was not tested against UE compilation
- The `ld: warning: ignoring duplicate libraries: 'libRA4FogOfWar.a'` indicates a CMakeLists.txt redundancy (FogOfWar linked twice)

### Module Build.cs Files (15 total)
All 15 modules have `.Build.cs` files. Dependency chain:
- RA4Core (no deps) → RA4Content → RA4Simulation → RA4Replay
- RA4Core → RA4Navigation, RA4Combat, RA4FogOfWar
- RA4Core → RA4Input, RA4AI, RA4Presentation
- RedAlert4 (depends on Engine, CoreUObject)
- RA4UI, RA4Campaign, RA4Network, RA4Editor

---

## Packaging

### Status: MISSING

- No `PackageBuild.ps1`, `Build Shipping.bat`, or equivalent
- No UE `BuildCookRun` script
- No `Makefile` or `justfile` for full builds
- No CI job for UE build or packaging
- The `.gitignore` correctly ignores `Binaries/`, `Build/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`

---

## Issues Found

1. **Duplicate FogOfWar link** in CMakeLists.txt — minor, non-blocking
2. **No UE build verification** — cannot confirm the game compiles in UE
3. **No packaging infrastructure** — no Shipping build exists
4. **ra4-ui web app has stale .d.ts files** — the web prototype has manually maintained type declarations alongside generated ones
