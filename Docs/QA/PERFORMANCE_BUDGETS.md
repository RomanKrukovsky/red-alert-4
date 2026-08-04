# Non-Functional Performance Budgets (`PERFORMANCE_BUDGETS.md`)

**Document Version**: 3.0  
**Target Hardware Baseline**: Mid-Tier Gaming PC (Quad-Core CPU, GTX 1660 / RX 580, 16 GB RAM, NVMe SSD)  

---

## 1. Frame Rate & Timing Budgets

| Metric | Target Budget | Hard Maximum Threshold | Notes / Justification |
| :--- | :--- | :--- | :--- |
| **Target Frame Rate** | **60 FPS / 120 FPS** | **60 FPS Minimum** | Essential for competitive RTS micro responsiveness. |
| **Total Frame Time** | **16.66 ms** | **16.66 ms (60 FPS)** | Budget shared between CPU Game, Render, and GPU. |
| **Game Thread Budget** | **<= 8.0 ms** | **10.0 ms** | Includes SimWorld tick, CommandBus, AI, and presentation. |
| **Render Thread Budget**| **<= 5.0 ms** | **6.0 ms** | Slate/UMG draw calls and scene rendering setup. |
| **GPU Execution Time** | **<= 6.0 ms** | **8.3 ms** | Shading, lighting, Niagara particles, post-processing. |

---

## 2. Memory & Capacity Budgets

| Metric | Target Budget | Hard Maximum Threshold | Notes / Justification |
| :--- | :--- | :--- | :--- |
| **System RAM Usage** | **<= 4.5 GB** | **8.0 GB** | Allows background streaming apps to run smoothly. |
| **VRAM Usage** | **<= 2.5 GB** | **4.0 GB** | Fits within GTX 1660 / RX 580 4GB VRAM limits. |
| **Active Simulation Entities** | **2,000 Entities** | **3,000 Entities** | Tested up to 500 entities in `ProvingGround` (<450ms / 1000 ticks). |
| **Active Physical Projectiles**| **500 Projectiles**| **1,000 Projectiles**| Ballistic, rocket, and laser projectiles. |
| **Pathfinding Workload** | **<= 2.0 ms / tick**| **3.0 ms / tick** | Flowfield path lookup scales O(1) per unit. |
| **AI Workload** | **<= 1.5 ms / tick**| **2.5 ms / tick** | HTN decision loop amortized across tick steps. |

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
