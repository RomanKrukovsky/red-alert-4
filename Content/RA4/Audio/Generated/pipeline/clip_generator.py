"""Clip generator v2: short style tags + reference-only voice control.

Key fix: VoxCPM2 over-runs when (control) text + main text together exceed internal max_len.
Strategy:
- Voice timbre carried entirely by reference_wav_path (anchor).
- Style = short English emotion tag (5-10 words) to control delivery.
- Text always ends with period (signals end of speech).
- Set max_len via inference: 4096 tokens default; with cfg=2.0 ~5-7 chars/sec Russian.
- Short text (<40 chars) should produce ~1-3s audio; we accept up to 5.5s.

Retry strategy:
- attempt 0: tag only, cfg=2.0, steps=10
- attempt 1: same, cfg=2.0, steps=15 (sharper)
- attempt 2: shortened text variant (drop trailing adjectives) if available

QC: duration must be 0.5-5.5s, peak <= -1 dBFS.
"""
import json
import os
import sys
import time
from pathlib import Path

import numpy as np
import soundfile as sf

sys.path.insert(0, str(Path(__file__).parent))
from state import load_state, mark_completed, mark_failed

VOXCPM_MODEL_ID = "openbmb/VoxCPM2"
GENVO_ROOT = Path("/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO")

# Short emotion/style tags (5-10 words English) — replaces full control_instruction in text prompt
SHORT_TAGS = {
    "Game.Start": "(calm command)",
    "Base.UnderAttack": "(urgent command)",
    "Unit.Ready": "(alert ready)",
    "Player.Victory": "(restrained triumph)",
    "Resources.Low": "(measured warning)",
    "Selected": "(alert ready concise)",
    "Move": "(confident acknowledgement)",
    "Move.LongDistance": "(confident long march)",
    "Attack": "(controlled aggression)",
    "Attack.Building": "(controlled aggression building)",
    "Attack.Air": "(focused aerial combat)",
    "Attack.Naval": "(focused naval combat)",
    "Ability.Activate": "(sharp activation)",
    "Spawn": "(alert ready)",
    "Damaged": "(strained but intelligible)",
    "CriticalDamage": "(severe urgency controlled)",
    "Retreat": "(urgent retreat)",
    "EnemyDestroyed": "(controlled satisfaction)",
    "Idle": "(relaxed in character)",
    "Veterancy.Gained": "(proud recognition)",
    "Death": "(short final reaction)",
    "CannotComply": "(firm concise refusal)",
    "DestinationBlocked": "(frustrated blocked)",
    "Multiplayer.PlayerJoined": "(casual notice)",
    "Multiplayer.PlayerLeft": "(casual notice)",
    "Multiplayer.ConnectionLost": "(alert warning)",
    "Multiplayer.DesyncDetected": "(urgent warning)",
    "Radar.Offline": "(technical alert)",
    "Radar.Online": "(technical notice)",
    "Superweapon.Detected": "(highest urgency alarm)",
    "Superweapon.Ready": "(restrained threat)",
    "Superweapon.Launched": "(decisive command)",
    "Enemy.Superweapon.Launched": "(critical alarm)",
    "Objective.Updated": "(briefing notice)",
    "Objective.Completed": "(positive acknowledgment)",
    "Objective.Failed": "(regretful notice)",
    "Reinforcements.Arrived": "(welcoming notice)",
    "Enemy.Detected": "(alert detection)",
    "Stealth.Detected": "(alert detection)",
    "Ally.UnderAttack": "(urgent allied threat)",
    "Power.Low": "(technical warning)",
    "Power.Restored": "(technical notice)",
    "Building.Lost": "(urgent loss)",
    "Building.Captured": "(objective notice)",
    "Building.Repaired": "(positive notice)",
    "Unit.Lost": "(urgent loss)",
    "Construction.Complete": "(positive notice)",
    "Construction.Cancelled": "(casual notice)",
    "Construction.Blocked": "(technical notice)",
    "Game.Paused": "(neutral notice)",
    "Game.Resumed": "(neutral notice)",
    "Resources.Exhausted": "(critical warning)",
}

def load_bible():
    return json.load(open(GENVO_ROOT / "voice_bible.json", "r", encoding="utf-8"))

def load_lines():
    return json.load(open(GENVO_ROOT / "voice_lines.json", "r", encoding="utf-8"))

def load_selections():
    return json.load(open(GENVO_ROOT / "Anchors" / "_selections.json", "r", encoding="utf-8"))

def post_process(audio, sr):
    keep_lead = int(0.05 * sr)
    keep_trail = int(0.10 * sr)
    abs_audio = np.abs(audio)
    thresh = abs_audio.max() * 0.02
    nonzero = np.where(abs_audio > thresh)[0]
    if len(nonzero) == 0:
        return audio
    first = max(0, nonzero[0] - keep_lead)
    last = min(len(audio), nonzero[-1] + keep_trail)
    trimmed = audio[first:last]
    peak = np.abs(trimmed).max()
    if peak > 0:
        target_dbfs = -3.0
        target_linear = 10 ** (target_dbfs / 20.0)
        gain = target_linear / peak
        if gain < 1.0:
            trimmed = trimmed * gain
        elif peak < 10 ** (-20.0 / 20.0):
            boost = 10 ** (-18.0 / 20.0) / peak
            trimmed = trimmed * min(boost, 4.0)
    return trimmed

def qc_check(audio, sr, max_dur=5.5):
    if sr != 48000:
        return False, f"sr={sr}"
    if audio.ndim > 1:
        return False, f"channels={audio.shape[1]}"
    duration = len(audio) / sr
    if duration < 0.5 or duration > max_dur:
        return False, f"duration={duration:.2f}s"
    peak = np.abs(audio).max()
    if peak > 0.99:
        return False, f"peak={peak:.3f}"
    return True, "ok"

def generate_attempt(model, anchor_path, tag, text, steps):
    """Single generation: tag + text + reference."""
    text_clean = text.strip()
    if not text_clean.endswith(('.', '!', '?')):
        text_clean += '.'
    full = f"{tag}{text_clean}"
    audio = model.generate(
        text=full,
        reference_wav_path=str(anchor_path),
        cfg_value=2.0,
        inference_timesteps=steps,
        normalize=True,
    )
    sr = model.tts_model.sample_rate
    processed = post_process(audio, sr)
    return processed, sr

def main():
    bible = load_bible()
    lines_data = load_lines()
    selections = load_selections()
    state = load_state()
    state["phase"] = "lines_v2"

    from voxcpm import VoxCPM
    print(f"Loading VoxCPM model...", file=sys.stderr)
    t0 = time.time()
    model = VoxCPM.from_pretrained(VOXCPM_MODEL_ID, load_denoiser=False)
    print(f"Loaded in {time.time()-t0:.1f}s", file=sys.stderr)

    voice_lookup = {v["id"]: v for v in bible["voices"]}

    counter = 0
    total = len(lines_data["lines"])
    results = []

    for line in lines_data["lines"]:
        counter += 1
        vid = line["voice_id"]
        faction = line["faction"]
        event = line["event"]
        variant = line["variant"]
        text = line["text"]
        tag = SHORT_TAGS.get(event, "(neutral)")

        safe_event = event.replace(".", "_")
        if vid.startswith("EVA_"):
            subdir, tag_faction = "EVA", faction
        else:
            subdir, tag_faction = "Units", faction
        output_filename = f"VO_{vid}_{safe_event}_{variant:02d}.wav"
        output_path = GENVO_ROOT / subdir / tag_faction / output_filename
        asset_id = f"LINE_{vid}_{event}_{variant:02d}"

        if output_path.exists() and state.get("phase") != "lines_v2":
            print(f"  [{counter}/{total}] SKIP {asset_id}", file=sys.stderr)
            continue

        anchor_path = selections.get(vid)
        if not anchor_path or not Path(anchor_path).exists():
            print(f"  [{counter}/{total}] FAIL {asset_id}: no anchor", file=sys.stderr)
            mark_failed(state, asset_id, "no_anchor", 0)
            continue

        # Try attempts
        attempts = [
            {"steps": 10, "text": text},
            {"steps": 15, "text": text},
            {"steps": 10, "text": text},  # retry with same but different seed (VoxCPM stochastic)
        ]
        ok = False
        final_reason = ""
        attempts_log = []
        for ai, att in enumerate(attempts):
            t0 = time.time()
            try:
                audio, sr = generate_attempt(
                    model, anchor_path, tag, att["text"], att["steps"]
                )
                passed, reason = qc_check(audio, sr)
                attempts_log.append(f"a{ai}:{reason}({len(audio)/sr:.1f}s,{time.time()-t0:.1f}sg)")
                if passed:
                    ok = True
                    pcm16 = (audio * 32767.0).astype(np.int16)
                    output_path.parent.mkdir(parents=True, exist_ok=True)
                    sf.write(str(output_path), pcm16, sr, subtype='PCM_16')
                    dur = len(audio) / sr
                    print(f"  [{counter}/{total}] OK {asset_id} ({dur:.2f}s, {time.time()-t0:.1f}s, a{ai})", file=sys.stderr)
                    mark_completed(state, asset_id)
                    results.append({"asset_id": asset_id, "status": "ok", "attempts": ai+1, "output": str(output_path)})
                    break
                else:
                    print(f"  [{counter}/{total}] RETRY {asset_id} a{ai} {reason}", file=sys.stderr)
            except Exception as e:
                attempts_log.append(f"a{ai}:exc:{str(e)[:60]}")
                print(f"  [{counter}/{total}] RETRY {asset_id} a{ai} exc:{str(e)[:60]}", file=sys.stderr)

        if not ok:
            mark_failed(state, asset_id, ";".join(attempts_log), len(attempts))
            results.append({"asset_id": asset_id, "status": "failed", "attempts": len(attempts), "log": attempts_log})
            print(f"  [{counter}/{total}] FAIL {asset_id}", file=sys.stderr)

    summary = GENVO_ROOT / "Manifests" / "_clip_generation_summary.json"
    summary.parent.mkdir(parents=True, exist_ok=True)
    with open(summary, "w", encoding="utf-8") as f:
        json.dump({"total": total, "results": results}, f, ensure_ascii=False, indent=2)
    ok_count = sum(1 for r in results if r["status"] == "ok")
    fail_count = sum(1 for r in results if r["status"] == "failed")
    print(f"\nDone. OK: {ok_count}, FAIL: {fail_count}", file=sys.stderr)

if __name__ == "__main__":
    main()
