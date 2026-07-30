# Voice & EVA Implementation Report

Report on voice event handling, localized subtitles, EVA priority queue, and VoxCPM2 CSV manifest export/import.

## 1. Unit Voice Events (624 Manifest Lines)
Every unit supports 8 canonical voice event tags:
- `Voice.Selected`
- `Voice.Move`
- `Voice.Attack`
- `Voice.Ability`
- `Voice.Damaged`
- `Voice.Elite`
- `Voice.Idle`
- `Voice.Death`

Total events: **78 Units x 8 Events = 624 Voice Events**.

## 2. Voice Subsystem & Subtitle Fallback
`URA4VoiceSubsystem` manages audio playback via `VoiceId` and `EventTag`:
- If `.WAV` / `USoundWave` asset is present -> plays audio.
- If audio asset is missing -> logs `MissingSoundWave` and displays canonical Russian subtitle text from String Table.
- `Voice.Death` events attach to dying entity location without being cut off by entity destruction.

## 3. EVA Priority System
EVA announcements use a priority queue:
1. Superweapon Warning & Mission Critical (Highest)
2. Base Under Attack / Power Shortage (High)
3. Unit Ready / Construction Complete (Normal)
4. Resource / Information Alerts (Low)
*Identical alerts within 3 seconds are merged to prevent audio clutter.*

## 4. VoxCPM2 Manifest Interchange
- **Manifest Path**: `Content/RA4/Audio/Generated/voice_manifest.csv`
- Contains: `Faction`, `UnitId`, `VoiceId`, `EventTag`, `Variant`, `TextRu`, `SoundWave`, `Priority`, `CooldownSeconds`, `Weight`, `Status`, `SourceLine`.
