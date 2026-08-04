# ADR-0005: Binary Replay Format with 30-Second Snapshot Checkpointing

## Context
Seeking forward in a 30-minute lockstep replay without snapshots requires re-simulating up to 108,000 ticks, causing severe UI freezes.

## Decision
Save a complete binary `SimWorld` snapshot checkpoint into `.ra4replay` streams every 30 seconds (1,800 ticks).

## Rationale
- Fast replay seeking: seeking to minute 25 jumps directly to the minute 24:30 snapshot and re-simulates only 1,800 ticks max.
- Compact filesize (~500 KB per match).

## Status
**ACCEPTED / IMPLEMENTED**.
