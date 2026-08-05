# ADR 0010 - Production Build Pipeline, Headless Dedicated Server Stripping, and Reproducible CI

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted.

## Context

Shipping an enterprise-grade C++ RTS requires deterministic, reproducible automated build pipelines. A dedicated server binary should not compile or package client-only assets (UMG textures, Slate UI, high-res audio, rendering shaders), minimizing server memory footprint, security attack surface, and deployment costs.

## Decision

1. **Automated BuildGraph / AutomationTool Targets**:
   - The build process is orchestrated via `AutomationTool` and `BuildGraph` scripts, defining four target profiles:
     - `RA4Editor`: Uncooked editor environment for developer iteration and editor plugin tools.
     - `RA4Client-Development`: Client executable with diagnostic logging, Insights profiling, and automation tests enabled.
     - `RA4Client-Shipping`: Fully optimized client binary with stripped symbols, enabled security flags, and pak/ucas chunking.
     - `RA4Server-LinuxDedicated`: Headless Linux server binary with audio, rendering, UMG, Slate, and client assets stripped out.
2. **Dedicated Server Asset Stripping**:
   - The server build configuration marks visual and audio assets as `EditorOnly` / `ClientOnly` in primary asset registries.
   - Dedicated server binaries load only simulation data assets, collision geometry data, navigation grids, and localization strings.
3. **Symbol Server & Crash Reporting**:
   - CI uploads PDBs and Linux DWARF debug symbols to an internal Symbol Server during every shipping build.
   - Production crash reports are automatically symbolic-resolved and categorized with build git commit hashes and match correlation IDs.
4. **Asset Validation & Determinism CI Gate**:
   - CI builds run automated `RA4Tests`: asset redirectors check, naming conventions check, circular dependency check, and headless 1,000-tick determinism simulation test before PR approval.

## Consequences

**Positive**:
- Dedicated Server memory footprint is reduced by > 75%, allowing high density on Linux server nodes.
- CI pipeline catches asset corruptions and simulation non-determinism before code merges into `main`.
- Symbol server integration ensures immediate stack trace resolution for production crashes.

**Negative**:
- Initial CI configuration requires maintenance of custom BuildGraph XML and server packaging scripts.
