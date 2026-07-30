#!/usr/bin/env python3
"""
RA4 Bible Parser — reads RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md
and produces a normalized JSON file.

The parser is structural: it walks section headers, summary tables, and unit
cards. It does NOT use fragile single-table regexes.

Output: Content/RA4/Data/Generated/ra4_content.normalized.json
"""
import json
import hashlib
import os
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def parse_table_row(line):
    """Split a markdown table row into cells, stripping pipe edges."""
    parts = line.strip().split("|")
    # Remove leading/trailing empty strings from edge pipes
    if parts and parts[0].strip() == "":
        parts.pop(0)
    if parts and parts[-1].strip() == "":
        parts.pop()
    return [p.strip() for p in parts]


def is_separator_row(line):
    """Check if a line is a markdown table separator (| --- | --- |)."""
    return bool(re.match(r"^\|[\s\-:|]+\|\s*$", line.strip()))


def is_unit_id(s):
    """Check if a string looks like a unit ID (SU_/AL_/CO_/CH_ prefix)."""
    return bool(re.match(r"^(SU_|AL_|CO_|CH_)[A-Za-z0-9_]+$", s.strip()))


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------

class BibleParser:
    def __init__(self, lines):
        self.lines = lines
        self.n = len(lines)
        self.idx = 0

    def current(self):
        return self.lines[self.idx] if self.idx < self.n else ""

    def peek(self, offset=0):
        i = self.idx + offset
        return self.lines[i] if i < self.n else ""

    def advance(self):
        line = self.current()
        self.idx += 1
        return line

    def parse(self):
        result = {
            "schemaVersion": "1.0.0",
            "sourceFile": "RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md",
            "sourceHash": "",
            "generatedAt": "",
            "economy": {},
            "combat": {},
            "factionResources": [],
            "factions": [],
            "units": [],
            "buildings": [],
            "voiceEvents": [],
            "evaLines": [],
            "issues": [],
        }

        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()

            if stripped.startswith("## 1. Глобальная экономика"):
                self.advance()
                result["economy"] = self.parse_economy()
            elif stripped.startswith("## 2. Боевая модель"):
                self.advance()
                result["combat"] = self.parse_combat()
            elif stripped.startswith("## 3. Фракционные ресурсы"):
                self.advance()
                result["factionResources"] = self.parse_faction_resources()
            elif stripped.startswith("# Фракция:"):
                faction = self.parse_faction()
                if faction:
                    result["factions"].append(faction)
            elif stripped == "## 4. Правила озвучки":
                self.advance()
                # Just skip, the rules are informational
            else:
                self.advance()

        return result

    def parse_economy(self):
        eco = {"resources": [], "matchStart": {}, "ore": {}, "construction": [], "power": {}, "commandLimit": [], "techLevels": []}
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if stripped.startswith("# ") or stripped.startswith("## ") and not stripped.startswith("### 1."):
                break
            if stripped.startswith("## ") and not stripped.startswith("### 1."):
                break

            if stripped.startswith("### 1.1."):
                self.advance()
                eco["resources"] = self.parse_generic_table()
            elif stripped.startswith("### 1.2."):
                self.advance()
                eco["matchStart"] = self.parse_kv_table()
            elif stripped.startswith("### 1.3."):
                self.advance()
                eco["ore"] = {"text": self.advance().strip()}
            elif stripped.startswith("### 1.4."):
                self.advance()
                eco["construction"] = self.parse_generic_table()
            elif stripped.startswith("### 1.5."):
                self.advance()
                eco["power"] = {"text": self.advance().strip()}
            elif stripped.startswith("### 1.6."):
                self.advance()
                eco["commandLimit"] = self.parse_generic_table()
            elif stripped.startswith("### 1.7."):
                self.advance()
                eco["techLevels"] = self.parse_generic_table()
            elif stripped.startswith("# Фракция:") or stripped.startswith("## ") and not stripped.startswith("### 1."):
                break
            else:
                self.advance()
        return eco

    def parse_combat(self):
        combat = {"armorTypes": [], "damageMatrix": [], "veterancy": [], "commands": ""}
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if stripped.startswith("# Фракция:") or (stripped.startswith("## ") and not stripped.startswith("### 2.")):
                break
            if stripped.startswith("### 2.1."):
                self.advance()
                combat["armorTypes"] = self.parse_generic_table()
            elif stripped.startswith("### 2.2."):
                self.advance()
                combat["damageMatrix"] = self.parse_damage_matrix()
            elif stripped.startswith("### 2.3."):
                self.advance()
                combat["veterancy"] = self.parse_generic_table()
            elif stripped.startswith("### 2.4."):
                self.advance()
                combat["commands"] = self.advance().strip()
            elif stripped.startswith("## ") and not stripped.startswith("### 2."):
                break
            else:
                self.advance()
        return combat

    def parse_damage_matrix(self):
        """Parse the damage matrix table with armor columns."""
        rows = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped.startswith("###") or stripped.startswith("## ") or stripped.startswith("# "):
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if cells and cells[0] == "Урон":
                self.advance()
                continue
            if len(cells) >= 2:
                rows.append(cells)
            self.advance()
        return rows

    def parse_faction_resources(self):
        rows = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped.startswith("# ") or stripped.startswith("## ") or stripped.startswith("### "):
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if cells and cells[0] == "Фракция":
                self.advance()
                continue
            if len(cells) >= 4:
                rows.append(cells)
            self.advance()
        return rows

    def parse_generic_table(self):
        """Parse a table until a non-table line or new section header."""
        rows = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped:
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if len(cells) >= 1:
                rows.append(cells)
            self.advance()
        return rows

    def parse_kv_table(self):
        """Parse a key-value table (| Параметр | Значение |)."""
        result = {}
        rows = self.parse_generic_table()
        for row in rows:
            if len(row) >= 2 and row[0] != "Параметр":
                result[row[0]] = row[1]
        return result

    def parse_faction(self):
        """Parse a faction section (# Фракция: X)."""
        header = self.advance().strip()
        name = header.replace("# Фракция:", "").strip()

        faction = {
            "name": name,
            "identity": "",
            "factionResource": {},
            "buildings": [],
            "eva": [],
            "unitSummary": [],
            "units": [],
        }

        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()

            if stripped.startswith("# Фракция:") or (stripped.startswith("# ") and "Фракция" not in stripped):
                break
            if stripped.startswith("## Фракционная идентичность"):
                self.advance()
                faction["identity"] = self.advance().strip()
            elif stripped.startswith("## Фракционный ресурс"):
                self.advance()
                faction["factionResource"] = self.parse_faction_resource_detail()
            elif stripped.startswith("## Здания и экономика фракции"):
                self.advance()
                faction["buildings"] = self.parse_buildings_table()
            elif stripped.startswith("## EVA"):
                self.advance()
                faction["eva"] = self.parse_eva_table()
            elif stripped.startswith("## Сводная таблица юнитов"):
                self.advance()
                faction["unitSummary"] = self.parse_unit_summary()
            elif stripped.startswith("## Подробные карточки юнитов"):
                self.advance()
                faction["units"] = self.parse_unit_cards()
            elif stripped.startswith("# "):
                break
            else:
                self.advance()

        return faction

    def parse_faction_resource_detail(self):
        """Parse faction resource description (could be table or text)."""
        result = {}
        line = self.current()
        stripped = line.strip()
        if stripped.startswith("|"):
            rows = self.parse_generic_table()
            for row in rows:
                if len(row) >= 2 and row[0] != "Параметр":
                    result[row[0]] = row[1]
        else:
            result["text"] = self.advance().strip()
        return result

    def parse_buildings_table(self):
        buildings = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped.startswith("## ") or stripped.startswith("# ") or stripped.startswith("### "):
                    break
                if stripped:
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if cells and cells[0] == "Здание":
                self.advance()
                continue
            if len(cells) >= 5:
                buildings.append({
                    "name": cells[0],
                    "cost": cells[1],
                    "buildTime": cells[2],
                    "power": cells[3],
                    "purpose": cells[4],
                })
            self.advance()
        return buildings

    def parse_eva_table(self):
        eva = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped.startswith("## ") or stripped.startswith("# ") or stripped.startswith("### "):
                    break
                if stripped:
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if cells and cells[0] == "Событие":
                self.advance()
                continue
            if len(cells) >= 2:
                eva.append({"event": cells[0], "line": cells[1]})
            self.advance()
        return eva

    def parse_unit_summary(self):
        units = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped.startswith("## ") or stripped.startswith("# ") or stripped.startswith("### "):
                    break
                if stripped:
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if cells and cells[0] == "Юнит":
                self.advance()
                continue
            if len(cells) >= 13:
                units.append({
                    "name": cells[0],
                    "id": cells[1],
                    "category": cells[2],
                    "tier": cells[3],
                    "cost": cells[4],
                    "buildTime": cells[5],
                    "commandLimit": cells[6],
                    "hp": cells[7],
                    "armor": cells[8],
                    "speed": cells[9],
                    "range": cells[10],
                    "dps": cells[11],
                    "role": cells[12],
                })
            self.advance()
        return units

    def parse_unit_cards(self):
        """Parse detailed unit cards (### N. Name (`ID`))."""
        units = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()

            # Next faction or major section
            if stripped.startswith("# Фракция:") or stripped.startswith("# ") and "Фракция" not in stripped:
                break

            # Unit card header: ### N. Name (`ID`)
            match = re.match(r"^### \d+\.\s+.+`((SU_|AL_|CO_|CH_)[A-Za-z0-9_]+)`", stripped)
            if match:
                unit_id = match.group(1)
                self.advance()
                unit = self.parse_single_unit_card(unit_id)
                if unit:
                    units.append(unit)
            else:
                self.advance()

        return units

    def parse_single_unit_card(self, unit_id):
        """Parse one unit card until the next ### header or faction change."""
        unit = {
            "id": unit_id,
            "params": {},
            "abilities": [],
            "balance": {},
            "voice": {},
        }

        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()

            if stripped.startswith("### ") or stripped.startswith("# Фракция:") or stripped.startswith("# "):
                break

            if stripped == "#### Способности":
                self.advance()
                unit["abilities"] = self.parse_abilities()
                continue

            if stripped == "#### Баланс и применение":
                self.advance()
                unit["balance"] = self.parse_balance()
                continue

            if stripped == "#### Озвучка":
                self.advance()
                unit["voice"] = self.parse_voice()
                continue

            # Params table: starts with | Параметр | Значение |
            if stripped.startswith("| Параметр"):
                self.advance()
                while self.idx < self.n:
                    pline = self.current()
                    pstripped = pline.strip()
                    if not pstripped.startswith("|"):
                        break
                    if is_separator_row(pline):
                        self.advance()
                        continue
                    pcells = parse_table_row(pline)
                    if len(pcells) >= 2 and pcells[0] != "Параметр":
                        unit["params"][pcells[0]] = pcells[1]
                    self.advance()
                continue

            self.advance()

        return unit

    def parse_abilities(self):
        """Parse bullet list of abilities."""
        abilities = []
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if stripped.startswith("####") or stripped.startswith("### ") or stripped.startswith("# "):
                break
            if stripped.startswith("- "):
                abilities.append(stripped[2:].strip())
                self.advance()
            elif stripped == "":
                self.advance()
            else:
                break
        return abilities

    def parse_balance(self):
        """Parse balance table (| Аспект | Описание |)."""
        result = {}
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped.startswith("####") or stripped.startswith("### ") or stripped.startswith("# "):
                    break
                if stripped:
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if cells and cells[0] == "Аспект":
                self.advance()
                continue
            if len(cells) >= 2:
                result[cells[0]] = cells[1]
            self.advance()
        return result

    def parse_voice(self):
        """Parse voice table (| Условие | Каноническая реплика |)."""
        result = {}
        while self.idx < self.n:
            line = self.current()
            stripped = line.strip()
            if not stripped.startswith("|"):
                if stripped.startswith("####") or stripped.startswith("### ") or stripped.startswith("# "):
                    break
                if stripped:
                    break
                self.advance()
                continue
            if is_separator_row(line):
                self.advance()
                continue
            cells = parse_table_row(line)
            if cells and cells[0] == "Условие":
                self.advance()
                continue
            if len(cells) >= 2:
                result[cells[0]] = cells[1]
            self.advance()
        return result


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    project_root = Path(__file__).resolve().parent.parent.parent
    bible_path = project_root / "RA4_Factions_Units_Economy_Voice_Bible_v2_Naming_Reset.md"

    if not bible_path.exists():
        print(f"ERROR: Bible not found at {bible_path}", file=sys.stderr)
        sys.exit(1)

    with open(bible_path, "r", encoding="utf-8") as f:
        content = f.read()

    source_hash = hashlib.sha256(content.encode("utf-8")).hexdigest()
    lines = content.split("\n")
    parser = BibleParser(lines)
    result = parser.parse()

    result["sourceHash"] = source_hash
    result["generatedAt"] = ""

    # Collect all units
    all_units = []
    all_buildings = []
    all_eva = []
    all_voice = []
    for faction in result["factions"]:
        for unit in faction.get("units", []):
            unit["faction"] = faction["name"]
            all_units.append(unit)
        for building in faction.get("buildings", []):
            building["faction"] = faction["name"]
            all_buildings.append(building)
        for eva in faction.get("eva", []):
            eva["faction"] = faction["name"]
            all_eva.append(eva)
        for unit in faction.get("units", []):
            for event_name, text in unit.get("voice", {}).items():
                all_voice.append({
                    "faction": faction["name"],
                    "unitId": unit["id"],
                    "event": event_name,
                    "text": text,
                })

    result["units"] = all_units
    result["buildings"] = all_buildings
    result["evaLines"] = all_eva
    result["voiceEvents"] = all_voice

    # Stats
    unit_ids = set()
    for u in all_units:
        unit_ids.add(u["id"])

    stats = {
        "factionCount": len(result["factions"]),
        "unitCount": len(all_units),
        "uniqueUnitIds": len(unit_ids),
        "buildingCount": len(all_buildings),
        "voiceEventCount": len(all_voice),
        "evaCount": len(all_eva),
    }
    result["stats"] = stats

    # Write output
    output_dir = project_root / "Content" / "RA4" / "Data" / "Generated"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / "ra4_content.normalized.json"

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=2)

    print(f"Wrote {output_path}")
    print(f"  Factions: {stats['factionCount']}")
    print(f"  Units: {stats['unitCount']} ({stats['uniqueUnitIds']} unique)")
    print(f"  Buildings: {stats['buildingCount']}")
    print(f"  Voice events: {stats['voiceEventCount']}")
    print(f"  EVA lines: {stats['evaCount']}")
    print(f"  Source SHA-256: {source_hash[:16]}...")

    # Check for issues
    if stats["uniqueUnitIds"] != 78:
        result.setdefault("issues", []).append(
            f"Expected 78 unique unit IDs, got {stats['uniqueUnitIds']}"
        )
    if stats["factionCount"] != 4:
        result.setdefault("issues", []).append(
            f"Expected 4 factions, got {stats['factionCount']}"
        )

    if result.get("issues"):
        issues_path = project_root / "docs" / "content" / "CONTENT_ISSUES.md"
        issues_path.parent.mkdir(parents=True, exist_ok=True)
        with open(issues_path, "w", encoding="utf-8") as f:
            f.write("# Content Issues\n\n")
            for issue in result["issues"]:
                f.write(f"- {issue}\n")
        print(f"Issues written to {issues_path}")


if __name__ == "__main__":
    main()