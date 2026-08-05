# ADR 0002 - All state changes go through validated commands

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted, implemented.

## Context

An RTS needs server authority, replays, AI, and mission scripting. Implementing
those as four separate paths into game state guarantees they will diverge.

## Decision

`Command` is the only way state changes. Player input, AI and mission scripts all
produce commands; `SimWorld::ApplyCommand` validates ownership, liveness,
affordability, tech prerequisites, placement legality, target validity and a
per-player per-tick rate limit, then applies the change.

Rejections are explicit (`CommandReject`) and emitted as events rather than dropped,
because "my order did nothing and I don't know why" is the hardest RTS bug to
diagnose.

## Consequences

Replays are the command stream plus the seed. The dedicated server needs no separate
validation layer. A client cannot buy unbounded server work: the rate limit is
enforced in the same place as everything else, and is covered by a test that fires
500 commands in one tick.
