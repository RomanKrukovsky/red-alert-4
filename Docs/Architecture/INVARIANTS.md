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
