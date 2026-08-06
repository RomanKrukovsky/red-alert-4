# Legal Audit & License Management (`LEGAL_AND_LICENSES.md`)

**Document Version**: 2.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Intellectual Property Provenance Audit

> [!IMPORTANT]
> **Official Legal Disclaimer**: This document documents internal asset licensing and legal risk mitigation strategy. Formal legal clearance, trademark filing, and copyright sign-off must be conducted by specialized intellectual property legal counsel prior to public release or commercial distribution.

---

## 2. Third-Party Asset & License Inventory

| Asset Component | Source / Origin | License Type | Commercial Use Status | Attribution Requirements |
| :--- | :--- | :--- | :--- | :--- |
| **PolyHaven Textures/Models** | PolyHaven.com | CC0 1.0 (Public Domain) | **Permitted** | None required. |
| **ambientCG Materials** | ambientCG.com | CC0 1.0 (Public Domain) | **Permitted** | None required. |
| **Kenney 3D Assets** | Kenney.nl | CC0 1.0 (Public Domain) | **Permitted** | None required. |
| **Oswald Font** | Google Fonts | SIL Open Font License 1.1 | **Permitted** | Include OFL license text in `Credits.md`. |
| **Inter Font** | Google Fonts | SIL Open Font License 1.1 | **Permitted** | Include OFL license text in `Credits.md`. |
| **React / Vite / Oxlint** | `ra4-ui/` | MIT License | **Permitted** | Retain copyright header in source. |
| **Freesound SFX (CC-BY)** | Freesound.org | CC-BY 3.0 / 4.0 | **Permitted** | Log author name & clip title in `Audio/ATTRIBUTION.md`. |
| **Sketchfab Models (NC)** | Sketchfab | CC-BY-NC 4.0 | **PROHIBITED FOR COMMERCIAL** | Must be replaced prior to commercial build. |
| **Druk Cyr Font** | Proprietary | Commercial License | **HIGH RISK** | Must acquire commercial desktop/embedding license or replace. |
| **CityPark (Unreal marketplace pack)** | Unknown — undocumented | **UNKNOWN** | **BLOCKER — unverified** | 4.1 GB / 1,117 files present on disk. Provenance never recorded. `RA4_Skirmish_Production` references its grass, trees and water-shader materials, so the flagship map depends on it. |
| **FactoryEnvironment (Unreal marketplace pack)** | Unknown — undocumented | **UNKNOWN** | **BLOCKER — unverified** | 7.0 GB / 2,119 files present on disk. Source of all 208 errors in the V-4 verification run (see NEXT_ACTIONS V-6). |
| **QuantumCharacter** | Unknown — undocumented | **UNKNOWN** | **BLOCKER — unverified** | 839 MB / 110 files; git-tracked. Supplies the infantry skeleton and animations. |
| **IndustryPropsPack6** | Unknown — undocumented | **UNKNOWN** | **BLOCKER — unverified** | 325 MB / 188 files; git-tracked. |
| **Brushify / Quixel / EpicGames (stubs)** | Unknown — undocumented | **UNKNOWN** | Needs triage | Directory stubs only (1-4 files, 0 B) — determine whether they are placeholders or partial imports. |

---

### 2.1 Unresolved provenance — audit 2026-08-06

An audit of `Content/ThirdParty/` found **eight packs on disk and only one of them
(`ambientCG`) named in the inventory above**. Method: `ls Content/ThirdParty/` cross-referenced
against a grep of this document; sizes from `du -sh`; map dependencies from `strings` on the
`.umap` files.

**Nothing here says the assets are unlicensed — it says nobody wrote down what they are.** That is
the same commercial blocker either way: an unverified license cannot be cleared by counsel, and
`ART_BIBLE`/`RELEASE_CRITERIA` cannot certify a build whose content provenance is unknown. Two
of the packs total 11 GB and are **not in version control** (see the `.gitignore` finding in
`RISK_REGISTER.md` RISK-20), so their licence terms cannot even be recovered from history.

Required per pack, before any commercial build: original marketplace/store URL, purchase or
acquisition record, licence text archived under `Docs/Legal/`, and a verdict in the table above.
`RA4_Skirmish_Production` must be re-checked afterwards: it currently pulls CityPark flora and a
water shader, so if that pack turns out to be non-redistributable the flagship map needs its art
replaced, not just a licence note.

## 3. License Attribution Logging

All CC-BY assets and open-source software licenses used in *Iron Resonance* are documented in the game credits and packaged in `Docs/Legal/ThirdPartyLicenses.txt`.
