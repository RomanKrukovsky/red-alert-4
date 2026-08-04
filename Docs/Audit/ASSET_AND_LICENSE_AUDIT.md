# Intellectual Property & License Audit (`ASSET_AND_LICENSE_AUDIT.md`)

**Audit Date**: August 4, 2026  
**Scope**: Legal compliance, trademark risks, third-party asset licensing, fonts, audio, 3D models, and code dependencies.

---

## 1. Trademark & Intellectual Property (IP) Risk Audit

> [!WARNING]
> The project contains high-risk references to registered trademarks owned by Electronic Arts Inc. (EA) / Westwood Studios (`Command & Conquer`, `Red Alert`, `Tiberium`, `EVA`, `Soviet`, `Allied`). Commercial release or public distribution requires full removal or renaming of protected terms.

### IP Reference Inventory

| Category | Term / Identifier | Location in Code / Data | Legal Risk Level | Required Remediation |
| :--- | :--- | :--- | :--- | :--- |
| **Title / Project** | `RedAlert4` / `Red Alert 4` | Project root, `.uproject`, C++ Module names | **CRITICAL** | Rename project target for commercial release. |
| **Faction Names** | `Soviet` | Data files & JSON definitions | **HIGH** | Rename to `Red Star Union` or `Eurasian Vanguard`. |
| **Faction Names** | `Alliance` / `Allied` | Data files & JSON definitions | **MEDIUM** | Neutralize terminology to `Global Alliance`. |
| **Lore / Audio** | `EVA` (Electronic Voice Assistant) | Voice manifest & sound triggers | **HIGH** | Rename audio system to `AURA` or `Tactical Advisor`. |
| **Lore / Resource** | `Tiberium` | Comments and legacy tests | **HIGH** | Replace with `Ore` / `Mineral Crystal`. |

---

## 2. 3D Models & Texture Licensing Audit

### Asset Provenance Audit
- **PolyHaven Assets**: CC0 (Public Domain Dedicated). Safe for commercial use.
- **ambientCG Assets**: CC0 (Public Domain Dedicated). Safe for commercial use.
- **Kenney.nl Assets**: CC0 (Public Domain Dedicated). Safe for commercial use.
- **Sketchfab Models**:
  - `CC-BY 4.0`: Requires attribution in credits file (`Credits.md`).
  - `CC-BY-NC 4.0`: **NON-COMMERCIAL ONLY**. Must be replaced prior to commercial launch.
  - `Editorial Use Only`: Must be replaced before any public release.

---

## 3. Audio & Voice Provenance Audit

- **TTS / AI Generated VO**: Audio files in `GeneratedVO/` were produced via synthetic text-to-speech tools. Clean copyright status if generated using commercial-tier AI speech licenses.
- **SFX Libraries**: Freesound.org sound effects licensed under CC0 or CC-BY. All CC-BY audio clips require attribution logging in `Audio/ATTRIBUTION.md`.

---

## 4. Typography & Font License Audit

- **`Oswald`**: SIL Open Font License 1.1 (OFL). Safe for embedded commercial use.
- **`Inter`**: SIL Open Font License 1.1 (OFL). Safe for embedded commercial use.
- **`Druk Cyr`**: Commercial proprietary font. Listed in `Assets/Noesis/Themes/Typography.xaml`.
  - **Status**: **HIGH LEGAL RISK**. Must verify desktop/embedding license or replace with an open-source alternative (e.g. `Bebas Neue` or `Anton`) before release.

---

## 5. Third-Party Code & Software Dependencies

- **Unreal Engine 5**: Covered by Epic Games Unreal Engine EULA.
- **React / Vite / TailwindCSS (`ra4-ui`)**: MIT License. Fully compliant.
- **NoesisGUI**: Commercial C++ XAML UI middleware. Requires active NoesisGUI license key for commercial deployment.
