# Copyright (c) Red Alert 4 project.
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
MAP_EXTENT = 12800.0
MAP_CENTRE = 6400.0
SEA_LEVEL = 0.0

def log(msg):
    unreal.log(f"[RA4 Env Setup] {msg}")

def safe_set_prop(obj, prop_name, value):
    try:
        obj.set_editor_property(prop_name, value)
    except Exception as e:
        try:
            setattr(obj, prop_name, value)
        except Exception:
            pass

def run():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError(f"Failed to load level {LEVEL_PATH}")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    # 1. Clean up old/redundant environment actors
    for actor in list(actor_subsystem.get_all_level_actors()):
        label = actor.get_actor_label()
        if label in ["RA4_Sun", "RA4_SkyLight", "RA4_SkyAtmosphere", "RA4_ExponentialHeightFog",
                      "RA4_PostProcessVolume", "RA4_SkySphere", "RA4_Ocean_Horizon", "RA4_Water_OceanPlane"]:
            actor_subsystem.destroy_actor(actor)

    # 2. Directional Light (Sun)
    sun_actor = actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 5000.0),
        unreal.Rotator(-50.0, 45.0, 0.0)
    )
    sun_actor.set_actor_label("RA4_Sun")
    sun_comp = sun_actor.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp:
        safe_set_prop(sun_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set_prop(sun_comp, "intensity", 75000.0)
        safe_set_prop(sun_comp, "light_color", unreal.Color(255, 248, 235, 255))
        safe_set_prop(sun_comp, "atmosphere_sun_light", True)
        safe_set_prop(sun_comp, "atmosphere_sun_light_index", 0)
        safe_set_prop(sun_comp, "cast_shadows", True)
        safe_set_prop(sun_comp, "dynamic_shadow_distance_movable_light", 25000.0)
    log("Created RA4_Sun (Directional Light)")

    # 3. Sky Atmosphere
    sky_atm_actor = actor_subsystem.spawn_actor_from_class(
        unreal.SkyAtmosphere,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    sky_atm_actor.set_actor_label("RA4_SkyAtmosphere")
    log("Created RA4_SkyAtmosphere")

    # 4. SkyLight
    skylight_actor = actor_subsystem.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 2000.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    skylight_actor.set_actor_label("RA4_SkyLight")
    skylight_comp = skylight_actor.get_component_by_class(unreal.SkyLightComponent)
    if skylight_comp:
        safe_set_prop(skylight_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set_prop(skylight_comp, "intensity", 2.5)
        safe_set_prop(skylight_comp, "light_color", unreal.Color(230, 240, 255, 255))
        safe_set_prop(skylight_comp, "real_time_capture", True)
        safe_set_prop(skylight_comp, "cast_shadows", True)
    log("Created RA4_SkyLight with RealTimeCapture")

    # 5. Exponential Height Fog
    fog_actor = actor_subsystem.spawn_actor_from_class(
        unreal.ExponentialHeightFog,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, -100.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    fog_actor.set_actor_label("RA4_ExponentialHeightFog")
    fog_comp = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_comp:
        safe_set_prop(fog_comp, "fog_density", 0.002)
        safe_set_prop(fog_comp, "fog_inscattering_luminance", unreal.LinearColor(0.6, 0.75, 0.95, 1.0))
        safe_set_prop(fog_comp, "b_enable_volumetric_fog", True)
    log("Created RA4_ExponentialHeightFog")

    # 6. Sky Sphere Dome Mesh (360 sky sphere)
    sky_mesh = unreal.load_asset("/Engine/EngineSky/SM_SkySphere.SM_SkySphere")
    if sky_mesh is None:
        sky_mesh = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    sky_mat = unreal.load_asset("/Engine/EngineSky/M_Sky_Pan_Inst.M_Sky_Pan_Inst")

    if sky_mesh:
        sky_sphere = actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(MAP_CENTRE, MAP_CENTRE, 0.0),
            unreal.Rotator(0.0, 0.0, 0.0)
        )
        sky_sphere.set_actor_label("RA4_SkySphere")
        sm_comp = sky_sphere.static_mesh_component
        sm_comp.set_static_mesh(sky_mesh)
        if sky_mat:
            sm_comp.set_material(0, sky_mat)
        sky_sphere.set_actor_scale3d(unreal.Vector(4000.0, 4000.0, 4000.0))
        sky_sphere.set_mobility(unreal.ComponentMobility.STATIC)
        log("Created RA4_SkySphere Dome")

    # 7. Horizon Ocean Plane (extends 2000m x 2000m to the horizon)
    plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane.Plane")
    mat_water = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/BaseMaterial/M_WaterShader_V_01")
    if mat_water is None:
        mat_water = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Landscape")

    if plane_mesh:
        ocean_actor = actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(MAP_CENTRE, MAP_CENTRE, SEA_LEVEL - 20.0),
            unreal.Rotator(0.0, 0.0, 0.0)
        )
        ocean_actor.set_actor_label("RA4_Ocean_Horizon")
        oc_comp = ocean_actor.static_mesh_component
        oc_comp.set_static_mesh(plane_mesh)
        if mat_water:
            oc_comp.set_material(0, mat_water)
        ocean_actor.set_actor_scale3d(unreal.Vector(2000.0, 2000.0, 1.0))
        ocean_actor.set_mobility(unreal.ComponentMobility.STATIC)
        log("Created RA4_Ocean_Horizon plane (2km extent)")

    # 8. Post Process Volume
    pp_actor = actor_subsystem.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 500.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    pp_actor.set_actor_label("RA4_PostProcessVolume")
    safe_set_prop(pp_actor, "unbound", True)
    safe_set_prop(pp_actor, "priority", 1.0)

    # Exposure, pinned. The sun above is 75,000 lux -- a physical daylight value,
    # correct for a SkyAtmosphere sun. Without an exposure setting to match it the
    # frame is pure white: everything clamps at the top of the range and the map
    # is invisible. That is not a subtle grading problem, it is the whole screen.
    #
    # Fixed rather than automatic, and that is a gameplay decision, not a taste
    # one. Fog of war darkens large parts of the frame on purpose (ADR-0030), and
    # auto-exposure exists precisely to cancel that out -- it would brighten the
    # unexplored ground back up as the camera panned over it, handing the player a
    # readable picture of terrain the rules say they cannot see. Min == max means
    # the exposure never moves, so fog stays as dark as the fog decides.
    #
    # EV100 14 is the standard outdoor-daylight stop for this sun intensity.
    try:
        pp = pp_actor.get_editor_property("settings")
        pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
        pp.set_editor_property("auto_exposure_bias", 14.0)
        pp.set_editor_property("auto_exposure_min_brightness", 14.0)
        pp.set_editor_property("auto_exposure_max_brightness", 14.0)
        for flag in ("override_auto_exposure_method",
                     "override_auto_exposure_bias",
                     "override_auto_exposure_min_brightness",
                     "override_auto_exposure_max_brightness"):
            pp.set_editor_property(flag, True)
        pp_actor.set_editor_property("settings", pp)
        log("Pinned exposure: manual, EV100 14, min == max so fog cannot be exposed away")
    except Exception as exc:
        log(f"EXPOSURE NOT SET -- the level will render white: {exc}")

    log("Created RA4_PostProcessVolume")

    # 9. Verify Landscape Material
    mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Landscape")
    if mat_ground is None:
        mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01")

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.LandscapeProxy):
            if mat_ground:
                actor.set_editor_property("landscape_material", mat_ground)
                log(f"Verified landscape material: {mat_ground.get_path_name()}")
            break

    # Save level
    level_editor.save_current_level()
    log("Environment and lighting setup complete and level saved!")

if __name__ == "__main__":
    run()
