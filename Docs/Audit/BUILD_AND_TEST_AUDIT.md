# RA4 — Build & Test Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`
**Environment:** macOS 15.3 arm64, Apple clang 21.0.0, CMake 4.1.1, UE 5.8.1, 14 cores

This document supersedes the previous version, whose numbers (378 tests, 5.616 s) and CI
description (`.github/workflows/ci.yml`, npm build step) do not match the repository.

## 1. Build systems — two, only one of which is exercised

| Pipeline | Definition | Status |
| --- | --- | --- |
| CMake headless core | `Tools/HeadlessBuild/CMakeLists.txt` | **VERIFIED GREEN** |
| Unreal Build Tool | `Source/*.Target.cs`, `Source/*/*.Build.cs` | **VERIFIED GREEN** (editor target) |
| Packaged build (`BuildCookRun`) | not scripted anywhere | **NEVER RUN** |

### 1.1 CMake headless — commands actually run

```
cmake -S Tools/HeadlessBuild -B <build> -DCMAKE_BUILD_TYPE=Release   # Configuring done
cmake --build <build> --config Release --parallel                     # exit 0
```

All targets link: `RA4Tests`, `RA4AITests`, `RA4InputTests`, `RA4PresentationTests`,
`RA4MatchDump`, plus 10 static libraries.

One benign warning, worth cleaning: `ld: warning: ignoring duplicate libraries:
'libRA4FogOfWar.a'` — `RA4FogOfWar` is linked twice in the `RA4Tests` target.

### 1.2 Unreal Build Tool — command actually run

```
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4Editor Mac Development -Project=.../RedAlert4.uproject
```

Result: `Result: Succeeded`, 23 actions, 14.66 s. All 15 module dylibs produced in
`Binaries/Mac/`. This contradicts the earlier `BLOCKED_PLUGIN_MISSING` claim for `RA4UI`.

## 2. Test inventory — measured, not quoted

| Executable | Measured | Previously claimed |
| --- | --- | --- |
| `RA4Tests` | **296 passed, 0 failed** (7.5 s) | 258 |
| `RA4AITests` | **46 passed** | 46 |
| `RA4InputTests` | **66 passed** | 51 |
| `RA4PresentationTests` | **23 passed** | 23 |
| **Total** | **431** | 378 / "395" |

`ctest --output-on-failure` → `100% tests passed, 0 tests failed out of 4` (13.72 s).

Neither 378 nor the release review's 395 corresponds to any measurable total. 431 is the
real figure at this commit.

### 2.1 Coverage distribution — the real risk

Test counts by prefix in `RA4Tests` show heavy investment in a few systems and near-zero in
others that are release-critical:

| Well covered | Count | Thinly covered | Count |
| --- | --- | --- | --- |
| `AI.*` | 44 | `Victory.*` | 2 |
| `Hud.*` | 22 | `Placement.*` | 2 |
| `MissionRuntime.*` | 21 | `Construction.*` | 1 |
| `Navigation.*` | 17 | `SaveSystem.*` | 1 |
| `KeyBindings.*` | 15 | `Checksum.*` | 1 |
| `BibleImport.*` | 15 | `Power.*` | 1 |
| `Orders.*` / `Lockstep.*` | 13 each | *harvester replacement* | **0** |
| | | *minimap* | **0** |
| | | *superweapon* | **0** |

A single test guards state hashing, and a single test guards save/restore — both are
determinism-critical invariants per `Docs/Architecture/INVARIANTS.md`.

## 3. Determinism — genuinely verified on this platform

```
RA4Tests --filter=VerticalSlice.FullMatch
  slice finished in 2816 ticks (140.8 s simulated), harvested 6000 credits,
  final checksum 4e6d9e69576c002b
```

Repeated invocation produced the identical checksum. Cross-platform identity (the point of
the CI job) remains **unverified** because CI has never run to completion.

## 4. CI — broken at the first job, therefore entirely non-functional

The real workflow is `.github/workflows/core.yml` (not `ci.yml`). Job graph:

```
compliance-scan ──> headless (ubuntu, macos, windows) ──> compare-checksums
                └─> sanitizers (ASan/UBSan)
```

### 4.1 Blocking defect: the compliance scanner does not exist

`compliance-scan` runs:

```yaml
python3 -m unittest discover -s Build/Compliance/tests -p "test_*.py" -v
python3 Build/Compliance/compliance_scan.py --scope both
```

Only two files exist under `Build/Compliance/`, both test fixtures:

```
Build/Compliance/tests/corpus/clean_repo/Assets/Game/logo.uasset
Build/Compliance/tests/corpus/missing_provenance/Assets/Game/logo.uasset
```

`compliance_scan.py` and the `test_*.py` files are absent. Because `headless` and
`sanitizers` both declare `needs: [compliance-scan]`, **every job in the pipeline is
unreachable**. No commit on this branch has ever been CI-validated.

### 4.2 Root cause: a case-collision in `.gitignore`

`.gitignore` line 8 ignores `Build/` (Unreal) and line 20 ignores `build/` (CMake). On the
case-insensitive macOS filesystem these collide. The re-inclusion rules

```
!Build/
Build/*
!Build/Compliance/
!Build/Compliance/**
```

do let `Build/Compliance/**` through — `git check-ignore -v` confirms
`!Build/Compliance/**` matches `compliance_scan.py`. So the file is *not* ignored; it simply
was never written. The scanner is referenced by CI and by ADR-011 but does not exist.

Consequence: the licensing gate that ADR-011 relies on to block non-compliant assets is
imaginary, which is directly relevant to the GPLv3 finding in ASSET_AND_LICENSE_AUDIT.md.

### 4.3 Secondary CI issues

- `RA4Tests` is invoked as `./build/RA4Tests` in CI but `./build/hb/RA4Tests` in the old
  docs and in `GOLD_MASTER_MANIFEST.md`'s hotfix procedure. The paths disagree.
- The workflow triggers only on `main` and `feature/*`. The current branch
  (`audit/stage1-full-project-audit`) matches neither, so nothing runs for it even if the
  scanner existed.

## 5. `RA4Tests` module is invisible to Unreal

`Source/RA4Tests/` contains 23 files / 7 436 lines but has **no `RA4Tests.Build.cs`** and does
not appear in `RedAlert4.uproject` (`grep -c RA4Tests RedAlert4.uproject` → 0). It is
compiled *only* by CMake. Practical effect: the test suite cannot be run through Unreal's
Automation framework or Gauntlet, so no test can ever cover engine-side behaviour
(rendering, actor lifecycle, UMG). This is why every gameplay-visual claim in this repo is
unverifiable — see GAMEPLAY_AUDIT.md.

## 6. Stale prebuilt binaries in the tree

`build/hb/` and `build/headless/` contain committed-era executables from before the audit
window. They were built from older sources and are the likely origin of "tests pass"
claims made while the tree did not link. Verified: a from-scratch configure+build was
required to obtain trustworthy results. Treat any pre-existing binary in `build/` as
untrusted evidence.

## 7. Reproduction commands

```bash
# headless core, from scratch
cmake -S Tools/HeadlessBuild -B /tmp/ra4-verify -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/ra4-verify --config Release --parallel
ctest --test-dir /tmp/ra4-verify --output-on-failure
/tmp/ra4-verify/RA4Tests && /tmp/ra4-verify/RA4AITests \
  && /tmp/ra4-verify/RA4InputTests && /tmp/ra4-verify/RA4PresentationTests

# determinism checksum
/tmp/ra4-verify/RA4Tests --filter=VerticalSlice.FullMatch

# unreal editor target
"/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4Editor Mac Development -Project="$PWD/RedAlert4.uproject"
```

## 8. Verdict

| Question | Answer |
| --- | --- |
| Does the core build? | Yes, clean |
| Do the tests pass? | Yes — 431/431 |
| Is determinism real? | Yes on macOS/arm64; cross-platform unproven |
| Does the editor build? | Yes |
| Does CI work? | **No — dead at job 1, never validated any commit** |
| Is there a packaged build? | **No** |
| Can engine-side behaviour be tested? | **No — `RA4Tests` is not a UBT module** |
