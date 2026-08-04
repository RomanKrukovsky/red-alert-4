# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 4.0  
**Current Milestone Target**: **Milestone 1: Architecture Baseline**  

---

## Active Unblocked Action Queue

Below are the enabled, unblocked tasks for immediate sub-agent execution:

| Task ID | Task Description | Target File | Status | Prerequisites |
| :--- | :--- | :--- | :--- | :--- |
| **M1-T1** | Add working directory fallback relative path resolution to `BibleContentLoader.cpp` | `Source/RA4Content/Private/BibleContentLoader.cpp` | **UNBLOCKED** | None |
| **M1-T2** | Add preprocessor guards (`#if WITH_NOESIS`) around Noesis ViewModels | `Source/RA4UI/Public/RA4NoesisHUDViewModel.h` | **UNBLOCKED** | None |
| **M1-T3** | Execute full 378 headless test suite and confirm baseline pass | `./build/hb/RA4Tests` | **UNBLOCKED** | M1-T1, M1-T2 |

---

## Execution Guidelines for Agents

- **Focus**: Execute ONLY tasks from the active queue for Milestone 1.
- **Do Not Start Milestone 2**: Tasks for Milestone 2 remain locked until Milestone Gate 1 is 100% passed.
- **Commit Format**: `feat(sim): [Task ID] short description`.
