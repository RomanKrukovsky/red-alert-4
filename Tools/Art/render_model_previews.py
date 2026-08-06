# Copyright (c) Red Alert 4 project. Offline model preview renderer.
#
# Renders ArtSource FBX exports into flat PNG previews using Blender, so model
# quality can be reviewed and diffed without launching the Unreal editor.
#
# Why this exists rather than an in-editor capture:
#   - Tools/Editor/capture_map_screenshots.py only writes .meta text files; it has
#     never produced an image, so it cannot be used to judge art.
#   - Launching UnrealEditor with a map argument starts PIE and the editor exits by
#     itself after roughly two minutes (see Docs/Art/PreviewPipeline.md), which makes
#     interactive capture unreliable in an automated session.
#   - The MCP editor bridge needs the RemoteControl plugin, which this project does
#     not enable.
# Blender is already a hard dependency of the art pipeline (the .blend sources live
# in ArtSource/RA4/Models), so it adds nothing new to the toolchain.
#
# Usage (headless):
#   blender -b --python Tools/Art/render_model_previews.py -- \
#       --out /tmp/previews --faction Soviet
#   blender -b --python Tools/Art/render_model_previews.py -- \
#       --out /tmp/previews --files a.fbx,b.fbx --contact-sheet
#
# Output: one PNG per model plus an optional contact sheet, and a manifest.json
# carrying triangle counts and bounds so regressions are measurable, not eyeballed.

import argparse
import json
import math
import os
import sys

import bpy
import mathutils


REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXPORTS_DIR = os.path.join(REPO_ROOT, "ArtSource", "RA4", "Exports")
FACTIONS = ("Soviet", "Alliance", "Coalition", "Chronolegion")

# A neutral studio look. Deliberately NOT the game's faction materials: the point
# is to judge silhouette, proportion and detail density, which faction colour
# tinting actively hides.
PREVIEW_BASE_COLOR = (0.55, 0.57, 0.60, 1.0)
PREVIEW_METALLIC = 0.60
PREVIEW_ROUGHNESS = 0.45
BACKGROUND_COLOR = (0.45, 0.55, 0.65, 1.0)

# Camera framing chosen to match how the player actually sees units in an RTS:
# a high three-quarter view rather than a flattering hero angle.
CAMERA_ELEVATION_DEG = 35.0
CAMERA_DISTANCE_SCALE = 1.35


def parse_args(argv):
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []
    p = argparse.ArgumentParser(prog="render_model_previews")
    p.add_argument("--out", required=True, help="output directory for PNGs")
    p.add_argument("--files", default="", help="comma-separated FBX paths")
    p.add_argument("--faction", default="", help=f"render one of: {', '.join(FACTIONS)}")
    p.add_argument("--all", action="store_true", help="render every faction")
    p.add_argument("--resolution", type=int, default=960)
    p.add_argument("--contact-sheet", action="store_true",
                   help="also write contact_sheet.png tiling every render")
    return p.parse_args(argv)


def collect_fbx(args):
    """Base LOD0 models only: LOD1-3 and _Turret/_Wheel/_Destroyed parts are
    components of a model, not models to review on their own."""
    if args.files:
        return [f if os.path.isabs(f) else os.path.join(REPO_ROOT, f)
                for f in args.files.split(",") if f]

    factions = list(FACTIONS) if args.all else ([args.faction] if args.faction else [])
    if not factions:
        raise SystemExit("nothing to render: pass --files, --faction or --all")

    found = []
    for faction in factions:
        d = os.path.join(EXPORTS_DIR, faction)
        if not os.path.isdir(d):
            print(f"WARN missing faction dir {d}")
            continue
        for name in sorted(os.listdir(d)):
            if not name.endswith(".fbx"):
                continue
            stem = name[:-4]
            if "_LOD" in stem or stem.endswith(("_Turret", "_Wheel", "_Destroyed")):
                continue
            found.append(os.path.join(d, name))
    return found


def reset_scene(resolution):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    # EEVEE only: this must run on a headless machine with no GPU guarantees, and
    # Cycles would turn a 36-model sweep into minutes per frame for no benefit at
    # preview fidelity.
    sc.render.engine = "BLENDER_EEVEE"
    sc.render.resolution_x = resolution
    sc.render.resolution_y = int(resolution * 0.75)
    sc.render.film_transparent = False

    world = bpy.data.worlds.new("PreviewWorld")
    sc.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = BACKGROUND_COLOR
    bg.inputs[1].default_value = 1.0


def measure(objs):
    mins = mathutils.Vector((1e9, 1e9, 1e9))
    maxs = mathutils.Vector((-1e9, -1e9, -1e9))
    tris = 0
    for o in objs:
        tris += len(o.data.polygons)
        for corner in o.bound_box:
            wv = o.matrix_world @ mathutils.Vector(corner)
            mins = mathutils.Vector(map(min, mins, wv))
            maxs = mathutils.Vector(map(max, maxs, wv))
    return mins, maxs, tris


def apply_preview_material(objs):
    mat = bpy.data.materials.new("Preview")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = PREVIEW_BASE_COLOR
    bsdf.inputs["Metallic"].default_value = PREVIEW_METALLIC
    bsdf.inputs["Roughness"].default_value = PREVIEW_ROUGHNESS
    for o in objs:
        o.data.materials.clear()
        o.data.materials.append(mat)


def setup_lighting_and_camera(center, size):
    sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", "SUN"))
    sun.data.energy = 4.0
    sun.rotation_euler = (math.radians(50), 0.0, math.radians(30))
    bpy.context.collection.objects.link(sun)

    # Fill light opposite the sun so the shadow side still reads.
    fill = bpy.data.objects.new("Fill", bpy.data.lights.new("Fill", "SUN"))
    fill.data.energy = 1.2
    fill.rotation_euler = (math.radians(60), 0.0, math.radians(-140))
    bpy.context.collection.objects.link(fill)

    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    bpy.context.collection.objects.link(cam)
    bpy.context.scene.camera = cam

    dist = size * CAMERA_DISTANCE_SCALE
    ang = math.radians(CAMERA_ELEVATION_DEG)
    planar = dist * math.cos(ang) * 0.7
    cam.location = center + mathutils.Vector(
        (planar, -planar, dist * math.sin(ang) + size * 0.25))
    to_target = center - cam.location
    cam.rotation_euler = to_target.to_track_quat("-Z", "Y").to_euler()


def render_one(path, out_dir, resolution):
    name = os.path.splitext(os.path.basename(path))[0]
    if not os.path.exists(path):
        print(f"SKIP missing {path}")
        return None

    reset_scene(resolution)
    bpy.ops.import_scene.fbx(filepath=path)
    objs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    if not objs:
        print(f"SKIP no mesh in {path}")
        return None

    mins, maxs, tris = measure(objs)
    center = (mins + maxs) / 2.0
    extent = maxs - mins
    size = max(extent.length, 0.1)

    apply_preview_material(objs)
    setup_lighting_and_camera(center, size)

    png = os.path.join(out_dir, name + ".png")
    bpy.context.scene.render.filepath = png
    bpy.ops.render.render(write_still=True)
    print(f"RENDERED {name} tris={tris} size={size:.2f}")
    return {
        "name": name,
        "source": os.path.relpath(path, REPO_ROOT),
        "png": os.path.basename(png),
        "triangles": tris,
        "bounds_m": [round(extent.x, 3), round(extent.y, 3), round(extent.z, 3)],
    }


def build_contact_sheet(records, out_dir, resolution):
    """Tiles the renders into one image. Reviewing a roster side by side is how
    silhouette clashes and scale mistakes become obvious.

    Implemented by texturing the finished PNGs onto flat planes under an
    orthographic camera rather than through the compositor: Blender 5.x removed
    Scene.node_tree, and an emission-shaded plane grid is both version-stable and
    exactly 1:1 in pixels when the ortho scale matches the grid size.
    """
    if not records:
        return
    tiles = [r for r in records if os.path.exists(os.path.join(out_dir, r["png"]))]
    if not tiles:
        return

    cols = int(math.ceil(math.sqrt(len(tiles))))
    rows = int(math.ceil(len(tiles) / cols))
    aspect = 0.75          # every tile is rendered at resolution x 0.75*resolution

    reset_scene(resolution)
    sc = bpy.context.scene
    sc.render.resolution_x = cols * resolution
    sc.render.resolution_y = int(rows * resolution * aspect)

    for i, rec in enumerate(tiles):
        img = bpy.data.images.load(os.path.join(out_dir, rec["png"]))
        mat = bpy.data.materials.new(f"Tile{i}")
        mat.use_nodes = True
        nt = mat.node_tree
        for n in list(nt.nodes):
            nt.nodes.remove(n)
        tex = nt.nodes.new("ShaderNodeTexImage")
        tex.image = img
        # Emission keeps the tile exactly as rendered: no relighting, no shading.
        emit = nt.nodes.new("ShaderNodeEmission")
        out = nt.nodes.new("ShaderNodeOutputMaterial")
        nt.links.new(tex.outputs["Color"], emit.inputs["Color"])
        nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])

        bpy.ops.mesh.primitive_plane_add(size=1.0)
        plane = bpy.context.active_object
        plane.scale = (1.0, aspect, 1.0)
        col, row = i % cols, i // cols
        plane.location = (col - (cols - 1) / 2.0,
                          ((rows - 1) / 2.0 - row) * aspect,
                          0.0)
        plane.data.materials.append(mat)

    cam = bpy.data.objects.new("SheetCam", bpy.data.cameras.new("SheetCam"))
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = float(cols)
    cam.location = (0.0, 0.0, 10.0)
    bpy.context.collection.objects.link(cam)
    sc.camera = cam

    sheet = os.path.join(out_dir, "contact_sheet.png")
    sc.render.filepath = sheet
    bpy.ops.render.render(write_still=True)
    print(f"CONTACT_SHEET {sheet} ({cols}x{rows}, {len(tiles)} tiles)")


def main():
    args = parse_args(sys.argv)
    os.makedirs(args.out, exist_ok=True)
    files = collect_fbx(args)
    print(f"models to render: {len(files)}")

    records = []
    for path in files:
        rec = render_one(path, args.out, args.resolution)
        if rec:
            records.append(rec)

    if args.contact_sheet:
        build_contact_sheet(records, args.out, args.resolution)

    manifest = os.path.join(args.out, "manifest.json")
    with open(manifest, "w", encoding="utf-8") as f:
        json.dump({"models": records}, f, indent=2)
    print(f"MANIFEST {manifest} entries={len(records)}")
    print("ALL_DONE")


if __name__ == "__main__":
    main()
