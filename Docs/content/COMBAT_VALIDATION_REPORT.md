# Combat Validation Report

Validation of weapons, 9x9 damage and armor matrix, veterancy ranks, hitscan/projectiles, and DPS alignment.

## 1. 9x9 Damage & Armor Matrix Verification
The damage matrix implements exact multiplier percentages defined in Section 2 of `RA4_Factions_Units_Economy_Voice_Bible.md`:

| Warhead / Armor | LightInfantry | HeavyInfantry | LightVehicle | HeavyVehicle | SiegeVehicle | Air | Naval | Building | Shielded |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Ballistic** | 100% | 75% | 75% | 50% | 50% | 75% | 50% | 30% | 50% |
| **Fragmentation** | 150% | 100% | 75% | 50% | 50% | 50% | 50% | 50% | 50% |
| **ArmorPiercing** | 50% | 75% | 125% | 150% | 150% | 75% | 125% | 75% | 50% |
| **Siege** | 75% | 75% | 75% | 100% | 100% | 10% | 125% | 200% | 50% |
| **Electric** | 125% | 125% | 100% | 85% | 85% | 100% | 100% | 50% | 200% |
| **Plasma** | 110% | 110% | 115% | 125% | 125% | 110% | 120% | 130% | 150% |
| **Cryogenic** | 120% | 100% | 100% | 90% | 90% | 100% | 100% | 50% | 75% |
| **Temporal** | 100% | 100% | 100% | 100% | 100% | 100% | 100% | 100% | 150% |
| **AntiAir** | 10% | 10% | 10% | 10% | 10% | 200% | 10% | 0% | 50% |

## 2. Veterancy Ranks & Thresholds
- **Recruit**: Base stats (0.0x cost threshold).
- **Veteran**: 1.0x unit cost destroyed -> +10% Damage, +8% HP, passive out-of-combat regen.
- **Elite**: 2.5x unit cost destroyed -> +10% Damage, +10% HP, ability upgrade.
- **Heroic**: 5.0x unit cost destroyed -> Heroic aura passive, special visual marker, heroic voice line.

## 3. Damage Calculation Pipeline
`FinalDamage = BaseDamage * DamageMatrix(Warhead, Armor) * VeterancyMultiplier * FactionModifiers * ShieldAbsorption`
*Zero random variance or critical hit chance is added, ensuring 100% deterministic combat outcomes.*
