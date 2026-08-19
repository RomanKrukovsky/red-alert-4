
import unreal

level = '/Game/Maps/RA4_Skirmish_Production'
if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(level):
    print('Failed to load level', level)
else:
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = eas.get_all_level_actors()
    print('Loaded level with', len(actors), 'actors')
    for a in actors:
        label = a.get_actor_label()
        cls = a.get_class().get_name()
        if 'Landscape' in cls or 'Landscape' in label or 'Ground' in label or 'Terrain' in label:
            print('Actor:', label, 'Class:', cls)
            if hasattr(a, 'static_mesh_component') and a.static_mesh_component:
                sm = a.static_mesh_component.static_mesh
                print('  StaticMesh:', sm.get_path_name() if sm else 'None')
                for i in range(a.static_mesh_component.get_num_materials()):
                    mat = a.static_mesh_component.get_material(i)
                    print('  Material', i, ':', mat.get_path_name() if mat else 'None')
            if 'Landscape' in cls:
                if hasattr(a, 'landscape_material'):
                    print('  LandscapeMaterial:', a.landscape_material.get_path_name() if a.landscape_material else 'None')
