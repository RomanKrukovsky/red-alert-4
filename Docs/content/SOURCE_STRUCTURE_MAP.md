# Source Structure Map

Mapping of `RA4_Factions_Units_Economy_Voice_Bible.md` sections to internal C++ data assets and normalized schemas.

| Markdown Section | Line Range | Target Data Asset / C++ Struct | Purpose |
| --- | --- | --- | --- |
| 1. General rules of the game | L1 - L450 | `URA4EconomyRulesDefinition`, `URA4TechTreeDefinition` | Global match rules, start credits, ore fields, power degradation, command limit |
| 2. Matrix of damage and armor types | L451 - L520 | `URA4DamageMatrixDefinition`, `DamageMatrix.h` | 9 Armor types x 9 Warhead types damage multipliers |
| 3. Factions (USSR) | L521 - L1210 | `URA4FactionDefinition`, `URA4UnitDefinition`, `URA4BuildingDefinition` | Soviet faction mechanics, 19 units, 13 buildings, voice lines || 4. Factions (Alliance) | L1211 - L1830 | `URA4FactionDefinition`, `URA4UnitDefinition`, `URA4BuildingDefinition` | Alliance intelligence mechanics, 20 units, 13 buildings, voice lines |
| 4. Factions (Coalition) | L1831 - L2615 | `URA4FactionDefinition`, `URA4UnitDefinition`, `URA4BuildingDefinition` | Coalition sync grid mechanics, 20 units, 13 buildings, voice lines |
| 4. Factions (Chronolegion) | L2616 - L3420 | `URA4FactionDefinition`, `URA4UnitDefinition`, `URA4BuildingDefinition` | ChronoLegion temporal stability, 19 units, 11 buildings, voice lines |
| 5. AI and auto-behavior | L3421 - L3450 | `URA4AIBehaviorDefinition`, `RA4AI` module | Target priorities, auto-retreat thresholds, formation offsets || 8. Technical UE5 Spec | L3464 - L3520 | Gameplay Tags, `voice_manifest.csv` | Tag hierarchy, voice manifest structure |