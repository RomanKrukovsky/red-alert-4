# ADR-009: Replay Recording, Playback & Desync Diagnostics

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
Replays and multiplayer sync debugging require exact match command logging and state hash validation.

## Decision
1. **Replay Header & Log**: Header stores match seed, map ID, player configuration, and engine version. Stream records per-frame command arrays.
2. **Desync Detection**: Checksums are logged per frame. If a checksum mismatch occurs between clients, `RA4Replay` pinpoints the first divergent frame and exports a subsystem state dump.
