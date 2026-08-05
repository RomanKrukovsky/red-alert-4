# ADR 0008 - Data Asset Damage Matrix and Content Hot-Reload Validation

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted.

## Context

RTS combat balance requires complex interactions between weapon damage types (Kinetic, High-Explosive, Energy, EMP, Chrono) and target armor classes (Light Infantry, Exoskeleton, Light Vehicle, Heavy Armor, Reinforced Building). Hardcoding damage multipliers in C++ or relying solely on generic `GameplayTags` checks without structured data assets leads to maintenance overhead, lack of designer transparency, and potential replay desynchronizations if content values are altered mid-patch.

## Decision

1. **Structured Data Asset Matrix**:
   - Armor profiles (`UArmorProfileData`), Weapon profiles (`UWeaponProfileData`), and the master Damage Interaction Matrix (`UDamageMatrixData`) are implemented as primary C++ Data Assets.
   - `GameplayTags` are strictly restricted to entity/system classification (e.g. `Role.Combat.AntiAir`, `Status.Effect.ChronoSickness`), while quantitative multipliers are defined within `UDamageMatrixData`.
2. **Combat Calculation Pipeline**:
   - Damage calculations evaluate: `FinalDamage = BaseDamage * DamageMatrixLookup(WeaponProfile, TargetArmorProfile) * ArmorMitigation * ElevationModifier * DirectHitFactor`.
3. **Hot-Reload Safety & Content Validation**:
   - During editor development, balance Data Assets can be hot-reloaded.
   - During multiplayer handshakes and replay verification, a 64-bit content hash (`ComputeContentHash()`) of all active Data Assets is computed. If a client has modified Data Assets locally, server connection and replay playback are safely aborted to prevent desynchronizations.

## Consequences

**Positive**:
- Designers can adjust game balance in Data Assets without recompiling C++ code.
- Replays recorded on legacy balance patches are protected from silent calculation errors.
- Combat logic executes via O(1) array index lookups between armor and weapon type IDs.

**Negative**:
- Designers must maintain the matrix table when adding new armor or weapon classes.
