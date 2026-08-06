# Red Alert 4 — Art & Presentation Agent Handoff

## Overview & Working Environment
- **Target Repository**: `<home>/Documents/red-alert-4-art`
- **Active Branch**: `agents/skirmish-art`
- **Primary Goal**: Complete production-ready visual representations, materials, sockets, construction stages, VFX, audio routing, and data-driven presentation mappings for USSR and Alliance factions without mutating core simulation (`SimWorld`, `AICommander`, economy, or `RA4_Skirmish_Production`).

---

## 1. Asset Inventory & Audit
- **Report Location**: `Saved/Reports/ArtAssetInventory.json`
- **Total Assets Scanned**: 565 assets indexed across StaticMesh, SkeletalMesh, Skeleton, AnimationSequence, AnimBlueprint, Material, Texture, Niagara, SoundWave, SoundCue, MetaSound, and DataAsset.
- **Scanner Tool**: `Tools/Editor/scan_art_assets.py`

---

## 2. Data-Driven Presentation Mapping
- **Mapping Class**: `URA4ArtMappingDataAsset` (`Source/RA4Presentation/Public/RA4Presentation/RA4ArtMapping.h`)
- **Data Asset Location**: `/Game/RA4/Art/Generated/DA_RA4_ArtMappings`

### Role Coverage Matrix (USSR & Alliance)
| Faction | Role | Bible ID | Asset Object Path | Sockets Configured |
|---|---|---|---|---|
| USSR | HQ (ConYard) | `SU_ConYard` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ConYard_Blockout` | Footprint, Origin |
| USSR | Power Plant | `SU_PowerPlant` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_PowerPlant_Blockout` | Chimney, CableSocket |
| USSR | Refinery | `SU_Refinery` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Refinery_Blockout` | UnloadBay, OreChute |
| USSR | Barracks | `SU_Barracks` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Barracks_Blockout` | RallyPoint, ExitDoor |
| USSR | War Factory | `SU_WarFactory` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_WarFactory_Blockout` | ExitBay, CraneSocket |
| USSR | Sentry Turret | `SU_SentryTurret` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SentryTurret_Blockout` | `Socket_Turret`, `Socket_Muzzle` |
| USSR | Harvester | `SU_Harvester` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Harvester_Blockout` | `Socket_Cargo`, `Socket_Engine` |
| USSR | Base Infantry | `SU_Conscript` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Conscript_Blockout` | `Socket_Muzzle` |
| USSR | Anti-Tank Infantry | `SU_ShockTrooper` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ShockTrooper_Blockout` | `Socket_Muzzle` |
| USSR | Support | `SU_Commissar` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Commissar_Blockout` | `Socket_Aura` |
| USSR | Scout Vehicle | `SU_Sickle` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SickleScout_Blockout` | `Socket_Turret`, `Socket_Engine` |
| USSR | Main Tank | `SU_HammerTank` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_HammerTank_Blockout` | `Socket_Turret`, `Socket_Muzzle` |
| USSR | AA Vehicle | `SU_Flak` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_FlakTrooper_Blockout` | `Socket_Turret`, `Socket_Muzzle` |
| USSR | Artillery | `SU_Buratino` | `/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Buratino_Blockout` | `Socket_Turret`, `Socket_Muzzle` |
| Alliance | HQ (ConYard) | `AL_ConYard` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ConYard_Blockout` | Footprint, Origin |
| Alliance | Power Plant | `AL_PowerPlant` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PowerPlant_Blockout` | ReactorCore |
| Alliance | Refinery | `AL_Refinery` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Refinery_Blockout` | UnloadBay |
| Alliance | Barracks | `AL_Barracks` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Barracks_Blockout` | RallyPoint |
| Alliance | War Factory | `AL_WarFactory` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_WarFactory_Blockout` | ExitBay |
| Alliance | Defense Turret | `AL_MultigunTurret` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_MultigunTurret_Blockout` | `Socket_Turret`, `Socket_Muzzle` |
| Alliance | Harvester | `AL_Prospector` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Prospector_Blockout` | `Socket_Cargo`, `Socket_Engine` |
| Alliance | Base Infantry | `AL_Peacekeeper` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Peacekeeper_Blockout` | `Socket_Muzzle` |
| Alliance | Anti-Tank Infantry | `AL_Javelin` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Javelin_Blockout` | `Socket_Muzzle` |
| Alliance | Support | `AL_Medic` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Medic_Blockout` | `Socket_Aura` |
| Alliance | Scout Vehicle | `AL_Jackal` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Jackal_Blockout` | `Socket_Turret`, `Socket_Engine` |
| Alliance | Main Tank | `AL_GuardianTank` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Guardian_Blockout` | `Socket_Turret`, `Socket_Muzzle` |
| Alliance | AA Vehicle | `AL_Aegis` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_AegisShield_Blockout` | `Socket_Turret`, `Socket_Muzzle` |
| Alliance | Artillery | `AL_Athena` | `/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Athena_Blockout` | `Socket_Turret`, `Socket_Muzzle` |

---

## 3. Visual Construction Stages
All buildings map to 5 distinct construction stages in Data Asset:
1. **Stage 0 (Delivery)**: Drop pod / transport frame delivery mesh
2. **Stage 1 (Foundation)**: Concrete slab & rebar foundation
3. **Stage 2 (Structure)**: Scaffolding and main structural chassis
4. **Stage 3 (Wiring)**: Power conduits & electrical connection sparks
5. **Stage 4 (Active)**: Fully functional active building

---

## 4. Materials & Shading System
- **Master PBR Material**: `/Game/RA4/Presentation/Materials/Blockout/M_RA4_BlockoutStructure`
- **Team Color Masking**: Team colors applied via scalar/vector parameter blend rather than solid red/blue fill.
- **Surface Presets**:
  - USSR Painted Metal: `/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_SU`
  - Alliance Painted Metal: `/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_AL`
  - Neutral / Concrete: `/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_Neutral`
  - Construction Stage: `/Game/RA4/Presentation/Materials/Blockout/MI_RA4_Blockout_UnderConstruction`

---

## 5. Audio & Voice Routing System
- **Audio Routing Subsystem**: Master -> Music / SFX / Voice / UI.
- **Voice Logic**: 
  - Voice Cooldown: 3.0s minimum delay between unit voice responses.
  - Repetition Prevention: Tracks last played line per unit ID to prevent repeating the same voice line twice.
  - Concurrency Limiter: Capped to 3 simultaneous active voice lines globally.

---

## 6. Verification Map
- **Map Path**: `/Game/Maps/RA4_ArtLab`
- **Contents**: Showcases USSR and Alliance buildings across all 5 construction stages, vehicle turrets & sockets, infantry poses, and material test stands.

---

## 7. Instructions for Integration Engineer
1. Load `DA_RA4_ArtMappings` from `/Game/RA4/Art/Generated/DA_RA4_ArtMappings` in `RA4EntityActor` or presentation subsystem.
2. Query `FindUnitArt(UnitId)` or `FindBuildingArt(BuildingId)` to retrieve meshes, sockets, materials, and audio triggers dynamically.
3. Attach weapon effects and projectiles to `Socket_Muzzle` and `Socket_Turret` returned by `FRA4UnitArtDefinition`.
