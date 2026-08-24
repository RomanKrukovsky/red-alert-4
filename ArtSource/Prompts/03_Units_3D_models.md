# BLOCK 3 — 3D game-ready models

Заменить блок `01_Style_common.md` на этот; список юнитов 1–39 из `02_Units_2D_concepts.md` оставить тем же.

```
PROJECT: SCARLET HORIZON — RTS game-ready 3D models
STYLE: production 3D asset, game-ready mesh, PBR material standards,
clean topology, quads-only, UVs unwrapped, no N-gons, no floating
vertices, scale in meters (infantry ~1.8m, light vehicle ~4m, MBT ~7m,
superheavy ~12m). 2048–4096 textures: albedo, normal, ORM (packed
occlusion/roughness/metallic), emissive for energy/optics. Unreal
Engine 5 compatible, LOD0 + LOD1 + LOD2, collision proxy mesh. Neutral
grey studio render for preview.
WORKFLOW: concept turnaround (front/side/rear/3-4 + top for vehicles),
then T-pose or neutral pose for infantry, then baked preview render.
NO textures baked from photos; original hand-painted + procedural PBR.
Faction color only on trim/insignia/emissive, not full body.
```