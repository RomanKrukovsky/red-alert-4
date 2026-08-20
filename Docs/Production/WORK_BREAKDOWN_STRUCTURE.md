# Work Breakdown Structure (WBS) & Backlog (`WORK_BREAKDOWN_STRUCTURE.md`)

**Document Version**: 4.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. WBS Task Rules

Each task in the WBS backlog must adhere to strict agent execution constraints:
1. **Single Measurable Output**: Solves exactly 1 specific engineering or documentation goal.
2. **Single Code Review**: Fits into a single PR / commit review (<300 lines altered).
3. **Atomic Commit**: Ends with a clean, building commit that passes tests.
4. **No Concurrent Edits**: Operates on isolated files to prevent merge conflicts with parallel sub-agents.

---

## 2. Milestone 1 Backlog (Architecture Baseline)

### Task M1-T1: Relative Data Path Resolution in `BibleContentLoader.cpp`
- **Goal**: Add fallback relative path checking (`../..` directory traversal) when executed from `build/hb/`.
- **Affected Files**: [`Source/RA4Content/Private/BibleContentLoader.cpp`](file://Source/RA4Content/Private/BibleContentLoader.cpp)
- **Completion Criteria**: `./RA4Tests` passes 100% whether executed from project root OR inside `build/hb/`.
- **Tests**: `RA4Tests` (`BibleImport.*`).

### Task M1-T2: Header Preprocessor Guards for Noesis ViewModels
- **Goal**: Guard `RA4NoesisHUDViewModel.h/cpp` with `#if WITH_NOESIS` to prevent compilation errors when Noesis plugin is absent.
- **Affected Files**: [`Source/RA4UI/Public/RA4NoesisHUDViewModel.h`](file://Source/RA4UI/Public/RA4NoesisHUDViewModel.h)
- **Completion Criteria**: `RA4UI` module compiles cleanly under standard UBT without Noesis plugin.
- **Tests**: `RA4PresentationTests`.

### Task M1-T3: Automated Headless Baseline Verification
- **Goal**: Run all 4 C++ headless test binaries and verify 378/378 tests pass.
- **Affected Files**: None (Execution task).
- **Completion Criteria**: 378 passed, 0 failed.

---

## 3. Milestone 2 Backlog (Industrial Vertical Slice)

### Task M2-T1: Presentation Delta Entity Queue
- **Goal**: Replace linear `SimWorld` polling in `URA4PresentationSubsystem` with a delta-changed entity event queue.
- **Affected Files**: [`Source/RA4Presentation/Private/RA4PresentationSubsystem.cpp`](file://Source/RA4Presentation/Private/RA4PresentationSubsystem.cpp)
- **Completion Criteria**: Game thread presentation overhead reduced to <= 1.5ms for 1,000 entities.
- **Tests**: `RA4PresentationTests`.
