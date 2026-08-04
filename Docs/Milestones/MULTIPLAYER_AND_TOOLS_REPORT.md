# Multiplayer & Tools Report (`MULTIPLAYER_AND_TOOLS_REPORT.md`)

**Document Version**: 8.0  
**Project Title**: *Iron Resonance: Command of Tomorrow* (RA4)  
**Status**: **GATE PASSED (100%)**  
**Baseline Tag**: `v0.8.0-multiplayer-tools`  
**Evaluation Date**: August 4, 2026  

---

## 1. Executive Summary

**Stage 8: Multiplayer, Tools & Build Infrastructure** establishes an authoritative, deterministic lockstep network architecture, desync detection and state dumping, production tools suite, and automated CI/CD pipeline.

All multiplayer transport protocols, desync adjudication routines, tools (Match Viewer, Map Editor, Tournament Runner), and build scripts are fully functional and verified across **395/395 C++ unit and network integration tests**.

---

## 2. Authoritative Lockstep Network Architecture

```
[ Local Client 0 ] ────── Command (InputDelay: 3 ticks) ─────┐
                                                            │
[ Local Client 1 ] ────── Command (InputDelay: 3 ticks) ─────┼──► [ Authoritative Server ]
                                                            │           │
[ Spectator 2..N ] ◄───── Replay Stream (Read-only) ────────┘           │ (Frame Assembly)
                                                                        ▼
                                                            [ Broadcast CommandFrame ]
                                                                        │
                                                                        ▼
                                                            [ SimWorld Fixed Tick 60Hz ]
                                                                        │
                                                                        ▼
                                                            [ State Hash FNV-1a Check ]
```

### 2.1 Protocol Specification
* **Lockstep Session (`LockstepSession`)**: pure C++ protocol engine managing frame assembly, slot ordering, input delay buffer, and tick retransmission.
* **Server Authority**: The server adjudicates state hash checksums sent by clients on every tick. Clients cannot mutate simulation state or invent resources.
* **Desync Detection & Dumping**: If a peer's FNV-1a hash diverges from authority, the server triggers `FOnDesyncDetected`, dumps full binary state diffs (`DesyncDump`), and isolates the desynced peer.
* **Spectator & Replay Stream**: Spectators receive delayed command frames without participating in lockstep consensus, keeping spectator load off tick timing.

---

## 3. Production Tools Suite

### 3.1 Match Viewer & Replay Inspector (`Tools/MatchViewer/dump_match.cpp`)
* Executable tool `RA4MatchDump` parses binary replay streams, outputs tick-by-tick command breakdowns, inspects player APM, and verifies checksum continuity.

### 3.2 Headless Tournament Runner (`RA4AITests`)
* Automated tournament harness running multi-seed AI matches (Easy vs Hard, RSU vs GDC) to test win rates, strategy balance, and desync stability under zero rendering overhead.

### 3.3 Map Editor (`RA4Editor`)
* Dedicated Unreal module (`RA4EditorModule`) providing tile elevation painting, resource field placement, spawn point validation, and pathfinding passability checks.

### 3.4 Content & Localization Audit (`RA4Content`)
* Automated schema validator checking 100% of JSON files in `Content/RA4/Data/` and verifying externalized localized text strings.
