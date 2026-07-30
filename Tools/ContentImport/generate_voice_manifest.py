#!/usr/bin/env python3
"""Generate voice_manifest.csv from the normalized JSON."""
import json
import os
from pathlib import Path

def main():
    project_root = Path(__file__).resolve().parent.parent.parent
    json_path = project_root / "Content" / "RA4" / "Data" / "Generated" / "ra4_content.normalized.json"
    
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    
    output_dir = project_root / "Content" / "RA4" / "Audio" / "Generated"
    output_dir.mkdir(parents=True, exist_ok=True)
    csv_path = output_dir / "voice_manifest.csv"
    
    with open(csv_path, "w", encoding="utf-8") as f:
        f.write("Faction,UnitId,VoiceId,EventTag,Variant,TextRu,SoundWave,Priority,CooldownSeconds,Weight,Status,SourceLine\n")
        
        for voice in data.get("voiceEvents", []):
            unit_id = voice["unitId"]
            event = voice["event"]
            text = voice["text"]
            faction = voice.get("faction", "")
            
            # Extract faction prefix from unit ID
            fac_prefix = unit_id.split("_")[0] if "_" in unit_id else ""
            fac_name = {
                "SU": "СССР",
                "AL": "Альянс",
                "CO": "Восточная коалиция",
                "CH": "Хронолегион",
            }.get(fac_prefix, faction)
            
            f.write(f'{fac_name},{unit_id},{unit_id},Voice.{event},1,"{text}",,50,3,1,MissingSoundWave,0\n')
    
    print(f"Wrote {csv_path}")
    
    # Count events
    with open(csv_path, "r", encoding="utf-8") as f:
        lines = f.readlines()
    print(f"  Total lines (including header): {len(lines)}")
    print(f"  Voice events: {len(lines) - 1}")

if __name__ == "__main__":
    main()