# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 6.0  
**Current Milestone Target**: **Milestone 4: Content Complete** (Gates 1, 2 & 3 PASSED)  

---

## Completed Milestones

- [X] **Milestone 1: Architecture Baseline** — 100% Passed (393/393 C++ unit tests pass).
- [X] **Milestone 2: Industrial Vertical Slice** — 100% Passed (End-to-end game flow, RSU vs GDC, 5 milestone report artifacts created).
- [X] **Milestone 3: Systems Complete** — 100% Passed (395/395 C++ unit tests pass; 2,000 entity stress benchmark passed; v0.6.0 baseline tagged).

---

## Active Unblocked Action Queue (Milestone 4: Content Complete)

Below are the enabled, unblocked tasks for immediate sub-agent execution:

| Task ID | Task Description | Target File | Status | Prerequisites |
| :--- | :--- | :--- | :--- | :--- |
| **M4-T1** | Verify integration of 78 unit 3D PBR mesh data definitions in `ContentDatabase` | `Source/RA4Content/Public/ContentDatabase.h` | **UNBLOCKED** | Gate 3 Pass |
| **M4-T2** | Validate 38 campaign mission objective manifests in `RA4Campaign` | `Source/RA4Campaign/Private/CampaignManager.cpp` | **UNBLOCKED** | Gate 3 Pass |
| **M4-T3** | Validate 624 voice line audio event triggers in `RA4Presentation` | `Source/RA4Presentation/Private/RA4PresentationModule.cpp` | **UNBLOCKED** | Gate 3 Pass |

---

## Execution Guidelines for Agents

- **Focus**: Execute ONLY tasks from the active queue for Milestone 4.
- **Commit Format**: `feat(content): [Task ID] short description`.
