# Map Agent Handoff Documentation — Red Alert 4 Skirmish Production Map

## Overview
This document summarizes the creation, layout architecture, validation, and asset handoff details for the production skirmish map `/Game/Maps/RA4_Skirmish_Production`.

---

## 1. Created & Modified Assets

### Level Package
- **`/Game/Maps/RA4_Skirmish_Production.umap`** ([Content/Maps/RA4_Skirmish_Production.umap](file:///Users/romanmolodyko/Documents/red-alert-4-map/Content/Maps/RA4_Skirmish_Production.umap))

### Automation & Tools Scripts
- **`Tools/Editor/inventory_map_assets.py`** ([Tools/Editor/inventory_map_assets.py](file:///Users/romanmolodyko/Documents/red-alert-4-map/Tools/Editor/inventory_map_assets.py)) — Scans and generates asset inventory report.
- **`Tools/Editor/make_production_skirmish_map.py`** ([Tools/Editor/make_production_skirmish_map.py](file:///Users/romanmolodyko/Documents/red-alert-4-map/Tools/Editor/make_production_skirmish_map.py)) — Deterministically generates the production skirmish map.
- **`Tools/Editor/verify_production_skirmish_map.py`** ([Tools/Editor/verify_production_skirmish_map.py](file:///Users/romanmolodyko/Documents/red-alert-4-map/Tools/Editor/verify_production_skirmish_map.py)) — Scene validator script verifying landscape, base plateaus, resource fields, player starts, and navmesh.
- **`Tools/Editor/capture_map_screenshots.py`** ([Tools/Editor/capture_map_screenshots.py](file:///Users/romanmolodyko/Documents/red-alert-4-map/Tools/Editor/capture_map_screenshots.py)) — Viewport camera automation for level reporting.

### Saved Reports & Automation Artifacts
- **`Saved/Reports/MapAssetInventory.json`** — Scanned inventory of 565 assets across `Content/`.
- **`Saved/Automation/MapAgent/`** — Automated viewport view configurations (`MapOverview`, `Base1_Overview`, `Base2_Overview`, `OreFields_Overview`, `CentralFront_Overview`).

---

## 2. Layout Architecture & Match Flow Design

- **Map Dimensions**: 64 x 64 tiles (12,800 uu x 12,800 uu / 128 m x 128 m).
- **Match Target**: 15–20 minute competitive/skirmish match length.
- **Base Layout**:
  - **Base 1 Plateau (P0)**: Flat construction pad at `(2400, 2400, 0)` with safe starting ore field `RA4_OreField_Safe_Base1` at `(2400, 4200, 5)`.
  - **Base 2 Plateau (P1)**: Flat construction pad at `(10400, 10400, 0)` with safe starting ore field `RA4_OreField_Safe_Base2` at `(10400, 8600, 5)`.
- **Contested Resources & Front**:
  - **Contested Ore Field A**: Positioned at `(5000, 7800, 5)`.
  - **Contested Ore Field B**: Positioned at `(7800, 5000, 5)`.
  - **Central Combat Front**: Central choke point at `(6400, 6400, 0)`.
- **Movement Corridors**:
  - **Main Arterial Road**: Connects Base 1, Center, and Base 2 along `(6400, 6400)`.
  - **Outer Flank Road**: Connects Base 1 and Base 2 via contested ore fields bypassing central choke.
  - **Cliff Wall Boundaries**: North-West and South-East natural cliff ridges blocking direct diagonal lines and channeling tactical movement.

---

## 3. Lighting & Atmosphere Configuration

- **Directional Light (`RA4_Sun`)**: Intensity 75,000 Lux, Pitch -48.0, Yaw 145.0, Movable, `atmosphere_sun_light = True`.
- **Sky Light (`RA4_SkyLight`)**: Movable, intensity 1.2 Lux ambient fill, real-time capture enabled.
- **Sky Atmosphere (`RA4_SkyAtmosphere`)**: Rayleigh and Mie atmospheric sky dome scattering.
- **Height Fog (`RA4_ExponentialHeightFog`)**: Exponential height fog density 0.002.
- **Post Process Volume (`RA4_PostProcessVolume`)**: Unbound (`bUnbound = True`), tuned auto-exposure and RTS contrast.

---

## 4. Scene Validator Results

- Command:
  ```bash
  "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/RedAlert4.uproject" -run=PythonScript -script="$PWD/Tools/Editor/verify_production_skirmish_map.py" -nullrhi -unattended
  ```
- Result: **SUCCESS (All 20 required actors and validation checks passed)**.
  - `RA4_Landscape_MainGround` present.
  - Base 1 & Base 2 plateaus and player starts (`RA4_PlayerStart_P0`, `RA4_PlayerStart_P1`) present.
  - 4 Ore fields (2 safe, 2 contested) present.
  - `RA4_NavMeshBoundsVolume` covering entire map extent.
  - Zero underground actors found (Z < -100 uu).

---

## 5. Instructions for Re-generation

To re-generate the production skirmish map at any time:
```bash
"/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/RedAlert4.uproject" -run=PythonScript -script="$PWD/Tools/Editor/make_production_skirmish_map.py" -nullrhi -unattended
```

---

## 6. Art Asset Replacement List for Integrator

When integrating assets from the `art` agent branch, replace the level mesh components:
1. `RA4_Landscape_MainGround` -> Replace basic ground mesh with custom multi-layer landscape component.
2. `RA4_Base1_Plateau` & `RA4_Base2_Plateau` -> Replace pad meshes with faction-specific concrete paving meshes.
3. `RA4_Cliff_NorthWestRidge` & `RA4_Cliff_SouthEastRidge` -> Replace blockout cliff walls with high-density rock cliff assets (`SM_Cliff_Rock_01`).
4. `RA4_OreField_*` -> Replace cylinder placeholders with animated ore node crystals and extraction node props.
