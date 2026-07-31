# Copyright (c) Red Alert 4 project. Map Asset Inventory Generator.
import os
import json
import pathlib

PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
CONTENT_DIR = PROJECT_ROOT / "Content"
REPORT_DIR = PROJECT_ROOT / "Saved" / "Reports"
REPORT_FILE = REPORT_DIR / "MapAssetInventory.json"

def scan_inventory():
    inventory = {
        "World": [],
        "Landscape": [],
        "StaticMesh": [],
        "Material": [],
        "Texture": [],
        "PCG": [],
        "Other": []
    }
    
    total_assets = 0
    
    if not CONTENT_DIR.exists():
        print(f"Content directory not found: {CONTENT_DIR}")
        return

    for root, dirs, files in os.walk(CONTENT_DIR):
        for file in files:
            if file.endswith(".uasset") or file.endswith(".umap"):
                total_assets += 1
                rel_path = os.path.relative_path = os.path.relpath(os.path.join(root, file), PROJECT_ROOT)
                file_stem = pathlib.Path(file).stem
                file_ext = pathlib.Path(file).suffix
                
                # Basic categorization based on naming conventions and subfolders
                game_path = "/Game/" + os.path.relpath(os.path.join(root, file_stem), CONTENT_DIR).replace("\\", "/")
                if file_ext == ".umap" or "Maps" in rel_path or file_stem.startswith("World") or file_stem.startswith("Map"):
                    inventory["World"].append({"name": file_stem, "path": game_path, "type": "WorldMap"})
                elif "Landscape" in file_stem or "Landscape" in rel_path:
                    inventory["Landscape"].append({"name": file_stem, "path": game_path, "type": "LandscapeAsset"})
                elif file_stem.startswith("SM_") or "StaticMesh" in rel_path or "Props" in rel_path or "Blockout" in rel_path or "IndustryProps" in rel_path:
                    inventory["StaticMesh"].append({"name": file_stem, "path": game_path, "type": "StaticMesh"})
                elif file_stem.startswith("M_") or file_stem.startswith("MI_") or "Materials" in rel_path or "Texture" in rel_path:
                    if file_stem.startswith("T_") or "Texture" in rel_path or "ambientCG" in rel_path:
                        inventory["Texture"].append({"name": file_stem, "path": game_path, "type": "Texture"})
                    else:
                        inventory["Material"].append({"name": file_stem, "path": game_path, "type": "Material"})
                elif "PCG" in file_stem or "PCG" in rel_path:
                    inventory["PCG"].append({"name": file_stem, "path": game_path, "type": "PCGGraph"})
                else:
                    inventory["Other"].append({"name": file_stem, "path": game_path, "type": "Asset"})

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    
    summary = {
        "Project": "Red Alert 4",
        "Subsystem": "Map / Environment",
        "ScanPath": str(CONTENT_DIR),
        "TotalAssetCount": total_assets,
        "Categories": {category: len(items) for category, items in inventory.items()},
        "Inventory": inventory
    }
    
    with open(REPORT_FILE, "w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)

    print(f"[Inventory] Saved asset inventory to {REPORT_FILE} (Total assets: {total_assets})")
    for cat, count in summary["Categories"].items():
        print(f"  - {cat}: {count}")

if __name__ == "__main__":
    scan_inventory()
