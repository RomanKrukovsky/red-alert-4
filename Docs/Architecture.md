# Architecture

## The one rule

The simulation is the only source of truth, and it knows nothing about Unreal.

`RA4Core`, `RA4Content`, `RA4Simulation` and `RA4Replay` contain no `UObject`, no
`AActor`, no rendering type and no engine header outside the module boilerplate.
They compile with a plain toolchain in about two seconds, which is why the
determinism suite can run on every commit instead of on every merge.

Everything else -- Actors, Niagara, animation, UMG, audio -- observes the simulation
and never writes to it.

```
input / AI / mission script
          |  Command
          v
   +----------------+        SimEvent        +------------------+
   |   SimWorld     | ---------------------> |  presentation    |
   | (authoritative)|                        |  (Actors, UI,    |
   +----------------+ <--------------------- |   audio, VFX)    |
          ^            (reads state, never    +------------------+
          |             writes it)
     CommandFrame from the server
```

## Modules

| Module | Contains | Depends on |
| --- | --- | --- |
| `RA4Core` | fixed-point math, RNG, entity ids, commands, serialization, checksums, tick constants | - |
| `RA4Content` | data definitions for units, buildings, weapons, factions; damage table; validation; content hash | `RA4Core` |
| `RA4Simulation` | match state, SoA storage, system scheduler, all gameplay systems | `RA4Core`, `RA4Content`, `RA4Navigation` |
| `RA4Replay` | replay container, recording, playback, checksum verification | above |
| `RA4Navigation` | deterministic tile topology, clearance, sectors, portals, group flow fields | `RA4Core` |
| `RA4Campaign` | campaign and mission data registry, prerequisite progression | `RA4Core` |
| `RedAlert4` | Unreal game module: presentation, input, UI, networking glue | all of the above |

Dependencies point one way. Nothing in the simulation may include `RedAlert4`.

## Determinism

The simulation must produce bit-identical state from identical inputs on every
platform and at every optimisation level. Concretely:

* **No floating point anywhere in the simulation.** All positions, speeds, damage
  and ranges are `Fixed` (48.16). Trigonometry is integer CORDIC, square root is an
  integer restoring algorithm. `double` appears only in `ToDoubleUnsafe()` for logs
  and in tests that check the integer implementations against libm.
* **One RNG stream.** Every stochastic decision draws from the match `Random`.
  Presentation randomness must never touch it.
* **Fixed system order.** Listed in `SimWorld::Tick`. Changing it changes results and
  therefore requires a replay format version bump.
* **No container-order dependence.** `std::unordered_map` is used for lookup only;
  anything that feeds state iterates insertion-ordered vectors or sorted arrays.
* **Deferred destruction.** Entities die at a single point in the tick so system
  iteration is never invalidated mid-pass.
* **Reserved storage.** Component vectors reserve the whole entity budget, so a
  spawn during iteration cannot reallocate under a system holding a reference.

The checksum in `ComputeStateChecksum()` covers every field that can influence future
state and deliberately excludes caches and event lists.

## Why not Chaos, not MassAI-only, not a Blueprint template

Chaos Physics is not deterministic across platforms and is not authoritative for
anything: movement, collision and hits are resolved by the simulation, and physics
is used only for debris and ragdolls that no one else observes.

MassEntity is the intended backend for massed cheap units, but it is reached through
`RA4Simulation`'s own storage and movement interfaces rather than directly, so an API
change in an experimental subsystem cannot stall the project.

A Blueprint-only marketplace template cannot be the core: it is not testable
headless, not deterministic, and its lifetime is someone else's decision.
