# RA4 — Content Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`

Supersedes the previous version, which listed three maps that do not exist
(`M_Skirmish_Desert`, `M_ProvingGround`, `M_Campaign_Ch01_M01`), a `Content/RA4/VFX/`
directory that does not exist, an `Audio/SFX/` directory that does not exist, and
`GeneratedVO/{Soviet,Alliance,Coalition,Chrono}` voice sets that are empty.

## 1. Gameplay data — real, structured, and mostly placeholder values

Source of truth is `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` (148 KB),
normalized by `Tools/ContentImport/parse_bible.py` into
`Content/RA4/Data/Generated/ra4_content.normalized.json`.

Measured contents:

| Key | Count |
| --- | --- |
| `factions` | 4 |
| `units` | **78** |
| `buildings` | **64** |
| `voiceEvents` | 624 |
| `evaLines` | 32 |
| `factionResources` | 4 |
| `economy` / `combat` | dicts (7 / 4 keys) |
| `issues` | **0** |

The schema carries `schemaVersion`, `sourceFile`, `sourceHash` and `generatedAt` — good
provenance discipline, and `issues: 0` means the importer validated cleanly.

**Correction to prior claims:** buildings are **64**, not 35. The "35 structures" figure in
`CONTENT_AUDIT`, `CURRENT_STATE` and `INDEPENDENT_RELEASE_REVIEW` matches nothing in the data.

Loading is well covered: 15 `BibleImport.*`, 4 `BibleContent.*`, 5 `Content.*` tests, plus
`ArmorMatrix.*` and 4 `Veterancy.*`.

### 1.1 The derived DataTables are stubs

`Tools/ContentImport/DataTables/DT_Units.json` gives every unit identical values:

```json
{ "Name": "SU_RubezhRifleman", "MaxHealth": 100, "ArmorClass": "LightInfantry",
  "WeaponId": "", "Cost": 100, "BuildTimeTicks": 40 }
```

100 HP / 100 cost / 40 ticks across the board, with `WeaponId` empty. This is scaffolding,
not balance. Any claim of balance validation (`Docs/content/ECONOMY_VALIDATION_REPORT.md`,
`Docs/Milestones/BALANCE_TELEMETRY`) rests on data that does not differentiate units.

Naming is original throughout (`SU_RubezhRifleman`, `SU_ZapalGrenadier`, `SU_ZaslonAATeam`),
with faction `СССР` — no EA identifiers leaked into the derived data. See
ASSET_AND_LICENSE_AUDIT §1 for the licensing problem with the *source* material.

## 2. Campaign content contradicts the data-driven ADR

```
$ grep -c "Missions.push_back" Source/RA4Campaign/Private/CampaignDatabase.cpp
50
$ find . -iname "*mission*.json"      →  (nothing)
```

~50 missions are constructed in C++ with `MissionId = std::string(Chapter) + "_mission_" + N`.
There are **no authored mission data files**. ADR-0004 ("content lives in data, not code")
is violated; editing a mission requires recompiling.

The claim of "38 authored campaign missions" is wrong in both count and nature.

That said, the runtime is real: `MissionRuntime.cpp` is covered by **21 `MissionRuntime.*`
tests** and 2 `Campaign.*` tests, so objective evaluation works — it is the *authoring* layer
that is missing.

## 3. Maps

8 project maps exist:

```
Content/Maps/RA4_Skirmish.umap
Content/Maps/RA4_Skirmish_Production.umap    ← GameDefaultMap
Content/Maps/RA4_Skirmish_Hills.umap
Content/Maps/RA4_Skirmish_Canyon.umap
Content/Maps/RA4_Skirmish_VisualIntegration.umap
Content/Maps/RA4_ArtLab.umap
Content/Maps/Art/RA4_ArtShowcase_Day.umap
Content/Maps/Art/RA4_ArtShowcase_Night.umap
```

Plus **11 third-party vendor demo maps** under `Content/ThirdParty/` (`Overview`,
`Showcase`, `Showcase_NotOptimized`, `Demonstration`, `TechArt`, …). These must be excluded
from packaging.

No campaign maps exist. Of the 8, three are art/lab scenes rather than playable levels, so
the playable map count is effectively **5 skirmish maps**, none verified by running.

## 4. Art

| Category | Count | Notes |
| --- | --- | --- |
| Blockout FBX (`Content/RA4/Art/Blockout/`) | **142** | prior claim of 142 is accurate |
| FBX in `ArtSource/` | 144 | working source |
| Total FBX in repo | 286 | |
| PBR vertical-slice models | 36 claimed (`afbb447`) | present as `.uasset`, count not independently verified |

Materials exist under `Content/RA4/Art/Materials/` and `Content/RA4/Presentation/Materials/`
(ambientCG-derived: Asphalt024B, Concrete003, Ground039, Metal022, PaintedMetal006).

`URA4ArtMapping` binds entity → mesh and has 1 `ArtMapping.*` test.

## 5. Audio

| Item | Reality |
| --- | --- |
| VO WAVs | **1088** files under `Content/RA4/Audio/`, verified genuine 24-bit/48 kHz speech |
| Music | **2** tracks (`Iron_Parade`, `Steel_Horizon_Pact`), wav + mp3 |
| EVA lines | 32 in normalized data; 624 voice events |
| `GeneratedVO/` | **empty directory tree** — `Anchors/Alliance/` and `Units/` contain no files |
| SFX | **none** — no `Audio/SFX/`; `Audio/` contains only `Music/` and `Voice/` |

The absence of any sound-effects library is a significant content gap: weapon fire,
explosions, build/placement, harvester loops and UI clicks all have no assets.

Audio JSON manifests are thorough (`voice_manifest.json`, `eva_manifest.json`,
`voice_bible.json`, `eva_runtime_policy.json`, `eva_pronunciation_ru.json`,
`music_catalog.json`), which is good — but `music_catalog.json` records no author or licence
(ASSET_AND_LICENSE_AUDIT §5).

## 6. VFX — absent

`Content/RA4/VFX/` does not exist. Repo-wide, `Niagara` appears in 3 source files and
`ParticleSystem` in none. The previously claimed muzzle flashes, projectile trails, energy
beams, Tesla arcs and nuclear mushroom clouds are not present in any form.

## 7. Localization

261 keys, `en` + `ru`, 260/261 translated, with `.po` + `.archive` + compiled `.locres` and
`Game.manifest`/`Game.locmeta`. `Culture=ru` default. **Genuinely production-quality.**

## 8. Content status summary

| Area | Status |
| --- | --- |
| Unit/building/faction data | **REAL** structure, **PLACEHOLDER** values |
| Content loading & validation | **VERIFIED** (24 tests) |
| Campaign missions | **HARDCODED** — violates ADR-0004 |
| Campaign maps | **ABSENT** |
| Skirmish maps | 5 exist, **UNVERIFIED** |
| Blockout art | **REAL** (142 FBX) |
| PBR art | present, count unverified |
| Voice | **REAL** (1088 files) |
| Music | 2 tracks, **provenance undocumented** |
| SFX | **ABSENT** |
| VFX | **ABSENT** |
| Localization | **REAL** and near-complete |
| Third-party demo maps | **11, must be excluded from packaging** |
