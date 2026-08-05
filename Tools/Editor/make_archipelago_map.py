"""Generate the tropical-archipelago skirmish map (RA4_Skirmish_Production).

Run headless:
    UnrealEditor-Cmd RedAlert4.uproject \
        -ExecutePythonScript="Tools/Editor/make_archipelago_map.py"

Or from an interactive editor via the Python console / MCP:
    exec(open("Tools/Editor/make_archipelago_map.py").read())

WHY THIS IS A SCRIPT AND NOT HAND-DRIVEN EDITOR WORK
----------------------------------------------------
An earlier pass built this map by driving the editor interactively. A
concurrent branch checkout reset the working tree and destroyed all of it,
because .umap edits live only on disk until committed. Encoding the build
here makes it reproducible and cheap to regenerate.

MAP SIZE CONTRACT
-----------------
World space is 0..MAP_EXTENT on X and Y (NOT centred on the origin).
This mirrors FRA4MatchBootstrap::kMapTiles and RA4::kTileSizeUnits:

    MAP_TILES  = 64
    TILE_UNITS = 200                  (SimTypes.h kTileSizeUnitsLocal)
    MAP_EXTENT = 64 * 200 = 12800
    centre     = (6400, 6400)

Player starts stay at (2400,2400) and (10400,10400) to match
make_production_skirmish_map.py, so simulation bootstrap is unaffected.

EMPIRICALLY VERIFIED CONSTANTS
------------------------------
Every number below was measured against a live landscape, not guessed.

1. Landscape config for 505x505 verts comes from
   LandscapeService.find_landscape_config_for_resolution(505, 505):
       4x4 components, quads_per_section=63, sections_per_component=2
   505 verts * 25.6 uu = 12928 uu, covering the 12800 uu contract.

2. Mesh pivot offsets, from UStaticMesh.get_bounds() (origin.z - extent.z):
       faction buildings  bottom_offset  0.0  -> place at terrain Z
       SM_Asphalt01_1     bottom_offset -31.0 -> place at terrain Z + 31
       SM_Bridge01_1      bottom_offset -73.1 -> place at terrain Z + 73.1
   Placing at a hardcoded Z buried all 10 buildings 520 uu underground.

3. create_plateau / create_mountain apply a height DELTA to existing terrain,
   they do not set an absolute height. After lowering the seabed to -500, a
   "height=400" plateau lands at -100, i.e. still underwater. This silently
   submerged all four small islands on the first attempt. ISLAND_RAISE below
   is therefore sized to clear SEA_LEVEL with margin, and assert_above_water()
   verifies it rather than trusting it.

4. apply_noise signature is
       apply_noise(label, center_x, center_y, world_radius, amplitude,
                   frequency, seed, octaves)
   Passing amplitude into the world_radius slot returns success=True while
   reporting vertices_modified=0 - a silent no-op. Always check
   result.vertices_modified.
"""

import unreal

# ---------------------------------------------------------------- contract ---
MAP_TILES = 64
TILE_UNITS = 200
MAP_EXTENT = MAP_TILES * TILE_UNITS          # 12800
CENTRE = MAP_EXTENT * 0.5                    # 6400

LANDSCAPE_LABEL = "RA4_Archipelago"
TERRAIN_MATERIAL = "/Game/RA4/Generated/Terrain/M_RA4_Terrain"

# 505x505 verts at 25.6 uu/quad -> 12928 uu, covers MAP_EXTENT.
LANDSCAPE_SCALE = 25.6
QUADS_PER_SECTION = 63
SECTIONS_PER_COMPONENT = 2
COMPONENT_COUNT = 4

SEA_LEVEL = 0.0                              # WaterBodyOcean plane sits at Z=0
SEABED_DEPTH = -500.0                        # base ocean floor
BRIDGE_DECK_Z = 193.1                        # deck clear of water, incl. pivot

# Pivot corrections measured from mesh bounds (see docstring note 2).
PIVOT_OFFSET = {
    "SM_Asphalt01_1": 31.0,
    "SM_Bridge01_1": 73.1,
}

ROAD_MESH = "/Game/ThirdParty/CityPark/Meshes/Road/SM_Asphalt01_1"
BRIDGE_MESH = "/Game/ThirdParty/CityPark/Meshes/Road/SM_Bridge01_1"
ROAD_LEN_Y = 890.0                           # extent.y 445 * 2
BRIDGE_LEN_Y = 322.0                         # extent.y 161 * 2

# Player starts must match make_production_skirmish_map.py.
P1 = (2400.0, 2400.0)
P2 = (10400.0, 10400.0)

# Islands as (centre_x, centre_y, radius, raise_height, name).
# raise_height is a DELTA on top of SEABED_DEPTH (docstring note 3), sized so
# every island clears SEA_LEVEL. Verified afterwards by assert_above_water().
ISLANDS = [
    (P1[0], P1[1], 2200.0, 1200.0, "P1_Base"),
    (P2[0], P2[1], 2200.0, 1200.0, "P2_Base"),
    (CENTRE, CENTRE, 900.0, 850.0, "Centre"),
    (1900.0, 7400.0, 600.0, 1120.0, "Satellite_NW"),
    (7900.0, 2400.0, 500.0, 1120.0, "Satellite_SE"),
    (5400.0, 10900.0, 400.0, 1120.0, "Satellite_NE"),
    (10900.0, 5400.0, 500.0, 1120.0, "Satellite_SW"),
]

TREE_MESHES = [
    "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_AmurCork01",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Elm01",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Maple01",
]
GRASS_MESHES = [
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass02",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass03",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass04",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass05",
]
ROCK_MESHES = [
    "/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock01",
    "/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock02",
    "/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock03",
]

BUILDING_LAYOUT = [
    ("ConYard", 0.0, 0.0),
    ("Barracks", 400.0, 0.0),
    ("PowerPlant", -400.0, 400.0),
    ("WarFactory", 0.0, -400.0),
    ("Refinery", 400.0, 400.0),
]

LS = unreal.LandscapeService
FS = unreal.FoliageService


# ------------------------------------------------------------------ helpers ---
def editor_actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def terrain_z(x, y):
    """Sampled landscape height at a world XY. Never assume a height."""
    h = LS.get_height_at_location(LANDSCAPE_LABEL, x, y)
    return h.height if hasattr(h, "height") else h


def spawn_mesh(mesh_path, x, y, z, yaw=0.0, scale=None, label=None):
    mesh = unreal.load_asset(mesh_path)
    if mesh is None:
        print("  [skip] mesh not found: %s" % mesh_path)
        return None
    actor = editor_actors().spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0.0, yaw, 0.0)
    )
    comp = actor.get_component_by_class(unreal.StaticMeshComponent)
    comp.set_static_mesh(mesh)
    if scale is not None:
        comp.set_world_scale3d(scale)
    if label:
        actor.set_actor_label(label)
    return actor


def clear_static_meshes():
    eas = editor_actors()
    removed = 0
    for actor in list(eas.get_all_level_actors()):
        if isinstance(actor, unreal.StaticMeshActor):
            eas.destroy_actor(actor)
            removed += 1
    print("Removed %d StaticMeshActors" % removed)


# --------------------------------------------------------------- landscape ---
def create_landscape():
    if LS.landscape_exists(LANDSCAPE_LABEL):
        print("Landscape %s already exists; deleting" % LANDSCAPE_LABEL)
        LS.delete_landscape(LANDSCAPE_LABEL)

    result = LS.create_landscape(
        location=unreal.Vector(0.0, 0.0, 0.0),
        rotation=unreal.Rotator(0.0, 0.0, 0.0),
        scale=unreal.Vector(LANDSCAPE_SCALE, LANDSCAPE_SCALE, LANDSCAPE_SCALE),
        sections_per_component=SECTIONS_PER_COMPONENT,
        quads_per_section=QUADS_PER_SECTION,
        component_count_x=COMPONENT_COUNT,
        component_count_y=COMPONENT_COUNT,
        landscape_label=LANDSCAPE_LABEL,
    )
    if not result.success:
        raise RuntimeError("create_landscape failed: %s" % result.error_message)
    LS.set_landscape_material(LANDSCAPE_LABEL, TERRAIN_MATERIAL)
    print("Landscape created and material assigned")


def sculpt_archipelago():
    # Drop the whole sheet to seabed so water reads as open ocean.
    LS.raise_lower_region(
        LANDSCAPE_LABEL, CENTRE, CENTRE, MAP_EXTENT, MAP_EXTENT, SEABED_DEPTH, 0.0
    )
    print("Seabed lowered to %.0f" % SEABED_DEPTH)

    for idx, (cx, cy, radius, raise_h, name) in enumerate(ISLANDS):
        # Flat-ish core plus a mound, so islands are not billiard-table mesas.
        LS.create_plateau(LANDSCAPE_LABEL, cx, cy, radius * 0.55, raise_h, radius * 0.45)
        LS.create_mountain(
            LANDSCAPE_LABEL, cx, cy, radius, 180.0, 0.9, True, 300 + idx
        )
        amplitude = 110.0 if radius > 1500.0 else 60.0
        noise = LS.apply_noise(
            LANDSCAPE_LABEL, cx, cy, radius, amplitude, 0.0015, 5000 + idx * 13, 5
        )
        # apply_noise can report success while doing nothing (docstring note 4).
        if noise.vertices_modified == 0:
            print("  [warn] %s: apply_noise modified 0 vertices" % name)
        LS.apply_erosion(
            LANDSCAPE_LABEL, cx, cy, radius * 1.15, 600, 0.35, 900 + idx
        )
        print("  %-14s peak_z=%8.1f" % (name, terrain_z(cx, cy)))

    # Level the two build pads so structures do not sit on a slope.
    for cx, cy, name in ((P1[0], P1[1], "P1"), (P2[0], P2[1], "P2")):
        LS.flatten_at_location(
            LANDSCAPE_LABEL, cx, cy, 900.0, terrain_z(cx, cy), 1.0, "Smooth"
        )
        print("  pad %s flattened at %.1f" % (name, terrain_z(cx, cy)))


def assert_above_water():
    """Fail loudly if any island is submerged.

    This exact defect shipped once already: create_mountain deltas applied onto
    the seabed left all four satellites between -3 and -137, i.e. invisible
    under the ocean plane, while every call had returned success.
    """
    submerged = []
    for cx, cy, _radius, _raise_h, name in ISLANDS:
        z = terrain_z(cx, cy)
        if z <= SEA_LEVEL:
            submerged.append((name, z))
    if submerged:
        raise RuntimeError(
            "Islands below sea level: "
            + ", ".join("%s=%.1f" % (n, z) for n, z in submerged)
        )
    print("All %d islands verified above sea level" % len(ISLANDS))


# -------------------------------------------------------------------- water ---
def add_water():
    eas = editor_actors()
    for actor in list(eas.get_all_level_actors()):
        if isinstance(actor, (unreal.WaterBodyOcean, unreal.WaterZone)):
            eas.destroy_actor(actor)

    centre = unreal.Vector(CENTRE, CENTRE, SEA_LEVEL)
    rot = unreal.Rotator(0.0, 0.0, 0.0)
    eas.spawn_actor_from_class(unreal.WaterZone, centre, rot)
    eas.spawn_actor_from_class(unreal.WaterBodyOcean, centre, rot)
    print("WaterZone + WaterBodyOcean placed at Z=%.0f" % SEA_LEVEL)


# ------------------------------------------------------------------- roads ---
def build_link(start, end, tag):
    """Lay road over land and bridge over water along start->end.

    Classifies the span by sampled height instead of assuming where the
    shoreline is; an earlier hardcoded route put two road segments on the
    seabed at Z=-469.
    """
    import math

    sx, sy = start
    ex, ey = end
    dx, dy = ex - sx, ey - sy
    dist = math.hypot(dx, dy)
    if dist <= 0.0:
        return 0
    yaw = math.degrees(math.atan2(dy, dx)) + 90.0   # mesh long axis is +Y

    samples = []
    steps = 200
    for i in range(steps + 1):
        t = i / float(steps)
        x, y = sx + dx * t, sy + dy * t
        samples.append((t, x, y, terrain_z(x, y)))

    runs = []
    is_water = samples[0][3] < SEA_LEVEL
    run_start = 0
    for i in range(1, len(samples)):
        w = samples[i][3] < SEA_LEVEL
        if w != is_water:
            runs.append((is_water, run_start, i - 1))
            is_water, run_start = w, i
    runs.append((is_water, run_start, len(samples) - 1))

    placed = 0
    for water, i0, i1 in runs:
        t0, t1 = samples[i0][0], samples[i1][0]
        seg_len = (t1 - t0) * dist
        if seg_len < 60.0:
            continue
        mesh = BRIDGE_MESH if water else ROAD_MESH
        base_len = BRIDGE_LEN_Y if water else ROAD_LEN_Y
        pivot = PIVOT_OFFSET[mesh.rsplit("/", 1)[-1]]
        count = max(1, int(round(seg_len / base_len)))
        for k in range(count):
            ct = t0 + (t1 - t0) * ((k + 0.5) / count)
            cx, cy = sx + dx * ct, sy + dy * ct
            z = BRIDGE_DECK_Z if water else terrain_z(cx, cy) + pivot
            spawn_mesh(
                mesh, cx, cy, z, yaw,
                unreal.Vector(1.0, (seg_len / count) / base_len, 1.0),
                "%s_%s_%d" % (tag, "Bridge" if water else "Road", k),
            )
            placed += 1
    return placed


def add_roads():
    inset = 400.0
    total = 0
    for (px, py), tag in ((P1, "P1_Centre"), (P2, "P2_Centre")):
        ux = 1.0 if CENTRE > px else -1.0
        uy = 1.0 if CENTRE > py else -1.0
        total += build_link(
            (px + ux * inset, py + uy * inset),
            (CENTRE - ux * inset, CENTRE - uy * inset),
            tag,
        )
    print("Placed %d road/bridge segments" % total)


# ---------------------------------------------------------------- buildings ---
def add_buildings():
    placed = 0
    for faction, code, (bx, by) in (("Soviet", "SU", P1), ("Alliance", "AL", P2)):
        for btype, ox, oy in BUILDING_LAYOUT:
            path = "/Game/RA4/Art/Buildings/%s/SM_%s_%s_%s" % (
                faction, faction, code, btype,
            )
            x, y = bx + ox, by + oy
            # Buildings pivot at their base, so terrain Z is correct as-is.
            if spawn_mesh(path, x, y, terrain_z(x, y), 0.0, None,
                          "%s_%s" % (faction, btype)):
                placed += 1
    print("Placed %d faction buildings" % placed)


def add_ore_fields():
    """Placeholder ore markers.

    The project has no crystal/ore meshes, so these are tinted cubes standing
    in for green/blue ore. Replace when real ore art lands.
    """
    cube = "/Engine/BasicShapes/Cube"
    green = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/MI_Grass01")
    blue = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/MI_Water1")
    scale = unreal.Vector(3.0, 3.0, 1.5)

    placed = 0
    for bx, by in (P1, P2):
        for ox, oy, mat, kind in (
            (500.0, 500.0, green, "Green"),
            (-500.0, -500.0, green, "Green2"),
            (500.0, -500.0, blue, "Blue"),
        ):
            x, y = bx + ox, by + oy
            # Cube pivot is centred: lift by half the scaled height.
            actor = spawn_mesh(
                cube, x, y, terrain_z(x, y) + 50.0 * scale.z, 0.0, scale,
                "Ore_%s_%d_%d" % (kind, int(x), int(y)),
            )
            if actor and mat:
                actor.get_component_by_class(
                    unreal.StaticMeshComponent
                ).set_material(0, mat)
                placed += 1
    print("Placed %d ore markers (placeholder cubes)" % placed)


# ------------------------------------------------------------------ foliage ---
def scatter_foliage():
    FS.clear_all_foliage()
    for cx, cy, radius, _raise_h, name in ISLANDS:
        large = radius > 1500.0
        inner = radius * 0.82          # keep foliage off the waterline
        for mesh in TREE_MESHES if large else TREE_MESHES[:2]:
            FS.scatter_foliage(mesh, cx, cy, inner, 50 if large else 12,
                               0.7, 1.3, True, True, 0, LANDSCAPE_LABEL)
        for mesh in GRASS_MESHES:
            FS.scatter_foliage(mesh, cx, cy, inner, 80 if large else 40,
                               0.5, 1.5, True, True, 0, LANDSCAPE_LABEL)
        if large:
            for mesh in ROCK_MESHES:
                FS.scatter_foliage(mesh, cx, cy, inner, 15,
                                   0.6, 1.4, True, True, 0, LANDSCAPE_LABEL)
        print("  foliage scattered on %s" % name)

    for mesh in TREE_MESHES + GRASS_MESHES:
        print("  %-16s instances=%d"
              % (mesh.rsplit("/", 1)[-1], FS.get_instance_count(mesh)))


# ------------------------------------------------------------------- verify ---
def verify():
    """Check every actor against sampled terrain and the waterline.

    Returns the problem count. This is the gate that caught 24 misplaced
    actors (buildings 520 uu underground, roads on the seabed) the first time.
    """
    problems = []
    checked = 0
    for actor in editor_actors().get_all_level_actors():
        if not isinstance(actor, unreal.StaticMeshActor):
            continue
        comp = actor.get_component_by_class(unreal.StaticMeshComponent)
        if comp is None or comp.static_mesh is None:
            continue
        name = comp.static_mesh.get_name()
        loc = actor.get_actor_location()
        tz = terrain_z(loc.x, loc.y)
        checked += 1

        if "Bridge" in name:
            if tz >= SEA_LEVEL:
                problems.append("%s spans land at (%.0f,%.0f)" % (name, loc.x, loc.y))
            elif loc.z <= SEA_LEVEL:
                problems.append("%s underwater at (%.0f,%.0f)" % (name, loc.x, loc.y))
        elif "Asphalt" in name:
            if tz < SEA_LEVEL:
                problems.append("%s over water at (%.0f,%.0f)" % (name, loc.x, loc.y))
            elif abs(loc.z - (tz + PIVOT_OFFSET[name])) > 6.0:
                problems.append("%s off surface at (%.0f,%.0f)" % (name, loc.x, loc.y))
        elif name != "Cube":
            if tz < SEA_LEVEL:
                problems.append("%s over water at (%.0f,%.0f)" % (name, loc.x, loc.y))
            elif abs(loc.z - tz) > 6.0:
                problems.append(
                    "%s off ground by %+.1f at (%.0f,%.0f)"
                    % (name, loc.z - tz, loc.x, loc.y)
                )

    print("Verified %d actors, %d problems" % (checked, len(problems)))
    for p in problems:
        print("  PROBLEM: %s" % p)

    analysis = LS.analyze_terrain(LANDSCAPE_LABEL)
    print("Terrain %.0f .. %.0f  avg_slope=%.1f"
          % (analysis.min_height, analysis.max_height,
             analysis.average_slope_degrees))
    return len(problems)


def height_map_ascii(width=64, height=32):
    """Top-down land/water map sampled from real heights.

    Cheap visual check that works headless; the editor's high-res screenshot
    path rendered an all-black image, so this is the reliable option.
    """
    rows = []
    for j in range(height):
        y = MAP_EXTENT - (j + 0.5) * MAP_EXTENT / height
        line = []
        for i in range(width):
            x = (i + 0.5) * MAP_EXTENT / width
            z = terrain_z(x, y)
            if z < -400.0:
                line.append("~")
            elif z < SEA_LEVEL:
                line.append("-")
            elif z < 200.0:
                line.append(".")
            elif z < 500.0:
                line.append("o")
            else:
                line.append("#")
        rows.append("".join(line))
    print("Top-down map (# high, o mid, . low, - shallow, ~ deep)")
    for j, row in enumerate(rows):
        print("%2d %s" % (j, row))


# --------------------------------------------------------------------- main ---
def main():
    print("=== Building archipelago skirmish map ===")
    clear_static_meshes()
    create_landscape()
    sculpt_archipelago()
    assert_above_water()
    add_water()
    add_roads()
    add_buildings()
    add_ore_fields()
    scatter_foliage()
    problems = verify()
    height_map_ascii()

    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print("Level saved")

    if problems:
        raise RuntimeError("%d placement problems; map not clean" % problems)
    print("=== Archipelago map complete and verified ===")


if __name__ == "__main__":
    main()
