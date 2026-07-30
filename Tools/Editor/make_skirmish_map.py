# Copyright (c) Red Alert 4 project.
#
# Builds the playable skirmish level without opening the editor UI.
#
# The level is deliberately minimal: a ground plane sized to the simulation's map,
# two lights, and a player start. Everything else in the match -- bases, ore, units --
# is spawned at runtime by URA4SimWorldSubsystem from the simulation, so the level
# holds no gameplay state and can be regenerated from this script at any time.
#
# Run headless:
#   UnrealEditor-Cmd RedAlert4.uproject -ExecutePythonScript="Tools/Editor/make_skirmish_map.py"

import unreal

# Must match FRA4MatchBootstrap::kMapTiles and RA4::kTileSizeUnits.
MAP_TILES = 64
TILE_UNITS = 200
MAP_EXTENT = MAP_TILES * TILE_UNITS          # 12800 uu == 128 m
MAP_CENTRE = MAP_EXTENT * 0.5

LEVEL_PATH = "/Game/Maps/RA4_Skirmish"


def log(message):
    unreal.log("[RA4 map] {}".format(message))


def spawn(actor_class, location, rotation=None):
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return subsystem.spawn_actor_from_class(
        actor_class,
        location,
        rotation if rotation is not None else unreal.Rotator(0.0, 0.0, 0.0),
    )


def build():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Deleting an asset while its world is loaded trips an engine assertion in UE
    # 5.6. Reuse the package and clear its authored actors instead. The match state
    # itself is runtime-only, so this remains a deterministic regeneration.
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        log("loading existing {}".format(LEVEL_PATH))
        if not level_editor.load_level(LEVEL_PATH):
            raise RuntimeError("load_level failed for {}".format(LEVEL_PATH))
        for actor in actor_subsystem.get_all_level_actors():
            actor_subsystem.destroy_actor(actor)
    else:
        log("creating {}".format(LEVEL_PATH))
        if not level_editor.new_level(LEVEL_PATH):
            raise RuntimeError("new_level failed for {}".format(LEVEL_PATH))

    # This map uses only movable lights. Disable baked lighting at the source so a
    # regenerated level never opens with stale "lighting needs to be rebuilt"
    # interactions.
    world = unreal.get_editor_subsystem(
        unreal.UnrealEditorSubsystem
    ).get_editor_world()
    world.get_world_settings().set_editor_property(
        "force_no_precomputed_lighting", True
    )

    # --- ground -------------------------------------------------------------
    # The engine cube is 100 uu, so the scale is the extent in metres. The plate is
    # sunk by half its thickness so its top surface sits exactly on Z = 0, which is
    # the plane RA4Coords::IntersectGroundPlane deprojects the cursor onto.
    thickness = 1.0
    floor = spawn(
        unreal.StaticMeshActor,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, -thickness * 50.0),
    )
    floor.set_actor_label("RA4_Ground")
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    material = unreal.load_asset("/Game/RA4/Materials/M_RA4Ground_Lit.M_RA4Ground_Lit")
    if material is None:
        material = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
    mesh_component = floor.static_mesh_component
    mesh_component.set_static_mesh(cube)
    if material is not None:
        mesh_component.set_material(0, material)
    floor.set_actor_scale3d(
        unreal.Vector(MAP_EXTENT / 100.0, MAP_EXTENT / 100.0, thickness)
    )
    # Static so it does not pay for movement updates it will never need.
    floor.set_mobility(unreal.ComponentMobility.STATIC)

    # --- lighting -----------------------------------------------------------
    # A sky atmosphere comes first: without one there is nothing for the sky light
    # to capture, so its real-time capture returns black and the whole scene sits in
    # near-darkness even with a sun in it.
    atmosphere = spawn(unreal.SkyAtmosphere, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 0.0))
    atmosphere.set_actor_label("RA4_SkyAtmosphere")

    sun = spawn(
        unreal.DirectionalLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 4000.0),
        unreal.Rotator(roll=0.0, pitch=-48.0, yaw=145.0),
    )
    sun.set_actor_label("RA4_Sun")
    # The convenience properties on light actors are not exposed to Python in 5.6;
    # fetch the component by class instead.
    sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_component is not None:
        # Daylight in lux. The previous value of 6 was roughly deep dusk, which is
        # why the first playable build looked empty rather than lit.
        sun_component.set_intensity(75000.0)
        sun_component.set_editor_property("atmosphere_sun_light", True)
        sun_component.set_mobility(unreal.ComponentMobility.MOVABLE)

    sky = spawn(unreal.SkyLight, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 2000.0))
    sky.set_actor_label("RA4_SkyLight")
    sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
    if sky_component is not None:
        sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        sky_component.set_editor_property("real_time_capture", True)
        # Ambient fill so units in shadow stay readable; a top-down RTS has no other
        # light source to separate a tank from the ground.
        sky_component.set_intensity(1.0)

    # --- player start -------------------------------------------------------
    # ARA4CameraPawn positions itself from the camera controller, but a PlayerStart
    # keeps GameMode from logging a spawn warning on every launch.
    start = spawn(
        unreal.PlayerStart, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 100.0)
    )
    start.set_actor_label("RA4_PlayerStart")

    log("saving")
    level_editor.save_current_level()
    log("done: ground {0}x{0} uu, centre ({1}, {1})".format(MAP_EXTENT, MAP_CENTRE))


build()
