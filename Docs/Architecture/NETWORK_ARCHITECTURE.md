# Network Architecture & Lockstep Protocol (`NETWORK_ARCHITECTURE.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Lockstep Network Architecture

The multiplayer network layer (`RA4Network`) operates on a **60Hz Lockstep Protocol**. All game instances process player commands on synchronized tick indices.

```
 Client 1 (Local)                  Server / Host                  Client 2 (Peer)
      |                                 |                                 |
      | --- [Local Command Frame N] --> |                                 |
      |                                 | <-- [Local Command Frame N] --- |
      |                                 |                                 |
      |                                 | -- [Assembled Frame Tick N] --> |
      | <-- [Assembled Frame Tick N] -- |                                 |
      v                                 v                                 v
[Execute Tick N]                [Execute Tick N]                [Execute Tick N]
```

---

## 2. Reconnect & Desync Diagnosis

### Client Reconnect Sequence
1. Reconnecting client requests current match snapshot from Authoritative Server.
2. Server serializes current `SimWorld` binary snapshot and sends it over TCP.
3. Client loads snapshot, initializes `SimWorld`, and fast-forwards buffered command frames to present tick.

### Desync Diagnosis Tool
- If state checksums diverge between peers at tick `T`, both clients write detailed debug dumps (`desync_tick_T.log`) containing entity positions, health, order queues, and RNG seeds for automated diffing.
