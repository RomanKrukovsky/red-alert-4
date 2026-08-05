# Architectural Invariants (`INVARIANTS.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## Non-Negotiable System Invariants

1. **INVARIANT 1: SimWorld Engine Decoupling**
   - `RA4Simulation` and `RA4Core` must contain zero UObjects, zero Unreal Engine headers, and zero direct engine API calls.

2. **INVARIANT 2: Deterministic Fixed-Point Arithmetic**
   - All simulation calculations (positions, velocities, hitboxes, ranges) MUST use `FixedPoint.h`. Usage of `float` or `double` in simulation code is prohibited.

3. **INVARIANT 3: One-Way State Access**
   - The Presentation layer (`RA4PresentationSubsystem`) reads `SimWorld` snapshots to update visual actors. Visual actors MUST NEVER mutate `SimWorld` memory directly.

4. **INVARIANT 4: 60Hz Lockstep Execution**
   - All simulation state updates occur strictly within fixed 16.66ms tick steps. No logic may execute outside `CommandBus::DispatchTick`.

5. **INVARIANT 5: 64-Bit State Hash Validation**
   - State hashes are calculated every 10 ticks and validated across all peers in lockstep frames. Divergence triggers immediate desync abort.

6. **INVARIANT 6: Belief State Is Simulation State** (ADR-0026)
   - Each player's perceived world (`RA4Intel`) is deterministic simulation state: fixed-point only, updated only inside the tick, serialized with saves and fed into the state hash. Belief divergence between peers is a desync.

7. **INVARIANT 7: Belief Is the Only Enemy-Information Interface**
   - When the unreliable-intelligence layer is enabled, presentation, UI and the AI commander read enemy information exclusively through `SimWorld::GetIntel()` perceived tracks. `PerceivedTrack` carries no ground-truth `EntityId`; the track↔entity association never leaves the simulation core.

8. **INVARIANT 8: Intel Kill Switch Restores Classic Behaviour**
   - With the intel layer disabled (shipped default), simulation results are bit-identical to a build without the module. Pinned by test `Intel.DisabledLayerDoesNotChangeSimulationResults`.
