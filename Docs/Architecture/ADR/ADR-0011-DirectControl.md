# ADR-0011: Authoritative Direct Vehicle Control

**Status:** Accepted (Stage 1 — simulation foundation IMPLEMENTED, presentation UNVERIFIED)
**Date:** 2026-08-04
**Decider:** Tech Director (GLM 5.2)

## Context

The product requires a first-person direct-control mode: at any moment during an
RTS match a player can press F on a selected compatible vehicle, the camera
moves inside, and the player drives / aims / fires manually. The rest of the
match (economy, AI, fog of war, network, replay) keeps running.

The architecture has a hard rule (CLAUDE.md, ADR-0002): the simulation is the
single source of truth. Presentation never mutates state. Every gameplay change
goes through a `Command`. This is what makes replay, lockstep and server
authority the same mechanism rather than three parallel systems.

## Decision

Direct control is **not** an exception to the command rule. It is four new
`CommandType` values that flow through the existing `CommandBus` like any
other order:

| Command | Effect |
|---|---|
| `DirectControlEnter` | Server validates ownership + liveness + eligibility, flips `DirectControlComp::Phase` Inactive→Entering, clears current orders. |
| `DirectControlExit` | Phase→Exiting, hands vehicle back to AI with a Stop. |
| `DirectControlDrive` | Quantized int8 throttle/steering/turret-yaw/turret-pitch + flags. Server re-scales using the UnitDef's speed/turn limits. |
| `DirectControlFire` | Flags select primary/secondary. Server reuses the **same** `FireWeapon` path as RTS — no first-person-only damage buff. |

Client never owns physics, movement, or damage. It owns input sampling,
camera, HUD, audio and VFX. The local `ARA4EntityActor` is **not** driven by
Chaos; it is interpolated from `SimWorld::TransformComp` exactly as in RTS
mode. This preserves determinism, replay, server authority, and the weapon
balance matrix.

### State

`DirectControlComp` is a new SoA column indexed by `EntityId::Index`, same as
all other components. Its phase machine is the only authoritative source of
"who is driving what":

```
Inactive → Entering → Active → Exiting → Inactive
                ↓                       ↓
            VehicleDestroyed         (cleared by SystemDirectControl)
```

`DestroyEntity` emits `DirectControlExited` so the presentation layer can
return the player to RTS view when the vehicle dies under them.

### Serialization

`DirectControlComp` is part of the save/replay stream. Save version bumped
from 1 to 2. The checksum feeds DirectControl fields so desyncs surface.
Replays recorded with v1 are not compatible; this is acceptable because
v1 was never shipped.

### Eligibility

The simulation enforces only safety (ownership + liveness + armed-or-turreted).
The presentation `DirectControlProfile` DataAsset (Stage 2) decides which
vehicles the client *offers* for possession. A unit with no weapon and no
turret is rejected with `DirectIneligibleUnit`.

## Alternatives considered

1. **Chaos physics on the client.** Rejected: violates ADR-0002, breaks
   determinism, makes replay impossible, desyncs server authority. The prompt
   explicitly forbids this.
2. **Parallel mini-game scene.** Rejected: the prompt requires the rest of
   the match to continue in real time.
3. **Possession via Unreal `Possess`.** Rejected: it would make the pawn
   authoritative, same problem as (1).
4. **Drive commands as raw floats.** Rejected: floats are not deterministic
   across ABIs. Quantized int8 axes keep the command fixed-size and ABI-stable.

## Status

| Layer | Status |
|---|---|
| Core command types | IMPLEMENTED |
| SimWorld validation | IMPLEMENTED |
| SystemDirectControl | IMPLEMENTED |
| Serialize/Deserialize | IMPLEMENTED |
| Checksum | IMPLEMENTED |
| Headless tests | IMPLEMENTED (8 passing) |
| UE-side subsystem | IMPLEMENTED |
| DirectControlProfile DataAsset | IMPLEMENTED |
| DirectControlComponent (presentation) | IMPLEMENTED (subsystem + controller wiring) |
| Camera sockets on Granit mesh | DECLARED |
| Combat HUD ViewModel | IMPLEMENTED (UMG/MVVM, no NoesisGUI plugin installed) |
| Combat HUD Widget Blueprint | DECLARED |
| MetaSound interior mix | DECLARED |
| Network replication | DECLARED |
| Test map | DECLARED |
| Runtime/packaging verification | UNVERIFIED |

## Consequences

- Replay format is version-bumped. Old replays incompatible.
- Save format is version-bumped. Old saves incompatible.
- Every vehicle is potentially direct-controllable from the simulation's
  perspective; the profile is the gate. This is intentional so the same
  code path serves tanks, walkers, ships and aircraft.
- The first-person camera cannot be authoritative. Camera placement is a
  presentation concern derived from the authoritative turret-facing field
  in `TransformComp`.

## Follow-ups (Stage 2+)

1. `URedAlert4DirectControlSubsystem` (UE) — reads `DirectControlComp` and drives the camera.
2. `UDirectControlProfile` DataAsset — per-vehicle camera, sensitivity, HUD, sockets.
3. Sockets on `SM_Soviet_SU_GranitMBT`: `DirectControlCamera`, `GunnerSight`, `CommanderSight`, `MuzzlePrimary`, `MuzzleSecondary`.
4. UMG/MVVM combat HUD with real ViewModel bindings (no NoesisGUI plugin is installed; see ADR-0006).
5. MetaSound layers for engine/tracks/turret/fire.
6. Network replication of `DirectControlComp::Controller` + rate-limited command submission.
7. Test map + functional tests + packaged smoke test.