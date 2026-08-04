# Opus Audit — Multiplayer Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Lockstep Protocol

### Architecture
- LockstepSession: engine-free protocol logic (engine-independent, testable without UE)
- URA4NetworkChannel: per-player UActorComponent RPC endpoint
- URA4NetworkManager: UWorldSubsystem managing sessions

### Protocol
1. Each player submits CommandFrame for tick T + InputDelay
2. Server assembles all frames in slot-order (deterministic)
3. Authoritative frame broadcast to all clients
4. Each client runs same frame against own SimWorld
5. Post-tick checksums sent to server for desync detection

## Test Coverage (13 tests — all PASS)

| Test | Proves |
|------|--------|
| CommandIsDeferredByInputDelay | Correct future tick scheduling |
| ZeroInputDelayIsClampedToOne | Bad config → lag, not order-dropping |
| FrameAssemblyFollowsSlotOrderNotArrivalOrder | Deterministic assembly |
| FrameIsIncompleteUntilEveryPlayerReports | Empty frames complete ticks |
| DuplicatePlayerFrameIsIgnored | Retransmit safety |
| AssemblyIsRepeatableForRetransmission | Idempotent assembly |
| PeerStallsUntilAuthoritativeFrameArrives | Client blocking |
| PruneReleasesRetiredTicks | Bookkeeping bounded |
| MatchingChecksumsDoNotReportDesync | No false positives |
| DivergentChecksumIsReportedWithTickAndPlayer | Desync with exact tick+player |
| ClientDoesNotAdjudicateChecksums | Only server declares desyncs |
| TwoPeersStayInSyncAcrossAFullMatch | Full integration: 120 ticks |
| DesyncIsCaughtOnTheTickItHappens | Corruption at tick 40 → detected at tick 40 |

## Missing

| Feature | Status |
|---------|--------|
| Reconnection | NOT IMPLEMENTED |
| Spectator mode | NOT IMPLEMENTED |
| LAN lobby UI | NOT IMPLEMENTED |
| Packet loss simulation | NOT TESTED |
| Jitter simulation | NOT TESTED |
| Real UDP/TCP transport | NOT IMPLEMENTED |
| NAT traversal | NOT IMPLEMENTED |

## Verdict

ACCEPT_WITH_FIXES. Protocol is architecturally sound and well-tested. Transport and lobby need implementation.
