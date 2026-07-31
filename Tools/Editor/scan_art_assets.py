#!/usr/bin/env python3
"""
Art Asset Inventory Scanner for Red Alert 4
Scans project Content directory for StaticMesh, SkeletalMesh, Skeleton, AnimationSequence,
AnimBlueprint, Material, Texture, Niagara, SoundWave, SoundCue, MetaSound assets,
and generates Saved/Reports/ArtAssetInventory.json.
"""

import os
import json
import re
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
CONTENT_DIR = PROJECT_ROOT / "Content"
REPORT_PATH = PROJECT_ROOT / "Saved" / "Reports" / "ArtAssetInventory.json"

ASSET_TYPE_RULES = [
    (r"/Maps/.*\.umap$", "Map"),
    (r"M_.*\.uasset$", "Material"),
    (r"MI_.*\.uasset$", "MaterialInstance"),
    (r"T_.*\.uasset$", "Texture"),
    (r"SM_.*\.uasset$", "StaticMesh"),
    (r"SK_.*\.uasset$", "SkeletalMesh"),
    (r"SKEL_.*\.uasset$", "Skeleton"),
    (r"Anim_.*|A_.*|AM_.*\.uasset$", "AnimationSequence"),
    (r"ABP_.*\.uasset$", "AnimBlueprint"),
    (r"NS_.*|FX_.*|NE_.*\.uasset$", "Niagara"),
    (r"SW_.*|S_.*|VO_.*\.uasset$", "SoundWave"),
    (r"SC_.*\.uasset$", "SoundCue"),
    (r"MS_.*\.uasset$", "MetaSound"),
    (r"DA_.*\.uasset$", "DataAsset"),
    (r"WBP_.*\.uasset$", "WidgetBlueprint"),
]

def classify_asset(rel_path_str: str, file_name: str) -> str:
    for pattern, asset_type in ASSET_TYPE_RULES:
        if re.search(pattern, file_name, re.IGNORECASE) or re.search(pattern, rel_path_str, re.IGNORECASE):
            return asset_type
    if "Materials/" in rel_path_str:
        return "Material"
    if "Audio/" in rel_path_str or "VO/" in rel_path_str or "EVA/" in rel_path_str:
        return "SoundWave"
    if "Art/" in rel_path_str or "Meshes/" in rel_path_str:
        return "StaticMesh"
    if "Animation/" in rel_path_str:
        return "AnimationSequence"
    if "VFX/" in rel_path_str:
        return "Niagara"
    return "UnknownAsset"

def scan_inventory():
    inventory = {
        "scan_time": "2026-07-31T16:24:00Z",
        "project": "RedAlert4",
        "total_assets": 0,
        "categories": {
            "StaticMesh": [],
            "SkeletalMesh": [],
            "Skeleton": [],
            "AnimationSequence": [],
            "AnimBlueprint": [],
            "Material": [],
            "MaterialInstance": [],
            "Texture": [],
            "Niagara": [],
            "SoundWave": [],
            "SoundCue": [],
            "MetaSound": [],
            "DataAsset": [],
            "Map": [],
            "Other": []
        },
        "role_coverage": {
            "USSR": {
                "HQ": False,
                "PowerPlant": False,
                "Refinery": False,
                "Barracks": False,
                "WarFactory": False,
                "Turret": False,
                "Harvester": False,
                "Conscript": False,
                "AntiTankInfantry": False,
                "CommissarSupport": False,
                "SickleScout": False,
                "HammerTank": False,
                "FlakAA": False,
                "BuratinoArtillery": False
            },
            "Alliance": {
                "HQ": False,
                "PowerPlant": False,
                "Refinery": False,
                "Barracks": False,
                "WarFactory": False,
                "Turret": False,
                "ProspectorHarvester": False,
                "Peacekeeper": False,
                "JavelinAntiTank": False,
                "MedicSupport": False,
                "JackalScout": False,
                "GuardianTank": False,
                "AegisAA": False,
                "AthenaArtillery": False
            }
        },
        "visual_audit": []
    }

    if not CONTENT_DIR.exists():
        print(f"Content directory not found at {CONTENT_DIR}")
        return

    all_files = list(CONTENT_DIR.glob("**/*"))
    for file_path in all_files:
        if not file_path.is_file() or file_path.suffix not in [".uasset", ".umap"]:
            continue

        rel_path = file_path.relative_to(CONTENT_DIR)
        rel_str = str(rel_path)
        game_path = "/Game/" + rel_str.replace("\\", "/").replace(".uasset", "").replace(".umap", "")
        asset_name = file_path.stem
        asset_type = classify_asset(rel_str, file_path.name)

        entry = {
            "name": asset_name,
            "game_path": game_path,
            "file_size": file_path.stat().st_size,
            "type": asset_type
        }

        if asset_type in inventory["categories"]:
            inventory["categories"][asset_type].append(entry)
        else:
            inventory["categories"]["Other"].append(entry)

        inventory["total_assets"] += 1

    sm_names = [e["name"] for e in inventory["categories"]["StaticMesh"]] + [e["name"] for e in inventory["categories"]["SkeletalMesh"]]
    
    for faction, roles in inventory["role_coverage"].items():
        prefix = "SU_" if faction == "USSR" else "AL_"
        for role_name in roles.keys():
            matching = [name for name in sm_names if prefix in name and role_name.lower() in name.lower()]
            if matching:
                roles[role_name] = True
                inventory["visual_audit"].append({
                    "faction": faction,
                    "role": role_name,
                    "status": "FoundExistingAsset",
                    "matches": matching
                })
            else:
                inventory["visual_audit"].append({
                    "faction": faction,
                    "role": role_name,
                    "status": "MissingOrPlaceholder",
                    "requires_blockout": True
                })

    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(REPORT_PATH, "w", encoding="utf-8") as f:
        json.dump(inventory, f, indent=2, ensure_ascii=False)

    print(f"Inventory saved to {REPORT_PATH} ({inventory['total_assets']} total assets scanned).")

if __name__ == "__main__":
    scan_inventory()
