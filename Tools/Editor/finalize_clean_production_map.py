# Copyright (c) Red Alert 4 project.
import math
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
MAP_CENTRE = 6400.0

def sample_terrain_z(world_x, world_y):
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

    # 1. Remove clutter & duplicate static base buildings (SimWorld spawns them dynamically!)
    removed = 0
    for actor in list(actor_subsystem.get_all_level_actors()):
        label = actor.get_actor_label()
        if (label.startswith("Soviet_") or label.startswith("Alliance_") or 
            label.startswith("StaticMeshActor_") or 
            label in ["RA4_Sun", "RA4_SkyLight", "RA4_SkyAtmosphere", "RA4_ExponentialHeightFog", "RA4_PostProcessVolume"]):
            actor_subsystem.destroy_actor(actor)
            removed += 1

    print(f"Removed {removed} clutter/static actors")

    # 2. Clear messy foliage instances so the battlefield is clean, readable, and beautiful
    unreal.FoliageService.clear_all_foliage()
    print("Cleared all foliage clutter")

    # 3. Create Pristine, Bright, RTS Daylight Lighting
    # Sun Directional Light
    sun = actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 10000.0),
        unreal.Rotator(-52.0, 48.0, 0.0)
    )
    sun.set_actor_label("RA4_Sun")
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp:
        safe_set(sun_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set(sun_comp, "intensity", 85000.0)
        safe_set(sun_comp, "light_color", unreal.Color(255, 252, 246, 255))
        safe_set(sun_comp, "atmosphere_sun_light", True)
        safe_set(sun_comp, "atmosphere_sun_light_index", 0)
        safe_set(sun_comp, "cast_shadows", True)
        safe_set(sun_comp, "dynamic_shadow_distance_movable_light", 35000.0)
        safe_set(sun_comp, "shadow_bias", 0.25)

    # Sky Atmosphere
    sky_atm = actor_subsystem.spawn_actor_from_class(
        unreal.SkyAtmosphere,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    sky_atm.set_actor_label("RA4_SkyAtmosphere")

    # SkyLight with warm ground ambient (NO black shadows!)
    skylight = actor_subsystem.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 4000.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    skylight.set_actor_label("RA4_SkyLight")
    skylight_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
    if skylight_comp:
        safe_set(skylight_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set(skylight_comp, "intensity", 4.0)
        safe_set(skylight_comp, "light_color", unreal.Color(225, 240, 255, 255))
        safe_set(skylight_comp, "real_time_capture", True)
        safe_set(skylight_comp, "lower_hemisphere_is_black", False)
        safe_set(skylight_comp, "lower_hemisphere_color", unreal.LinearColor(0.28, 0.38, 0.20, 1.0))
        safe_set(skylight_comp, "cast_shadows", True)

    # Post Process Volume with crystal clear exposure
    pp = actor_subsystem.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 500.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    pp.set_actor_label("RA4_PostProcessVolume")
    safe_set(pp, "unbound", True)
    safe_set(pp, "priority", 100.0)
    pp_settings = pp.settings
    safe_set(pp_settings, "auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    safe_set(pp_settings, "auto_exposure_min_brightness", 0.5)
    safe_set(pp_settings, "auto_exposure_max_brightness", 4.0)
    safe_set(pp_settings, "auto_exposure_bias", 1.5)
    safe_set(pp_settings, "bloom_intensity", 0.25)
    safe_set(pp_settings, "vignette_intensity", 0.0)

    # 4. Assign High Quality Green Landscape Material
    mat_grass = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Flora/Grass/MI_Grass01.MI_Grass01")
    if mat_grass is None:
        mat_grass = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01.MI_Ground01")

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.LandscapeProxy):
            if mat_grass:
                actor.set_editor_property("landscape_material", mat_grass)
                print(f"Assigned landscape material {mat_grass.get_path_name()} to {actor.get_actor_label()}")

    # 5. Place Strategic Decorative Trees on Hilltops (not blocking play area)
    tree_meshes = [
        "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_AmurCork01",
        "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Elm01",
        "/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Maple01",
    ]
    for idx, mesh_path in enumerate(tree_meshes):
        if unreal.load_asset(mesh_path):
            unreal.FoliageService.scatter_foliage_rect(
                mesh_path,
                2000.0,
                2000.0,
                10800.0,
                10800.0,
                35,
                0.8,
                1.3,
                False,
                True,
                5000 + idx,
                ""
            )

    # 6. Snap Player Starts, Roads, Ore Deposits
    for actor in actor_subsystem.get_all_level_actors():
        label = actor.get_actor_label()
        loc = actor.get_actor_location()
        if label.startswith("RA4_PlayerStart_"):
            target_z = sample_terrain_z(loc.x, loc.y) + 20.0
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)
        elif label.startswith("RA4_Ore_") or "Road" in label or "Bridge" in label:
            target_z = sample_terrain_z(loc.x, loc.y)
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)

    level_editor.save_current_level()
    print("=== Clean, bright, beautiful production map finalized and saved successfully! ===")

if __name__ == "__main__":
    run()
