# Copyright (c) Red Alert 4 project.
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"

def inspect_details():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        print(f"FAILED to load {LEVEL_PATH}")
        return

    for a in actor_subsystem.get_all_level_actors():
        if isinstance(a, unreal.LandscapeProxy):
            print(f"Landscape Actor: {a.get_actor_label()} ({a.get_name()})")
            print(f"  Landscape Material: {a.landscape_material}")
            if a.landscape_material:
                print(f"  Landscape Material Path: {a.landscape_material.get_path_name()}")
                print(f"  Landscape Material Class: {a.landscape_material.get_class().get_name()}")
        elif isinstance(a, unreal.DirectionalLight):
            print(f"DirectionalLight: {a.get_actor_label()}")
            print(f"  Rotation: {a.get_actor_rotation()}")
            comp = a.get_component_by_class(unreal.DirectionalLightComponent)
            if comp:
                print(f"  Intensity: {comp.get_editor_property('intensity')}")
                print(f"  LightColor: {comp.get_editor_property('light_color')}")
                print(f"  AtmosphereSunLight: {comp.get_editor_property('atmosphere_sun_light')}")
                print(f"  AtmosphereSunLightIndex: {comp.get_editor_property('atmosphere_sun_light_index')}")
        elif isinstance(a, unreal.SkyLight):
            print(f"SkyLight: {a.get_actor_label()}")
            comp = a.get_component_by_class(unreal.SkyLightComponent)
            if comp:
                print(f"  Intensity: {comp.get_editor_property('intensity')}")
                print(f"  RealTimeCapture: {comp.get_editor_property('real_time_capture')}")
                print(f"  SourceType: {comp.get_editor_property('source_type')}")
        elif isinstance(a, unreal.PostProcessVolume):
            print(f"PostProcessVolume: {a.get_actor_label()}")
            s = a.settings
            print(f"  AutoExposureMinBrightness: {s.auto_exposure_min_brightness}")
            print(f"  AutoExposureMaxBrightness: {s.auto_exposure_max_brightness}")

if __name__ == "__main__":
    inspect_details()
