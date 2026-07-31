# Copyright (c) Red Alert 4 project.
# Builds the production skirmish level /Game/Maps/RA4_Skirmish_Production without opening the editor UI.

import unreal

MAP_TILES = 64
TILE_UNITS = 200
MAP_EXTENT = MAP_TILES * TILE_UNITS          # 12800 uu == 128 m
MAP_CENTRE = MAP_EXTENT * 0.5

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"


def log(message):
    unreal.log("[RA4 Production Map] {}".format(message))


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

    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        log("Loading existing map {}".format(LEVEL_PATH))
        if not level_editor.load_level(LEVEL_PATH):
            raise RuntimeError("load_level failed for {}".format(LEVEL_PATH))
        for actor in actor_subsystem.get_all_level_actors():
            actor_subsystem.destroy_actor(actor)
    else:
        log("Creating new map {}".format(LEVEL_PATH))
        if not level_editor.new_level(LEVEL_PATH):
            raise RuntimeError("new_level failed for {}".format(LEVEL_PATH))

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    # Load existing materials or fallback basic material
    mat_ground = unreal.load_asset("/Game/RA4/Materials/M_RA4Ground_Lit.M_RA4Ground_Lit")
    if mat_ground is None:
        mat_ground = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")

    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    cylinder_mesh = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")

    # --- 1. Main Ground Landscape Base ---
    ground = spawn(unreal.StaticMeshActor, unreal.Vector(MAP_CENTRE, MAP_CENTRE, -10.0))
    ground.set_actor_label("RA4_Landscape_MainGround")
    g_comp = ground.static_mesh_component
    g_comp.set_static_mesh(cube_mesh)
    if mat_ground:
        g_comp.set_material(0, mat_ground)
    ground.set_actor_scale3d(unreal.Vector(MAP_EXTENT / 100.0, MAP_EXTENT / 100.0, 0.2))
    ground.set_mobility(unreal.ComponentMobility.STATIC)

    # --- 2. Base Plateaus (Base 1 & Base 2) ---
    base1_pos = unreal.Vector(2400.0, 2400.0, 0.0)
    base2_pos = unreal.Vector(10400.0, 10400.0, 0.0)

    plat1 = spawn(unreal.StaticMeshActor, base1_pos)
    plat1.set_actor_label("RA4_Base1_Plateau")
    p1_comp = plat1.static_mesh_component
    p1_comp.set_static_mesh(cube_mesh)
    if mat_ground:
        p1_comp.set_material(0, mat_ground)
    plat1.set_actor_scale3d(unreal.Vector(36.0, 36.0, 0.1)) # 3600 x 3600 uu base pad
    plat1.set_mobility(unreal.ComponentMobility.STATIC)

    plat2 = spawn(unreal.StaticMeshActor, base2_pos)
    plat2.set_actor_label("RA4_Base2_Plateau")
    p2_comp = plat2.static_mesh_component
    p2_comp.set_static_mesh(cube_mesh)
    if mat_ground:
        p2_comp.set_material(0, mat_ground)
    plat2.set_actor_scale3d(unreal.Vector(36.0, 36.0, 0.1))
    plat2.set_mobility(unreal.ComponentMobility.STATIC)

    # --- 3. Roads Network ---
    road_center = spawn(unreal.StaticMeshActor, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 1.0), unreal.Rotator(0, 45, 0))
    road_center.set_actor_label("RA4_Road_MainArterial")
    rc_comp = road_center.static_mesh_component
    rc_comp.set_static_mesh(cube_mesh)
    if mat_ground:
        rc_comp.set_material(0, mat_ground)
    road_center.set_actor_scale3d(unreal.Vector(110.0, 8.0, 0.02))
    road_center.set_mobility(unreal.ComponentMobility.STATIC)

    road_flank = spawn(unreal.StaticMeshActor, unreal.Vector(4800.0, 8000.0, 1.0), unreal.Rotator(0, -45, 0))
    road_flank.set_actor_label("RA4_Road_OuterFlank")
    rf_comp = road_flank.static_mesh_component
    rf_comp.set_static_mesh(cube_mesh)
    if mat_ground:
        rf_comp.set_material(0, mat_ground)
    road_flank.set_actor_scale3d(unreal.Vector(70.0, 6.0, 0.02))
    road_flank.set_mobility(unreal.ComponentMobility.STATIC)

    # --- 4. Natural Cliff Boundaries & Central Choke Ridges ---
    ridge_nw = spawn(unreal.StaticMeshActor, unreal.Vector(5200.0, 7600.0, 150.0), unreal.Rotator(0, 30, 0))
    ridge_nw.set_actor_label("RA4_Cliff_NorthWestRidge")
    rnw_comp = ridge_nw.static_mesh_component
    rnw_comp.set_static_mesh(cube_mesh)
    if mat_ground:
        rnw_comp.set_material(0, mat_ground)
    ridge_nw.set_actor_scale3d(unreal.Vector(24.0, 8.0, 3.0))
    ridge_nw.set_mobility(unreal.ComponentMobility.STATIC)

    ridge_se = spawn(unreal.StaticMeshActor, unreal.Vector(7600.0, 5200.0, 150.0), unreal.Rotator(0, 30, 0))
    ridge_se.set_actor_label("RA4_Cliff_SouthEastRidge")
    rse_comp = ridge_se.static_mesh_component
    rse_comp.set_static_mesh(cube_mesh)
    if mat_ground:
        rse_comp.set_material(0, mat_ground)
    ridge_se.set_actor_scale3d(unreal.Vector(24.0, 8.0, 3.0))
    ridge_se.set_mobility(unreal.ComponentMobility.STATIC)

    # --- 5. Ore Deposits & Resource Fields ---
    safe_ore1 = spawn(unreal.StaticMeshActor, unreal.Vector(2400.0, 4200.0, 5.0))
    safe_ore1.set_actor_label("RA4_OreField_Safe_Base1")
    so1_comp = safe_ore1.static_mesh_component
    so1_comp.set_static_mesh(cylinder_mesh)
    if mat_ground:
        so1_comp.set_material(0, mat_ground)
    safe_ore1.set_actor_scale3d(unreal.Vector(8.0, 8.0, 0.1))
    safe_ore1.set_mobility(unreal.ComponentMobility.STATIC)

    safe_ore2 = spawn(unreal.StaticMeshActor, unreal.Vector(10400.0, 8600.0, 5.0))
    safe_ore2.set_actor_label("RA4_OreField_Safe_Base2")
    so2_comp = safe_ore2.static_mesh_component
    so2_comp.set_static_mesh(cylinder_mesh)
    if mat_ground:
        so2_comp.set_material(0, mat_ground)
    safe_ore2.set_actor_scale3d(unreal.Vector(8.0, 8.0, 0.1))
    safe_ore2.set_mobility(unreal.ComponentMobility.STATIC)

    cont_ore_a = spawn(unreal.StaticMeshActor, unreal.Vector(5000.0, 7800.0, 5.0))
    cont_ore_a.set_actor_label("RA4_OreField_Contested_A")
    coa_comp = cont_ore_a.static_mesh_component
    coa_comp.set_static_mesh(cylinder_mesh)
    if mat_ground:
        coa_comp.set_material(0, mat_ground)
    cont_ore_a.set_actor_scale3d(unreal.Vector(12.0, 12.0, 0.1))
    cont_ore_a.set_mobility(unreal.ComponentMobility.STATIC)

    cont_ore_b = spawn(unreal.StaticMeshActor, unreal.Vector(7800.0, 5000.0, 5.0))
    cont_ore_b.set_actor_label("RA4_OreField_Contested_B")
    cob_comp = cont_ore_b.static_mesh_component
    cob_comp.set_static_mesh(cylinder_mesh)
    if mat_ground:
        cob_comp.set_material(0, mat_ground)
    cont_ore_b.set_actor_scale3d(unreal.Vector(12.0, 12.0, 0.1))
    cont_ore_b.set_mobility(unreal.ComponentMobility.STATIC)

    # --- 6. Lighting & Atmosphere ---
    atmosphere = spawn(unreal.SkyAtmosphere, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 0.0))
    atmosphere.set_actor_label("RA4_SkyAtmosphere")

    sun = spawn(
        unreal.DirectionalLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 4000.0),
        unreal.Rotator(roll=0.0, pitch=-48.0, yaw=145.0),
    )
    sun.set_actor_label("RA4_Sun")
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp is not None:
        sun_comp.set_intensity(75000.0)
        sun_comp.set_editor_property("atmosphere_sun_light", True)
        sun_comp.set_mobility(unreal.ComponentMobility.MOVABLE)

    sky = spawn(unreal.SkyLight, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 2000.0))
    sky.set_actor_label("RA4_SkyLight")
    sky_comp = sky.get_component_by_class(unreal.SkyLightComponent)
    if sky_comp is not None:
        sky_comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        sky_comp.set_editor_property("real_time_capture", True)
        sky_comp.set_intensity(1.2)

    fog = spawn(unreal.ExponentialHeightFog, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 100.0))
    fog.set_actor_label("RA4_ExponentialHeightFog")

    pp = spawn(unreal.PostProcessVolume, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 500.0))
    pp.set_actor_label("RA4_PostProcessVolume")
    pp.set_editor_property("unbound", True)

    # --- 7. Player Starts ---
    start0 = spawn(unreal.PlayerStart, unreal.Vector(2400.0, 2400.0, 50.0), unreal.Rotator(0, 45, 0))
    start0.set_actor_label("RA4_PlayerStart_P0")

    start1 = spawn(unreal.PlayerStart, unreal.Vector(10400.0, 10400.0, 50.0), unreal.Rotator(0, -135, 0))
    start1.set_actor_label("RA4_PlayerStart_P1")

    # --- 8. NavMesh Bounds Volume ---
    nav = spawn(unreal.NavMeshBoundsVolume, unreal.Vector(MAP_CENTRE, MAP_CENTRE, 200.0))
    nav.set_actor_label("RA4_NavMeshBoundsVolume")
    nav.set_actor_scale3d(unreal.Vector(MAP_EXTENT / 200.0, MAP_EXTENT / 200.0, 10.0))

    log("Saving level package {}".format(LEVEL_PATH))
    level_editor.save_current_level()
    log("Production Skirmish Map build complete!")


if __name__ == "__main__":
    build()
