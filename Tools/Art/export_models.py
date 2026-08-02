#!/usr/bin/env python3
"""Export deterministic FBX files from RA4 editable Blender sources.

Run through Blender:

    blender --background --python Tools/Art/export_models.py -- --all
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import bpy


PROJECT_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = Path(__file__).with_name("model_manifest.json")


def arguments() -> argparse.Namespace:
    raw = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--stable-id", action="append", default=[])
    return parser.parse_args(raw)


def load_manifest() -> dict[str, Any]:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def validate_source(entry: dict[str, Any], asset_name: str) -> None:
    scene = bpy.context.scene
    if scene.get("ra4_stable_id") != entry["stable_id"]:
        raise RuntimeError(f"Stable ID mismatch in source: {entry['stable_id']}")
    if abs(scene.unit_settings.scale_length - 0.01) > 1.0e-6:
        raise RuntimeError(f"{entry['stable_id']} is not authored in centimetres")
    if bpy.data.objects.get(asset_name) is None:
        raise RuntimeError(f"{entry['stable_id']} has no primary {asset_name} object")
    if bpy.data.objects.get(f"UCX_{asset_name}_00") is None:
        raise RuntimeError(f"{entry['stable_id']} has no UCX collision")
    for lod_index in (1, 2, 3):
        if bpy.data.objects.get(f"{asset_name}_LOD{lod_index}") is None:
            raise RuntimeError(f"{entry['stable_id']} is missing LOD{lod_index}")


def export_selection(target: Path) -> None:
    bpy.ops.export_scene.fbx(
        filepath=str(target),
        use_selection=True,
        object_types={"MESH", "EMPTY"},
        global_scale=1.0,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y",
        axis_up="Z",
        use_space_transform=True,
        bake_space_transform=False,
        use_mesh_modifiers=True,
        use_triangles=True,
        mesh_smooth_type="FACE",
        use_tspace=True,
        add_leaf_bones=False,
        bake_anim=False,
        path_mode="AUTO",
        embed_textures=False,
    )


def export(entry: dict[str, Any], source_root: Path, export_root: Path) -> Path:
    source = source_root / entry["faction"] / f"SRC_{entry['faction']}_{entry['stable_id']}.blend"
    if not source.exists():
        raise FileNotFoundError(source)
    bpy.ops.wm.open_mainfile(filepath=str(source))
    asset_name = f"SM_{entry['faction']}_{entry['stable_id']}"
    validate_source(entry, asset_name)

    target_dir = export_root / entry["faction"]
    target_dir.mkdir(parents=True, exist_ok=True)
    target = target_dir / f"{asset_name}.fbx"

    # Unreal's combined-static-mesh importer does not reliably retain FBX socket
    # helpers when a model is assembled from many named pieces.  Keep an exact,
    # deterministic sidecar instead of silently losing the authoring data.
    socket_data = []
    for obj in sorted((item for item in bpy.context.scene.objects if item.type == "EMPTY"), key=lambda item: item.name):
        location = obj.matrix_world.translation
        socket_data.append(
            {
                "name": str(obj.get("ra4_socket_name", obj.name)),
                # FBX -Y/Z export maps Blender Y to Unreal X and Blender X to -Y.
                "location_cm": [round(location.y, 4), round(-location.x, 4), round(location.z, 4)],
                # All generated gameplay sockets point along Unreal +X.  Their
                # positions are authoritative; pitch is animated at runtime.
                "rotation_degrees": [0.0, 0.0, 0.0],
            }
        )
    render_objects = [
        obj
        for obj in bpy.context.scene.objects
        if obj.type == "MESH" and not obj.name.startswith("UCX_") and obj.get("ra4_lod") is None
    ]
    used_materials = {
        slot.material.name
        for obj in render_objects
        for slot in obj.material_slots
        if slot.material is not None
    }
    material_order = [item.name for item in bpy.data.materials if item.name in used_materials]
    (target_dir / f"{asset_name}.sockets.json").write_text(
        json.dumps(
            {
                "stable_id": entry["stable_id"],
                "sockets": socket_data,
                "material_order": material_order,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    # Main FBX contains editable LOD0 parts, UCX collision and Unreal socket
    # helpers. LOD meshes are exported separately so combine_meshes cannot fold
    # all four LODs into LOD0 during import.
    original_empty_names: dict[bpy.types.Object, str] = {}
    bpy.ops.object.select_all(action="DESELECT")
    for obj in bpy.context.scene.objects:
        obj.hide_set(False)
        obj.hide_viewport = False
        is_lod = obj.type == "MESH" and obj.get("ra4_lod") is not None
        if obj.type == "EMPTY":
            original_empty_names[obj] = obj.name
            obj.name = f"SOCKET_{asset_name}_{obj.get('ra4_socket_name', obj.name)}"
            obj.select_set(True)
        elif obj.type == "MESH" and not is_lod:
            obj.select_set(True)
    export_selection(target)
    for obj, original_name in original_empty_names.items():
        obj.name = original_name

    for lod_index in (1, 2, 3):
        bpy.ops.object.select_all(action="DESELECT")
        lod = bpy.data.objects[f"{asset_name}_LOD{lod_index}"]
        original_lod_name = lod.name
        lod.name = f"LOD{lod_index:02d}_{asset_name}"
        lod.select_set(True)
        export_selection(target_dir / f"{asset_name}_LOD{lod_index}.fbx")
        lod.name = original_lod_name

    print(f"EXPORTED {entry['stable_id']} -> {target}")
    return target


def main() -> None:
    args = arguments()
    manifest = load_manifest()
    selected = set(args.stable_id)
    if not args.all and not selected:
        raise SystemExit("Pass --all or at least one --stable-id")
    entries = [entry for entry in manifest["assets"] if args.all or entry["stable_id"] in selected]
    missing = selected - {entry["stable_id"] for entry in entries}
    if missing:
        raise SystemExit(f"Unknown Stable ID(s): {', '.join(sorted(missing))}")
    source_root = PROJECT_ROOT / manifest["source_root"]
    export_root = PROJECT_ROOT / manifest["export_root"]
    for entry in entries:
        export(entry, source_root, export_root)


if __name__ == "__main__":
    main()
