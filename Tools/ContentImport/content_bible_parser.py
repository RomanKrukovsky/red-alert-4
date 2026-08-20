#!/usr/bin/env python3
"""
RA4 Content Bible parser and UE5 Data Table converter.

Reads Content/RA4/Data/Generated/ra4_content.normalized.json -- this project's own
content -- and exports DT_Units.json, DT_Weapons.json and DT_ArmorMatrix.json.

The SAGE XML ingestion path was REMOVED on 2026-08-06 along with the 123 third-party
.xsd schemas it read (Tools/ContentImport/RA3_XML_Source, a vendor asset
namespace) and the two scripts that downloaded them. Those files were
third-party material with no licence recorded, which CLAUDE.md forbids and ADR-011
(Clean-Room Compliance) exists to block -- the compliance scanner flagged 128
vendor identifiers across them the first time it was actually run.

The generated data tables are unaffected: they are produced from the bible, whose
unit names are this project's own (SU_RubezhRifleman and so on), not EA's.
"""

import os
import sys
import json
import argparse
from typing import Dict, Any, List


def ingest_normalized_json(json_path: str, data: Dict[str, Any]):
    if not os.path.exists(json_path):
        return
    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            norm = json.load(f)
            
        units_list = norm.get("units", [])
        for u in units_list:
            data["units"].append({
                "id": u.get("id", "Unknown"),
                "name": u.get("nameKey", u.get("id")),
                "faction": u.get("faction", "Neutral"),
                "kind": u.get("kind", "Unit"),
                "max_health": u.get("health", 100),
                "armor": u.get("armor", "LightInfantry"),
                "weapon": u.get("weaponPrimary", ""),
                "cost": u.get("cost", 100),
                "build_time_ticks": u.get("buildTimeTicks", 40)
            })
            
        buildings_list = norm.get("buildings", [])
        for b in buildings_list:
            data["units"].append({
                "id": b.get("id", "Unknown"),
                "name": b.get("nameKey", b.get("id")),
                "faction": b.get("faction", "Neutral"),
                "kind": "Building",
                "max_health": b.get("health", 500),
                "armor": "Building",
                "weapon": b.get("weaponPrimary", ""),
                "cost": b.get("cost", 500),
                "build_time_ticks": b.get("buildTimeTicks", 100)
            })
            
        print(f"[Content Parser] Ingested normalized Bible JSON: {len(units_list)} units, {len(buildings_list)} buildings.")
    except Exception as e:
        print(f"[Content Parser] Error reading normalized JSON: {e}")

def convert_to_ue5_datatables(data: Dict[str, Any], output_dir: str):
    os.makedirs(output_dir, exist_ok=True)
    
    # DT_Units
    dt_units = []
    seen = set()
    for u in data.get("units", []):
        uid = u["id"]
        if uid in seen:
            continue
        seen.add(uid)
        dt_units.append({
            "Name": uid,
            "DisplayNameKey": u.get("name", uid),
            "Faction": u.get("faction", "Neutral"),
            "Kind": u.get("kind", "Unit"),
            "MaxHealth": u.get("max_health", 100),
            "ArmorClass": u.get("armor", "LightInfantry"),
            "WeaponId": u.get("weapon", ""),
            "Cost": u.get("cost", 0),
            "BuildTimeTicks": u.get("build_time_ticks", 0)
        })
    with open(os.path.join(output_dir, "DT_Units.json"), "w", encoding="utf-8") as f:
        json.dump(dt_units, f, indent=2)

    # DT_Weapons
    dt_weapons = []
    w_seen = set()
    for w in data.get("weapons", []):
        wid = w["id"]
        if wid in w_seen:
            continue
        w_seen.add(wid)
        dt_weapons.append({
            "Name": wid,
            "Damage": w.get("damage", 0),
            "WarheadClass": w.get("warhead", "Ballistic"),
            "MinRange": w.get("min_range", 0.0),
            "MaxRange": w.get("max_range", 500.0),
            "CooldownTicks": w.get("cooldown_ticks", 20)
        })
    with open(os.path.join(output_dir, "DT_Weapons.json"), "w", encoding="utf-8") as f:
        json.dump(dt_weapons, f, indent=2)

    # DT_ArmorMatrix
    dt_armor = []
    matrix = data.get("armor_matrix", {})
    for armor_type, warheads in matrix.items():
        row = {"Name": armor_type}
        row.update(warheads)
        dt_armor.append(row)
    with open(os.path.join(output_dir, "DT_ArmorMatrix.json"), "w", encoding="utf-8") as f:
        json.dump(dt_armor, f, indent=2)

def main():
    parser = argparse.ArgumentParser(description="RA4 Content Bible parser and UE5 DataTable converter")
    parser.add_argument("--bible-json", type=str, default="Content/RA4/Data/Generated/ra4_content.normalized.json", help="Path to normalized Bible JSON")
    parser.add_argument("--output-dir", type=str, default="Tools/ContentImport/DataTables", help="Directory for UE5 DataTables")
    args = parser.parse_args()

    data = {"units": [], "weapons": [], "armor_matrix": {}}

    # 1. Ingest normalized Bible JSON
    if os.path.exists(args.bible_json):
        ingest_normalized_json(args.bible_json, data)

    # 2. Save intermediate JSON
    os.makedirs(args.output_dir, exist_ok=True)
    inter_path = os.path.join(args.output_dir, "intermediate_content_data.json")
    with open(inter_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"[Content Parser] Saved intermediate data to {inter_path}")

    # 3. Generate UE5 DataTables
    convert_to_ue5_datatables(data, args.output_dir)
    print(f"[Content Parser] Generated UE5 DataTables in {args.output_dir}")

if __name__ == "__main__":
    main()
