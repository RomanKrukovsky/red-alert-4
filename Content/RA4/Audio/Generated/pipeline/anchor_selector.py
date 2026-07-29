"""Anchor auto-selector: score 3 anchors per VoiceId and pick the best.

Scoring metrics:
- voice_consistency_score: 1 / (1 + stdev(RMS_energy_first_50%)) — low variance = stable timbre
- clarity_score: median_rms_active / median_rms_silent — higher = clearer speech
- duration_score: 1.5–3.5s sweet spot (linear falloff outside)

Total = weighted sum. Pick highest.
"""
import json
import os
import sys
from pathlib import Path

import numpy as np
import soundfile as sf

GENVO_ROOT = Path("/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO")

def compute_metrics(wav_path):
    audio, sr = sf.read(wav_path)
    if audio.ndim > 1:
        audio = audio.mean(axis=1)
    duration = len(audio) / sr
    # Compute RMS energy in 50ms windows
    hop = sr // 20  # 50ms
    if len(audio) < hop * 2:
        return None
    n_frames = len(audio) // hop
    frames = audio[:n_frames * hop].reshape(n_frames, hop)
    rms_frames = np.sqrt(np.mean(frames ** 2, axis=1)) + 1e-9
    # First 50% of file
    half = len(rms_frames) // 2
    first_half = rms_frames[:half]
    consistency_score = 1.0 / (1.0 + np.std(first_half) / (np.mean(first_half) + 1e-9))
    # Active vs silent regions
    silent_threshold = np.median(rms_frames) * 0.3
    active = rms_frames[rms_frames > silent_threshold]
    silent = rms_frames[rms_frames <= silent_threshold]
    if len(silent) == 0:
        silent = np.array([1e-6])
    clarity_score = np.median(active) / np.median(silent) if len(active) > 0 else 1.0
    # Duration score
    if 1.5 <= duration <= 3.5:
        duration_score = 1.0
    elif duration < 1.5:
        duration_score = max(0.0, duration / 1.5)
    else:
        duration_score = max(0.0, 1.0 - (duration - 3.5) / 3.0)
    peak_db = 20 * np.log10(np.abs(audio).max() + 1e-12)
    return {
        "consistency": round(float(consistency_score), 4),
        "clarity": round(float(min(clarity_score, 5.0)), 4),
        "duration": round(float(duration), 2),
        "duration_score": round(float(duration_score), 4),
        "peak_db": round(float(peak_db), 2),
        "sample_rate": sr
    }

def score_voice(vid, anchor_dir):
    candidates = []
    for i in [1, 2, 3]:
        p = anchor_dir / f"anchor_{i:02d}.wav"
        if not p.exists():
            continue
        m = compute_metrics(p)
        if m is None:
            continue
        # Total score: weighted
        total = m["consistency"] * 0.5 + min(m["clarity"], 3.0) / 3.0 * 0.3 + m["duration_score"] * 0.2
        candidates.append({
            "anchor_num": i,
            "file": str(p.relative_to(GENVO_ROOT)),
            "absolute_path": str(p),
            **m,
            "total_score": round(float(total), 4)
        })
    if not candidates:
        return None
    candidates.sort(key=lambda c: c["total_score"], reverse=True)
    # If top two within 5%, prefer anchor_02 (most universal)
    if len(candidates) >= 2 and (candidates[0]["total_score"] - candidates[1]["total_score"]) < 0.05:
        for c in candidates:
            if c["anchor_num"] == 2:
                selected = c
                reason = "tie_within_5pct_preferring_anchor_02"
                break
        else:
            selected = candidates[0]
            reason = "top_score"
    else:
        selected = candidates[0]
        reason = "best_overall_score"
    return {
        "voice_id": vid,
        "candidates": candidates,
        "selected_anchor_num": selected["anchor_num"],
        "selected_file": selected["absolute_path"],
        "selection_reason": reason
    }

def main():
    bible_path = GENVO_ROOT / "voice_bible.json"
    with open(bible_path, "r", encoding="utf-8") as f:
        bible = json.load(f)
    selections = {}
    for voice in bible["voices"]:
        vid = voice["id"]
        faction = voice["faction"]
        anchor_dir = GENVO_ROOT / "Anchors" / faction / vid
        sel = score_voice(vid, anchor_dir)
        if sel is None:
            print(f"SKIP {vid}: no anchors", file=sys.stderr)
            continue
        out_path = anchor_dir / "selection.json"
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(sel, f, ensure_ascii=False, indent=2)
        selections[vid] = sel["selected_file"]
        print(f"{vid}: anchor_{sel['selected_anchor_num']:02d} (score={sel['candidates'][sel['selected_anchor_num']-1 if False else 0]['total_score']:.3f})", file=sys.stderr)
    # Save global selection index
    global_path = GENVO_ROOT / "Anchors" / "_selections.json"
    with open(global_path, "w", encoding="utf-8") as f:
        json.dump(selections, f, ensure_ascii=False, indent=2)
    print(f"\n{len(selections)} voices selected. Global index: {global_path}", file=sys.stderr)

if __name__ == "__main__":
    main()
