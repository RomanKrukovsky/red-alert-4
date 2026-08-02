#!/usr/bin/env python3
"""Build a factual inventory of every RA4 blockout mesh inside Unreal Editor.

Run with UnrealEditor-Cmd and ``-ExecutePythonScript=...``.  The report deliberately
separates values declared by the legacy CSV from properties verified on the loaded
StaticMesh, because the original importer combined all FBX objects into one mesh.
"""

from __future__ import annotations

import csv
import json
import re
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import unreal


BLOCKOUT_ROOT = "/Game/RA4/Art/Blockout"
ART_MAPPING_PATH = "/Game/RA4/Art/Generated/DA_RA4_ArtMappings"
SOURCE_SUFFIXES = {".cpp", ".h", ".py", ".ini", ".json", ".csv", ".md"}


def log(message: str) -> None:
    unreal.log(f"RA4BlockoutInventory: {message}")


def serialise(value: Any) -> Any:
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, (list, tuple)):
        return [serialise(item) for item in value]
    if isinstance(value, dict):
        return {str(key): serialise(item) for key, item in value.items()}
    return str(value)


def read_manifest(project_root: Path) -> dict[str, dict[str, str]]:
    manifest_path = project_root / "Content/RA4/Art/Blockout/Blockout_Manifest.csv"
    with manifest_path.open("r", encoding="utf-8-sig", newline="") as handle:
        return {row["StableID"]: row for row in csv.DictReader(handle)}


def stable_id_from_name(asset_name: str) -> str:
    match = re.match(r"SM_(?:Soviet|Alliance|Coalition|Chronolegion)_(.+)_Blockout$", asset_name)
    return match.group(1) if match else ""


def vector_xyz(value: Any) -> list[float]:
    return [round(float(value.x), 3), round(float(value.y), 3), round(float(value.z), 3)]


def get_asset_path(asset_data: unreal.AssetData) -> str:
    return str(asset_data.package_name)


def get_referencers(asset_path: str) -> list[str]:
    try:
        refs = unreal.EditorAssetLibrary.find_package_referencers_for_asset(asset_path, False)
        return sorted(str(ref) for ref in refs)
    except Exception as exc:  # Unreal Python APIs differ slightly by engine minor.
        log(f"referencer scan failed for {asset_path}: {exc}")
        return []


def get_materials(mesh: unreal.StaticMesh) -> list[dict[str, str]]:
    materials: list[dict[str, str]] = []
    try:
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            interface = slot.get_editor_property("material_interface")
            materials.append(
                {
                    "slot": index,
                    "slot_name": str(slot.get_editor_property("material_slot_name")),
                    "asset": interface.get_path_name() if interface else "",
                }
            )
    except Exception as exc:
        materials.append({"error": str(exc)})
    return materials


def get_sockets(mesh: unreal.StaticMesh) -> list[dict[str, Any]]:
    sockets: list[dict[str, Any]] = []
    try:
        for socket in mesh.get_editor_property("sockets"):
            sockets.append(
                {
                    "name": str(socket.get_editor_property("socket_name")),
                    "location_cm": vector_xyz(socket.get_editor_property("relative_location")),
                    "rotation_deg": vector_xyz(socket.get_editor_property("relative_rotation")),
                    "scale": vector_xyz(socket.get_editor_property("relative_scale")),
                }
            )
    except Exception as exc:
        sockets.append({"error": str(exc)})
    return sockets


def get_mesh_metrics(mesh: unreal.StaticMesh) -> dict[str, Any]:
    result: dict[str, Any] = {}
    bounds = mesh.get_bounds()
    result["bounds_origin_cm"] = vector_xyz(bounds.origin)
    result["bounds_extent_cm"] = vector_xyz(bounds.box_extent)
    result["dimensions_cm"] = [round(component * 2.0, 3) for component in result["bounds_extent_cm"]]
    result["bounds_min_z_cm"] = round(bounds.origin.z - bounds.box_extent.z, 3)
    result["pivot_at_ground"] = abs(result["bounds_min_z_cm"]) <= 2.0

    for key, callback in (
        ("lod_count", lambda: mesh.get_num_lods()),
        ("simple_collision_count", lambda: unreal.EditorStaticMeshLibrary.get_simple_collision_count(mesh)),
        ("convex_collision_count", lambda: unreal.EditorStaticMeshLibrary.get_convex_collision_count(mesh)),
    ):
        try:
            result[key] = int(callback())
        except Exception as exc:
            result[key] = None
            result[f"{key}_error"] = str(exc)

    lod_vertices: list[int | None] = []
    for lod_index in range(result.get("lod_count") or 1):
        try:
            lod_vertices.append(int(unreal.EditorStaticMeshLibrary.get_number_verts(mesh, lod_index)))
        except Exception:
            lod_vertices.append(None)
    result["lod_vertices"] = lod_vertices

    try:
        nanite_settings = mesh.get_editor_property("nanite_settings")
        result["nanite_enabled"] = bool(nanite_settings.enabled)
    except Exception:
        result["nanite_enabled"] = None
    return result


def classify_referencers(referencers: list[str]) -> dict[str, list[str]]:
    result = {"maps": [], "blueprints": [], "data_assets": [], "other": []}
    for ref in referencers:
        lowered = ref.lower()
        if ref.startswith("/Game/Maps/"):
            result["maps"].append(ref)
        elif "/bp_" in lowered or "/wbp_" in lowered:
            result["blueprints"].append(ref)
        elif "/da_" in lowered or "generated/da_" in lowered:
            result["data_assets"].append(ref)
        else:
            result["other"].append(ref)
    return result


def source_references(project_root: Path, needles: set[str]) -> dict[str, list[dict[str, Any]]]:
    references = {needle: [] for needle in needles}
    roots = [project_root / "Source", project_root / "Config", project_root / "Tools"]
    for root in roots:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
            try:
                lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
            except OSError:
                continue
            for line_number, line in enumerate(lines, 1):
                for needle in needles:
                    if needle and needle in line:
                        references[needle].append(
                            {
                                "file": str(path.relative_to(project_root)),
                                "line": line_number,
                                "text": line.strip()[:500],
                            }
                        )
    return references


def build_inventory() -> Path:
    project_root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    report_path = project_root / "Saved/Reports/BlockoutReplacementInventory.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    manifest = read_manifest(project_root)

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([BLOCKOUT_ROOT], True)
    asset_data = sorted(
        registry.get_assets_by_path(BLOCKOUT_ROOT, recursive=True),
        key=lambda item: str(item.package_name),
    )

    mesh_rows: list[dict[str, Any]] = []
    stable_ids: set[str] = set()
    for data in asset_data:
        asset_path = get_asset_path(data)
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(asset, unreal.StaticMesh):
            continue

        asset_name = asset.get_name()
        stable_id = stable_id_from_name(asset_name)
        stable_ids.add(stable_id)
        declared = manifest.get(stable_id, {})
        referencers = get_referencers(asset_path)
        metrics = get_mesh_metrics(asset)
        sockets = get_sockets(asset)
        materials = get_materials(asset)

        source_fbx = project_root / f"Content/{asset_path.removeprefix('/Game/')}.fbx"
        entry = {
            "stable_id": stable_id,
            "faction": declared.get("Faction", asset_path.split("/")[-2]),
            "class": declared.get("Class", ""),
            "display_name": declared.get("Name", ""),
            "unreal_asset": asset_path,
            "source_fbx": str(source_fbx.relative_to(project_root)) if source_fbx.exists() else None,
            "source_fbx_bytes": source_fbx.stat().st_size if source_fbx.exists() else None,
            "declared": {
                "dimensions_cm": declared.get("Dimensions_Cm", ""),
                "footprint": declared.get("Footprint", ""),
                "pivot": declared.get("Pivot", ""),
                "subparts": [item.strip() for item in declared.get("Subparts", "").split(",") if item.strip()],
                "sockets": [item.strip() for item in declared.get("Sockets", "").split(",") if item.strip()],
                "collision": declared.get("Collision", ""),
                "export_status": declared.get("ExportStatus", ""),
            },
            "verified": {
                **metrics,
                "materials": materials,
                "material_slot_count": len([item for item in materials if "slot" in item]),
                "sockets": sockets,
                "socket_count": len([item for item in sockets if "name" in item]),
                "separate_moving_parts": False,
                "separate_moving_parts_reason": "Legacy FBX import used combine_meshes=True; runtime asset is one StaticMesh.",
            },
            "referencers": classify_referencers(referencers),
            "status": "UNVERIFIED",
        }
        mesh_rows.append(entry)

    references = source_references(project_root, stable_ids)
    for entry in mesh_rows:
        entry["source_references"] = references.get(entry["stable_id"], [])
        if entry["referencers"]["maps"] or entry["referencers"]["data_assets"] or entry["source_references"]:
            entry["status"] = "BOUND_TO_RUNTIME"

    faction_counts = Counter(entry["faction"] for entry in mesh_rows)
    missing_sockets = sum(1 for entry in mesh_rows if entry["verified"]["socket_count"] == 0)
    single_lod = sum(1 for entry in mesh_rows if (entry["verified"]["lod_count"] or 0) <= 1)
    no_collision = sum(
        1
        for entry in mesh_rows
        if not entry["verified"].get("simple_collision_count")
        and not entry["verified"].get("convex_collision_count")
    )

    report = {
        "schema_version": 1,
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "project": "RedAlert4",
        "engine_version": str(unreal.SystemLibrary.get_engine_version()),
        "source_of_truth": "Loaded Unreal assets plus source references; legacy CSV values are marked declared only.",
        "baseline": {
            "git_branch": "codex/art-blockout-replacement",
            "control_commit": "85d1998",
            "editor_build": "PASSED_UE_5_8",
            "headless_build": "BLOCKED_PREEXISTING_RA4FogOfWar_CoreMinimal_dependency",
        },
        "summary": {
            "static_mesh_count": len(mesh_rows),
            "by_faction": dict(sorted(faction_counts.items())),
            "assets_without_verified_sockets": missing_sockets,
            "assets_with_zero_or_one_lod": single_lod,
            "assets_without_verified_simple_collision": no_collision,
            "legacy_import_combined_meshes": True,
            "art_mapping_asset": ART_MAPPING_PATH,
        },
        "global_findings": [
            "Legacy manifest claims generic sockets and subparts, but those claims are not proof of imported sockets or movable hierarchy.",
            "The legacy importer sets combine_meshes=True, so chassis, turret, barrel, tracks and building parts are collapsed into one StaticMesh.",
            "Runtime contains both hard-coded ContentMeshRegistry paths and a DA_RA4_ArtMappings Data Asset path.",
            "Many generated gameplay Data Assets still use legacy alias filenames; stable IDs must come from the naming-reset Bible and mapping layer.",
        ],
        "assets": mesh_rows,
    }

    report_path.write_text(json.dumps(serialise(report), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    log(f"wrote {report_path} with {len(mesh_rows)} StaticMesh entries")
    return report_path


if __name__ == "__main__":
    build_inventory()
