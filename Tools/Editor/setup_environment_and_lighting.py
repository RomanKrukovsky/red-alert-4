# Copyright (c) Red Alert 4 project.
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
MAP_EXTENT = 12800.0
MAP_CENTRE = 6400.0
SEA_LEVEL = 0.0

# Report to a file as well as the log. unreal.log output does not reach the
# commandlet's captured stdout under -unattended, so for three rounds this script
# appeared to run and say nothing while in fact not running at all. A check whose
# result you cannot see is not a check.
REPORT_PATH = "/tmp/ra4_env_report.txt"
_lines = []


def log(msg):
    _lines.append(str(msg))
    try:
        unreal.log(f"[RA4 Env Setup] {msg}")
    except Exception:
        pass


def _flush():
    try:
        with open(REPORT_PATH, "w") as fh:
            fh.write("\n".join(_lines) + "\n")
    except Exception:
        pass

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
        # Sun detached from the atmosphere, deliberately.
        #
        # With atmosphere_sun_light on, the SkyAtmosphere modulates this light by
        # its computed transmittance -- and measured here, that transmittance was
        # effectively zero: the lit pass contributed nothing and the map rendered
        # black, while r.SkyAtmosphere 0 blew the identical frame out to white.
        # Both planet transform modes were tried and neither changed it.
        #
        # Off, the directional light delivers its 75,000 lux straight to the
        # scene. The SkyAtmosphere still draws the sky; it just no longer stands
        # between the sun and the ground.
        safe_set_prop(sun_comp, "atmosphere_sun_light", False)
        safe_set_prop(sun_comp, "cast_shadows", True)
        safe_set_prop(sun_comp, "dynamic_shadow_distance_movable_light", 25000.0)
    log("Created RA4_Sun (Directional Light)")

    # 3. Sky Atmosphere
    # Planet surface placed BELOW the terrain, not at it.
    #
    # Anchoring the planet top at Z=0 puts it exactly at this map's sea level, so
    # every point of ground at or under the waterline -- which on an archipelago
    # is most of it -- sits inside the planet, and the atmosphere absorbs the
    # entire light path to it. That is the black ground with lit buildings: the
    # buildings stand at Z=76..511 and catch the sun, the ground does not.
    #
    # Measured on the way here: r.SkyAtmosphere 0 blows the same frame out to
    # white, so the sun was always delivering its 75,000 lux and the atmosphere
    # was eating it.
    ATMOSPHERE_FLOOR = -2000.0
    sky_atm_actor = actor_subsystem.spawn_actor_from_class(
        unreal.SkyAtmosphere,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, ATMOSPHERE_FLOOR),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    sky_atm_actor.set_actor_label("RA4_SkyAtmosphere")
    sky_atm_comp = sky_atm_actor.get_component_by_class(unreal.SkyAtmosphereComponent)
    if sky_atm_comp:
        # Pin where the planet is. This was left at whatever the component
        # defaulted to, and the arithmetic said it was wrong: at 75,000 lux with
        # exposure at EV100 14 the ground should be over-bright, and instead the
        # map rendered black. Sunlight reaching zero is what happens when the
        # sample point sits below the planet surface -- the atmosphere absorbs the
        # whole path. Turning the atmosphere off with r.SkyAtmosphere 0 blew the
        # same frame out to white, which confirms the light was there all along
        # and the atmosphere was eating it.
        #
        # PlanetTopAtAbsoluteWorldOrigin puts the planet surface at Z=0, which is
        # this project's sea level (SEA_LEVEL above), so the terrain sits on the
        # planet rather than inside it.
        safe_set_prop(sky_atm_comp, "transform_mode",
                      unreal.SkyAtmosphereTransformMode.PLANET_TOP_AT_COMPONENT_TRANSFORM)
        log("SkyAtmosphere: planet top at Z=%.0f, below the terrain" % ATMOSPHERE_FLOOR)
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
        # A sky dome must not cast shadows or take part in lighting. This one is a
        # sphere scaled 4000x centred on the map, so the entire playable area sits
        # inside it: with shadow casting left on, it puts the whole map in
        # permanent eclipse. That is the black interior with a correctly lit
        # border -- the border is the dome and the ocean plane seen from outside
        # the shadow, the interior is everything underneath it.
        #
        # Rendering with ShowFlag.Lighting 0 showed the landscape bright green,
        # which is what ruled the fog out: unlit draws BaseColor, and BaseColor
        # already has the fog multiplied into it.
        safe_set_prop(sm_comp, "cast_shadow", False)
        safe_set_prop(sm_comp, "b_cast_dynamic_shadow", False)
        safe_set_prop(sm_comp, "b_cast_static_shadow", False)
        safe_set_prop(sm_comp, "b_affect_distance_field_lighting", False)
        log("Created RA4_SkySphere Dome (shadow casting off)")

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
    import os
    EV = float(os.environ.get("RA4_EV", "14.0"))
    try:
        pp = pp_actor.get_editor_property("settings")
        # Histogram with min == max, not AEM_MANUAL. Manual mode ignores these two
        # and reads the camera's shutter/ISO/aperture instead, so setting them
        # there does nothing. Pinning min to max is what actually freezes the
        # exposure while still letting the engine compute it in EV100.
        #
        # And bias is not the same knob: it is a compensation in stops where
        # higher means brighter, while min/max are the EV100 the scene is exposed
        # for, where higher means darker. Setting both to one number -- which is
        # what the first attempt did -- has them pulling against each other.
        pp.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
        pp.set_editor_property("auto_exposure_bias", 0.0)
        pp.set_editor_property("auto_exposure_min_brightness", EV)
        pp.set_editor_property("auto_exposure_max_brightness", EV)
        for flag in ("override_auto_exposure_method",
                     "override_auto_exposure_bias",
                     "override_auto_exposure_min_brightness",
                     "override_auto_exposure_max_brightness"):
            pp.set_editor_property(flag, True)
        pp_actor.set_editor_property("settings", pp)
        log(f"Pinned exposure: manual, EV100 {EV}, min == max so fog cannot be exposed away")
    except Exception as exc:
        log(f"EXPOSURE NOT SET -- the level will render white: {exc}")

    log("Created RA4_PostProcessVolume")

    # 9. Landscape material -- ours first.
    #
    # This step used to assign CityPark's MI_Landscape unconditionally, which
    # undid RA4LayeredTerrainSetup every time it ran. That commandlet builds
    # M_RA4_TerrainLayered with the four paintable layers (dirt, sand, grass,
    # rock) and the fog-of-war nodes from ADR-0030; MI_Landscape is a third-party
    # material with neither. So the ground lost its layers and its fog, and
    # whichever of the two tools ran last decided what the map looked like.
    #
    # CityPark stays only as a fallback for a project state where our own material
    # has not been generated yet. It is also one of the three undocumented packs in
    # the licence audit, so depending on it by default is a commercial risk as well
    # as a visual regression.
    mat_ground = unreal.load_asset("/Game/RA4/Generated/Terrain/M_RA4_TerrainLayered")
    source = "project"
    if mat_ground is None:
        mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Landscape")
        source = "CityPark fallback -- run RA4LayeredTerrainSetup to get the real one"
    if mat_ground is None:
        mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01")
        source = "CityPark fallback -- run RA4LayeredTerrainSetup to get the real one"

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.LandscapeProxy):
            if mat_ground:
                actor.set_editor_property("landscape_material", mat_ground)
                log(f"Landscape material set to {mat_ground.get_path_name()} ({source})")
            else:
                log("NO LANDSCAPE MATERIAL FOUND -- the ground will render untextured")
            break

    # Save level
    level_editor.save_current_level()
    log("Environment and lighting setup complete and level saved!")

# Called unconditionally, and that matters: UnrealEditor-Cmd -run=pythonscript
# does not execute the file as "__main__", so the usual guard meant run() was
# never called. The commandlet still reported "Python script executed
# successfully" and exited 0, so every change this file makes -- exposure, the
# sky atmosphere transform, the landscape material -- silently did nothing while
# looking like it had worked. Three rounds of "still dark" came from exactly that.
import traceback

try:
    run()
except Exception:
    log("EXCEPTION:\n" + traceback.format_exc())
_flush()
