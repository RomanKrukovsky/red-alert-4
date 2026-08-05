# ADR-003: Command Protocol & Issuer Verification

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
In lockstep multiplayer RTS games, client inputs must be packaged into deterministic commands, validated by ownership and sequence counters, and dispatched simultaneously across all connected simulation instances.

## Decision
1. **Command Bus**: Inputs are converted into `Command` structs containing `ExecutionFrame`, `SequenceIndex`, `PlayerIndex`, `Type`, and target coordinates / handles.
2. **Rejection & Validation**: `SimWorld` validates ownership (`Command.PlayerIndex == Entity.OwnerPlayer`), prerequisite tech tree state, and handle generation validity before applying orders.
