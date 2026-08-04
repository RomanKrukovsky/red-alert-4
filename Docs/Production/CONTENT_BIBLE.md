# Production Content & Asset Bible (`CONTENT_BIBLE.md`)

**Document Version**: 2.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Content Asset Architecture & Data Schemas

All gameplay content assets are authored as data definitions ingested by `RA4Content` (`BibleContentLoader`):

```
  [ ra4_content.normalized.json ]
                 |
                 v
       [ BibleContentLoader ]
                 |
                 v
        [ ContentDatabase ]
       /         |         \
      v          v          v
  EntityDef   DamageMatrix  VoiceSetDef
```

---

## 2. Voice & EVA Script Specification

### Audio Event Matrix (624 Events across 4 Factions)
Each unit type has an assigned `VoiceSetDef` with 8 standard tactical audio events:
1. `Select`: Unit selected response ("Awaiting orders", "Standing by").
2. `Move`: Movement order response ("Moving out", "Coordinates set").
3. `Attack`: Target attack order ("Engaging target", "Fire at will").
4. `UnderFire`: Under attack notification ("Taking fire!", "Under attack!").
5. `Retreat`: Retreat order ("Falling back!", "Tactical retreat").
6. `Promoted`: Veterancy rank up ("Promoted to Veteran!", "Heroic status!").
7. `Destroyed`: Unit destruction radio static.
8. `SpecialAbility`: Secondary ability activation ("Overdrive active", "Cloak engaged").

---

## 3. 3D Model & PBR Material Pipeline

### Technical Asset Standards
- **Polygon Count Budget**:
  - Infantry: 3,000 - 5,000 tris.
  - Medium Vehicles: 12,000 - 18,000 tris.
  - Heavy Superunits: 35,000 - 50,000 tris.
  - Base Structures: 20,000 - 40,000 tris.
- **PBR Texture Maps**:
  - `_BC`: Base Color with Team Color Mask in Alpha channel.
  - `_N`: Tangent-space Normal Map.
  - `_RMA`: Packing Roughness (R), Metallic (G), Ambient Occlusion (B).
  - `_E`: Emissive Map for faction glow accents.

---

## 4. Visual Effects (Niagara VFX) Specification

- **Muzzle Flashes**: Faction-specific colors (RSU = Red/Orange, GDC = Blue/White, PAS = Green, TRO = Purple).
- **Beam & Railgun Effects**: Ribbon particles for railgun shots and Tesla arcs.
- **Explosions & Destruction**: Physics-driven debris particles, dust clouds, and decal impact craters.
