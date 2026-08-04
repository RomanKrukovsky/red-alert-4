# Opus Audit — Multiplayer Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Lockstep Protocol

### Implementation
- `Source/RA4Network/Public/RA4NetworkChannel.h` — per-player RPC component
- `Source/RA4Network/Public/RA4NetworkManager.h` — UWorldSubsystem managing sessions
- `Source/RA4Simulation/Public/RA4Simulation/LockstepSession.h` — engine-free protocol logic

### Protocol Design
1. Each player submits a `CommandFrame` for tick `T + InputDelay`
2. Server assembles all player frames into one authoritative frame per tick
3. Assembly order is slot-order (deterministic), not arrival-order
4. Authoritative frame is broadcast to all clients
5. Each client runs the same frame against its own SimWorld
6. Post-tick checksums are sent to server for desync detection

### Input Delay
- Default: `kDefaultInputDelay` ticks
- Zero is clamped to 1 (a delay of 0 would require packets to arrive before they were sent)
- Priming: empty frames are sent for ticks 0..InputDelay-1 to prevent stalls

---

## Test Coverage (13 network tests)

| Test | What it proves |
|------|---------------|
| `CommandIsDeferredByInputDelay` | Commands scheduled on correct future tick |
| `ZeroInputDelayIsClampedToOne` | Bad config produces lag, not order-dropping |
| `FrameAssemblyFollowsSlotOrderNotArrivalOrder` | Deterministic assembly regardless of packet order |
| `FrameIsIncompleteUntilEveryPlayerReports` | Empty frames from idle players complete the tick |
| `DuplicatePlayerFrameIsIgnored` | Retransmits don't duplicate commands |
| `AssemblyIsRepeatableForRetransmission` | Same frame can be reassembled identically |
| `PeerStallsUntilAuthoritativeFrameArrives` | Client blocks on missing server frame |
| `PruneReleasesRetiredTicks` | Bookkeeping doesn't grow unbounded |
| `MatchingChecksumsDoNotReportDesync` | No false positives |
| `DivergentChecksumIsReportedWithTickAndPlayer` | Desync detected with exact tick and player |
| `ClientDoesNotAdjudicateChecksums` | Only server declares desyncs |
| `TwoPeersStayInSyncAcrossAFullMatch` | Full integration: 120-tick match, two SimWorlds stay identical |
| `DesyncIsCaughtOnTheTickItHappens` | Corruption at tick 40 detected at tick 40 |

---

## UE Network Integration

### URA4NetworkChannel
- UActorComponent on PlayerController
- Replicates: PlayerIndex, MatchNumPlayers, MatchInputDelay
- Server RPC: `ServerSubmitFrame` (with validation: max 64KB payload)
- Client RPC: `ClientReceiveFrame`
- Server RPC: `ServerSubmitChecksum` (with validation: always true — intentional)
- Player slot is stamped from the channel, never from the payload — prevents impersonation

### URA4NetworkManager
- UWorldSubsystem
- Manages `LockstepSession` and channel registry
- Server assembles and broadcasts authoritative frames
- Weak pointers to channels — PlayerController disconnect doesn't leak

---

## Missing Features

| Feature | Status |
|---------|--------|
| Reconnection after disconnect | NOT IMPLEMENTED |
| Spectator mode | NOT IMPLEMENTED |
| LAN lobby UI | NOT IMPLEMENTED |
| Packet loss simulation | NOT TESTED |
| Jitter simulation | NOT TESTED |
| Malformed command handling | Validated but not tested |
| Version mismatch detection | Content hash only — no protocol version |
| Actual UDP/TCP transport | NOT IMPLEMENTED — tests use direct method calls |
| NAT traversal | NOT IMPLEMENTED |
| Matchmaking | NOT IMPLEMENTED |

---

## Verdict

The lockstep protocol is **architecturally sound and well-tested**. The engine-free `LockstepSession` is the right design — it can be tested without Unreal. However:

1. **No real network transport** — tests bypass the network layer entirely
2. **No reconnection** — a disconnect ends the match permanently
3. **No lobby** — players cannot find or join games through UI
4. **Only LAN scope** — no internet play infrastructure

**Status**: ACCEPT_WITH_FIXES. Protocol is solid. Transport and lobby need implementation.
