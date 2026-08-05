# Non-Functional Performance Budgets (`PERFORMANCE_BUDGETS.md`)

**Document Version**: 3.1  
**Target Hardware Baseline**: Mid-Tier Gaming PC (Quad-Core CPU, GTX 1660 / RX 580, 16 GB RAM, NVMe SSD)  

---

## 1. Frame Rate & Timing Budgets

> **Read this first.** Two different rates are in play and older revisions of this document conflated
> them. **Presentation** targets 60 FPS (16.66 ms per rendered frame) — that is what section 1 budgets.
> **Simulation** runs at 20 Hz (50 ms per tick, `kTicksPerSecond` in `SimConfig.h`) — that is what the
> per-tick budgets in sections 2 and 4 refer to. One sim tick spans roughly three rendered frames, so a
> "2.0 ms / tick" figure is not 2.0 ms of every frame's budget. Where a row below says "per tick", it
> means per 50 ms sim step.

| Metric | Target Budget | Hard Maximum Threshold | Notes / Justification |
| :--- | :--- | :--- | :--- |
| **Target Frame Rate** | **60 FPS / 120 FPS** | **60 FPS Minimum** | Essential for competitive RTS micro responsiveness. |
| **Total Frame Time** | **16.66 ms** | **16.66 ms (60 FPS)** | Budget shared between CPU Game, Render, and GPU. |
| **Game Thread Budget** | **<= 8.0 ms** | **10.0 ms** | Per rendered frame. Includes the amortized share of the 20 Hz SimWorld tick, CommandBus, AI, and presentation update. |
| **Render Thread Budget**| **<= 5.0 ms** | **6.0 ms** | Slate/UMG draw calls and scene rendering setup. |
| **GPU Execution Time** | **<= 6.0 ms** | **8.3 ms** | Shading, lighting, Niagara particles, post-processing. |

---

## 2. Memory & Capacity Budgets

| Metric | Target Budget | Hard Maximum Threshold | Notes / Justification |
| :--- | :--- | :--- | :--- |
| **System RAM Usage** | **<= 4.5 GB** | **8.0 GB** | Allows background streaming apps to run smoothly. |
| **VRAM Usage** | **<= 2.5 GB** | **4.0 GB** | Fits within GTX 1660 / RX 580 4GB VRAM limits. |
| **Active Simulation Entities** | **2,000 Entities** | **3,000 Entities** | Measured only up to 500 entities in `ProvingGround` (<450 ms / 1,000 ticks). The 2,000 target is extrapolated, not measured; treat it as unproven until a 2,000-entity benchmark exists. |
| **Active Physical Projectiles**| **500 Projectiles**| **1,000 Projectiles**| Ballistic, rocket, and laser projectiles. |
| **Pathfinding Workload** | **<= 2.0 ms / sim tick**| **3.0 ms / sim tick** | Flowfield path lookup scales O(1) per unit. Per 50 ms sim step, not per frame. |
| **AI Workload** | **<= 1.5 ms / sim tick**| **2.5 ms / sim tick** | HTN decision loop amortized across tick steps. Per 50 ms sim step, not per frame. |

---

## 3. Network, Storage & Startup Budgets

| Metric | Target Budget | Hard Maximum Threshold | Notes / Justification |
| :--- | :--- | :--- | :--- |
| **Network Bandwidth (Client)**| **<= 8.0 KB/s** | **15.0 KB/s** | Lockstep command frames require minimal data. |
| **Replay File Size** | **<= 800 KB** | **1.5 MB** | 30-minute match replay format (`.ra4replay`). |
| **Save Game Size** | **<= 2.0 MB** | **5.0 MB** | Binary snapshot checkpoint format. |
| **Initial Game Startup** | **<= 5.0 seconds** | **8.0 seconds** | Desktop to interactive Main Menu screen. |
| **Shader Compilation Stalls** | **0 Stalls > 16ms**| **0 Stalls** | PSO pre-warming cache loaded at initial launch screen. |
| **Target Crash Frequency** | **< 0.01%** | **< 0.05%** | Less than 1 crash per 10,000 match sessions. |

---

## 4. Perception-Warfare Systems Budgets (ADR-0021..0026)

**Added**: 2026-08-05. Basis: actual tick = 50 ms (`kTicksPerSecond = 20`, `SimConfig.h`), NOT the 16.66 ms
stated elsewhere in this document's older sections. Game-thread budget of 8 ms/frame still applies;
sim tick work amortizes across 2–3 render frames. Budgets below are per SIM TICK unless noted.
All are gating criteria: implementation PRs for these systems must include a benchmark that measures
the metric and fails CI above the hard maximum. Numbers marked (p) are provisional until first
measured on the ProvingGround harness; provisional numbers may be renegotiated ONCE with evidence,
then freeze.

### 4.1 Intel / KnowledgeMap (ADR-0021, implemented as PerceivedWorld — ADR-0026)

| Metric | Target | Hard Max | Notes |
| :--- | :--- | :--- | :--- |
| Report ingestion + track update, 4 players | <= 0.8 ms/tick (p) | 1.5 ms/tick | At 2,000 entities, hard cap on tracks per ADR-0026. |
| Confidence decay pass (amortized round-robin) | <= 0.2 ms/tick (p) | 0.5 ms/tick | 1/N of records per tick; N chosen so full sweep <= 2 s. |
| Extra state-hash cost from intel state | <= 0.3 ms per checksum tick (p) | 0.6 ms | Checksum every 20 ticks (`kChecksumIntervalTicks`). |
| Memory: PerceivedWorld, per player | <= 4 MB | 8 MB | Track table + cell layer; counted in the 4.5 GB RAM budget. |
| Save-size growth with intel enabled | <= 0.5 MB | 1.0 MB | On top of the 2.0 MB save budget; v3 format. |
| Kill switch (intel disabled) overhead | 0 measurable | 0 | Bit-equal hashes with pre-intel baseline (already tested: Intel.* kill-switch test). |

### 4.2 Command Network / CommandGraph (ADR-0022)

| Metric | Target | Hard Max | Notes |
| :--- | :--- | :--- | :--- |
| Edge recompute, 200 command nodes (amortized) | <= 0.3 ms/tick (p) | 0.8 ms/tick | Event-triggered on node create/destroy + K-tick refresh. |
| In-flight order queue processing | <= 0.1 ms/tick (p) | 0.3 ms/tick | Supersede rule bounds queue: newest order per group wins. |
| Healthy-path delivery latency (gameplay budget) | <= 4 ticks (200 ms) | 6 ticks (300 ms) | Above this the mechanic reads as input lag; Classic mode = 0 ticks. |
| Memory: CommandGraph, per match | <= 1 MB | 2 MB | Nodes + edges + in-flight orders. |

### 4.3 Battlefield Memory / TerrainStateLayer + Wrecks (ADR-0024)

| Metric | Target | Hard Max | Notes |
| :--- | :--- | :--- | :--- |
| Terrain cell mutations per tick (peak battle) | <= 0.2 ms/tick (p) | 0.5 ms/tick | Event-driven writes only; no per-tick full scans. |
| Wreck entity cap per map | 300 | 500 | Deterministic oldest-lowest-value despawn above cap. |
| Nav-cost overlay refresh after cell change | <= 1.0 ms amortized (p) | 2.0 ms | Charged against the existing 2.0 ms/tick pathfinding budget, not in addition to it. |
| TheaterState blob size per mission | <= 1.0 MB | 2.0 MB | Campaign save attachment; versioned from v1. |
| Memory: TerrainStateLayer | <= 8 MB | 16 MB | Grid at fog-grid resolution family. |

### 4.4 Combined ceiling

Sum of all perception-warfare sim work (4.1 + 4.2 + 4.3) MUST stay under **2.5 ms per sim tick**
(hard max 4.0 ms) at the 2,000-entity / 4-player baseline, so that total sim tick cost including
existing pathfinding (2.0 ms) and AI (1.5 ms) budgets remains inside the 50 ms tick with >= 5x headroom
on baseline hardware. ADR-0025 (Adaptive Opponent) has NO in-tick budget: all analysis is post-match
tooling; its only runtime artifact is the DoctrineBias blob read once at match start (<= 64 KB, load < 5 ms).
