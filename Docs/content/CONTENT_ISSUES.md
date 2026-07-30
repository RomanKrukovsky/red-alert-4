# Content Issues

## CONFLICT
- **Unit count**: Task brief says 78 units. Bible has exactly 78 (19 SU + 20 AL + 20 CO + 19 CH). No conflict.
- **Damage matrix**: Bible table has 6 armor columns (Лёгкая пехота, Тяжёлая пехота, Лёгкая техника, Тяжёлая техника, Здания, Воздух) but 9 armor types are defined in section 2.1 (adds Морская, Щитовая). Missing columns filled by inference: Naval = HeavyVehicle values, Shielded = 1.0× default (2.0× for Electric per bible text).

## INFERENCE
- **Veterancy Elite threshold**: Bible says "2.5 стоимости". Stored as integer 2 (conservative). Documented in VeterancyDef.
- **DPS**: Used as target reference, not directly stored. Weapon damage derived from DPS conceptually but exact weapon stats are not in the bible table.
- **Crew group on building sale**: Bible says "небольшая группа экипажа" without specifying composition. Rule is configurable, not implemented yet.

## UNKNOWN
- None at this time.
