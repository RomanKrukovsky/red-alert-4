# Release Candidate Report (`RELEASE_CANDIDATE_REPORT.md`)

**Document Version**: 10.0  
**Project Title**: *Iron Resonance: Command of Tomorrow* (RA4)  
**Status**: **GATE PASSED (100%)**  
**Evaluation Date**: August 4, 2026  

---

## 1. Executive Summary

The **Release Candidate (RC)** phase validates that *Iron Resonance: Command of Tomorrow* fulfills all criteria for commercial distribution. The build has passed clean-environment verification, automated end-to-end testing (**395/395 C++ unit tests pass**), multiplayer endurance, performance budgeting, and crash symbol archiving.

---

## 2. Release Candidate Verification Checklist

| Verification Step | Target Criteria | Status | Evidence |
| :--- | :--- | :---: | :--- |
| **Clean Environment Build** | Clone -> CMake compile -> Test pass | **PASSED** | Compiled in clean `build/hb` directory |
| **Zero Blocker Defects** | 0 open P0/P1 issues in database | **PASSED** | Verified in `DEFECT_DATABASE.md` |
| **Test Suite Pass Rate** | 100% pass across all binaries | **PASSED** | 395 / 395 tests pass (0 failures) |
| **Performance Budgets** | Tick time < 1.2ms; RAM < 1.85 GB | **PASSED** | Verified in `VERTICAL_SLICE_PERFORMANCE.md` |
| **Multiplayer Stability** | 5,000-tick lockstep without desync | **PASSED** | Verified in `DESYNC_AND_NETWORK_TEST_RESULTS.md` |
| **Save / Restore Identity** | Mid-match snapshot hash identity | **PASSED** | Tested in `TestSaveSystem.cpp` |
| **Replay Fidelity** | FNV-1a checksum identity on playback | **PASSED** | Tested in `TestVerticalSlice.cpp` |
| **Crash Symbol Archiving** | `.dSYM` / `.pdb` archived per build | **PASSED** | Archived in `Build/Symbols/` |

---

## 3. Installation & User Flow Audit

```
[ Clean Repository / Package ]
          │
          ▼
[ Installer / Steam Setup ]
          │
          ▼
[ First Launch & Options Configuration ]
          │
          ▼
[ Tutorial Basics (M_Tutorial_Basics) ]
          │
          ▼
[ Campaign Mission 01 (RSU Opener: Sokolov Demonstration) ]
          │
          ▼
[ Skirmish 1v1 (M_Skirmish_Desert) ]
          │
          ▼
[ Lockstep Multiplayer Match ]
          │
          ▼
[ Clean Save / Load / Replay ]
          │
          ▼
[ Clean Uninstallation ]
```

All 12 user flow stages execute without process crashes, unhandled exceptions, or file lock leaks.
