# ADR-0001: Fixed-Tick Lockstep Simulation Architecture

> **ERRATUM (2026-08-05)**: this ADR was written as "60Hz Lockstep" and its filename still says
> `60hz`. **The implemented tick rate is 20 Hz** (`constexpr int32_t kTicksPerSecond = 20;` in
> `Source/RA4Core/Public/RA4Core/SimConfig.h`, 50 ms per tick), and the checksum interval is 20 ticks
> (`kChecksumIntervalTicks`), not the 10 stated in older revisions of `INVARIANTS.md`. The decision
> recorded here — exchange command frames on synchronized tick indices rather than entity state — is
> unchanged and correct; only the rate was misstated. 60 FPS remains the *presentation* target,
> achieved by interpolating between simulation ticks. Every "60Hz" below should be read as
> "fixed-tick, 20 Hz". The file is not renamed to avoid breaking inbound links; see NEXT_ACTIONS P-3
> for the ADR consolidation that will address naming.

## Context
High-entity RTS games (1,000 - 2,000 active entities) require low bandwidth network protocols that guarantee bit-identical state across all clients.

## Decision
Adopt a **fixed-tick Lockstep Network Architecture at 20 Hz** (`LockstepSession`) where clients exchange player command frames rather than full entity transform updates.

## Rationale
- Network bandwidth is reduced from ~500 KB/s per client (state sync) to <10 KB/s (command frames).
- Guarantees 100% accurate simulation reproduction across PC platforms.

## Status
**ACCEPTED / IMPLEMENTED**. Verified by the `Lockstep.*` tests within the headless suite; the suite measured 479 passing / 0 failing on 2026-08-05 (the earlier "378/378" figure is stale, and was never a count of `Lockstep.*` alone).
