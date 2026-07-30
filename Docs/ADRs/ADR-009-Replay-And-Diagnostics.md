# ADR-009: Replay Recording, Playback & Desync Diagnostics

## Status
Accepted

## Context
Replays and multiplayer sync debugging require exact match command logging and state hash validation.

## Decision
1. **Replay Header & Log**: Header stores match seed, map ID, player configuration, and engine version. Stream records per-frame command arrays.
2. **Desync Detection**: Checksums are logged per frame. If a checksum mismatch occurs between clients, `RA4Replay` pinpoints the first divergent frame and exports a subsystem state dump.
