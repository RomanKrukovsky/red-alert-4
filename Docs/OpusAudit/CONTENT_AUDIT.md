# Opus Audit — Content Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Asset Inventory

| Category | Count | Notes |
|----------|-------|-------|
| Total .uasset files | 4531 | Including ThirdParty |
| ThirdParty .uasset | 3523 | 77.7% of all content |
| Custom (non-ThirdParty) .uasset | 1008 | RA4 game content |
| .umap (maps) | 19 | 6 RA4 maps + 13 ThirdParty |
| .wav (audio) | 1412 | In Audio/ directory |
| .fbx (3D models) | 286 | In ArtSource/ |
| .blend (Blender) | 41 | In ArtSource/ |
| .png (textures) | 126 | In SCREENSHOTS/ |
| .jpg (images) | 31 | In SCREENSHOTS/ |

---

## ThirdParty Content (77.7% of all assets)

| Pack | Location | License Status |
|------|----------|----------------|
| Brushify | Content/ThirdParty/Brushify | Commercial — requires license |
| CityPark | Content/ThirdParty/CityPark | Unknown — no license file found |
| EpicGames/CityPark | Content/ThirdParty/EpicGames/CityPark | Epic Marketplace — check EULA |
| EpicGames/ElectricDreams | Content/ThirdParty/EpicGames/ElectricDreams | Epic Marketplace — check EULA |
| EpicGames/OpenWorldDemo | Content/ThirdParty/EpicGames/OpenWorldDemo | Epic Marketplace — check EULA |
| EpicGames/RuralAustralia | Content/ThirdParty/EpicGames/RuralAustralia | Epic Marketplace — check EULA |
| FactoryEnvironment | Content/ThirdParty/FactoryEnvironment | Unknown — no license file found |
| IndustryPropsPack6 | Content/ThirdParty/IndustryPropsPack6 | Unknown — no license file found |
| QuantumCharacter | Content/ThirdParty/QuantumCharacter | Unknown — no license file found |
| Quixel/MegascansTrees | Content/ThirdParty/Quixel | Free for Unreal use — requires Epic account |
| ambientCG | Content/ThirdParty/ambientCG | CC0 — no restrictions |

**CRITICAL**: No LICENSE, EULA, README, or provenance files found in any ThirdParty directory. `find Content/ThirdParty -name "LICENSE*" -o -name "EULA*" -o -name "README*" | wc -l` = 0.

---

## Custom RA4 Content

### Maps (6 RA4-specific)
1. `Content/Maps/RA4_Skirmish.umap`
2. `Content/Maps/RA4_Skirmish_Canyon.umap`
3. `Content/Maps/RA4_Skirmish_Hills.umap`
4. `Content/Maps/RA4_Skirmish_Production.umap`
5. `Content/Maps/RA4_Skirmish_VisualIntegration.umap`
6. `Content/Maps/RA4_ArtLab.umap`

### Blockout Art (per-faction placeholder meshes)
- `Content/RA4/Art/Blockout/Alliance/` — 37 .uasset files
- `Content/RA4/Art/Blockout/Chronolegion/` — present
- `Content/RA4/Art/Blockout/Soviet/` — present
- `Content/RA4/Art/Blockout/EasternCoalition/` — present

All are `_Blockout` suffixed — placeholder art, not final assets.

### Audio
- `Audio/` directory contains .wav source files
- `Content/RA4/Audio/` exists but generated SoundWave assets are gitignored
- 1412 .wav files total

### Content Database (JSON Bible)
- Default content defined in `Source/RA4Content/Private/DefaultContent.cpp`
- Content IDs follow pattern: `building.sov.*`, `unit.sov.*`, `resource.*`
- Faction test IDs: Soviet (conscript, heavy tank, harvester, con yard, power, refinery, war factory, turret, barracks), Alliance (rifleman, light tank, con yard)
- Full bible in `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` (78 units, 4 factions)

### UI Content
- `Content/RA4UI/` contains Art, Audio, Components, Fonts, Localization, Materials, Themes, Widgets
- Web prototype in `ra4-ui/src/` with 28+ React components

---

## Content Validation

The headless `ContentDatabase::Validate()` checks:
- Missing localization keys
- Negative or zero health
- Dangling weapon references
- Zero-speed units
- Invalid faction IDs

This is good but limited — it validates schema, not visual correctness.

---

## Issues

1. **77.7% of content is ThirdParty with no documented licenses** — release blocker
2. **All RA4 art is blockout/placeholder** — no production-quality faction art
3. **No localization strings found** — Content/Localization/Game/ directory exists but no .locres files
4. **Audio source files exist but integration into UE not verified**
5. **Content IDs reference EA-inspired faction names** (Soviet, Alliance) — IP migration needed
