# ADR-006: Combat System, Damage Matrix & Veterancy

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
RTS balance relies on rock-paper-scissors combat mechanics expressed through warhead classes against armor classes.

## Decision
1. **Damage Matrix**: 2D lookup table (`WarheadClass` $\times$ `ArmorClass`) stored as fixed-point per-mille multipliers.
2. **Veterancy Ranks**: Four ranks (`Recruit`, `Veteran`, `Elite`, `Heroic`) scaling damage output, max HP, and passive abilities.
