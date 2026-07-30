# Content Import Report

## Source
- **File**: `RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md`
- **Lines**: 3691
- **SHA-256**: 16a78f699011f730...

## Output
- **Normalized JSON**: `Content/RA4/Data/Generated/ra4_content.normalized.json`
- **Voice Manifest**: `Content/RA4/Audio/Generated/voice_manifest.csv`

## Statistics

| Metric | Count |
|--------|-------|
| Factions | 4 |
| Units | 78 (19 SU + 20 AL + 20 CO + 19 CH) |
| Unique unit IDs | 78 |
| Buildings | 64 (16 per faction) |
| Voice events | 624 (78 units × 8 events) |
| EVA lines | 32 (8 per faction) |
| Damage matrix cells | 81 (9 warheads × 9 armor types) |
| Veterancy levels | 4 (Recruit, Veteran, Elite, Heroic) |
| Faction resources | 4 (Mobilization, Intelligence, Synchronization, TemporalStability) |

## Issues
- None detected. All 78 unit IDs are unique and match expected counts.
- Hero units present for all factions: SU_Hero_Morozova, AL_Hero_Hart, CO_Hero_Mei, CH_Hero_Voss.

## Re-run
```bash
python3 Tools/ContentImport/parse_bible.py
python3 Tools/ContentImport/generate_voice_manifest.py
```
Idempotent: re-running without source changes produces identical output.
