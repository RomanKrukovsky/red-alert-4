# Agent Active Action Queue (`NEXT_ACTIONS.md`)

**Document Version**: 9.0  
**Current Milestone Target**: **Milestone 8: Release Candidate (RC)** (Gates 1..7 PASSED)  

---

## Completed Milestones

- [X] **Milestone 1: Architecture Baseline** — 100% Passed (393/393 C++ unit tests pass).
- [X] **Milestone 2: Industrial Vertical Slice** — 100% Passed (End-to-end game flow, RSU vs GDC, 5 milestone report artifacts created).
- [X] **Milestone 3: Systems Complete** — 100% Passed (395/395 C++ unit tests pass; 2,000 entity stress benchmark passed; v0.6.0 baseline tagged).
- [X] **Milestone 4: Content Complete** — 100% Passed (78 units, 35 structures, 38 campaign missions, 624 voice events validated; v0.7.0 baseline tagged).
- [X] **Milestone 5: Multiplayer, Tools & Infrastructure** — 100% Passed (Authoritative lockstep server, desync dump, CI/CD pipeline, Match Viewer; v0.8.0 baseline tagged).
- [X] **Milestone 6 & 7: Alpha & Beta** — 100% Passed (Code freeze, 100-match soak test, 0 open P0/P1 defects, cross-faction balance matrix verified; v0.9.0 baseline tagged).

---

## Active Unblocked Action Queue (Milestone 8: Release Candidate)

Below are the enabled, unblocked tasks for immediate sub-agent execution:

| Task ID | Task Description | Target File | Status | Prerequisites |
| :--- | :--- | :--- | :--- | :--- |
| **M8-T1** | Verify IP counsel legal clearance checklist in `LEGAL_AND_LICENSES.md` | `Docs/Production/LEGAL_AND_LICENSES.md` | **UNBLOCKED** | Gate 7 Pass |
| **M8-T2** | Validate Steam / EGS platform SDK build configuration in `RedAlert4.uproject` | `RedAlert4.uproject` | **UNBLOCKED** | Gate 7 Pass |

---

## Execution Guidelines for Agents

- **Focus**: Execute ONLY tasks from the active queue for Release Candidate.
- **Commit Format**: `rel(rc): [Task ID] short description`.
