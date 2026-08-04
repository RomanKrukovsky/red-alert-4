# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 7.0  
**Current Milestone Target**: **Milestone 5: Feature Complete** (Gates 1..4 PASSED)  

---

## Completed Milestones

- [X] **Milestone 1: Architecture Baseline** — 100% Passed (393/393 C++ unit tests pass).
- [X] **Milestone 2: Industrial Vertical Slice** — 100% Passed (End-to-end game flow, RSU vs GDC, 5 milestone report artifacts created).
- [X] **Milestone 3: Systems Complete** — 100% Passed (395/395 C++ unit tests pass; 2,000 entity stress benchmark passed; v0.6.0 baseline tagged).
- [X] **Milestone 4: Content Complete** — 100% Passed (78 units, 35 structures, 38 campaign missions, 624 voice events validated; v0.7.0 baseline tagged).

---

## Active Unblocked Action Queue (Milestone 5: Feature Complete)

Below are the enabled, unblocked tasks for immediate sub-agent execution:

| Task ID | Task Description | Target File | Status | Prerequisites |
| :--- | :--- | :--- | :--- | :--- |
| **M5-T1** | Validate 1v1, 2v2, and 3v3 ranked match lobby flow in `RA4Network` | `Source/RA4Network/Public/LockstepSession.h` | **UNBLOCKED** | Gate 4 Pass |
| **M5-T2** | Validate Map Editor save/load & brush triggers in `RA4Editor` | `Source/RA4Editor/Public/RA4EditorModule.h` | **UNBLOCKED** | Gate 4 Pass |
| **M5-T3** | Execute 100-match automated AI regression suite | `Source/RA4Tests/Private/TestAI.cpp` | **UNBLOCKED** | Gate 4 Pass |

---

## Execution Guidelines for Agents

- **Focus**: Execute ONLY tasks from the active queue for Milestone 5.
- **Commit Format**: `feat(feature): [Task ID] short description`.
