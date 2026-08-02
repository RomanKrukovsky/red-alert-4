#!/usr/bin/env python3
"""Generate the first RA4 art vertical slice as deterministic Blender sources.

The output is intentionally labelled GENERATED_DRAFT.  It is a polished,
editable pre-production replacement, not a claim that procedural geometry alone
is final art.  Run through Blender, for example:

    blender --background --python Tools/Art/generate_models.py -- --all
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

import bpy


PROJECT_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = Path(__file__).with_name("model_manifest.json")

FACTION_STYLE = {
    "Soviet": {
        "metal": (0.11, 0.13, 0.12, 1.0),
        "paint": (0.38, 0.035, 0.025, 1.0),
        "accent": (0.95, 0.42, 0.03, 1.0),
        "emissive": (1.0, 0.12, 0.02, 1.0),
    },
    "Alliance": {
        "metal": (0.14, 0.18, 0.22, 1.0),
        "paint": (0.035, 0.18, 0.52, 1.0),
        "accent": (0.12, 0.72, 1.0, 1.0),
        "emissive": (0.08, 0.6, 1.0, 1.0),
    },
    "Coalition": {
        "metal": (0.10, 0.15, 0.13, 1.0),
        "paint": (0.02, 0.38, 0.25, 1.0),
        "accent": (0.95, 0.62, 0.04, 1.0),
        "emissive": (0.2, 1.0, 0.55, 1.0),
    },
    "Chronolegion": {
        "metal": (0.09, 0.075, 0.15, 1.0),
        "paint": (0.31, 0.06, 0.52, 1.0),
        "accent": (0.0, 0.82, 0.78, 1.0),
        "emissive": (0.2, 1.0, 0.94, 1.0),
    },
}


def arguments() -> argparse.Namespace:
    raw = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--stable-id", action="append", default=[])
    return parser.parse_args(raw)


def load_manifest() -> dict[str, Any]:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def reset_scene() -> None:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "CENTIMETERS"
    scene.unit_settings.scale_length = 0.01
    # Geometry output does not depend on the renderer. Blender 5.1 exposes Eevee
    # under the BLENDER_EEVEE identifier.
    scene.render.engine = "BLENDER_EEVEE"


def material(name: str, color: tuple[float, float, float, float], *, metallic: float, roughness: float, emission: float = 0.0) -> bpy.types.Material:
    result = bpy.data.materials.new(name)
    result.diffuse_color = color
    result.use_nodes = True
    principled = result.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = color
    principled.inputs["Metallic"].default_value = metallic
    principled.inputs["Roughness"].default_value = roughness
    if emission > 0.0:
        principled.inputs["Emission Color"].default_value = color
        principled.inputs["Emission Strength"].default_value = emission
    result["ra4_master_material"] = name
    return result


def make_materials(faction: str) -> dict[str, bpy.types.Material]:
    style = FACTION_STYLE[faction]
    return {
        "metal": material("MI_RA4_Metal", style["metal"], metallic=0.85, roughness=0.36),
        "paint": material(f"MI_RA4_Paint_{faction}", style["paint"], metallic=0.48, roughness=0.28),
        "accent": material(f"MI_RA4_Accent_{faction}", style["accent"], metallic=0.35, roughness=0.24),
        "dark": material("MI_RA4_RubberTrack", (0.018, 0.022, 0.025, 1.0), metallic=0.05, roughness=0.72),
        "glass": material("MI_RA4_ArmouredGlass", (0.025, 0.12, 0.16, 1.0), metallic=0.22, roughness=0.12),
        "emissive": material(f"MI_RA4_Emissive_{faction}", style["emissive"], metallic=0.1, roughness=0.18, emission=4.0),
        "concrete": material("MI_RA4_Concrete", (0.17, 0.18, 0.18, 1.0), metallic=0.0, roughness=0.82),
    }


def assign(obj: bpy.types.Object, mat: bpy.types.Material) -> None:
    if obj.type == "MESH":
        obj.data.materials.append(mat)


def finish_mesh(obj: bpy.types.Object, bevel: float = 4.0, segments: int = 2) -> bpy.types.Object:
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel > 0.0:
        modifier = obj.modifiers.new("ProductionBevel", "BEVEL")
        modifier.width = bevel
        modifier.segments = segments
        modifier.limit_method = "ANGLE"
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    for polygon in obj.data.polygons:
        polygon.use_smooth = False
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    try:
        bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=0.02)
    except RuntimeError:
        pass
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.select_set(False)
    return obj


def box(name: str, location: tuple[float, float, float], dimensions: tuple[float, float, float], mat: bpy.types.Material, bevel: float = 4.0, rotation_z: float = 0.0) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=(0.0, 0.0, rotation_z))
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    assign(obj, mat)
    return finish_mesh(obj, bevel=min(bevel, min(dimensions) * 0.18))


def cylinder(name: str, location: tuple[float, float, float], radius: float, depth: float, mat: bpy.types.Material, vertices: int = 16, rotation: tuple[float, float, float] = (0.0, 0.0, 0.0), bevel: float = 3.0) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    assign(obj, mat)
    return finish_mesh(obj, bevel=min(bevel, radius * 0.16))


def sphere(name: str, location: tuple[float, float, float], scale: tuple[float, float, float], mat: bpy.types.Material, segments: int = 20) -> bpy.types.Object:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=max(8, segments // 2), location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    assign(obj, mat)
    return finish_mesh(obj, bevel=0.0)


def torus(name: str, location: tuple[float, float, float], major_radius: float, minor_radius: float, mat: bpy.types.Material, rotation: tuple[float, float, float] = (0.0, 0.0, 0.0)) -> bpy.types.Object:
    bpy.ops.mesh.primitive_torus_add(major_radius=major_radius, minor_radius=minor_radius, major_segments=24, minor_segments=8, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    assign(obj, mat)
    return finish_mesh(obj, bevel=0.0)


def empty_socket(name: str, location: tuple[float, float, float], rotation: tuple[float, float, float] = (0.0, 0.0, 0.0)) -> bpy.types.Object:
    obj = bpy.data.objects.new(name, None)
    bpy.context.scene.collection.objects.link(obj)
    obj.empty_display_type = "ARROWS"
    obj.empty_display_size = 24.0
    obj.location = location
    obj.rotation_euler = rotation
    obj["ra4_socket_name"] = name
    return obj


def create_collision(asset_name: str, dimensions: list[float], kind: str) -> bpy.types.Object:
    width, length, height = dimensions
    collision_height = height * (0.72 if kind == "vehicle" else 0.9)
    obj = box(
        f"UCX_{asset_name}_00",
        (0.0, 0.0, collision_height * 0.5),
        (width * 0.88, length * 0.88, collision_height),
        bpy.data.materials["MI_RA4_Concrete"],
        bevel=2.0,
    )
    obj.display_type = "WIRE"
    obj.hide_render = True
    obj["ra4_collision"] = True
    return obj


def add_building(entry: dict[str, Any], asset_name: str, mats: dict[str, bpy.types.Material]) -> None:
    width, length, height = entry["dimensions_cm"]
    faction = entry["faction"]
    role = entry["role"]

    box(asset_name, (0, 0, 18), (width, length, 36), mats["concrete"], bevel=5)
    box(f"{asset_name}_FoundationTrim", (0, 0, 42), (width * 0.94, length * 0.94, 22), mats["paint"], bevel=5)

    if faction == "Soviet":
        box(f"{asset_name}_MainMass", (0, 25, height * 0.38), (width * 0.72, length * 0.66, height * 0.62), mats["metal"], bevel=12)
        box(f"{asset_name}_ArmourBrow", (0, -length * 0.24, height * 0.55), (width * 0.82, length * 0.14, height * 0.18), mats["paint"], bevel=7)
        for side in (-1, 1):
            cylinder(f"{asset_name}_Exhaust_{side:+d}", (side * width * 0.31, length * 0.18, height * 0.58), width * 0.055, height * 0.55, mats["dark"], vertices=12)
    elif faction == "Alliance":
        box(f"{asset_name}_MainMass", (0, 15, height * 0.34), (width * 0.68, length * 0.62, height * 0.52), mats["metal"], bevel=22)
        for side in (-1, 1):
            box(f"{asset_name}_Module_{side:+d}", (side * width * 0.31, 0, height * 0.31), (width * 0.20, length * 0.52, height * 0.38), mats["paint"], bevel=18, rotation_z=math.radians(side * 4))
        sphere(f"{asset_name}_SensorDome", (0, length * 0.08, height * 0.68), (width * 0.15, width * 0.15, height * 0.12), mats["glass"])
    elif faction == "Coalition":
        box(f"{asset_name}_MainMass", (0, 20, height * 0.34), (width * 0.64, length * 0.60, height * 0.50), mats["metal"], bevel=10)
        for level in range(3):
            z = height * (0.55 + level * 0.11)
            scale = 0.54 - level * 0.09
            box(f"{asset_name}_Roof_{level}", (0, 5, z), (width * scale, length * (scale + 0.04), height * 0.055), mats["paint" if level < 2 else "accent"], bevel=7)
        for side in (-1, 1):
            box(f"{asset_name}_Fin_{side:+d}", (side * width * 0.29, -length * 0.12, height * 0.48), (width * 0.045, length * 0.38, height * 0.38), mats["accent"], bevel=3)
    else:
        box(f"{asset_name}_MainMass", (0, 10, height * 0.30), (width * 0.62, length * 0.55, height * 0.44), mats["metal"], bevel=18)
        torus(f"{asset_name}_ChronoRing", (0, 0, height * 0.66), width * 0.22, width * 0.035, mats["emissive"], rotation=(math.radians(90), 0, 0))
        for index, angle in enumerate((0, 120, 240)):
            radians = math.radians(angle)
            box(f"{asset_name}_FloatingNode_{index}", (math.cos(radians) * width * 0.28, math.sin(radians) * length * 0.22, height * 0.62), (width * 0.12, length * 0.12, height * 0.20), mats["paint"], bevel=10, rotation_z=radians)

    if role == "headquarters":
        cylinder(f"{asset_name}_CommandCore", (0, 0, height * 0.70), width * 0.16, height * 0.42, mats["accent"], vertices=12, bevel=6)
        cylinder(f"{asset_name}_Antenna", (0, 0, height * 0.94), width * 0.018, height * 0.22, mats["dark"], vertices=10, bevel=1)
    elif role == "power_plant":
        for side in (-1, 1):
            cylinder(f"{asset_name}_Reactor_{side:+d}", (side * width * 0.19, 0, height * 0.55), width * 0.13, height * 0.52, mats["accent"], vertices=16, bevel=6)
            torus(f"{asset_name}_ReactorRing_{side:+d}", (side * width * 0.19, 0, height * 0.62), width * 0.145, width * 0.025, mats["emissive"])
    elif role == "refinery":
        box(f"{asset_name}_OreHopper", (width * 0.24, length * 0.08, height * 0.46), (width * 0.32, length * 0.38, height * 0.40), mats["accent"], bevel=8)
        cylinder(f"{asset_name}_Processor", (-width * 0.20, 0, height * 0.54), width * 0.12, height * 0.54, mats["paint"], vertices=12, bevel=5)
        box(f"{asset_name}_Conveyor", (0, -length * 0.34, height * 0.22), (width * 0.50, length * 0.22, height * 0.12), mats["dark"], bevel=4)
    elif role == "barracks":
        box(f"{asset_name}_Entrance", (0, -length * 0.40, height * 0.24), (width * 0.32, length * 0.14, height * 0.40), mats["accent"], bevel=8)
        for side in (-1, 1):
            box(f"{asset_name}_BunkerWing_{side:+d}", (side * width * 0.32, 0, height * 0.25), (width * 0.22, length * 0.48, height * 0.30), mats["paint"], bevel=10)
    elif role == "war_factory":
        box(f"{asset_name}_Hangar", (0, 0, height * 0.39), (width * 0.72, length * 0.62, height * 0.58), mats["metal"], bevel=16)
        box(f"{asset_name}_AssemblyDoor", (0, -length * 0.42, height * 0.25), (width * 0.48, length * 0.08, height * 0.40), mats["dark"], bevel=5)
        box(f"{asset_name}_ExitRamp", (0, -length * 0.49, height * 0.04), (width * 0.50, length * 0.18, height * 0.05), mats["accent"], bevel=2)

    empty_socket("VFX_Impact", (0, 0, height * 0.58))
    empty_socket("UnitExit", (0, -length * 0.55, 8))
    empty_socket("Rally", (0, -length * 0.75, 8))
    empty_socket("SelectionOrigin", (0, 0, 0))
    empty_socket("Exhaust", (width * 0.30, length * 0.18, height * 0.72))


def add_wheel(asset_name: str, index: int, location: tuple[float, float, float], radius: float, width: float, mats: dict[str, bpy.types.Material]) -> None:
    cylinder(f"{asset_name}_Wheel_{index:02d}", location, radius, width, mats["dark"], vertices=16, rotation=(0, math.radians(90), 0), bevel=2)
    cylinder(f"{asset_name}_Hub_{index:02d}", location, radius * 0.42, width * 1.03, mats["accent"], vertices=12, rotation=(0, math.radians(90), 0), bevel=2)


def add_vehicle(entry: dict[str, Any], asset_name: str, mats: dict[str, bpy.types.Material]) -> None:
    width, length, height = entry["dimensions_cm"]
    faction = entry["faction"]
    role = entry["role"]

    hover = faction == "Chronolegion"
    walker = faction == "Coalition" and role == "scout"
    wheeled = faction == "Alliance" or role == "scout"

    box(asset_name, (0, 0, height * 0.34), (width * 0.78, length * 0.82, height * 0.38), mats["metal"], bevel=12)
    box(f"{asset_name}_UpperHull", (0, length * 0.02, height * 0.58), (width * 0.58, length * 0.56, height * 0.22), mats["paint"], bevel=10)
    box(f"{asset_name}_Glacis", (0, length * 0.34, height * 0.48), (width * 0.55, length * 0.18, height * 0.20), mats["accent"], bevel=7)

    if walker:
        for side in (-1, 1):
            for fore in (-1, 1):
                # The Kamakiri's stance carries its gameplay width.  Keep the
                # feet near the footprint edge so the silhouette is readable
                # and does not get enlarged by runtime bounds normalization.
                x = side * width * 0.40
                y = fore * length * 0.27
                box(f"{asset_name}_Leg_{side:+d}_{fore:+d}", (x, y, height * 0.27), (width * 0.10, length * 0.10, height * 0.42), mats["dark"], bevel=5, rotation_z=math.radians(side * fore * 8))
                box(f"{asset_name}_Foot_{side:+d}_{fore:+d}", (x, y + fore * length * 0.04, height * 0.06), (width * 0.20, length * 0.20, height * 0.08), mats["accent"], bevel=5)
    elif hover:
        for side in (-1, 1):
            torus(f"{asset_name}_HoverRing_{side:+d}", (side * width * 0.34, 0, height * 0.22), width * 0.14, width * 0.035, mats["emissive"], rotation=(math.radians(90), 0, 0))
            box(f"{asset_name}_HoverPod_{side:+d}", (side * width * 0.34, 0, height * 0.25), (width * 0.16, length * 0.72, height * 0.18), mats["dark"], bevel=8)
    elif wheeled:
        wheel_radius = height * 0.19
        wheel_width = width * 0.11
        index = 0
        for side in (-1, 1):
            for longitudinal in (-0.30, 0.0, 0.30):
                add_wheel(asset_name, index, (side * width * 0.41, longitudinal * length, wheel_radius), wheel_radius, wheel_width, mats)
                index += 1
    else:
        for side in (-1, 1):
            box(f"{asset_name}_Track_{side:+d}", (side * width * 0.39, 0, height * 0.22), (width * 0.16, length * 0.86, height * 0.28), mats["dark"], bevel=height * 0.08)
            for longitudinal in (-0.28, 0.0, 0.28):
                add_wheel(asset_name, int((side + 1) * 2 + (longitudinal + 0.3) * 3), (side * width * 0.41, longitudinal * length, height * 0.20), height * 0.14, width * 0.13, mats)

    if role == "harvester":
        box(f"{asset_name}_OreBin", (0, -length * 0.17, height * 0.70), (width * 0.50, length * 0.42, height * 0.34), mats["accent"], bevel=12)
        cylinder(f"{asset_name}_Excavator", (0, length * 0.43, height * 0.28), width * 0.17, width * 0.56, mats["dark"], vertices=14, rotation=(0, math.radians(90), 0), bevel=3)
        muzzle = (0, length * 0.48, height * 0.42)
    elif role == "scout":
        cylinder(f"{asset_name}_SensorMast", (0, 0, height * 0.82), width * 0.035, height * 0.34, mats["dark"], vertices=10, bevel=1)
        sphere(f"{asset_name}_Sensor", (0, 0, height * 0.98), (width * 0.10, width * 0.10, height * 0.08), mats["emissive"], segments=16)
        cylinder(f"{asset_name}_LightCannon", (0, length * 0.30, height * 0.68), width * 0.045, length * 0.42, mats["accent"], vertices=12, rotation=(math.radians(90), 0, 0), bevel=2)
        muzzle = (0, length * 0.51, height * 0.68)
    elif role == "main_battle_tank":
        cylinder(f"{asset_name}_Turret", (0, 0, height * 0.76), width * 0.23, height * 0.22, mats["paint"], vertices=14, bevel=7)
        cylinder(f"{asset_name}_Barrel", (0, length * 0.35, height * 0.78), width * 0.045, length * 0.72, mats["accent"], vertices=12, rotation=(math.radians(90), 0, 0), bevel=2)
        box(f"{asset_name}_Mantlet", (0, length * 0.12, height * 0.78), (width * 0.24, length * 0.16, height * 0.16), mats["dark"], bevel=6)
        muzzle = (0, length * 0.71, height * 0.78)
    else:
        cylinder(f"{asset_name}_Turret", (0, -length * 0.08, height * 0.72), width * 0.21, height * 0.20, mats["paint"], vertices=14, bevel=6)
        if faction == "Soviet":
            for side in (-1, 1):
                box(f"{asset_name}_RocketRack_{side:+d}", (side * width * 0.17, length * 0.12, height * 0.87), (width * 0.23, length * 0.48, height * 0.20), mats["accent"], bevel=5)
            muzzle = (0, length * 0.38, height * 0.89)
        else:
            cylinder(f"{asset_name}_ArtilleryBarrel", (0, length * 0.41, height * 0.84), width * 0.040, length * 0.94, mats["emissive" if faction == "Chronolegion" else "accent"], vertices=12, rotation=(math.radians(90), 0, 0), bevel=2)
            muzzle = (0, length * 0.88, height * 0.84)

    empty_socket("Turret", (0, 0, height * 0.73))
    empty_socket("Muzzle", muzzle, (math.radians(90), 0, 0))
    empty_socket("ProjectileSpawn", muzzle, (math.radians(90), 0, 0))
    empty_socket("VFX_Impact", (0, 0, height * 0.56))
    empty_socket("Engine", (0, -length * 0.40, height * 0.40))
    empty_socket("Exhaust", (width * 0.18, -length * 0.44, height * 0.48))
    empty_socket("SelectionOrigin", (0, 0, 0))
    # Existing runtime mapping names are retained as aliases until the mapping asset
    # is migrated to the canonical naming-reset sockets.
    empty_socket("Socket_Turret", (0, 0, height * 0.73))
    empty_socket("Socket_Muzzle", muzzle, (math.radians(90), 0, 0))
    empty_socket("Socket_Engine", (0, -length * 0.40, height * 0.40))


def create_lods(asset_name: str) -> None:
    source_objects = [
        obj
        for obj in list(bpy.context.scene.objects)
        if obj.type == "MESH" and not obj.name.startswith("UCX_") and "_LOD" not in obj.name
    ]
    for lod_index, ratio in ((1, 0.55), (2, 0.28), (3, 0.12)):
        duplicates = []
        for source in source_objects:
            duplicate = source.copy()
            duplicate.data = source.data.copy()
            bpy.context.scene.collection.objects.link(duplicate)
            duplicates.append(duplicate)
        bpy.ops.object.select_all(action="DESELECT")
        for duplicate in duplicates:
            duplicate.select_set(True)
        bpy.context.view_layer.objects.active = duplicates[0]
        bpy.ops.object.join()
        lod = bpy.context.object
        lod.name = f"{asset_name}_LOD{lod_index}"
        modifier = lod.modifiers.new(f"LOD{lod_index}_Decimate", "DECIMATE")
        modifier.ratio = ratio
        modifier.use_collapse_triangulate = True
        try:
            bpy.ops.object.modifier_apply(modifier=modifier.name)
        except RuntimeError:
            pass
        lod.hide_viewport = True
        lod.hide_render = True
        lod["ra4_lod"] = lod_index
        lod.select_set(False)


def build(entry: dict[str, Any], source_root: Path) -> Path:
    reset_scene()
    mats = make_materials(entry["faction"])
    asset_name = f"SM_{entry['faction']}_{entry['stable_id']}"
    if entry["kind"] == "building":
        add_building(entry, asset_name, mats)
    else:
        add_vehicle(entry, asset_name, mats)
    create_collision(asset_name, entry["dimensions_cm"], entry["kind"])
    create_lods(asset_name)

    scene = bpy.context.scene
    scene["ra4_stable_id"] = entry["stable_id"]
    scene["ra4_status"] = "GENERATED_DRAFT"
    scene["ra4_dimensions_cm"] = entry["dimensions_cm"]
    scene["ra4_blockout_replacement"] = entry["blockout"]
    scene["ra4_target"] = entry["target"]

    target_dir = source_root / entry["faction"]
    target_dir.mkdir(parents=True, exist_ok=True)
    target = target_dir / f"SRC_{entry['faction']}_{entry['stable_id']}.blend"
    # Generated sources are reproducible; Blender's numbered backup would add
    # stale binary noise whenever one manifest entry is regenerated in place.
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(target), compress=True)
    print(f"GENERATED_DRAFT {entry['stable_id']} -> {target}")
    return target


def main() -> None:
    args = arguments()
    manifest = load_manifest()
    selected = set(args.stable_id)
    if not args.all and not selected:
        raise SystemExit("Pass --all or at least one --stable-id")
    source_root = PROJECT_ROOT / manifest["source_root"]
    entries = [entry for entry in manifest["assets"] if args.all or entry["stable_id"] in selected]
    missing = selected - {entry["stable_id"] for entry in entries}
    if missing:
        raise SystemExit(f"Unknown Stable ID(s): {', '.join(sorted(missing))}")
    for entry in entries:
        build(entry, source_root)


if __name__ == "__main__":
    main()
