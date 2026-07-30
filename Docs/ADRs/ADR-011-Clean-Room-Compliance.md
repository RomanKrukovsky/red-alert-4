# ADR-011: Clean-Room Compliance & IP Isolation Policy

## Status
Accepted

## Context
Red Alert 4 is an original RTS project built on Unreal Engine 5. It incorporates architectural lessons from classic RTS engines (SAGE, Zero Hour, RA3), but MUST maintain absolute IP isolation from third-party proprietary source code, XML schemas, assets, or balance tables.

## Decision
1. **Zero External Code**: No EA source code, XML schema files, W3X maps, TGA textures, or `.big` archives are permitted inside production folders (`Source/`, `Content/`, `Plugins/`).
2. **Neutral Naming & Original Entities**: All test entities, factions, and units use neutral, original names (`TestInfantry`, `TestVehicle`, `TestFactory`, `TestProjectile`, `FactionAlpha`, `FactionBeta`).
3. **Automated Compliance Verification**: `Build/Compliance/compliance_scan.py` runs during CI builds to block forbidden directory structures, unauthorized file extensions, and trademarked terms.
