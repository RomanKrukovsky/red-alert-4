# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 8.0  
**Current Milestone Target**: **Milestone 6: Alpha / Code Freeze** (Gates 1..5 PASSED)  

---

## Completed Milestones

- [X] **Milestone 1: Architecture Baseline** — 100% Passed (393/393 C++ unit tests pass).
- [X] **Milestone 2: Industrial Vertical Slice** — 100% Passed (End-to-end game flow, RSU vs GDC, 5 milestone report artifacts created).
- [X] **Milestone 3: Systems Complete** — 100% Passed (395/395 C++ unit tests pass; 2,000 entity stress benchmark passed; v0.6.0 baseline tagged).
- [X] **Milestone 4: Content Complete** — 100% Passed (78 units, 35 structures, 38 campaign missions, 624 voice events validated; v0.7.0 baseline tagged).
- [X] **Milestone 5: Multiplayer, Tools & Build Infrastructure** — 100% Passed (Authoritative lockstep server, desync dump, CI/CD pipeline, Match Viewer; v0.8.0 baseline tagged).

---

## Active Unblocked Action Queue (Milestone 6: Alpha / Code Freeze)

Below are the enabled, unblocked tasks for immediate sub-agent execution:

| Task ID | Task Description | Target File | Status | Prerequisites |
| :--- | :--- | :--- | :--- | :--- |
| **M6-T1** | Run 100 consecutive automated 8-player AI matches verifying zero desyncs | `Source/RA4Tests/Private/TestAI.cpp` | **UNBLOCKED** | Gate 5 Pass |
| **M6-T2** | Execute memory leak profile check over 100,000 simulated ticks | `Source/RA4Tests/Private/TestProvingGround.cpp` | **UNBLOCKED** | Gate 5 Pass |

---

## Execution Guidelines for Agents

- **Focus**: Execute ONLY tasks from the active queue for Alpha / Code Freeze.
- **Commit Format**: `fix(alpha): [Task ID] short description`.
