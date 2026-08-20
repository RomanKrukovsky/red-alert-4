# EVA Voiceover Generation Guide — Red Alert 4

**Version:** 1.0.0  
**Target Pipeline:** `openbmb/VoxCPM2` (PyTorch / MPS / CUDA)  
**Script Harness:** `generate_eva_voxcpm.py`  
**Output Target:** `Content/RA4/Audio/Generated/WAV/` (48 kHz, Mono, PCM 16-bit WAV)  

---

## 1. Overview & Architecture

The **EVA Voiceover Generation Pipeline** synthesizes high-quality Russian voice lines for all factions in *Red Alert 4*. The pipeline relies on `openbmb/VoxCPM2` for text-to-speech design and Hi-Fi reference voice cloning.

### Core Modules (`GeneratedVO/pipeline/`)

1. **`voice_bible.py`** — Defines faction voice specifications (`GeneratedVO/voice_bible.json`).
2. **`line_writer.py`** — Constructs text variants for all gameplay events (`GeneratedVO/voice_lines.json`).
3. **`anchor_generator.py`** — Generates 3 reference voice anchors per VoiceId using zero-shot `voxcpm design`.
4. **`anchor_selector.py`** — Auto-scores anchors by RMS consistency, signal-to-silence ratio, and duration, picking the optimal reference clip.
5. **`clip_generator.py`** — Synthesizes gameplay lines via `voxcpm clone` using the selected voice anchor.
6. **`qc.py`** — Trims leading/trailing silence, normalizes peak levels to -1.0 dBFS, and converts float32 audio to 48 kHz PCM 16-bit mono WAV.
7. **`generate_eva_voxcpm.py`** — Unified CLI entry-point driving end-to-end execution, anchor selection, and batch generation.

---

## 2. Environment Setup & Prerequisites

### Hardware Requirements
* **Apple Silicon:** M1/M2/M3/M4 Max with 36GB+ unified memory (MPS backend).
* **NVIDIA GPU:** RTX 3080/4080/6000 or Tesla T4/A100 with 16GB+ VRAM (CUDA backend).

### Software Environment
Ensure virtual environment activation and dependencies:
```bash
cd <home>/Documents/red-alert-4
source .venv/bin/activate

# Verify VoxCPM installation
python -c "import voxcpm; print(voxcpm.__version__)"
```

---

## 3. CLI Command Reference

`generate_eva_voxcpm.py` provides a flexible CLI for anchor generation, anchor selection, line cloning, post-processing, and full batch runs.

### General Usage Syntax
```bash
python generate_eva_voxcpm.py [OPTIONS]
```

### Modes (`--mode`)
* `anchors`: Synthesize voice reference anchors (3 per VoiceId).
* `select`: Run QC scoring and auto-select best reference anchor per VoiceId.
* `lines`: Synthesize event dialogue lines using the selected reference anchor.
* `qc`: Run audio normalization, trimming, and validation on output WAVs.
* `full` (Default): Run end-to-end pipeline (anchors -> select -> lines -> qc -> manifest).

### Command Examples

#### 1. Generate Voice Anchors for Soviet EVA on MPS
```bash
python generate_eva_voxcpm.py \
  --mode anchors \
  --faction EVA_Soviet \
  --device mps \
  --cfg 2.0 \
  --steps 10
```

#### 2. Generate Lines for Allied EVA with Custom Output Directory
```bash
python generate_eva_voxcpm.py \
  --mode lines \
  --voice-id EVA_Alliance \
  --device mps \
  --output-dir Content/RA4/Audio/Generated/WAV
```

#### 3. Run Complete End-to-End Pipeline for All Factions
```bash
python generate_eva_voxcpm.py \
  --mode full \
  --device mps \
  --cfg 2.0 \
  --steps 10 \
  --normalize \
  --no-denoiser
```

#### 4. Run QC Audit and Normalization on Existing WAV Directory
```bash
python generate_eva_voxcpm.py \
  --mode qc \
  --input-dir Content/RA4/Audio/Generated/WAV
```

---

## 4. Parameter Reference

| Parameter | Type | Default | Description |
|---|---|---|---|
| `--mode` | string | `full` | Execution mode (`anchors`, `select`, `lines`, `qc`, `full`). |
| `--faction` | string | `ALL` | Target faction tag (`EVA_Soviet`, `EVA_Alliance`, `EVA_Coalition`, `EVA_Chrono`). |
| `--voice-id` | string | `ALL` | Specific VoiceId target (e.g. `EVA_Soviet`). |
| `--device` | string | `mps` | Hardware compute device (`mps`, `cuda`, `cpu`). |
| `--cfg` | float | `2.0` | Classifier-Free Guidance scale (1.5 – 2.5 recommended). |
| `--steps` | int | `10` | Diffusion sampling steps (10 for fast generation, 15-20 for final polish). |
| `--normalize` | flag | `True` | Apply Russian pronunciation dictionary normalization prior to synthesis. |
| `--no-denoiser` | flag | `True` | Output dry un-effected audio (prevents baked noise reduction artifacts). |
| `--output-dir` | string | `Content/RA4/Audio/Generated/WAV` | Destination directory for final generated audio files. |
| `--batch-size` | int | `4` | Number of concurrent synthesis jobs. |

---

## 5. Input Configuration Files

The generator reads from and validates against the following JSON schema files:

1. **`Content/RA4/Audio/Generated/eva_runtime_policy.json`**  
   Contains event priority mappings (100–35), cooldown timers (e.g. `BASE_UNDER_ATTACK: 12.0s`), concurrency groups, and aggregation parameters.

2. **`Content/RA4/Audio/Generated/eva_pronunciation_ru.json`**  
Contains phonetic mappings for technical codes (`T1` -> `Tier One`), callsigns (`Alpha-1` -> `Alpha One`), and acronyms (`MCV` -> `MCV`).
3. **`GeneratedVO/voice_bible.json`**  
   Defines prompt control instructions for each VoiceId.

---

## 6. Output Directory Structure

```
Content/RA4/Audio/Generated/
├── eva_runtime_policy.json
├── eva_pronunciation_ru.json
├── Anchors/
│   ├── EVA_Soviet/
│   │   ├── anchor_01.wav
│   │   ├── anchor_02.wav
│   │   ├── anchor_03.wav
│   │   └── selection.json
│   ├── EVA_Alliance/
│   ├── EVA_Coalition/
│   └── EVA_Chrono/
├── WAV/
│   ├── EVA_Soviet/
│   │   ├── EVA_Soviet_BaseUnderAttack_01.wav
│   │   ├── EVA_Soviet_BuildingComplete_01.wav
│   │   └── ...
│   ├── EVA_Alliance/
│   ├── EVA_Coalition/
│   ├── EVA_Chrono/
│   ├── voice_manifest.csv
│   └── unreal_voice_import.csv
└── state.json
```

---

## 7. Quality Assurance (QC) Standards

Every generated WAV file must pass automated validation in `qc.py`:

* **Sample Rate & Format:** 48,000 Hz, 1 Channel (Mono), 16-bit PCM.
* **Peak Level:** Normalized to **-1.0 dBFS** (Hard reject if peak > -0.5 dBFS).
* **Silence Trimming:** Leading silence trimmed to **50–100ms**; trailing silence trimmed to **100–150ms** (`top_db=30`).
* **Duration Constraints:** EVA clips must fall between **0.7s and 4.0s**.

---

## 8. Troubleshooting & Edge Cases

### Issue 1: High Pitch Squeaks / Gargling VoxCPM Artifacts
* **Symptom:** Audio output contains unnatural high-pitch squeaks, metallic gargling, or robotic stuttering.
* **Solution:**  
  1. Reduce CFG scale to `--cfg 1.8`.
  2. Increase sampling steps to `--steps 15`.
  3. Ensure `--no-denoiser` is enabled.
  4. Regenerate anchor if artifacts persist across all lines of a VoiceId.

### Issue 2: Mangled Pronunciation of Tech Codes or Numbers
* **Symptom:** VoxCPM spells out "T-1" as "Te-one" or stutters on "MCV".* **Solution:**  
  1. Inspect `Content/RA4/Audio/Generated/eva_pronunciation_ru.json`.
2. Add explicit stress marks (`́`) or phonetic spelling to `spoken` field (e.g., `"spoken": "MCV"`).  3. Run script with `--normalize`.

### Issue 3: Dynamic Range Clipping & Distortion
* **Symptom:** Audio clips when imported into MetaSounds or played back on high volume.
* **Solution:**  
  * Re-run `python generate_eva_voxcpm.py --mode qc`. `qc.py` will soft-limit and re-peak normalize the file to -1.0 dBFS.

### Issue 4: CUDA / MPS Memory Out of Memory (OOM)
* **Symptom:** `RuntimeError: MPS backend out of memory` or `CUDA out of memory`.
* **Solution:**  
  1. Pass `--batch-size 1` to force sequential clip generation.
  2. Clear torch MPS/CUDA cache between voice IDs:
     ```python
     import torch
     if torch.backends.mps.is_available():
         torch.mps.empty_cache()
     elif torch.cuda.is_available():
         torch.cuda.empty_cache()
     ```

### Issue 5: Truncated Audio / Early Cutoff
* **Symptom:** Voice line cuts off the last syllable (e.g. "Base under ata...").* **Solution:**  
  * Increase silence trimming threshold from `top_db=30` to `top_db=35` or `40` in `qc.py`.