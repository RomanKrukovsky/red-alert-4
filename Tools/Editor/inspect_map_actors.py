# Copyright (c) Red Alert 4 project.
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"

def inspect():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        print(f"FAILED to load {LEVEL_PATH}")
        return

    actors = actor_subsystem.get_all_level_actors()
    print(f"=== Level Actors in {LEVEL_PATH} (Total: {len(actors)}) ===")

    for a in actors:
        cls = a.get_class().get_name()
        label = a.get_actor_label()
        loc = a.get_actor_location()
        print(f"Actor: {label:35} | Class: {cls:30} | Loc: ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")

        if isinstance(a, unreal.DirectionalLight):
            comp = a.get_component_by_class(unreal.DirectionalLightComponent)
            if comp:
                print(f"   [DirectionalLight] Intensity: {comp.get_editor_property('intensity')}, Sun: {comp.get_editor_property('atmosphere_sun_light')}, Mobility: {comp.get_editor_property('mobility')}")
        elif isinstance(a, unreal.SkyLight):
            comp = a.get_component_by_class(unreal.SkyLightComponent)
            if comp:
                print(f"   [SkyLight] Intensity: {comp.get_editor_property('intensity')}, RealTimeCapture: {comp.get_editor_property('real_time_capture')}, Mobility: {comp.get_editor_property('mobility')}")
        elif isinstance(a, unreal.LandscapeProxy):
            print(f"   [Landscape] Mat: {a.landscape_material.get_path_name() if a.landscape_material else 'None'}")
        elif isinstance(a, unreal.PostProcessVolume):
            print(f"   [PostProcessVolume] Unbound: {a.get_editor_property('unbound')}")
        elif isinstance(a, unreal.StaticMeshActor):
            sm = a.static_mesh_component.static_mesh
            print(f"   [StaticMeshActor] Mesh: {sm.get_path_name() if sm else 'None'}, Mat0: {a.static_mesh_component.get_material(0).get_path_name() if a.static_mesh_component.get_material(0) else 'None'}")

if __name__ == "__main__":
    inspect()
