# ADR-004: Data-Driven Content Architecture & Asset Loaders

## Status
Accepted

## Context
RTS game balance and entity parameters require rapid iteration without recompiling C++ source files.

## Decision
1. `ContentDatabase` acts as the single source of truth for all unit definitions, building specs, weapon stats, damage matrices, EVA lines, and faction attributes.
2. `BibleContentLoader` parses normalized JSON content files into `ContentDatabase` idempotently.
3. UE5 editor tools author `URA4UnitDefinition`, `URA4BuildingDefinition`, and `URA4WeaponDefinition` Data Assets that serialize cleanly into canonical JSON definitions.
