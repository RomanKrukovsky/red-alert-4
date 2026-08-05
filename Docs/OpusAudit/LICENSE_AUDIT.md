# Opus Audit — License Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## ThirdParty Licenses

| Pack | License | Risk |
|------|---------|------|
| ambientCG | CC0 | NONE |
| Quixel Megascans | Free for UE | LOW |
| Brushify | Commercial — Pro required | HIGH |
| CityPark | Unknown — no license file | MEDIUM |
| FactoryEnvironment | Unknown | MEDIUM |
| IndustryPropsPack6 | Unknown | MEDIUM |
| QuantumCharacter | Unknown | MEDIUM |
| EpicGames samples | Free for UE | LOW |

`find Content/ThirdParty -name "LICENSE*" -o -name "EULA*" | wc -l` = **0** — no license documentation found.

## Trademark Issues

- **"Red Alert 4"** — EA trademark used in: repo name, uproject, module name, throughout codebase
- **"Command & Conquer"** — referenced in docs and test file names (Test_RA3PipelineAndCommandBus.cpp)
- **Original IP migration** — documented but NOT executed

## Project License
No LICENSE file exists in the repository. No open-source or proprietary license declared.

## ra4-ui Dependencies
All MIT/ISC/Apache-2.0 — no copyleft risk.

## UE Plugins
All standard Epic EULA. ModelContextProtocol plugin — license unverified.

## Required Actions
1. Add LICENSE file
2. Complete IP migration away from "Red Alert 4"
3. Audit and document ThirdParty licenses
4. Verify Brushify commercial license
5. Remove unlicensed content before release
