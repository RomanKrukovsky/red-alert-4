# Source Traceability

Every implemented system is traced to its source in the bible.

| System | Bible Section | Bible Lines | Data Asset | Runtime | Test |
|--------|--------------|-------------|------------|---------|------|
| Credits | 1.1 | 107-113 | PlayerState.Credits | SimWorld | Economy tests |
| Energy | 1.5 | 140-142 | BuildingInfo.Power | SystemPower | Power tests |
| Command limit | 1.6 | 143-155 | PlayerState.CommandLimitMax | Production validation | Existing |
| Ore fields (45k/75k) | 1.3 | 127 | ResourceNodeDef | SystemHarvesters | Economy tests |
| Harvester cargo (1200) | 1.3 | 127 | UnitInfo.CargoCapacity | SystemHarvesters | Economy tests |
| Building cancel (90%/60%) | 1.4 | 132-133 | ProductionInfo.CancelRefundPercent | SystemConstruction | Production tests |
| Unit cancel (80%) | 1.4 | 134 | ProductionInfo.CancelRefundPercent | SystemProduction | Production tests |
| Building sell (50%) | 1.4 | 135 | BuildingInfo.SellRefundPercent | (pending) | (pending) |
| Building repair (30%) | 1.4 | 136 | (config rule) | (pending) | (pending) |
| Unit repair (25%) | 1.4 | 137 | (config rule) | (pending) | (pending) |
| Capture (8s disable) | 1.4 | 138 | (config rule) | (pending) | (pending) |
| Power shutoff priority | 1.5 | 141 | (priority list) | SystemPower | Power tests |
| Armor types (9) | 2.1 | 164-176 | ArmorClass enum | DamageMatrixDef | Combat tests |
| Damage matrix (9×6+) | 2.2 | 177-189 | DamageMatrixDef | GetDamageMultiplier | BibleImport tests |
| Veterancy thresholds | 2.3 | 190-197 | VeterancyDef | SystemVeterancy | Veterancy tests |
| Commands (9 types) | 2.4 | 198-200 | CommandType enum | SystemOrders | Order tests |
| Mobilization (Soviet) | 3 | 204 | FactionResourceDef | (data loaded) | BibleImport tests |
| Intelligence (Alliance) | 3 | 205 | FactionResourceDef | (data loaded) | BibleImport tests |
| Synchronization (Coalition) | 3 | 206 | FactionResourceDef | (data loaded) | BibleImport tests |
| TemporalStability (Chrono) | 3 | 207 | FactionResourceDef | (data loaded) | BibleImport tests |
| Voice events (624) | 4 | 210 | VoiceSetDef + VoiceLineDef | (data loaded) | BibleImport tests |
| EVA lines (32) | per faction | Faction sections | EvaLineDef | (data loaded) | BibleImport tests |
| USSR 19 units | Faction: СССР | 212-1037 | EntityDef × 19 | Loaded | BibleImport tests |
| Alliance 20 units | Faction: Альянс | 1038-1901 | EntityDef × 20 | Loaded | BibleImport tests |
| Coalition 20 units | Faction: Восточная коалиция | 1902-2767 | EntityDef × 20 | Loaded | BibleImport tests |
| ChronoLegion 19 units | Faction: Хронолегион | 2768-3691 | EntityDef × 19 | Loaded | BibleImport tests |
| USSR 16 buildings | СССР buildings | 217-236 | EntityDef × 16 | Loaded | BibleImport tests |
| Alliance 16 buildings | Альянс buildings | 1043-1062 | EntityDef × 16 | Loaded | BibleImport tests |
| Coalition 16 buildings | Коалиция buildings | 1907-1928 | EntityDef × 16 | Loaded | BibleImport tests |
| ChronoLegion 16 buildings | Хронолегион buildings | 2773-2793 | EntityDef × 16 | Loaded | BibleImport tests |
