"""QC + manifest writer for voice pipeline."""
import csv
import json
import os
import re
from pathlib import Path

import numpy as np
import soundfile as sf

GENVO_ROOT = Path("/Users/romanmolodyko/Documents/red-alert-4/GeneratedVO")

C_AND_C_QUOTES = [
    r"ядерный удар",
    r"nuclear launch",
    r"construction complete",
    r"строительство завершено",
    r"allied forces",
    r"союзные силы",
    r"for the motherland",
    r"за родину",
]

EVENT_PRIORITY = {
    "Enemy.Superweapon.Launched": 100,
    "Superweapon.Detected": 95,
    "Superweapon.Launched": 95,
    "Superweapon.Ready": 90,
    "Objective.Updated": 90,
    "Objective.Completed": 85,
    "Objective.Failed": 85,
    "Base.UnderAttack": 80,
    "Ally.UnderAttack": 80,
    "Building.Lost": 75,
    "Unit.Lost": 60,
    "Unit.Ready": 50,
    "Multiplayer.DesyncDetected": 70,
    "Multiplayer.ConnectionLost": 65,
    "Radar.Offline": 55,
    "Radar.Online": 35,
    "Power.Low": 45,
    "Power.Restored": 35,
    "Resources.Low": 55,
    "Resources.Exhausted": 75,
    "Stealth.Detected": 50,
    "Enemy.Detected": 50,
    "Reinforcements.Arrived": 50,
    "Building.Captured": 55,
    "Building.Repaired": 30,
    "CriticalDamage": 45,
    "Death": 25,
    "Damaged": 25,
    "Retreat": 35,
    "Veterancy.Gained": 30,
    "DestinationBlocked": 20,
    "CannotComply": 20,
    "Ability.Activate": 25,
    "Attack.Air": 25,
    "Attack.Building": 25,
    "Attack.Naval": 25,
    "Attack": 22,
    "Move.LongDistance": 20,
    "Move": 18,
    "Selected": 30,
    "Spawn": 15,
    "Idle": 10,
    "EnemyDestroyed": 22,
    "Game.Start": 70,
    "Game.Paused": 50,
    "Game.Resumed": 50,
    "Player.Victory": 90,
    "Player.Defeat": 80,
    "Multiplayer.PlayerJoined": 15,
    "Multiplayer.PlayerLeft": 15,
    "Construction.Complete": 35,
    "Construction.Cancelled": 20,
    "Construction.Blocked": 25,
}

def get_priority(event):
    return EVENT_PRIORITY.get(event, 20)

def get_cooldown(event):
    if event in ("CriticalDamage", "Death"):
        return 0
    if event.startswith("EVA.") or event.startswith("Multiplayer.") or event.startswith("Superweapon.") or event.startswith("Objective."):
        return 3
    return 5

def get_concurrency_group(voice_id, faction):
    if voice_id.startswith("EVA_"):
        return f"eva_{faction}"
    return f"unit_{faction}"

def qc_audio(path):
    """Return (ok, dict_metrics)."""
    try:
        audio, sr = sf.read(str(path))
        if audio.ndim > 1:
            return False, {"reason": "not_mono"}
        if sr != 48000:
            return False, {"reason": f"sr={sr}"}
        duration = len(audio) / sr
        peak = float(np.abs(audio).max())
        rms = float(np.sqrt(np.mean(audio.astype(np.float64) ** 2)))
        peak_db = 20 * np.log10(peak + 1e-12)
        rms_db = 20 * np.log10(rms + 1e-12)
        return True, {
            "sample_rate": sr,
            "duration_sec": round(duration, 2),
            "peak_db": round(peak_db, 2),
            "rms_db": round(rms_db, 2),
            "peak_linear": round(peak, 4),
        }
    except Exception as e:
        return False, {"reason": str(e)[:60]}

def check_red_alert_quotes(text):
    for pattern in C_AND_C_QUOTES:
        if re.search(pattern, text, re.IGNORECASE):
            return True
    return False

def scan_files():
    """Walk all voice files + build manifest rows."""
    rows = []
    for subdir in ["EVA", "Units"]:
        base = GENVO_ROOT / subdir
        if not base.exists():
            continue
        for faction_dir in base.iterdir():
            if not faction_dir.is_dir():
                continue
            for wav in sorted(faction_dir.glob("*.wav")):
                ok, m = qc_audio(wav)
                if not ok:
                    print(f"  QC FAIL {wav.name}: {m.get('reason')}")
                    continue
                # Parse filename: VO_<VoiceId>_<EventSafe>_<Variant>.wav
                stem = wav.stem
                m_re = re.match(r"^VO_(EVA_[A-Za-z]+|[A-Za-z]+_[A-Za-z]+)_(.+)_(\d{2})$", stem)
                if not m_re:
                    print(f"  PARSE FAIL {wav.name}")
                    continue
                vid = m_re.group(1)
                event_safe = m_re.group(2)
                variant = int(m_re.group(3))
                event = event_safe.replace("_", ".")
                # Find text from lines
                text = ""
                style_prefix = ""
                lines_path = GENVO_ROOT / "voice_lines.json"
                if lines_path.exists():
                    lines = json.load(open(lines_path, "r", encoding="utf-8"))
                    for ln in lines["lines"]:
                        if ln["voice_id"] == vid and ln["event"] == event and ln["variant"] == variant:
                            text = ln["text"]
                            style_prefix = ln.get("style_prefix", "")
                            break
                red_alert_violation = check_red_alert_quotes(text) if text else False
                # Find anchor
                anchor_file = ""
                sel_path = GENVO_ROOT / "Anchors" / "_selections.json"
                if sel_path.exists():
                    sel = json.load(open(sel_path, "r", encoding="utf-8"))
                    anchor_full = sel.get(vid, "")
                    if anchor_full:
                        anchor_file = str(Path(anchor_full).relative_to(GENVO_ROOT))
                # Find control_instruction from bible
                bible_path = GENVO_ROOT / "voice_bible.json"
                control_instruction = ""
                if bible_path.exists():
                    bible = json.load(open(bible_path, "r", encoding="utf-8"))
                    for v in bible["voices"]:
                        if v["id"] == vid:
                            control_instruction = v["control_instruction"]
                            break
                rows.append({
                    "AssetId": f"LINE_{vid}_{event}_{variant:02d}",
                    "Faction": faction_dir.name,
                    "UnitId": vid if not vid.startswith("EVA_") else "EVA",
                    "VoiceId": vid,
                    "EventTag": event,
                    "Variant": variant,
                    "TextRu": text,
                    "ControlInstruction": control_instruction,
                    "AnchorFile": anchor_file,
                    "OutputFile": str(wav.relative_to(GENVO_ROOT)),
                    "DurationSeconds": m["duration_sec"],
                    "SampleRate": m["sample_rate"],
                    "PeakDb": m["peak_db"],
                    "GenerationMode": "voice_clone_with_anchor",
                    "CfgValue": 2.0,
                    "InferenceTimesteps": 10,
                    "Status": "ok" if not red_alert_violation else "ok_quote_check_needed",
                    "Notes": f"red_alert_quote={red_alert_violation}; style_prefix={style_prefix}",
                    "GenerationTimestamp": "2026-07-28",
                })
    return rows

def write_voice_manifest(rows):
    out = GENVO_ROOT / "Manifests" / "voice_manifest.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        return
    fields = list(rows[0].keys())
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)
    print(f"Wrote {out} with {len(rows)} rows")

def write_unreal_import(rows):
    out = GENVO_ROOT / "Manifests" / "unreal_voice_import.csv"
    out.parent.mkdir(parents=True, exist_ok=True)
    fields = ["SoundWaveName", "SourceFile", "FactionTag", "UnitTag", "EventTag",
              "SubtitleKey", "Priority", "CooldownSeconds", "Weight", "ConcurrencyGroup"]
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        for r in rows:
            sw_name = f"VO_{r['VoiceId']}_{r['EventTag'].replace('.','_')}_{r['Variant']:02d}"
            subtitle = f"{r['VoiceId']}_{r['EventTag'].replace('.','_')}_{r['Variant']:02d}"
            w.writerow({
                "SoundWaveName": sw_name,
                "SourceFile": f"GeneratedVO/{r['OutputFile']}",
                "FactionTag": r["Faction"],
                "UnitTag": r["VoiceId"] if not r["VoiceId"].startswith("EVA_") else "EVA",
                "EventTag": r["EventTag"],
                "SubtitleKey": subtitle,
                "Priority": get_priority(r["EventTag"]),
                "CooldownSeconds": get_cooldown(r["EventTag"]),
                "Weight": 1.0,
                "ConcurrencyGroup": get_concurrency_group(r["VoiceId"], r["Faction"]),
            })
    print(f"Wrote {out}")

def main():
    print("Scanning voice files...")
    rows = scan_files()
    print(f"Found {len(rows)} voice files")
    write_voice_manifest(rows)
    write_unreal_import(rows)
    print(f"\nDONE. {len(rows)} rows in manifest.")

if __name__ == "__main__":
    main()
