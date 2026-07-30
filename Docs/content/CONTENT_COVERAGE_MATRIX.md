# Content Coverage Matrix

| Bible Section | Lines | Implemented | Data Asset | Runtime | Test |
|---------------|-------|-------------|------------|---------|------|
| 1.1 Main resources | 106-113 | Yes | FactionDef | SimWorld | Economy tests |
| 1.2 Match start | 114-125 | Yes | FactionDef | SimWorld::Initialize | Existing |
| 1.3 Ore & harvesting | 126-128 | Yes | ResourceNodeDef | SystemHarvesters | Economy tests |
| 1.4 Construction/refund | 129-139 | Yes | ProductionInfo | SystemConstruction | Production tests |
| 1.5 Power system | 140-142 | Yes | BuildingInfo | SystemPower | Power tests |
| 1.6 Command limit | 143-155 | Yes | PlayerState | Production validation | Existing |
| 1.7 Tech tiers | 156-162 | Partial | (derived from buildings) | Prerequisites check | Production tests |
| 2.1 Armor types | 164-176 | Yes | ArmorClass enum | DamageMatrixDef | Combat tests |
| 2.2 Damage matrix | 177-189 | Yes | DamageMatrixDef | GetDamageMultiplier | BibleImport tests |
| 2.3 Veterancy | 190-197 | Yes | VeterancyDef | SystemVeterancy | Veterancy tests |
| 2.4 Commands | 198-200 | Yes | CommandType | SystemOrders | Order tests |
| 3 Faction resources | 201-208 | Partial | FactionResourceDef | (data loaded, runtime pending) | BibleImport tests |
| 4 Voice rules | 209-211 | Yes | VoiceSetDef | (data loaded) | BibleImport tests |
| USSR faction | 212-1037 | Yes | Entities + VoiceSets | Loaded | BibleImport tests |
| Alliance faction | 1038-1901 | Yes | Entities + VoiceSets | Loaded | BibleImport tests |
| Coalition faction | 1902-2767 | Yes | Entities + VoiceSets | Loaded | BibleImport tests |
| ChronoLegion faction | 2768-3691 | Yes | Entities + VoiceSets | Loaded | BibleImport tests |
| EVA lines (all) | per faction | Yes | EvaLineDef | Loaded | BibleImport tests |
| Buildings (all) | per faction | Yes | EntityDef (Building) | Loaded | BibleImport tests |
| Unit cards (78) | per faction | Yes | EntityDef + VoiceSetDef | Loaded | BibleImport tests |
| Voice events (624) | per unit | Yes | VoiceLineDef | Loaded | BibleImport tests |
