#!/usr/bin/env python3
"""Import generated RA4 models, collision, sockets and LODs into Unreal Editor."""

from __future__ import annotations

import json
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
MANIFEST_PATH = PROJECT_ROOT / "Tools/Art/model_manifest.json"


def selected_entries(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(r"(?:-|/)RA4StableId=([^\s]+)", command_line)
    if not match:
        return manifest["assets"]
    selected = {item.strip() for item in match.group(1).split(",") if item.strip()}
    return [entry for entry in manifest["assets"] if entry["stable_id"] in selected]


def make_import_options() -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_as_skeletal", False)
    data = options.static_mesh_import_data
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("generate_lightmap_u_vs", True)
    data.set_editor_property("auto_generate_collision", False)
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("convert_scene_unit", True)
    return options


def socket_names(mesh: unreal.StaticMesh, kind: str) -> list[str]:
    expected = (
        ["VFX_Impact", "UnitExit", "Rally", "SelectionOrigin", "Exhaust"]
        if kind == "building"
        else [
            "Turret",
            "Muzzle",
            "ProjectileSpawn",
            "VFX_Impact",
            "Engine",
            "Exhaust",
            "SelectionOrigin",
            "Socket_Turret",
            "Socket_Muzzle",
            "Socket_Engine",
        ]
    )
    result: list[str] = []
    for name in expected:
        try:
            if mesh.find_socket(name) is not None:
                result.append(name)
        except Exception:
            pass
    return sorted(result)


def dimensions(mesh: unreal.StaticMesh) -> list[float]:
    extent = mesh.get_bounds().box_extent
    return [round(extent.x * 2.0, 3), round(extent.y * 2.0, 3), round(extent.z * 2.0, 3)]


def dimensions_match(expected: list[float], actual: list[float]) -> bool:
    # Render bounds include gun barrels, exhausts and sensors, so requiring the
    # decorative silhouette to equal the gameplay footprint is incorrect.  The
    # long axis must stay close and the remaining silhouette must remain within
    # a guarded envelope. Runtime SetVisualScale still normalizes to the exact
    # simulation footprint.
    expected_ground = sorted(float(value) for value in expected[:2])
    actual_ground = sorted(float(value) for value in actual[:2])
    long_axis_ok = abs(actual_ground[1] - expected_ground[1]) <= max(5.0, expected_ground[1] * 0.10)
    short_axis_ok = expected_ground[0] * 0.65 <= actual_ground[0] <= expected_ground[0] * 1.40
    height_ok = expected[2] * 0.70 <= actual[2] <= expected[2] * 1.35
    return long_axis_ok and short_axis_ok and height_ok


def install_collision(mesh: unreal.StaticMesh) -> None:
    unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
    unreal.EditorStaticMeshLibrary.add_simple_collisions(
        mesh,
        unreal.ScriptingCollisionShapeType.BOX,
    )


def install_sockets(mesh: unreal.StaticMesh, sidecar_path: Path) -> list[str]:
    if not sidecar_path.exists():
        return []
    data = json.loads(sidecar_path.read_text(encoding="utf-8"))
    installed = []
    for definition in data["sockets"]:
        existing = mesh.find_socket(definition["name"])
        if existing is not None:
            mesh.remove_socket(existing)
        socket = unreal.new_object(unreal.StaticMeshSocket, outer=mesh)
        socket.set_editor_property("socket_name", definition["name"])
        socket.set_editor_property("relative_location", unreal.Vector(*definition["location_cm"]))
        pitch, yaw, roll = definition["rotation_degrees"]
        socket.set_editor_property("relative_rotation", unreal.Rotator(roll, pitch, yaw))
        mesh.add_socket(socket)
        installed.append(definition["name"])
    return sorted(installed)


def install_materials(mesh: unreal.StaticMesh, faction: str, sidecar_path: Path) -> tuple[list[str], list[str]]:
    if not sidecar_path.exists():
        return [], []
    definitions = json.loads(sidecar_path.read_text(encoding="utf-8"))
    slot_names = definitions.get("material_order", [])
    assigned = []
    for index, slot in enumerate(slot_names):
        if mesh.get_material(index) is None:
            break
        if "Rubber" in slot:
            material_name = "MI_RA4_Surface_Dark"
        elif "Concrete" in slot:
            material_name = "MI_RA4_Surface_Concrete"
        elif "Glass" in slot:
            material_name = "MI_RA4_Surface_Glass"
        elif "Emissive" in slot:
            material_name = f"MI_RA4_Emissive_{faction}"
        else:
            material_name = f"MI_RA4_Surface_{faction}"
        material = unreal.load_asset(f"/Game/RA4/Art/Materials/{material_name}")
        if isinstance(material, unreal.MaterialInterface):
            mesh.set_material(index, material)
            assigned.append(material_name)
    return slot_names[: len(assigned)], assigned


def import_entry(entry: dict[str, Any]) -> dict[str, Any]:
    asset_name = f"SM_{entry['faction']}_{entry['stable_id']}"
    source_dir = PROJECT_ROOT / "ArtSource/RA4/Exports" / entry["faction"]
    source_fbx = source_dir / f"{asset_name}.fbx"
    if not source_fbx.exists():
        return {"stable_id": entry["stable_id"], "status": "BLOCKED", "reason": f"missing {source_fbx}"}

    destination_path, destination_name = entry["target"].rsplit("/", 1)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source_fbx))
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", make_import_options())
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    mesh = unreal.EditorAssetLibrary.load_asset(entry["target"])
    if not isinstance(mesh, unreal.StaticMesh):
        return {"stable_id": entry["stable_id"], "status": "BLOCKED", "reason": "StaticMesh import failed"}

    lod_results = []
    for lod_index in (1, 2, 3):
        lod_file = source_dir / f"{asset_name}_LOD{lod_index}.fbx"
        try:
            imported_index = unreal.EditorStaticMeshLibrary.import_lod(mesh, lod_index, str(lod_file))
            lod_results.append({"lod": lod_index, "result": int(imported_index)})
        except Exception as exc:
            lod_results.append({"lod": lod_index, "error": str(exc)})

    install_collision(mesh)
    sidecar_path = source_dir / f"{asset_name}.sockets.json"
    installed_sockets = install_sockets(mesh, sidecar_path)
    material_slots, installed_materials = install_materials(mesh, entry["faction"], sidecar_path)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    actual_dimensions = dimensions(mesh)
    sockets = socket_names(mesh, entry["kind"])
    convex_collision_count = int(unreal.EditorStaticMeshLibrary.get_convex_collision_count(mesh))
    simple_collision_count = int(unreal.EditorStaticMeshLibrary.get_simple_collision_count(mesh))
    lod_count = int(mesh.get_num_lods())
    valid = dimensions_match(entry["dimensions_cm"], actual_dimensions)
    status = "IMPORTED" if valid and simple_collision_count > 0 and lod_count >= 4 else "BLOCKED"
    return {
        "stable_id": entry["stable_id"],
        "status": status,
        "target": entry["target"],
        "expected_dimensions_cm": entry["dimensions_cm"],
        "actual_dimensions_cm": actual_dimensions,
        "dimensions_match": valid,
        "lod_count": lod_count,
        "lod_imports": lod_results,
        "simple_collision_count": simple_collision_count,
        "convex_collision_count": convex_collision_count,
        "sockets": sockets,
        "socket_count": len(sockets),
        "installed_sockets": installed_sockets,
        "material_slots": material_slots,
        "installed_materials": installed_materials,
    }


def main() -> None:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    entries = selected_entries(manifest)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Game/RA4/Art"], True)
    results = []
    for entry in entries:
        unreal.log(f"RA4ModelImport: importing {entry['stable_id']}")
        results.append(import_entry(entry))

    report = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "engine_version": str(unreal.SystemLibrary.get_engine_version()),
        "results": results,
        "summary": {
            "requested": len(results),
            "imported": sum(result["status"] == "IMPORTED" for result in results),
            "blocked": sum(result["status"] == "BLOCKED" for result in results),
        },
    }
    report_path = PROJECT_ROOT / "Saved/Reports/ModelImportReport.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    unreal.log(f"RA4ModelImport: report {report_path} {report['summary']}")


if __name__ == "__main__":
    main()
