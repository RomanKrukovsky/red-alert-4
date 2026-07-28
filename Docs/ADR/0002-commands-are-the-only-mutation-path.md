# ADR 0002 - All state changes go through validated commands

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
