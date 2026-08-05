# System Architecture Specification (`ARCHITECTURE.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. High-Level Architecture Overview

The system is structured into four distinct execution layers:

```
+-------------------------------------------------------------------------------+
|                           1. PRESENTATION & UI LAYER                          |
|   RA4Presentation | RA4UI | Slate/UMG | Camera | Selection Decals             |
+-------------------------------------------------------------------------------+
                                        | Reads State & Enqueues Commands
                                        v
+-------------------------------------------------------------------------------+
|                            2. NETWORK & LOCKSTEP LAYER                        |
|   RA4Network | LockstepSession | Packet Framer | Desync Diagnostic Engine      |
+-------------------------------------------------------------------------------+
                                        |
                                        v
+-------------------------------------------------------------------------------+
|                         3. DETERMINISTIC SIMULATION LAYER                     |
|   SimWorld (20 Hz) | CommandBus | RA4Combat | RA4Navigation | RA4AI | FogOfWar |
+-------------------------------------------------------------------------------+
                                        |
                                        v
+-------------------------------------------------------------------------------+
|                            4. CORE & CONTENT DATABASE                         |
|   RA4Core (FixedPoint, Hash64) | RA4Content (ContentDatabase, Loader)        |
+-------------------------------------------------------------------------------+
```

---

## 2. Layer Subsystem Responsibilities

1. **Presentation & UI Layer**: Renders 3D meshes, plays audio, handles WASD camera controls, marquee selection box, and updates HUD widgets. Must never mutate simulation state.
2. **Network & Lockstep Layer**: Assembles player command frames, handles UDP network packets, manages input delay buffer, and adjudicates 64-bit state checksums.
3. **Deterministic Simulation Layer**: Executes game rules, unit movement, combat calculations, AI decision loops, and fog of war calculations on a fixed 20 Hz tick (`kTicksPerSecond`, `SimConfig.h`; 50 ms per tick). Presentation renders at 60+ FPS by interpolating between ticks — render rate never affects simulation results.
4. **Core & Content Database**: Provides fixed-point math, 64-bit hashing, normalized JSON content data, and data-driven unit definitions.
