# Diagnostic script to dump all lighting, post-process, fog, and exposure parameters
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
OUT_PATH = "/tmp/ra4_diag.txt"

def safe_get(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception:
        try:
            return getattr(obj, name)
        except Exception:
            return "N/A"

def diagnose():
    lines = []
    def p(text=""):
        print(text)
        lines.append(str(text))

    p("================ RENDERING DIAGNOSTIC REPORT ================")
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        p(f"FAILED TO LOAD LEVEL: {LEVEL_PATH}")
        return

    actors = actor_subsystem.get_all_level_actors()
    p(f"Total actors in level: {len(actors)}")

    # 1. Inspect Directional Lights
    dir_lights = [a for a in actors if isinstance(a, unreal.DirectionalLight)]
    p(f"\n--- Directional Lights (Count: {len(dir_lights)}) ---")
    for a in dir_lights:
        comp = a.get_component_by_class(unreal.DirectionalLightComponent)
        p(f"Actor: {a.get_actor_label()}")
        p(f"  Intensity: {safe_get(comp, 'intensity')}")
        p(f"  LightColor: {safe_get(comp, 'light_color')}")
        p(f"  AtmosphereSunLight: {safe_get(comp, 'atmosphere_sun_light')}")
        p(f"  CastShadows: {safe_get(comp, 'cast_shadows')}")

    # 2. Inspect Sky Lights
    sky_lights = [a for a in actors if isinstance(a, unreal.SkyLight)]
    p(f"\n--- Sky Lights (Count: {len(sky_lights)}) ---")
    for a in sky_lights:
        comp = a.get_component_by_class(unreal.SkyLightComponent)
        p(f"Actor: {a.get_actor_label()}")
        p(f"  Intensity: {safe_get(comp, 'intensity')}")
        p(f"  LightColor: {safe_get(comp, 'light_color')}")
        p(f"  RealTimeCapture: {safe_get(comp, 'real_time_capture')}")
        p(f"  LowerHemisphereIsBlack: {safe_get(comp, 'lower_hemisphere_is_black')}")
        p(f"  LowerHemisphereColor: {safe_get(comp, 'lower_hemisphere_color')}")

    # 3. Inspect Exponential Height Fog
    fogs = [a for a in actors if isinstance(a, unreal.ExponentialHeightFog)]
    p(f"\n--- Exponential Height Fog (Count: {len(fogs)}) ---")
    for a in fogs:
        comp = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
        p(f"Actor: {a.get_actor_label()}")
        p(f"  FogDensity: {safe_get(comp, 'fog_density')}")
        p(f"  FogInscatteringLuminance: {safe_get(comp, 'fog_inscattering_luminance')}")
        p(f"  VolumetricFog: {safe_get(comp, 'volumetric_fog_emissive')}")
        p(f"  Location: {a.get_actor_location()}")

    # 4. Inspect Post Process Volumes
    pps = [a for a in actors if isinstance(a, unreal.PostProcessVolume)]
    p(f"\n--- Post Process Volumes (Count: {len(pps)}) ---")
    for a in pps:
        s = safe_get(a, "settings")
        p(f"Actor: {a.get_actor_label()}")
        p(f"  Unbound: {safe_get(a, 'unbound')}")
        p(f"  Priority: {safe_get(a, 'priority')}")
        p(f"  AutoExposureMethod: {safe_get(s, 'auto_exposure_method')}")
        p(f"  AutoExposureMinBrightness: {safe_get(s, 'auto_exposure_min_brightness')}")
        p(f"  AutoExposureMaxBrightness: {safe_get(s, 'auto_exposure_max_brightness')}")
        p(f"  AutoExposureBias: {safe_get(s, 'auto_exposure_bias')}")
        p(f"  BloomIntensity: {safe_get(s, 'bloom_intensity')}")

    # 5. Inspect Sky Atmosphere
    sky_atms = [a for a in actors if isinstance(a, unreal.SkyAtmosphere)]
    p(f"\n--- Sky Atmosphere (Count: {len(sky_atms)}) ---")
    for a in sky_atms:
        comp = a.get_component_by_class(unreal.SkyAtmosphereComponent)
        p(f"Actor: {a.get_actor_label()}")
        p(f"  TransformMode: {safe_get(comp, 'transform_mode')}")
        p(f"  Location: {a.get_actor_location()}")

    # 6. Check all other actors in level
    p(f"\n--- Other Lighting / Mesh Actors ---")
    for a in actors:
        lbl = a.get_actor_label()
        cls = a.get_class().get_name()
        if "Light" in cls or "Fog" in cls or "Volume" in cls or "Atmosphere" in cls or "Sky" in lbl or "Sun" in lbl or "Ocean" in lbl:
            p(f"  [{cls}] {lbl} at {a.get_actor_location()}")

    p("=============================================================")
    with open(OUT_PATH, "w") as f:
        f.write("\n".join(lines) + "\n")

if __name__ == "__main__":
    diagnose()
