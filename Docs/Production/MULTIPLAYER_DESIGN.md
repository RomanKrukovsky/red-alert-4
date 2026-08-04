# Multiplayer & Competitive System Design (`MULTIPLAYER_DESIGN.md`)

**Document Version**: 2.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  
**Netcode Engine**: 60Hz Lockstep Network Session (`RA4Network`)  

---

## 1. Multiplayer Architecture Overview

Multiplayer is powered by a deterministic **60Hz Lockstep Protocol**. Every client executes the identical C++ simulation world locally using synchronized command frames dispatched over UDP/TCP.

### Key Netcode Performance Specs
- **Simulation Tick Rate**: Fixed 60 ticks per second (16.66ms per tick).
- **Target Input Delay**: Dynamic 2 - 4 ticks (33ms - 66ms) depending on client ping.
- **Desync Detection**: 64-bit state hash calculated every 10 ticks. Mismatched hashes immediately flag desync.

---

## 2. Matchmaking & Rating System

### Rating Engine (Glicko-2)
- Players have separate Skill Ratings (SR) for 1v1, 2v2, and 3v3 ladder queues.
- **Placement Matches**: 5 placement matches per season establish initial league rank.
- **Seasonal Ladders**: 6 Competitive Tiers:
  1. *Bronze*
  2. *Silver*
  3. *Gold*
  4. *Platinum*
  5. *Diamond*
  6. *Grandmaster* (Top 200 regional players).

---

## 3. Replay Engine & Spectator System

- **Replay Recording**: Automatically records all match command frames into `.ra4replay` binary files (~500 KB per 20-minute match).
- **Replay Playback**:
  - Instantaneous seeking via state snapshot checkpoints stored every 30 seconds.
  - Playback speed control (0.5x, 1.0x, 2.0x, 4.0x, 8.0x).
- **Spectator Mode**:
  - 2-minute spectator delay buffer for tournament broadcasts.
  - Fog-of-War controls: View Player 1, View Player 2, or Omniscient Vision.
  - Live production overlay, APM graphs, resource collection rates, and army value charts.

---

## 4. Anti-Cheat & Security Policy

1. **Deterministic Lockstep Authority**: Clients only send user input commands (`Move`, `Attack`, `Build`); client memory cannot cheat resource values or unit health because state is dictated by lockstep execution.
2. **Desync Abort**: If a client attempts to modify local simulation state illegally, state hash mismatch triggers an instant desync report, penalizing the cheating client.
