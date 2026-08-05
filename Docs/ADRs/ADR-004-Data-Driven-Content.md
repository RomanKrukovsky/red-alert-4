# ADR-004: Data-Driven Content Architecture & Asset Loaders

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADRs/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

## Status
Accepted

## Context
RTS game balance and entity parameters require rapid iteration without recompiling C++ source files.

## Decision
1. `ContentDatabase` acts as the single source of truth for all unit definitions, building specs, weapon stats, damage matrices, EVA lines, and faction attributes.
2. `BibleContentLoader` parses normalized JSON content files into `ContentDatabase` idempotently.
3. UE5 editor tools author `URA4UnitDefinition`, `URA4BuildingDefinition`, and `URA4WeaponDefinition` Data Assets that serialize cleanly into canonical JSON definitions.
