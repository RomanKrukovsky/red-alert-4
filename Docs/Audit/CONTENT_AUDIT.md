# Content & Asset Inventory Audit (`CONTENT_AUDIT.md`)

**Audit Date**: August 4, 2026  
**Scope**: Project content files, 3D models, textures, audio, maps, and gameplay data JSONs.

---

## 1. Gameplay Data & Bible Integrity

### Data Source Baseline
- Master Markdown Specification: `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` (148 KB).
- Normalized Runtime Data: `Content/RA4/Data/Generated/ra4_content.normalized.json`.
- Importer Validation (`TestBibleImport.cpp`):
  - 4 Factions defined: Soviets, Alliance, Eastern Coalition, Chrono Legion.
  - 78 Unique Unit Definitions verified.
  - 35 Building footprint and power definitions.
  - Damage Matrix: Complete warhead/armor multiplier matrix (Ballistic, Fragmentation, ArmorPiercing, Siege, Electric, AntiAir vs LightInfantry, HeavyVehicle, Building, Air).
  - Veterancy Ranks: Recruit, Veteran (+10% damage), Elite (+25% damage), Heroic (passive self-heal).

---

## 2. 3D Models & Mesh Inventory

### A. Blockout Model Library (`Content/RA4/Art/Blockout/`)
- **Count**: 142 generated FBX blockout models covering all 4 factions.
- **Coverage**: 100% of units and structures have functional collision and visual bounds mesh representations.
- **Status**: Loaded dynamically via `URA4ArtMapping`.

### B. High-Poly PBR Production Models (`Content/RA4/Art/`)
- **Vertical Slice Models**: 36 PBR textured models (8 Soviet, 8 Alliance, 8 Coalition, 8 Chrono, 4 Neutral).
- **Format**: SkeletalMesh & StaticMesh with PBR material instances (Albedo, Normal, Roughness, Metallic, Faction Color Mask).

---

## 3. Audio & Voice Lines Inventory (`Audio/`, `GeneratedVO/`)

- **EVA & Unit Voice Lines**: 624 audio events defined across 4 factions.
- **Voice Sets**:
  - Soviet Commander / Units: Russian accented voice lines authored in `GeneratedVO/Soviet/`.
  - Alliance Commander / Units: US/UK accented voice lines in `GeneratedVO/Alliance/`.
  - Coalition Commander / Units: Asian accented voice lines in `GeneratedVO/Coalition/`.
  - Chrono Legion: Synthesized temporal voice filters in `GeneratedVO/Chrono/`.
- **Sound Effects (SFX)**: Weapon fire, explosion SFX, building placement, harvester mining loop, GUI clicks in `Audio/SFX/`.

---

## 4. Maps & Environment Inventory (`Content/Maps/`)

1. **`M_Skirmish_Desert`** (`Content/Maps/M_Skirmish_Desert.umap`): Primary 2-player skirmish map with desert terrain, 2 starting base spots, 4 ore fields, and cliffs.
2. **`M_ProvingGround`** (`Content/Maps/M_ProvingGround.umap`): Headless stress test & unit battle map.
3. **`M_Campaign_Ch01_M01`** (`Content/Maps/Campaign/`): Chapter 1 Mission 1 intro campaign map.

---

## 5. VFX, Materials & Terrain Surfaces

- **Niagara Systems**: Weapon muzzle flashes, projectile trails, energy beams, Tesla coil arcs, nuclear explosion mushrooms in `Content/RA4/VFX/`.
- **Landscape & Terrain**: Landscape material using 3-layer blend (Sand, Rock, Ore Deposit).
- **Selection Decals**: Dynamic team-color selection rings (`M_SelectionDecal`).
