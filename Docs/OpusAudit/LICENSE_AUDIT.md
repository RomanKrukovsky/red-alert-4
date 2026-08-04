# Opus Audit — License Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## ThirdParty Content Licenses

### Packs Found in Content/ThirdParty/

| Pack | Asset Count | License Status | Risk |
|------|-------------|----------------|------|
| ambientCG | 5 texture sets | CC0 (public domain) | NONE |
| Quixel/MegascansTrees | Unknown | Free for Unreal use | LOW — requires Epic account |
| Brushify | Unknown | Commercial — Pro license required | HIGH — no license file found |
| CityPark | Unknown | Epic Marketplace | MEDIUM — check marketplace EULA |
| EpicGames/CityPark | Unknown | Epic Marketplace sample | LOW — free for learning |
| EpicGames/ElectricDreams | Unknown | Epic Marketplace sample | LOW — free for learning |
| EpicGames/OpenWorldDemo | Unknown | Epic Marketplace sample | LOW — free for learning |
| EpicGames/RuralAustralia | Unknown | Epic Marketplace sample | LOW — free for learning |
| FactoryEnvironment | Unknown | Epic Marketplace | MEDIUM — check marketplace EULA |
| IndustryPropsPack6 | Unknown | Epic Marketplace | MEDIUM — check marketplace EULA |
| QuantumCharacter | Unknown | Epic Marketplace | MEDIUM — check marketplace EULA |

### Evidence
```
find Content/ThirdParty -name "LICENSE*" -o -name "EULA*" -o -name "README*" | wc -l
→ 0
```

No license documentation exists for any ThirdParty content.

---

## Project Naming / Trademark Issues

### "Red Alert 4" — EA Trademark
- Repository name: `red-alert-4`
- Uproject: `RedAlert4.uproject`
- Main module: `Source/RedAlert4/`
- Multiple references throughout codebase
- This is a registered trademark of Electronic Arts Inc.

### "Command & Conquer" References
- `Docs/Implementation/RA3_SAGE_IMPLEMENTATION_GAP.md` — references RA3/SAGE engine
- `Source/RA4Tests/Private/Test_RA3PipelineAndCommandBus.cpp` — file named after RA3
- Some content IDs echo C&C lore (Soviet faction, Allied faction)

### Original IP Migration Status
- `Docs/Production/ORIGINAL_IP_MIGRATION.md` exists — acknowledges the issue
- Migration has NOT been executed
- `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md` documents renamed factions but implementation is partial

---

## Code Licenses

| File/Module | License |
|-------------|---------|
| RA4Core | Project license (unspecified) |
| RA4Content | Project license (unspecified) |
| RA4Simulation | Project license (unspecified) |
| All RA4 modules | Copyright headers: "Copyright (c) Red Alert 4 project" |
| ra4-ui | Uses React, Zustand, Lucide — all MIT |

**Issue**: The project itself has no LICENSE file. No open-source or proprietary license is declared.

---

## Open-Source Dependencies (ra4-ui)

| Package | License |
|---------|---------|
| react | MIT |
| react-dom | MIT |
| react-router-dom | MIT |
| zustand | MIT |
| zod | MIT |
| lucide-react | ISC |
| classnames | MIT |
| vite | MIT |
| typescript | Apache-2.0 |

All are permissive. No copyleft contamination risk for the web prototype.

---

## UE Plugins Enabled

| Plugin | License Implications |
|--------|---------------------|
| GameplayAbilities | Epic EULA — free for UE projects |
| CommonUI | Epic EULA |
| ModelViewViewModel | Epic EULA |
| EnhancedInput | Epic EULA |
| FunctionalTestingEditor | Epic EULA |
| PythonScriptPlugin | Epic EULA |
| EditorScriptingUtilities | Epic EULA |
| ModelContextProtocol | Third-party — verify license |
| AllToolsets | Epic EULA |
| ToolsetRegistry | Epic EULA |

**Note**: `ModelContextProtocol` plugin — license needs verification.

---

## Critical Issues

1. **No LICENSE file for the project itself** — cannot determine distribution rights
2. **"Red Alert 4" is an EA trademark** — cannot release under this name without license
3. **77.7% of content is ThirdParty with no documented licenses** — legal blocker
4. **Brushify requires commercial license** — no evidence of purchase
5. **ModelContextProtocol plugin** — license unverified
6. **No ESRB/PEGI rating process** — required for commercial release

---

## Required Actions

1. Add a LICENSE file to the repository root
2. Complete IP migration away from "Red Alert 4" / "Command & Conquer" branding
3. Audit all ThirdParty marketplace packs and document license terms
4. Verify Brushify commercial license status
5. Verify ModelContextProtocol plugin license
6. Remove or replace any unlicensed ThirdParty content before release
