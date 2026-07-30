#!/usr/bin/env python3
"""
Real RA3 XML & RA4 Content Bible Parser & UE5 Data Table Converter for Red Alert 4.

Supports:
1. Parsing official EA SAGE XML Asset Declarations (GameObject, WeaponTemplate, ArmorTemplate).
2. Direct ingestion of RA4 Content Bible (ra4_content.normalized.json) for 78 unique units & 64 buildings.
3. Exporting UE5 DataTables (DT_Units.json, DT_Weapons.json, DT_ArmorMatrix.json).
"""

import os
import sys
import json
import argparse
import xml.etree.ElementTree as ET
from typing import Dict, Any, List

def clean_tag(tag: str) -> str:
    return tag.split('}')[-1] if '}' in tag else tag

def parse_weapon_template(elem: ET.Element) -> Dict[str, Any]:
    weapon_id = elem.get('id', '')
    name = elem.get('Name', weapon_id)
    damage = 0
    warhead = "Ballistic"
    min_range = float(elem.get('MinTargetRange', '0.0') or 0.0)
    max_range = float(elem.get('AttackRange', '500.0') or 500.0)
    cooldown = 20
    
    for child in elem.iter():
        tag = clean_tag(child.tag)
        if tag == 'DamageNugget':
            damage = int(float(child.get('Damage', '0') or 0))
            warhead = child.get('DamageType', warhead)
        elif tag == 'FiringDuration':
            cooldown = int(float(child.text or child.get('Value', '20') or 20))
            
    return {
        "id": weapon_id,
        "name": name,
        "damage": damage,
        "warhead": warhead,
        "min_range": min_range,
        "max_range": max_range,
        "cooldown_ticks": cooldown
    }

def parse_armor_template(elem: ET.Element) -> Dict[str, Any]:
    armor_id = elem.get('id', '')
    coefficients: Dict[str, float] = {}
    
    for child in elem.iter():
        tag = clean_tag(child.tag)
        if tag == 'Armor':
            damage_type = child.get('DamageType', '')
            percent_str = child.get('Percent', '100').replace('%', '').strip()
            try:
                percent = float(percent_str) / 100.0
            except ValueError:
                percent = 1.0
            if damage_type:
                coefficients[damage_type] = percent
                
    return {
        "id": armor_id,
        "coefficients": coefficients
    }

def parse_game_object(elem: ET.Element) -> Dict[str, Any]:
    object_id = elem.get('id', '')
    name = elem.get('Name', object_id)
    side = elem.get('Side', 'Neutral')
    kind_of = elem.get('KindOf', '')
    
    kind = "Unit"
    if "STRUCTURE" in kind_of:
        kind = "Building"
    elif "INFANTRY" in kind_of:
        kind = "Infantry"
    elif "VEHICLE" in kind_of:
        kind = "Vehicle"
    elif "AIRCRAFT" in kind_of:
        kind = "Aircraft"

    max_health = 100
    armor = "LightInfantry"
    weapon = ""
    cost = int(elem.get('BuildCost', '0') or 0)
    build_time = int(elem.get('BuildTime', '0') or 0)
    
    for child in elem.iter():
        tag = clean_tag(child.tag)
        if tag in ('ActiveBody', 'RespawnBody', 'HierarchyBody', 'HighPriorityBody'):
            try:
                max_health = int(float(child.get('MaxHealth', '100')))
            except ValueError:
                pass
        elif tag == 'ArmorSet':
            armor = child.get('Armor', armor)
        elif tag in ('Weapon', 'WeaponSlotHardpoint', 'WeaponSlotTurret'):
            w_name = child.get('Weapon', child.get('Template', ''))
            if w_name and not weapon:
                weapon = w_name
        elif tag == 'GameData':
            try:
                cost = int(float(child.get('BuildCost', str(cost))))
                build_time = int(float(child.get('BuildTime', str(build_time))))
            except ValueError:
                pass

    return {
        "id": object_id,
        "name": name,
        "faction": side,
        "kind": kind,
        "kind_of": kind_of,
        "max_health": max_health,
        "armor": armor,
        "weapon": weapon,
        "cost": cost,
        "build_time_ticks": build_time
    }

def parse_xml_file(filepath: str, data: Dict[str, Any]):
    try:
        tree = ET.parse(filepath)
        root = tree.getroot()
        for elem in root.iter():
            tag = clean_tag(elem.tag)
            if tag == 'GameObject':
                data["units"].append(parse_game_object(elem))
            elif tag == 'WeaponTemplate':
                data["weapons"].append(parse_weapon_template(elem))
            elif tag == 'ArmorTemplate':
                arm = parse_armor_template(elem)
                if arm["id"]:
                    data["armor_matrix"][arm["id"]] = arm["coefficients"]
    except Exception as e:
        pass

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
            
        print(f"[RA3 Parser] Ingested normalized Bible JSON: {len(units_list)} units, {len(buildings_list)} buildings.")
    except Exception as e:
        print(f"[RA3 Parser] Error reading normalized JSON: {e}")

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
    parser = argparse.ArgumentParser(description="Real RA3 XML & Content Bible Parser for Red Alert 4")
    parser.add_argument("--input-dir", type=str, help="Directory containing RA3 Modding XML files")
    parser.add_argument("--bible-json", type=str, default="Content/RA4/Data/Generated/ra4_content.normalized.json", help="Path to normalized Bible JSON")
    parser.add_argument("--output-dir", type=str, default="Tools/ContentImport/DataTables", help="Directory for UE5 DataTables")
    args = parser.parse_args()

    data = {"units": [], "weapons": [], "armor_matrix": {}}

    # 1. Parse XML files if provided
    if args.input_dir and os.path.exists(args.input_dir):
        print(f"[RA3 Parser] Parsing XML files in {args.input_dir}...")
        file_count = 0
        for root_path, _, files in os.walk(args.input_dir):
            for file in files:
                if file.endswith('.xml') or file.endswith('.xsd'):
                    file_count += 1
                    parse_xml_file(os.path.join(root_path, file), data)
        print(f"[RA3 Parser] Processed {file_count} XML files.")

    # 2. Ingest normalized Bible JSON
    if os.path.exists(args.bible_json):
        ingest_normalized_json(args.bible_json, data)

    # 3. Save intermediate JSON
    os.makedirs(args.output_dir, exist_ok=True)
    inter_path = os.path.join(args.output_dir, "intermediate_ra3_data.json")
    with open(inter_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"[RA3 Parser] Saved intermediate data to {inter_path}")

    # 4. Generate UE5 DataTables
    convert_to_ue5_datatables(data, args.output_dir)
    print(f"[RA3 Parser] Generated UE5 DataTables in {args.output_dir}")

if __name__ == "__main__":
    main()
