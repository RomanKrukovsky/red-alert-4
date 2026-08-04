# Map Editor & Modding Architecture (`MODDING_AND_EDITOR_DESIGN.md`)

**Document Version**: 2.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  
**Editor Module**: `RA4Editor`  

---

## 1. Resonance Map Editor Overview

The *Resonance Map Editor* is an in-engine tool built on Unreal Engine 5 Slate/Editor tools, enabling players and modders to author custom multiplayer skirmish maps and scripted campaign missions.

### Editor Features
- **Terrain Sculpting**: Heightmap brush tools, cliff generation, water plane placement, and Aethelite ore field brush.
- **Texture Painting**: Multi-layer landscape painting (Sand, Dirt, Rock, Concrete, Snow).
- **Entity Placement**: Drag-and-drop placement of faction structures, civilian buildings, neutral bridges, and resource nodes.

---

## 2. Event-Trigger Scripting Engine

Custom campaign missions utilize a visual **Event-Condition-Action (ECA)** trigger graph:

```
  [ EVENT ]                    [ CONDITION ]                     [ ACTION ]
Player Enters Area  -->  Player Has Unit Count >= 5  -->  Spawn Ambush Wave & Play VO
```

### Supported Actions
- Spawn Unit Wave / Reinforcements.
- Trigger Cinematic Camera Move / Cutscene.
- Change Player Alliance / Faction State.
- Display Objectives / Tactical Messages.

---

## 3. Data & Asset Modding Pipeline

- **JSON Data Overrides**: Modders can override unit stats, armor multipliers, weapon damage, build costs, and tech tree prerequisites by placing custom JSON files in `%USERPROFILE%/Documents/IronResonance/Mods/`.
- **Custom Asset Imports**: Support for custom 3D FBX models, PBR material textures, and WAV/OGG audio files.
- **Steam Workshop Integration**: One-click publish and subscribe for maps and mod packages.
