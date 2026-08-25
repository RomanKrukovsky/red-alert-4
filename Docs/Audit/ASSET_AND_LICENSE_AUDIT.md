# RA4 — Asset, IP & License Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`

This document supersedes the previous version, which missed the single largest legal
exposure in the repository and cited fonts (`Oswald`, `Inter`, `Druk Cyr`) and directories
(`GeneratedVO/` audio, Sketchfab models) that do not exist here.

---

## 1. BLOCKING: GPLv3 code from Electronic Arts is committed to this repository

This is the most serious finding of the audit.

### Evidence

```
$ git -C Tools/ContentImport/RA3_Mod_Files remote -v
origin  https://github.com/electronicarts/CnC_Modding_Support.git (fetch)

$ head -3 Tools/ContentImport/RA3_Mod_Files/LICENSE.md
Electronic Arts Inc. has released the contents of this repository
(https://github.com/ElectronicArts/CnC_Modding_Support) under the
GPL V3 license below, with additional terms at the bottom.
```

Two distinct exposures:

**1a. A nested clone of EA's repository sits inside the working tree.**
`Tools/ContentImport/RA3_Mod_Files/` is a blobless git clone of EA's
`CnC_Modding_Support` (26 MB, mostly `.git`). It is **not** covered by `.gitignore`
(`git check-ignore` reports no match), so any `git add -A` would commit it as a gitlink or,
worse, as content.

**1b. 123 files of EA-authored GPLv3 material are already committed.**

```
$ git ls-files Tools/ContentImport/RA3_XML_Source | wc -l
123
```

These are externally authored XML schemas (`AssetTypePlayerTemplate.xsd`,
`AssetTypeObjectCreationList.xsd`, …), map libraries (`Lib_Camp_Restrictions/map.xml`,
`Lib_End_Mission/map.xml`) and `RA3Music.h`. They were retrieved by
`Tools/ContentImport/fetch_ra3_xmls.py`, which hardcodes
`https://raw.githubusercontent.com/electronicarts/CnC_Modding_Support/main/Red%20Alert%203`.

### Why this is blocking

GPLv3 is a strong copyleft licence. RA4 is intended as a proprietary commercial product.
Distributing a binary derived from, or a repository incorporating, GPLv3-licensed material
obliges the distributor to license the combined work under GPLv3 and to publish complete
corresponding source. That is incompatible with the stated commercial goal. EA also adds
terms on top of the GPL text, which must be read in full before any reuse decision.

### Mitigating fact — the derived data is clean of EA content

The generated tables use original RA4 identifiers, not EA's:

```json
{ "Name": "SU_RubezhRifleman", "Faction": "СССР", "MaxHealth": 100,
  "ArmorClass": "LightInfantry", "Cost": 100, "BuildTimeTicks": 40 }
```

No prior-title unit names, stats or balance values were carried across. Every unit in
`DT_Units.json` is a uniform 100 HP / 100 cost / 40 ticks — placeholder scaffolding, not
imported balance. The legal problem is the **presence of the GPLv3 source material in the
repository and in history**, not contamination of the shipped data.

### Required remediation (do not perform during this audit stage)

1. Obtain a legal opinion on whether the schemas were ever used to shape RA4's data model —
   ADR-011 and `Docs/content/SOURCE_TRACEABILITY.md` should be read as part of that.
2. Delete `Tools/ContentImport/RA3_Mod_Files/` and `Tools/ContentImport/RA3_XML_Source/`.
3. Purge them from git history (`git filter-repo`), because deletion alone leaves the blobs
   distributable in the repository.
4. Retire or gate `fetch_ra3_xmls.py` / `fetch_all_ea_xmls.py` so the material cannot return.
5. Record the decision in an ADR.

---

## 2. Third-party content: 14.2 GB on disk, 2 entries in the registry

`Content/AssetRegistry/ThirdPartyAssets.json` is a genuinely good idea — structured
provenance with licence, commercial-use flag and reviewer. It is also almost entirely
unpopulated relative to what is actually present.

| On disk | Size | In registry? | Licence file present? |
| --- | --- | --- | --- |
| `Content/ThirdParty/FactoryEnvironment` | 8.7 GB | **No** | **No** |
| `Content/ThirdParty/CityPark` | 4.1 GB | **No** | **No** |
| `Content/ThirdParty/QuantumCharacter` | 864 MB | **No** | **No** |
| `Content/ThirdParty/IndustryPropsPack6` | 368 MB | **No** | **No** |
| `Content/ThirdParty/ambientCG` | 154 MB | Indirectly (CC0 texture entry) | **No** |
| `Content/ThirdParty/Brushify` | 0 B (empty) | No | n/a |
| `Content/ThirdParty/EpicGames` | 0 B (empty) | No | n/a |
| `Content/ThirdParty/Quixel` | 0 B (empty) | No | n/a |

Registry contains exactly two entries: `TPA-KENNEY-RTS-KIT-01` and
`TPA-POLYHAVEN-TEXTURES-01`.

The four large packs are recognisable Epic Games Marketplace / Fab content
(FactoryEnvironment, CityPark, IndustryPropsPack6, QuantumCharacter). Marketplace content is
typically licensed to a **named account** under the Epic Content Licence — usually usable in
shipped products, but not redistributable as source assets, and the entitlement must be
evidenced. None of that evidence exists in the repository.

Empty `Brushify`, `EpicGames` and `Quixel` directories imply intended-but-absent
dependencies; they should either be filled with evidence or removed.

### Fabricated checksums in the registry

```
TPA-KENNEY-RTS-KIT-01  checksumSHA256:
  e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
```

That value is the SHA-256 of the **empty string**. It is not a hash of any asset. The same
constant reappears in `Docs/Milestones/GOLD_MASTER_MANIFEST.md` as the checksum of the
"Gold Master Source Archive". Both are placeholders presented as verification.

`localImportPath` for the Kenney building kit is given as `Source/RA4Content/` — a C++
module directory that contains no art.

---

## 3. Trademark and naming exposure

`Config/DefaultGame.ini` carries an explicit disclaimer, which is the right instinct:

```
Description=Internal working title. No Electronic Arts licence; no Command & Conquer content.
```

Current state of protected/α-risky terms:

| Term | Files | Assessment |
| --- | --- | --- |
| `Red Alert 4` / `RedAlert4` | project name, `.uproject`, module prefix | **Working title only.** "Red Alert" is an EA mark; must not ship |
| `EVA` | 35 | EA uses EVA in C&C. Generic-sounding but associated; rename before release |
| `Soviet` | 34 | Historical term, not an EA mark. Low risk |
| `Allied` / `Alliance` | 5 | Generic. Low risk |
| `Tiberium` | **0** | Clean — previous audit's claim of Tiberium references is wrong |

The previous report asserted `Tiberium` appeared "in comments and legacy tests"; a repo-wide
search finds zero occurrences. That claim was fabricated.

Note also that `Docs/Milestones/GOLD_MASTER_MANIFEST.md` renames the product to *"Iron
Resonance: Command of Tomorrow"* with factions "Red Star Union / Global Defense Coalition /
Pan-Asian Syndicate / Temporal Resonance Order", which contradicts both CLAUDE.md's four
factions (СССР, Альянс, Восточная коалиция, Хронолегион) and the actual content data. The
rename exists only in that document.

---

## 4. Fonts

Actual fonts in the repository — exactly two files:

```
Content/RA4UI/Fonts/RA4_RobotoCondensedRegular.ttf
Content/RA4UI/Fonts/RA4_RobotoCondensedSemiBold.ttf
```

Roboto Condensed is licensed by Google under the **Apache License 2.0** (older releases) or
**SIL OFL 1.1** (current releases). Both permit embedding and commercial use; Apache-2.0
requires the licence text to be reproduced. **No licence file accompanies either font.**

The previous audit's claims about `Oswald`, `Inter` and a "HIGH LEGAL RISK" commercial
`Druk Cyr` font in `Assets/Noesis/Themes/Typography.xaml` are unsupported: searches for
`Oswald` and `Druk` return zero matches anywhere in the repository.

**Action:** add `Content/RA4UI/Fonts/LICENSE.txt` with the correct upstream licence text
and record both fonts in the third-party registry.

---

## 5. Audio

### Music — 2 tracks, provenance undocumented

```
Content/RA4/Audio/Music/Iron_Parade.{wav,mp3}
Content/RA4/Audio/Music/Steel_Horizon_Pact.{wav,mp3}
```

`music_catalog.json` records title, duration, sample rate, LUFS target and a description —
but **no author, no licence, no source, no generation method**. For a commercial release,
music with unrecorded provenance is unusable. If these are AI-generated, the generating
service's terms decide whether commercial rights transfer, and that must be captured.

### Voice — 1088 files, real audio, undocumented model licence

1088 WAVs under `Content/RA4/Audio/`. Sampled files are genuine 24-bit / 48 kHz speech, not
placeholder tones — verified by decoding: 25 079 distinct sample values across 30 000
samples, zero-crossing rate 0.06, peak 6 642 108 (speech-typical; a synthetic tone would show
few distinct values and a near-constant ZCR).

They are produced by `Tools/VoiceGeneration/generate_soviet_voxcpm.py` and
`Tools/Audio/generate_eva_voxcpm.py` using "VoxCPM". **No licence, model version, or terms
for VoxCPM are recorded anywhere.** Whether synthetic speech may be shipped commercially,
and whether any voice was cloned from a reference speaker (the script mentions "voice
cloning reference conditioning"), are unresolved and materially risky — cloning a real
person's voice without consent creates personality-rights exposure independent of copyright.

`GeneratedVO/` exists as an empty directory tree; the previous audit described it as
containing the TTS output.

### Sound effects

No SFX library is present. The previous audit's Freesound.org / CC-BY attribution claims
describe assets that do not exist in this repository.

---

## 6. Third-party code and plugins

| Dependency | Licence | Status |
| --- | --- | --- |
| Unreal Engine 5.8 | Epic UE EULA | Standard; 5 % royalty terms apply |
| `GameplayAbilities`, `CommonUI`, `ModelViewViewModel`, `EnhancedInput`, `FunctionalTestingEditor`, `PythonScriptPlugin`, `EditorScriptingUtilities` | Epic, bundled | Fine — all resolve in UE 5.8 |
| `ModelContextProtocol`, `AllToolsets`, `ToolsetRegistry` | resolve inside `UE_5.8/Engine/Plugins` | **Verify origin.** These are not stock UE plugin names; they appear to be locally installed engine-side plugins. A plugin present in one developer's engine but not in a clean 5.8 install will break every other machine and CI |
| `ra4-ui` npm tree (React/Vite) | MIT and similar | Fine, but the whole app is dead weight — see UI_AUDIT.md |
| NoesisGUI | commercial | **Not actually used.** Not in `.uproject`; only 6 orphan `.xaml` files remain |

`ModelContextProtocol` / `AllToolsets` / `ToolsetRegistry` deserve attention: they resolved
here because they exist in this machine's engine installation. If they are not part of a
stock Epic 5.8 distribution, the project is silently dependent on a modified engine, which
is both a reproducibility and a licensing question.

---

## 7. Secrets

`.env` at the repository root contains live credentials (`OPENAI_API_KEY`,
`OPENROUTER_API_KEY`, `OPENCODE_API_KEY`). **It is correctly gitignored and has never been
committed** — verified with `git log --all -- .env` (empty) and `git ls-files` (no match).
No action required beyond continued vigilance.

---

## 8. Summary table

| # | Finding | Severity |
| --- | --- | --- |
| 1 | GPLv3 EA material committed (123 files) + nested EA clone untracked-but-unignored | **BLOCKING** |
| 2 | 14.2 GB of unregistered, unlicensed third-party content | **CRITICAL** |
| 3 | Fabricated SHA-256 checksums (empty-string hash) in registry and gold-master manifest | **CRITICAL** |
| 4 | Music with no author/licence recorded | **CRITICAL** |
| 5 | VoxCPM TTS licence and voice-cloning consent unrecorded | **CRITICAL** |
| 6 | Compliance scanner referenced by ADR-011 and CI does not exist | **CRITICAL** |
| 7 | Fonts lack accompanying licence text | **IMPORTANT** |
| 8 | `ModelContextProtocol`/`AllToolsets`/`ToolsetRegistry` provenance unverified | **IMPORTANT** |
| 9 | "Red Alert 4" / `EVA` naming must change before release | **IMPORTANT** |
| 10 | `.env` secrets correctly excluded | **OK** |
