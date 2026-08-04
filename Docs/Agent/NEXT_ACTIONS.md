# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 4.1  
**Current Milestone Target**: **Milestone 2: Industrial Vertical Slice** (Milestone Gate 1 PASSED)  

---

## Completed Milestones

- [X] **Milestone 1: Architecture Baseline** — 100% Passed (393/393 C++ unit tests pass; BibleContentLoader path fallback verified; module boundaries verified).

---

## Active Unblocked Action Queue (Milestone 2: Industrial Vertical Slice)

Below are the enabled, unblocked tasks for immediate sub-agent execution:

| Task ID | Task Description | Target File | Status | Prerequisites |
| :--- | :--- | :--- | :--- | :--- |
| **M2-T1** | Add Red Star Union (RSU) & GDC faction ability data structures to `SimWorld` | `Source/RA4Simulation/Public/RA4Simulation/SimWorld.h` | **UNBLOCKED** | Gate 1 Pass |
| **M2-T2** | Implement presentation HUD snapshot polling interface in `RA4Presentation` | `Source/RA4Presentation/Public/RA4PresentationModule.h` | **UNBLOCKED** | Gate 1 Pass |
| **M2-T3** | Implement 15-minute headless lockstep soak test verifying zero desyncs | `Source/RA4Tests/Private/TestNetwork.cpp` | **UNBLOCKED** | Gate 1 Pass |

---

## Execution Guidelines for Agents

- **Focus**: Execute ONLY tasks from the active queue for Milestone 2.
- **Commit Format**: `feat(sim): [Task ID] short description`.
