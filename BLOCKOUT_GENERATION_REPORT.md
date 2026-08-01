# RA4 — Report on the generation of a set of blockout models (v2.0 Production Blockout)

**Date:** 2026-07-29  
**Status:** A complete set of 140 original Success blockout models has been generated, exported to FBX and linked to the asset registry.  
**Compliance with Naming Reset rules:** 100% (only new Stable IDs from the bible v2.0 were used).

---

## 1. General overview of the work performed

A complete functional set has been created **140 3D blockout models** for 4 factions:
- **USSR (`Soviet`)**: 16 buildings + 19 units/equipment = 35 objects
- **Alliance (`Alliance`)**: 16 buildings + 19 units/equipment = 35 objects
- **Eastern Coalition (`Coalition`)**: 16 buildings + 19 units/equipment = 35 objects
- **Chronolegion**: 16 buildings + 19 units/vehicles = 35 objects

all objects are made to the exact scale of Unreal Engine 5 (1 unit = 1 cm), with reference points (Pivot) in the center of the base at ground level `(0,0,0)`, correct Footprint dimensions (in a grid of cells 200x200 cm) and sockets for towers, guns, spawn, effects and inputs.

---

## 2. Factional visual identity

### 2.1. USSR (`Soviet`)
- **Style:** Heavy Soviet-Russian industrial school. Armor plates with screws, monumental shapes, reactor tubes, caterpillar tracks, rectangular silhouettes.
- **Color Palette:** Dark gray base metal (`#4D5966`) with bright scarlet red markers (`#D62828`).

### 2.2. Alliance (`Alliance`)
- **Style:** Modular US-NATO military Vehicles. Sharp armor bevel angles, composite materials, prismatic emitters, modular canopies.
- **Color palette:** Light blue-gray body (`#5C6B73`) with cobalt blue elements (`#0077B6`).

### 2.3. Eastern Coalition (`Coalition`)
- **Style:** Synthesis of Chinese, Japanese and Indian engineering schools. Octagonal and pagoda-shaped elements, multi-legged walkers, raised bows of ships.
- **Color Palette:** Bronze slate base metal (`#3D4A41`) with jade green markers (`#2A9D8F`).

### 2.4. Chronolegion
- **Style:** Experimental temporal constructions. Floating stasis rings, quantum crystals, asymmetrical prismatic bodies.
- **Color Palette:** Dark purple slate body (`#3A1240`) with magenta blue highlights (`#7209B7` / `#4CC9F0`).

---

## 3. Summary register of generated objects

### 3.1. USSR (`Soviet`)
| Stable ID | Class | Dimensions (WxLxH, cm) | Footprint | Moving Parts / Sockets | Path to FBX |
| --- | --- | --- | --- | --- | --- |
| `SU_MCV_MobileYard` | Building | 800x700x500 | 4x4 | Housing, Antennas / `SOCKET_Spawn`, `SOCKET_Entrance` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MCV_MobileYard_Blockout.fbx` |
| `SU_ConYard` | Building | 800x800x600 | 4x4 | Headquarters Castle, Tower / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ConYard_Blockout.fbx` |
| `SU_PowerPlant` | Building | 600x600x500 | 3x3 | Cooling towers, thermal power plants / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_PowerPlant_Blockout.fbx` |
| `SU_Refinery` | Building | 800x800x450 | 4x4 | Loading ramp, Silo / `SOCKET_Entrance` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Refinery_Blockout.fbx` |
| `SU_Barracks` | Building | 600x600x400 | 3x3 | Gate, Shooting Range / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Barracks_Blockout.fbx` |
| `SU_WarFactory` | Building | 800x800x500 | 4x4 | Hangar, Crane / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_WarFactory_Blockout.fbx` |
| `SU_Airfield` | Building | 800x800x450 | 4x4 | Runway, Tower / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Airfield_Blockout.fbx` |
| `SU_NavalYard` | Building | 1000x1000x500 | 5x5 | Doc, Pierce / `SOCKET_Spawn` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_NavalYard_Blockout.fbx` |
| `SU_Radar` | Building | 400x400x700 | 2x2 | Radar dish / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Radar_Blockout.fbx` |
| `SU_TechCenter` | Building | 600x600x550 | 3x3 | Energokupol / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_TechCenter_Blockout.fbx` |
| `SU_GunTurret` | Building | 400x400x300 | 2x2 | Turret, Barrel / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GunTurret_Blockout.fbx` |
| `SU_AATurret` | Building | 400x400x350 | 2x2 | Missile block / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_AATurret_Blockout.fbx` |
| `SU_TeslaTower` | Building | 400x400x550 | 2x2 | Reel Perun / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_TeslaTower_Blockout.fbx` |
| `SU_Bunker` | Building | 400x400x250 | 2x2 | Embrasure / `SOCKET_Entrance` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Bunker_Blockout.fbx` |
| `SU_SuperweaponDome` | Building | 1000x1000x850 | 5x5 | Generator dome / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SuperweaponDome_Blockout.fbx` |
| `SU_SuperweaponSilo` | Building | 1000x1000x950 | 5x5 | Rocket mine / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SuperweaponSilo_Blockout.fbx` |
| `SU_RubezhRifleman` | Infantry | 60x60x180 | 1x1 | Torso, AK / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RubezhRifleman_Blockout.fbx` |
| `SU_ZapalGrenadier` | Infantry | 70x70x185 | 1x1 | Backpack, Grenade Launcher / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZapalGrenadier_Blockout.fbx` |
| `SU_ZaslonAATeam` | Infantry | 65x65x180 | 1x1 | MANPADS / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZaslonAATeam_Blockout.fbx` |
| `SU_MasterEngineer` | Infantry | 60x60x175 | 1x1 | Sapper's Backpack / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MasterEngineer_Blockout.fbx` |
| `SU_RazryadTrooper` | Infantry | 80x80x195 | 1x1 | Tesla suit / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RazryadTrooper_Blockout.fbx` |
| `SU_VektorOfficer` | Infantry | 60x60x180 | 1x1 | Radio station / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_VektorOfficer_Blockout.fbx` |
| `SU_BogatyrOreCarrier` | Collector | 600x360x280 | 1x1 | Bucket, Wheels / `SOCKET_VFX` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BogatyrOreCarrier_Blockout.fbx` |
| `SU_RysScout` | Light Vehicles | 380x220x180 | 1x1 | Wheels, Machine Gun / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RysScout_Blockout.fbx` |
| `SU_GranitMBT` | Tank | 550x330x220 | 1x1 | Turret, Barrel, Tracks / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GranitMBT_Blockout.fbx` |
| `SU_ZarevoMLRS` | Artillery | 600x320x250 | 1x1 | Missile Pack, Cabin / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZarevoMLRS_Blockout.fbx` |
| `SU_GromoboyRam` | Special equipment | 580x340x240 | 1x1 | Electric ram / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GromoboyRam_Blockout.fbx` |
| `SU_VoevodaHeavyTank` | Heavy Tank | 720x460x350 | 1x1 | Two towers, Trunks / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_VoevodaHeavyTank_Blockout.fbx` |
| `SU_KrechetInterceptor` | Aviation | 650x500x180 | 1x1 | Wings, Nozzles / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_KrechetInterceptor_Blockout.fbx` |
| `SU_KorshunGunship` | Aviation | 700x550x250 | 1x1 | Screw, Turret / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_KorshunGunship_Blockout.fbx` |
| `SU_GromadaAirship` | Aviation | 1300x600x450 | 1x1 | Airship Hull, Bomb Bay / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GromadaAirship_Blockout.fbx` |
| `SU_BuranPatrolBoat` | Naval | 1000x350x280 | 1x1 | Hull, Tower / `SOCKET_Turret`, `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BuranPatrolBoat_Blockout.fbx` |
| `SU_MorokSubmarine` | Naval | 1400x350x320 | 1x1 | Cabin, Torpedo tube / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_MorokSubmarine_Blockout.fbx` |
| `SU_SvyatogorCruiser` | Naval | 2000x600x500 | 1x1 | Chopping, Missile silos / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_SvyatogorCruiser_Blockout.fbx` |
| `SU_Hero_Morozova` | Hero | 70x70x185 | 1x1 | Rifle, Suit / `SOCKET_Muzzle` | `Content/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Hero_Morozova_Blockout.fbx` |

*(Tables for `Alliance`, `Coalition` and `Chronolegion` are done in a similar way and are recorded in the CSV manifest `Content/RA4/Art/Blockout/Blockout_Manifest.csv`)*.

---

## 4. Quality and verification of finished FBX assets
1. **Pivot & Transforms:** all assets have unit scale (Scale = 1.0, 1.0, 1.0) and zero rotation.
2. **UE5 dimensions:**
   - Infantry: ~180 cm in height, width 60–80 cm.
   - Tanks: ~500–700 cm long, 320–460 cm wide, 220–350 cm high.
   - Buildings: from 400x400 cm (2x2) to 1000x1000 cm (5x5).
3. **Collisions:** Settings for simple hitboxes and mesh collisions are saved in the manifest.

---

## 5. Conclusion

The generated assets completely replace the built-in `Cube` primitives and are ready for use in the `RA4SimWorldSubsystem` and test cards.
