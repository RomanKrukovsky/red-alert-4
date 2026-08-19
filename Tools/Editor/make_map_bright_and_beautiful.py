# Copyright (c) Red Alert 4 project.
import math
import random
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
MAP_EXTENT = 12800.0
MAP_CENTRE = 6400.0
SEA_LEVEL = 0.0

GRASS_MESHES = [
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_grass02",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_grass03",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_grass04",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_grass05",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass10_1",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass11_1",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass12_1",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Lawn01_1",
]

TREE_MESHES = [
    "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_AmurCork01",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Elm01",
    "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Maple01",
]

def sample_terrain_z(world_x, world_y):
    """Deterministic mathematical height formula matching RA4LandscapeCommandlet rolling hills."""
    Seed = 20260730
    SeedPhase = float(Seed % 1000) * 0.01
    v = 0.0
    v += math.sin(world_x * 0.00028 + SeedPhase) * math.cos(world_y * 0.00024 + SeedPhase * 1.7)
    v += 0.5 * math.sin(world_x * 0.0006 - SeedPhase * 0.5) * math.cos(world_y * 0.0005 + SeedPhase)
    v += 0.25 * math.sin((world_x + world_y) * 0.0009 + SeedPhase * 2.0)
    height_norm = v / 1.75
    amplitude_units = 220.0
    return height_norm * amplitude_units

def safe_set(obj, name, val):
    try:
        obj.set_editor_property(name, val)
    except Exception:
        try:
            setattr(obj, name, val)
        except Exception:
            pass

def run():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError(f"Failed to load level {LEVEL_PATH}")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    # 1. Recreate Lighting & Sky Setup (BRIGHT & CRISP)
    for a in list(actor_subsystem.get_all_level_actors()):
        lbl = a.get_actor_label()
        if lbl in ["RA4_Sun", "RA4_SkyLight", "RA4_SkyAtmosphere", "RA4_ExponentialHeightFog", "RA4_PostProcessVolume"]:
            actor_subsystem.destroy_actor(a)

    # Sun Directional Light
    # Shining from overhead-left at 55 deg pitch, 60 deg yaw
    sun = actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 8000.0),
        unreal.Rotator(-55.0, 60.0, 0.0)
    )
    sun.set_actor_label("RA4_Sun")
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp:
        safe_set(sun_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set(sun_comp, "intensity", 85000.0)
        safe_set(sun_comp, "light_color", unreal.Color(255, 252, 245, 255))
        safe_set(sun_comp, "atmosphere_sun_light", True)
        safe_set(sun_comp, "atmosphere_sun_light_index", 0)
        safe_set(sun_comp, "cast_shadows", True)
        safe_set(sun_comp, "dynamic_shadow_distance_movable_light", 30000.0)
        safe_set(sun_comp, "cascade_distribution_exponent", 2.0)
        safe_set(sun_comp, "shadow_bias", 0.3)
    print("Created bright RA4_Sun DirectionalLight")

    # Sky Atmosphere
    sky_atm = actor_subsystem.spawn_actor_from_class(
        unreal.SkyAtmosphere,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    sky_atm.set_actor_label("RA4_SkyAtmosphere")

    # SkyLight with lower hemisphere illumination (NO black shadows!)
    skylight = actor_subsystem.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 3000.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    skylight.set_actor_label("RA4_SkyLight")
    skylight_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
    if skylight_comp:
        safe_set(skylight_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set(skylight_comp, "intensity", 3.5)
        safe_set(skylight_comp, "light_color", unreal.Color(220, 238, 255, 255))
        safe_set(skylight_comp, "real_time_capture", True)
        safe_set(skylight_comp, "lower_hemisphere_is_black", False)
        safe_set(skylight_comp, "lower_hemisphere_color", unreal.LinearColor(0.25, 0.32, 0.18, 1.0))
        safe_set(skylight_comp, "cast_shadows", True)
    print("Created RA4_SkyLight with ground bounce ambient")

    # Exponential Height Fog (Light morning/day haze)
    fog = actor_subsystem.spawn_actor_from_class(
        unreal.ExponentialHeightFog,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, -150.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    fog.set_actor_label("RA4_ExponentialHeightFog")
    fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_comp:
        safe_set(fog_comp, "fog_density", 0.001)
        safe_set(fog_comp, "fog_inscattering_luminance", unreal.LinearColor(0.7, 0.82, 0.95, 1.0))
        safe_set(fog_comp, "directional_inscattering_exponent", 8.0)
        safe_set(fog_comp, "b_enable_volumetric_fog", True)

    # Post Process Volume with bright, vivid exposure
    pp = actor_subsystem.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 500.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    pp.set_actor_label("RA4_PostProcessVolume")
    safe_set(pp, "unbound", True)
    safe_set(pp, "priority", 10.0)
    pp_settings = pp.settings
    safe_set(pp_settings, "auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    safe_set(pp_settings, "auto_exposure_min_brightness", 0.2)
    safe_set(pp_settings, "auto_exposure_max_brightness", 5.0)
    safe_set(pp_settings, "auto_exposure_bias", 1.2)
    safe_set(pp_settings, "bloom_intensity", 0.4)
    safe_set(pp_settings, "vignette_intensity", 0.1)

    # 2. Assign Bright Grass/Ground Material to Landscape
    mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Flora/Grass/MI_Grass01.MI_Grass01")
    if mat_ground is None:
        mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01.MI_Ground01")

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.LandscapeProxy):
            if mat_ground:
                actor.set_editor_property("landscape_material", mat_ground)
                print(f"Assigned landscape material {mat_ground.get_path_name()} to {actor.get_actor_label()}")

    # 3. Rescale Grass Foliage to Natural RTS Size (scale 0.25 to 0.50, NOT 2.5 meters!)
    unreal.FoliageService.clear_all_foliage()
    print("Cleared oversized foliage")

    total_grass = 0
    for idx, mesh_path in enumerate(GRASS_MESHES):
        if unreal.load_asset(mesh_path) is None:
            continue
        res = unreal.FoliageService.scatter_foliage_rect(
            mesh_path,
            1200.0,
            1200.0,
            11600.0,
            11600.0,
            2500,
            0.25, # min_scale (25-30 cm lawn grass)
            0.48, # max_scale (45-50 cm lawn grass)
            True, # align to normal
            True, # random yaw
            1000 + idx,
            ""
        )
        total_grass += res.instances_added

    print(f"Scattered {total_grass} properly-scaled lawn grass instances")

    # Trees around hills
    total_trees = 0
    for idx, mesh_path in enumerate(TREE_MESHES):
        if unreal.load_asset(mesh_path) is None:
            continue
        res = unreal.FoliageService.scatter_foliage_rect(
            mesh_path,
            1500.0,
            1500.0,
            11300.0,
            11300.0,
            70,
            0.7,
            1.2,
            False,
            True,
            2000 + idx,
            ""
        )
        total_trees += res.instances_added
    print(f"Scattered {total_trees} trees")

    # 4. Snap All Buildings & PlayerStarts to the Terrain Surface
    for actor in actor_subsystem.get_all_level_actors():
        label = actor.get_actor_label()
        loc = actor.get_actor_location()

        if label.startswith("Soviet_") or label.startswith("Alliance_") or "Building" in label:
            target_z = sample_terrain_z(loc.x, loc.y)
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)

        elif label.startswith("RA4_PlayerStart_"):
            target_z = sample_terrain_z(loc.x, loc.y) + 30.0
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)

        elif "Road" in label or "Bridge" in label:
            target_z = sample_terrain_z(loc.x, loc.y)
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)

        elif label.startswith("RA4_Ore_"):
            target_z = sample_terrain_z(loc.x, loc.y)
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)

    level_editor.save_current_level()
    print("=== Level saved with bright daylight, ground bounce ambient, and properly scaled grass ===")

if __name__ == "__main__":
    run()
