# Legal Audit & License Management (`LEGAL_AND_LICENSES.md`)

**Document Version**: 2.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Intellectual Property Provenance Audit

> [!IMPORTANT]
> **Official Legal Disclaimer**: This document documents internal asset licensing and legal risk mitigation strategy. Formal legal clearance, trademark filing, and copyright sign-off must be conducted by specialized intellectual property legal counsel prior to public release or commercial distribution.

---

## 1b. Project naming (owner ruling, 2026-08-06)

An earlier audit flagged the three names in this repository as an inconsistency.
The project owner has ruled that they are one project, deliberately layered:

| Name | Role |
| :--- | :--- |
| *Iron Resonance: Command of Tomorrow* | The commercial title. Correct in published docs. |
| RA4 | Internal working shorthand, used in code, modules (`RA4Recon`) and paths. |
| `red-alert-4` | Repository directory only. |

So "Iron Resonance" in these documents is **not** a stale name and must not be
"corrected" to RA4. What still matters legally is the opposite direction: the
working shorthand and the repository name reference another company's franchise,
so neither may appear in shipped strings, store metadata, or player-facing UI. The
commercial title is the one that ships.

---

## 2. Third-Party Asset & License Inventory

| Asset Component | Source / Origin | License Type | Commercial Use Status | Attribution Requirements |
| :--- | :--- | :--- | :--- | :--- |
| **PolyHaven Textures/Models** | PolyHaven.com | CC0 1.0 (Public Domain) | **Permitted** | None required. |
| **ambientCG Materials** | ambientCG.com | CC0 1.0 (Public Domain) | **Permitted** | None required. |
| **Kenney 3D Assets** | Kenney.nl | CC0 1.0 (Public Domain) | **Permitted** | None required. |
| **Oswald Font** | Google Fonts | SIL Open Font License 1.1 | **Permitted** | Include OFL license text in `Credits.md`. |
| **Inter Font** | Google Fonts | SIL Open Font License 1.1 | **Permitted** | Include OFL license text in `Credits.md`. |
| **Freesound SFX (CC-BY)** | Freesound.org | CC-BY 3.0 / 4.0 | **Permitted** | Log author name & clip title in `Audio/ATTRIBUTION.md`. |
| **QuantumCharacter** | **UNRECORDED — owner input required** | **UNKNOWN** | **BLOCKS RELEASE** | 839 MB, tracked in git, used by `RA4EntityActor.cpp` and `create_art_mappings_and_lab.py` for infantry meshes/animations. No license file in the pack directory. |
| **IndustryPropsPack6** | **UNRECORDED — owner input required** | **UNKNOWN** | **BLOCKS RELEASE** | 325 MB, tracked in git, referenced by `RA4SkirmishGameMode.cpp` and `inventory_map_assets.py`. No license file in the pack directory. |
| **CityPark** | **UNRECORDED — owner input required** | **UNKNOWN** | **BLOCKS RELEASE** | Untracked in git but referenced 15 times by `Tools/Editor/make_archipelago_map.py` (trees, rocks, roads, bridges). A fresh clone therefore builds the map silently missing them — see `THIRD_PARTY_PACK_AUDIT.md`. |
| **FactoryEnvironment** | **UNRECORDED** | **UNKNOWN** | **QUARANTINED** | 7 GB, never tracked in git, referenced by nothing (verified by grep across `Source/`, `Tools/`, `Config/`, `Content/RA4/` and byte search of all `.umap`). Moved out of `Content/` under V-6; it caused 208 of 212 resave errors. |

### Removed entries (were listed, are not on disk)

Two rows were deleted on 2026-08-06 because they described assets this project does
not contain. Both were verified absent by `find`, not by reading:

| Former entry | Why removed |
| :--- | :--- |
| Sketchfab Models (CC-BY-NC) | No file under any path matching `*sketchfab*`. The entry described a commercial blocker that does not exist, which is as misleading as omitting a real one. |
| Druk Cyr Font (proprietary) | No file matching `*druk*` anywhere outside `.git`. The UI now uses Oswald and Inter, both OFL. |

`Content/ThirdParty/Brushify`, `.../Quixel` and `.../EpicGames` contain only
`.gitkeep` placeholders (0 bytes of content) and are therefore not license
liabilities today — but the empty directories imply an intent to add packs, so any
future import must add a row above BEFORE the assets land.

---

## 3. License Attribution Logging

All CC-BY assets and open-source software licenses used in RA4 are to be documented
in the game credits and packaged in `Docs/Legal/ThirdPartyLicenses.txt`.

> [!WARNING]
> **Neither file exists yet** (verified 2026-08-06 by `ls`). This section described
> a process, in the present tense, that has never run: `Docs/Legal/ThirdPartyLicenses.txt`
> and the `ATTRIBUTION.md` the CC-BY rows above require are both absent. Every
> CC-BY and OFL row in the table is therefore **out of compliance right now**, not
> merely pending. Attribution files must be authored before any build ships.
