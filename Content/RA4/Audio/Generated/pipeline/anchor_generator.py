"""Anchor generator: 3 anchors per VoiceId via VoxCPM2 Voice Design (control only).

Strategy: load VoxCPM model ONCE, generate all anchors sequentially in-process.
Voice Design mode = (control instruction) prefix in text + reference_wav_path=None.

Output: GeneratedVO/Anchors/<Faction>/<VoiceId>/anchor_<01|02|03>.wav
"""
import json
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from state import load_state, mark_completed

VOXCPM_MODEL_ID = "openbmb/VoxCPM2"
GENVO_ROOT = Path("/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO")
BIBLE_PATH = GENVO_ROOT / "voice_bible.json"

def load_bible():
    with open(BIBLE_PATH, "r", encoding="utf-8") as f:
        return json.load(f)

def generate_anchor(model, control_instruction, text, output_path, cfg=2.0, steps=10):
    """Generate one anchor using Voice Design mode (control only, no reference)."""
    import soundfile as sf
    full_text = f"({control_instruction}){text}"
    audio = model.generate(
        text=full_text,
        cfg_value=cfg,
        inference_timesteps=steps,
        reference_wav_path=None,
        normalize=True,
    )
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    sf.write(str(output_path), audio, model.tts_model.sample_rate)
    return audio, model.tts_model.sample_rate

def main():
    bible = load_bible()
    state = load_state()
    state["phase"] = "anchors"
    from voxcpm import VoxCPM
    print(f"Loading VoxCPM model ({VOXCPM_MODEL_ID})...", file=sys.stderr)
    t0 = time.time()
    model = VoxCPM.from_pretrained(VOXCPM_MODEL_ID, load_denoiser=False)
    print(f"Loaded in {time.time()-t0:.1f}s", file=sys.stderr)

    results = []
    total = len(bible["voices"]) * 3  # 3 anchors per voice
    counter = 0
    for voice in bible["voices"]:
        vid = voice["id"]
        faction = voice["faction"]
        for i, text in enumerate(voice["sample_texts"], start=1):
            counter += 1
            anchor_id = f"{vid}_anchor_{i:02d}"
            asset_id = f"ANCHOR_{vid}_{i:02d}"
            out_path = GENVO_ROOT / "Anchors" / faction / vid / f"anchor_{i:02d}.wav"
            if out_path.exists():
                print(f"  [{counter}/{total}] SKIP {asset_id} (exists)", file=sys.stderr)
                results.append({"voice_id": vid, "anchor_num": i, "file": str(out_path), "skipped": True})
                continue
            t0 = time.time()
            try:
                audio, sr = generate_anchor(
                    model,
                    voice["control_instruction"],
                    text,
                    out_path,
                )
                dur = len(audio) / sr
                results.append({
                    "voice_id": vid,
                    "anchor_num": i,
                    "file": str(out_path),
                    "duration_sec": round(dur, 2),
                    "sample_rate": sr,
                })
                mark_completed(state, asset_id)
                print(f"  [{counter}/{total}] OK {asset_id} ({dur:.2f}s, {time.time()-t0:.1f}s gen)", file=sys.stderr)
            except Exception as e:
                print(f"  [{counter}/{total}] FAIL {asset_id}: {e}", file=sys.stderr)
                results.append({"voice_id": vid, "anchor_num": i, "error": str(e)})

    summary_path = GENVO_ROOT / "Anchors" / "_generation_summary.json"
    with open(summary_path, "w", encoding="utf-8") as f:
        json.dump({"total": total, "results": results}, f, ensure_ascii=False, indent=2)
    print(f"\nDone. Summary: {summary_path}", file=sys.stderr)
    ok = sum(1 for r in results if "duration_sec" in r or r.get("skipped"))
    fail = sum(1 for r in results if "error" in r)
    print(f"OK: {ok}, FAIL: {fail}", file=sys.stderr)

if __name__ == "__main__":
    main()
