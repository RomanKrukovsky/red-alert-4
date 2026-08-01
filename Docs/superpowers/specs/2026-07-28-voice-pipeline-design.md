# Voice Pipeline for Red Alert 4 — Design Spec

**Date:** 2026-07-28
**Status:** Approved (4/4 sections confirmed)
**Author:** OpenCode voice director agent

## Purpose

Generate original Russian voice-over package for Red Alert 4 (industrial RTS) using openbmb/VoxCPM2. Four factions: Soviet Union, Alliance, Vostochnaya Coalition, Khronolegion. All voices must be original (no imitation of real actors or existing Command & Conquer voice actors). Pipeline produces WAV files at 48 kHz mono PCM 16-bit, ready for direct import into Unreal Engine MetaSounds.

## Scope

**Test package (this session):** 4 EVA + 4 Rifleman + 4 MainTank + 4 Fighter + 4 Captain = 20 VoiceId. Each voice gets 3 anchors (60 anchors). Test package = 100 voice lines (5 per voice × 20 voices). voice_review.html presented to user for Accept/Reject/Regenerate.

**Full package (after test approval):** 4 EVA + 60 unit roles = 64 VoiceId. 192 anchors. 420 EVA event variants + ~3240 unit event variants = ~3852 line files. ~24 hours wall-time on MPS.

## Architecture

Five Python modules under `/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO/pipeline/`:

1. **voice_bible.py** — generates `GeneratedVO/voice_bible.json` with all VoiceId definitions (gender_age, control_instruction, ru_pacing, prosody_caps, forbidden_traits, sample_texts).
2. **line_writer.py** — generates `voice_lines.json` with all event text variants per (VoiceId, Event).
3. **anchor_generator.py** — for each VoiceId, generates 3 anchors via `voxcpm design` (control instruction only, no reference). MPS device. cfg=2.0, steps=10. Output: `Anchors/<Faction>/<VoiceId>/anchor_<01|02|03>.wav`.
4. **anchor_selector.py** — auto-QC scores 3 anchors per VoiceId, selects best. Saves `Anchors/<Faction>/<VoiceId>/selection.json`.
5. **clip_generator.py** — for each line in voice_lines.json, runs `voxcpm clone` with `reference_wav_path=<selected anchor>` + `control="<event-specific style>"` + optional `(emotion)prefix`. Up to 3 retries on artifacts.

Plus: **qc.py** (post-process to PCM 16-bit, normalize, librosa trim), **review_html.py** (interactive review page), **manifest_writer.py** (voice_manifest.csv + unreal_voice_import.csv).

`state.json` tracks completion for resume capability.

## Voice Catalog (test)

20 VoiceIds across 4 factions:

| Faction | EVA | Rifleman | MainTank | Fighter | Captain |
|---------|-----|----------|----------|---------|---------|
| Soviet | EVA_Soviet | USSR_Rifleman | USSR_MainTank | USSR_Fighter | USSR_Captain |
| Alliance | EVA_Alliance | Alliance_Rifleman | Alliance_MainTank | Alliance_Fighter | Alliance_Captain |
| Coalition | EVA_Coalition | Coalition_Rifleman | Coalition_MainTank | Coalition_Fighter | Coalition_Captain |
| Chrono | EVA_Chrono | Chrono_Rifleman | Chrono_MainTank | Chrono_Fighter | Chrono_Captain |

Full VoiceId naming: `<FactionTag>_<RoleTag>` where RoleTag matches EVA archetype.

## EVA Control Instructions

- **EVA_Soviet:** "Authoritative mature female military command voice, low contralto, precise Russian diction, restrained emotion, firm concise delivery, calm under pressure."
- **EVA_Alliance:** "Professional female tactical AI voice, clean and intelligent, confident, modern, precise, slightly warm, medium-fast pace, excellent Russian diction."
- **EVA_Coalition:** "Calm disciplined female strategic command voice, elegant and precise, controlled emotion, measured pace, subtle authority, clear standard Russian."
- **EVA_Chrono:** "Androgynous timeless command voice, calm and unsettling, extremely precise, restrained emotion, deliberate micro-pauses, clear Russian, no baked-in audio effects."

## Anchor Workflow

Each VoiceId gets 3 anchors of different semantic context:

1. **anchor_01** — Identity: "Ya — [rol Factions]. Gotov k rabote."
2. **anchor_02** — Command: "Podrazdelenie, slushat moyu komandu. Vypolnyat."
3. **anchor_03** — Combat report: "Kontakt s protivnikom. Zaprashivayu podderzhku."

Selection scoring (auto, before any human review):

- `voice_consistency_score`: inverse variance of RMS energy in first 50% of file
- `clarity_score`: signal-to-silence ratio (median RMS in active regions / median RMS in silent regions)
- `duration_score`: 1.5–3.5s sweet spot

Pick highest total. If top two within 5% → pick anchor_02 as most universal.

Selected anchor used as `reference_wav_path` for all subsequent line generation via Hi-Fi Cloning.

## Generation Parameters

- **Anchors:** `voxcpm design` — control instruction only, no reference. cfg=2.0, steps=10.
- **Lines (drafts):** `voxcpm clone` — anchor reference + event-specific control. cfg=2.0, steps=10.
- **Lines (finals):** if draft passes QC, can regenerate with steps=15–20 for better quality. Only applied if it improves without instability.
- **Device:** `mps` (M4 Max confirmed). VoxCPM auto-downgrades bf16 → float32 on MPS.
- **Flags:** `--normalize` (Russian text normalization for numbers/abbreviations), `--no-denoiser` (clean dry voice, no baked effects).
- **Emotion prefix:** for emotional lines, wrap style in parens before text: `(urgent command)Trevoga. Baza under atakoy.`

## File Format

- WAV, 48 kHz, mono, PCM 16-bit
- Peak ≤ -1.0 dBFS (hard reject)
- Duration: 0.5–4.0s for unit lines, 0.7–4.0s for EVA lines
- No music, no ambient, no FX in the file (all applied later in UE MetaSounds)
- Leading silence: 50–100ms, trailing: 100–150ms (via librosa trim, top_db=30)
- No aggressive noise reduction or compression in post

**Post-process pipeline** (qc.py): voxcpm outputs float32 → soundfile re-encode to PCM 16-bit → librosa trim leading/trailing silence → soft normalize if peak > -3 dBFS or < -20 dBFS → final QC metrics write to manifest.

## QC Criteria

Hard reject if:
- sample_rate ≠ 48000 Hz
- channels ≠ 1
- peak_dBFS > -0.5 (clipping)
- duration_sec < 0.5 or > 5.0 (silence/truncation)
- file_size < 8 KB (empty)

Soft warnings (file kept):
- rms_dBFS outside [-28, -12]
- silence_ratio > 0.35

Artifact detection (retry trigger):
- spectral anomaly (high-frequency noise spike)
- voice cut-off mid-phrase
- stutter/repeat artifacts

Retry escalation:
- Retry 1: same control, steps=15
- Retry 2: same control + different anchor (anchor_01 or anchor_03 if anchor_02 was selected)
- Retry 3: same control + steps=20
- After 3 failures: file written to `Rejected/` with reason, status=`failed` in manifest

## Manifest Format

**voice_manifest.csv** columns:
`AssetId, Faction, UnitId, VoiceId, EventTag, Variant, TextRu, ControlInstruction, AnchorFile, OutputFile, DurationSeconds, SampleRate, PeakDb, GenerationMode, CfgValue, InferenceTimesteps, Status, Notes, GenerationTimestamp`

**unreal_voice_import.csv** columns:
`SoundWaveName, SourceFile, FactionTag, UnitTag, EventTag, SubtitleKey, Priority, CooldownSeconds, Weight, ConcurrencyGroup`

Priority defaults (from TZ):
- EVA.EnemySuperweapon: 100
- EVA.Objective: 90
- EVA.BaseUnderAttack: 80
- EVA.UnitReady: 50
- Unit.CriticalDamage: 45
- Unit.Selected: 30
- Unit.Idle: 10
- Others: 20

CooldownSeconds: EVA=3, Units=5, Death/CriticalDamage=0.
ConcurrencyGroup: `eva_<faction>` or `unit_<faction>_<class>`.

## File Layout

```
GeneratedVO/
├── Anchors/
│   ├── Soviet/EVA_Soviet/{anchor_01,anchor_02,anchor_03}.wav + selection.json
│   └── ... (one per VoiceId)
├── EVA/
│   └── <Faction>/VO_<Faction>_EVA_<Event>_<Variant>.wav
├── Units/
│   └── <Faction>/VO_<Faction>_<Role>_<Event>_<Variant>.wav
├── Manifests/
│   ├── voice_manifest.csv
│   └── unreal_voice_import.csv
├── Reports/
│   └── voice_review.html
├── Rejected/
│   └── VO_<Faction>_<...>_<reason>.wav
├── pipeline/
│   ├── voice_bible.py
│   ├── line_writer.py
│   ├── anchor_generator.py
│   ├── anchor_selector.py
│   ├── clip_generator.py
│   ├── qc.py
│   ├── review_html.py
│   ├── manifest_writer.py
│   ├── state.py
│   └── run.py
├── state.json
└── voice_bible.json
```

## Test Package Events (5 per VoiceId)

- **EVA:** Game.Start, Base.UnderAttack, Unit.Ready, Player.Victory, Resources.Low
- **Rifleman:** Selected, Move, Attack, EnemyDestroyed, Idle
- **MainTank:** Selected, Attack, Attack.Building, Damaged, CriticalDamage
- **Fighter:** Selected, Attack.Air, Ability.Activate, EnemyDestroyed, Death
- **Captain:** Selected, Move, Attack, CannotComply, Death

= 100 line files for test review.

## Review Workflow

After test generation:
1. `qc.py` produces `voice_review.html` with audio players per line.
2. Each entry shows: VoiceId, Event, Text, audio player, three buttons (Accept, Regenerate, Reject).
3. User reviews in browser. State stored back in `state.json` as `review_status`.
4. Accepted files locked — never regenerated.
5. Regenerate requests re-queued in state.json with new variant number.
6. Reject requests move file to `Rejected/` and remove from manifest.

Only after user approves test package → proceed to full generation.

## Resume Capability

`state.json` schema:
```json
{
  "phase": "anchors|lines|done",
  "completed": ["VO_Soviet_EVA_Selected_01.wav", ...],
  "failed": [{"file": "...", "reason": "...", "retries": 3}],
  "review_status": {"VO_...": "accepted|regenerate|rejected"},
  "last_processed": "VO_Chrono_Captain_Move_03",
  "timestamp": "..."
}
```

Pipeline reads state, skips `completed`, continues from `last_processed`. Accepted files are never overwritten (per TZ).

## Rerun Commands

```bash
# Regenerate all anchors for one faction:
python3 -m pipeline.anchor_generator --factions Soviet --output GeneratedVO/Anchors

# Regenerate specific role + events:
python3 -m pipeline.clip_generator --factions Soviet --voices MainTank --events Selected Move Attack

# Dry-run with new text:
python3 -m pipeline.clip_generator --voices EVA_Alliance --text "Novaya replika." --output /tmp/test.wav

# QC + regenerate manifests only:
python3 -m pipeline.qc --regenerate-manifests

# Full resume:
python3 -m pipeline.run --resume --skip-accepted
```

## Final Report

At end of session prints:
- `total_generated`: count of all WAV files in GeneratedVO
- `total_accepted`: review_status==accepted
- `total_rejected`: review_status==rejected OR generation_failed
- `total_retried`: count of files where retries > 0
- VoiceId table: id, faction, role, anchor_duration, total_lines, accepted_count, avg_duration
- Path to voice_manifest.csv and unreal_voice_import.csv
- Estimated time saved on rerun (resume)

## Verification

Before final delivery:
- `verify_manifest_match_text()` — every TextRu column matches audio duration roughly (≥30 chars/sec floor, ≤25 chars/sec ceiling for Russian at cfg=2.0)
- `verify_no_duplicate_phrases()` — no two VoiceId share the exact same line text (catches copy-paste mistakes)
- `verify_no_red_alert_quotes()` — text regex against known C&C quotes (EVA "Nuclear launch detected", "Construction complete", etc.) — fail if matched
- `verify_voice_id_isolation()` — compute MFCC similarity across VoiceIds in same faction. Flag if > 0.85 (too similar).

## Out of Scope

- Radio filters, faction FX, music beds — applied later in Unreal Engine MetaSounds
- Hero voice acting for specific characters (one-off bespoke voices)
- Localization to non-Russian languages
- Lip-sync timing alignment to animation

## Risks

1. **Voice drift across lines:** VoxCPM2 Hi-Fi Cloning is reasonably stable but ~5% of lines may drift in timbre. Mitigated by 3 anchors + auto-selection + retry on artifacts.
2. **Russian pronunciation errors:** VoxCPM2 sometimes misplaces stress. Mitigated by writing test phrases first, using yo where ambiguous, listening to anchor picks before committing.
3. **Long generation time:** ~24h for full package. Mitigated by resume + parallel review during generation.

## Approval

All 4 design sections approved by user. Proceeding to implementation.