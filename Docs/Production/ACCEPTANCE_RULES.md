# Production Acceptance Rules & Quality Standards (`ACCEPTANCE_RULES.md`)

**Document Version**: 4.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Code Review & PR Acceptance Rules

Every pull request or sub-agent commit must satisfy the following non-negotiable rules before merging into `main`:

1. **Rule 1: Zero Test Failures**
   - `./build/hb/RA4Tests`, `./build/hb/RA4AITests`, `./build/hb/RA4InputTests`, and `./build/hb/RA4PresentationTests` MUST achieve 100% pass status (378/378 pass baseline).
2. **Rule 2: Zero Simulation Warnings**
   - `RA4Simulation`, `RA4Core`, and `RA4Content` must compile with zero compiler warnings.
3. **Rule 3: Single Atomic Task**
   - Each PR must solve exactly one WBS task from `WORK_BREAKDOWN_STRUCTURE.md`.
4. **Rule 4: Isolated File Ownership**
   - Sub-agents working concurrently MUST NOT edit the same source file to prevent git merge conflicts.
5. **Rule 5: Fixed-Point Compliance**
   - Floating-point calculations in `RA4Simulation` are automatically flagged by linter and rejected.
