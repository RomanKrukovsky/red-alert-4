# Architectural Invariants (`INVARIANTS.md`)

**Document Version**: 3.2 (corrected against Source/RA4Core/SimConfig.h — prior version claimed 60Hz/16.66ms and FixedPoint.h, contradicting the actual code)  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## Non-Negotiable System Invariants

1. **INVARIANT 1: SimWorld Engine Decoupling**
   - `RA4Simulation` and `RA4Core` must contain zero UObjects, zero Unreal Engine headers, and zero direct engine API calls.

2. **INVARIANT 2: Deterministic Fixed-Point Arithmetic**
   - All simulation calculations (positions, velocities, hitboxes, ranges) MUST use `RA4Core/Fixed.h` (`RA4::Fixed`). Usage of `float` or `double` in simulation code is prohibited.

3. **INVARIANT 3: One-Way State Access**
   - The Presentation layer (`RA4PresentationSubsystem`) reads `SimWorld` snapshots to update visual actors. Visual actors MUST NEVER mutate `SimWorld` memory directly.

4. **INVARIANT 4: Fixed-Tick Lockstep Execution (20 Hz)**
   - All simulation state updates occur strictly within fixed 50 ms tick steps (`kTicksPerSecond = 20`, `SimConfig.h`). The presentation layer may render at 60+ FPS by interpolation; render rate never changes simulation results. No logic may execute outside `CommandBus::DispatchTick`.

5. **INVARIANT 5: 64-Bit State Hash Validation**
   - State hashes are calculated every 20 ticks (`kChecksumIntervalTicks`, `SimConfig.h`) and validated across all peers in lockstep frames. Divergence triggers immediate desync abort.

6. **INVARIANT 6: Belief State Is Simulation State** (ADR-0026)
   - Each player's perceived world (`RA4Intel`) is deterministic simulation state: fixed-point only, updated only inside the tick, serialized with saves and fed into the state hash. Belief divergence between peers is a desync.

7. **INVARIANT 7: Belief Is the Only Enemy-Information Interface**
   - When the unreliable-intelligence layer is enabled, presentation, UI and the AI commander read enemy information exclusively through `SimWorld::GetIntel()` perceived tracks. `PerceivedTrack` carries no ground-truth `EntityId`; the track↔entity association never leaves the simulation core.

8. **INVARIANT 8: Intel Kill Switch Restores Classic Behaviour**
   - With the intel layer disabled (shipped default), simulation results are bit-identical to a build without the module. Pinned by test `Intel.DisabledLayerDoesNotChangeSimulationResults`.

9. **INVARIANT 9: Belief Is Written Only By The Simulation** (ADR-0021 K1)
   - No code outside `RA4Simulation`/`RA4Intel` may mutate any player's perceived world. A public mutable accessor to belief state is a violation of this invariant, not a convenience.
   - **Fixed 2026-08-05** (branch `fix/intel-invariant-blockers`): `GetPerceivedWorldMutable` removed; all `PerceivedWorld` writer methods are private, reachable only by `IntelSystem` (friend) and the deterministic test harness via `PerceivedWorldTestAccess`.

10. **INVARIANT 10: No Ground Truth In The Belief Read Surface** (ADR-0021 K3)
   - Any type handed to presentation, UI or the AI commander must contain no field that reveals objective truth about entities the reading player does not own — including flags describing whether a contact is real. A comment saying a field is internal does not make it internal.
   - **Fixed 2026-08-05** (branch `fix/intel-invariant-blockers`): `bPhantom` removed from `PerceivedTrack`; phantom truth lives in a core-internal side table (`PerceivedWorld::PhantomFlags`), private accessors only. Pinned by test `Intel.PhantomTruthLivesOutsideTheReadSurface`, whose `static_assert` mirror of the read-surface layout fails compilation if a field is added to `PerceivedTrack` without review.

11. **INVARIANT 11: Belief Is Replay-Reconstructible** (ADR-0021 K2)
   - "What did player P believe at tick T" must be answerable from a replay plus a player id alone. Belief may not depend on any state that is not in the replay.
   - **Not yet verified**: no test reconstructs a belief view from a replay. Gates M1.
