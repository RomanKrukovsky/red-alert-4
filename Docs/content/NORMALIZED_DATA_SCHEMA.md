# Normalized Data Schema

Documentation for `Content/RA4/Data/Generated/ra4_content.normalized.json` structure.

```json
{
  "source_hash": "sha256_hex_string",
  "schema_version": "1.0.0",
  "generated_at": "ISO-8601 timestamp",
  "total_factions": 4,
  "total_units": 78,
  "total_buildings": 50,
  "total_voice_events": 624,
  "damage_matrix": { ... },
  "units": [
    {
      "id": "SU_Conscript",
      "name_ru": "Призывник",
      "faction": "Soviet",
      "category": "Infantry",
      "tier": 1,
      "cost": 100,
      "build_time": 4,
      "command_limit": 1,
      "hp": 120,
      "armor_type": "LightInfantry",
      "speed": 90,
      "range": 250,
      "dps": 18,
      "role": "Легкая пехота",
      "primary_weapon": "Автомат АК-74",
      "requirements": ["SU_Barracks"],
      "abilities": ["Молотов", "Укрытие"],
      "voice_lines": { "Selected": "Призывник готов!", ... }
    }
  ]
}
```
