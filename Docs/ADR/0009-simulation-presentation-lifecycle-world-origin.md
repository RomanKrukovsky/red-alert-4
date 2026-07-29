# ADR 0009 - Simulation-to-Presentation Lifecycle, Spatial Picking, and World-Origin Mapping

Status: accepted.

## Context

Running thousands of simulation entities requires efficient mapping between C++ fixed-point simulation space and Unreal 3D world space. Furthermore, spatial picking (single clicks vs box selection) and displaying UI healthbars for 500+ visible units can easily bottleneck rendering if each entity instantiates standalone physics colliders and UMG UserWidgets.

## Decision

1. **Simulation Space ↔ World Space Quantization**:
   - Fixed-point 48.16 simulation coordinates map directly to Unreal World Space via quantization scale factors (1 SimUnit = 1 Unreal Centimeter).
   - For large maps, simulation coordinates remain double-precision fixed-point integers to avoid float precision degradation.
2. **Entity Selection Architecture (Spatial Picker)**:
   - **Single Click**: Executes 1 lightweight raycast against terrain geometry / presentation Actor bounds to obtain a world coordinate, followed by an O(log N) spatial hash lookup in the C++ simulation grid.
   - **Box Selection (Drag-Select)**: Executes 0 physics raycasts. The selection rectangle is transformed into a 3D frustum / AABB, which queries the C++ simulation's spatial hash grid directly in C++.
3. **Tiered UI & Healthbar Rendering**:
   - **Tier 1 (Selected, Damaged, Hero, Superweapon Units)**: Rendered via a single batched Slate/Canvas custom rendering pass or GPU UI Instanced Shader.
   - **Tier 2 (Offscreen, Full-Health, Unimportant Units)**: Healthbar rendering is completely culled. No individual UMG `UUserWidget` components are spawned per unit.
4. **Presentation Lifecycle & Object Pooling**:
   - Presentation Actors and Mass Representation instances observe `SimWorld` events.
   - Destroyed units return their visual representation to an object pool (`PresentationObjectPool`) to avoid runtime GC stalls and heap allocations during massive battles.

## Consequences

**Positive**:
- Drag-selecting 300 units executes instantaneously without physics scene locking.
- Slate UI rendering overhead remains flat regardless of unit counts.
- Zero garbage collection stalls from unit spawning and destruction.

**Negative**:
- Presentation adapters must maintain pooling state and sync visual transforms smoothly across simulation ticks.
