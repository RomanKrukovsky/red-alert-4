# Project Remediation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the abandoned React prototype, standardize the project on Unreal CommonUI/UMG/Slate, and restore all automated quality gates found broken by the 2026-08-25 audit.

**Architecture:** The shipping UI remains entirely native Unreal: CommonUI owns screen routing and modal layers, UMG owns menus and HUD composition, and Slate owns high-frequency batched widgets such as the minimap. The deterministic headless core remains engine-independent; fixes are limited to component storage invariants, camera behavior tests, visibility boundaries, compliance, CI, and operator documentation.

**Tech Stack:** Unreal Engine 5.8, C++20, CommonUI, UMG, Slate, CMake/CTest, Python 3 compliance tooling, GitHub Actions.

**Spec:** `Docs/superpowers/specs/2026-08-20-rts-ui-design.md`

## Global Constraints

- Preserve all pre-existing staged and unstaged user changes.
- Do not commit or push because the worktree already contains user-owned changes.
- Keep imports/includes at the top of each module.
- Keep the deterministic simulation free of Unreal Engine dependencies.
- Use CommonUI for routing, UMG for composition, and Slate only for high-frequency custom rendering.
- Remove React source, dependencies, generated output, screenshots bundled only for the prototype, and active documentation that instructs contributors to use React.
- Run every headless test suite and the compliance unit tests before completion.

---

### Task 1: Remove the legacy web UI

**Files:**
- Delete: `ra4-ui/`
- Delete: `design.md`
- Modify: `Docs/Production/RISK_REGISTER.md`
- Modify: `Docs/Production/LEGAL_AND_LICENSES.md`
- Modify: `Docs/Audit/CURRENT_STATE.md`
- Modify: `Docs/Agent/PROJECT_STATE.md`
- Modify: `Docs/Agent/NEXT_ACTIONS.md`

**Interfaces:**
- Consumes: native UI architecture from the UI design specification.
- Produces: a repository with one production UI stack and no npm/React dependency.

- [ ] Record the tracked file count and active references to `ra4-ui`.
- [ ] Remove the exact `ra4-ui/` directory and the obsolete React implementation brief.
- [ ] Change active project-state documents to name CommonUI/UMG/Slate as the only supported UI stack.
- [ ] Verify `git ls-files ra4-ui` and active-source searches return no React implementation.

### Task 2: Repair deterministic entity component storage

**Files:**
- Modify: `Source/RA4Simulation/Private/SimWorld.cpp`
- Test: `Source/RA4Tests/Private/TestSimulation.cpp`

**Interfaces:**
- Consumes: `SimWorld::AllocateEntity()` and the existing component getters.
- Produces: aligned component vectors for every fresh and recycled entity slot.

- [ ] Use the existing crashing simulation tests as the RED reproduction under AddressSanitizer.
- [ ] Add a focused test that creates multiple entities and observes default status, transport, and passenger components.
- [ ] Run the focused test and confirm the pre-fix crash/failure.
- [ ] Append all component records in the fresh-slot branch before indexed reset.
- [ ] Run the focused test, the core suite, and the sanitizer suite.

### Task 3: Preserve the north-up camera contract

**Files:**
- Modify: `Source/RA4Input/Private/CameraController.cpp`
- Modify: `Source/RA4Input/Public/RA4Input/CameraController.h`
- Test: `Source/RA4Tests/Private/TestInput.cpp`

**Interfaces:**
- Consumes: `CameraController::ResetRotation()`, `Update()`, and screen-relative pan controls.
- Produces: normalized 270-degree north-up yaw with correct screen-relative movement.

- [ ] Use the seven existing failing camera tests as the RED reproduction.
- [ ] Add a behavioral assertion that reset restores north-up orientation.
- [ ] Update old zero-yaw expectations to derive movement from the new opening orientation.
- [ ] Store reset yaw as `270.0f`, which is equivalent to `-90.0f` and respects the public `[0, 360)` invariant.
- [ ] Run the complete input suite.

### Task 4: Restore visibility and compliance gates

**Files:**
- Modify: `Docs/Architecture/VISIBILITY_CALLSITE_INVENTORY.md`
- Delete: `Build/Compliance/compliance_scan.py`
- Delete: `Build/Compliance/tests/test_compliance_scan.py`
- Modify: `build/Compliance/compliance_scan.py`
- Modify: `build/Compliance/tests/test_compliance_scan.py`
- Modify: `Content/AssetRegistry/ThirdPartyAssets.json`
- Rename: `Source/RA4Tests/Private/TestRA3Gameplay.cpp` to `Source/RA4Tests/Private/TestGameplayMechanics.cpp`
- Modify: production source and documentation reported by the compliance scan.

**Interfaces:**
- Consumes: compliance policy and the recon inventory test.
- Produces: one portable compliance tool path and zero indexed violations.

- [ ] Classify the autoplay objective-state reader in the visibility inventory.
- [ ] Add a compliance regression test proving text dotfiles such as `.gitignore` do not require binary provenance.
- [ ] Run it and confirm RED.
- [ ] Fix binary-file detection and run compliance unit tests.
- [ ] Remove the case-colliding uppercase scanner copies and tracked Python bytecode.
- [ ] Replace prohibited legacy identifiers and absolute workstation paths with neutral, repository-relative wording.
- [ ] Register the CityPark asset in the existing third-party provenance record.
- [ ] Run the indexed compliance scan and recon tests until both are green.

### Task 5: Align CI and build metadata with the real project

**Files:**
- Modify: `.github/workflows/core.yml`
- Modify: `Tools/HeadlessBuild/CMakeLists.txt`
- Modify: `Source/RA4UI/RA4UI.Build.cs`

**Interfaces:**
- Consumes: CMake headless suites and Unreal 5.8 CommonUI modules.
- Produces: CI that makes no false React or automatic Gauntlet claims and a clean UI module dependency list.

- [ ] Remove the duplicated `CommonUI` module entry.
- [ ] Remove stale build comments that claim an Unreal/Gauntlet merge gate that does not exist.
- [ ] Add repository-cleanliness checks for case-colliding paths and tracked Python bytecode.
- [ ] Keep the existing cross-platform deterministic and sanitizer jobs intact.
- [ ] Validate workflow YAML and run the headless build locally.

### Task 6: Repair contributor and project documentation

**Files:**
- Modify: `README.md`
- Modify: `QUICK_START.md`
- Modify: `CONTRIBUTING.md`
- Modify: `HANDOFF.md`
- Modify: `Docs/Agent/PROJECT_STATE.md`
- Modify: `Docs/Agent/PROJECT_STATUS.md`

**Interfaces:**
- Consumes: verified CMake/CTest and Unreal 5.8 commands.
- Produces: one accurate setup path without machine-specific locations or invented scripts.

- [ ] Replace stale UE 5.3/5.6 guidance with UE 5.8.
- [ ] Replace missing script references with commands that exist in the repository.
- [ ] Remove fragile hard-coded test totals and local absolute paths.
- [ ] Describe the native CommonUI/UMG/Slate stack and removal of the web prototype.
- [ ] Follow the documented quick-start commands in a clean temporary build directory.

### Task 7: Full verification

**Files:**
- Verify only; no new production interfaces.

**Interfaces:**
- Consumes: all preceding task outputs.
- Produces: evidence for build, tests, compliance, and repository hygiene.

- [ ] Configure and build all headless targets from scratch.
- [ ] Run core, input, presentation, and AI suites with full failure output.
- [ ] Run compliance unit tests and the indexed scan.
- [ ] Run AddressSanitizer/UndefinedBehaviorSanitizer tests.
- [ ] Confirm no tracked React/npm artifacts, tracked bytecode, or case-colliding paths remain.
- [ ] Review the final diff without overwriting unrelated user changes.
