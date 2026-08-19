# Copyright (c) Red Alert 4 project.
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"

def run():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError(f"Failed to load {LEVEL_PATH}")

    removed = 0
    for actor in list(actor_subsystem.get_all_level_actors()):
        cls_name = actor.get_class().get_name()
        label = actor.get_actor_label()
        if "WaterBrush" in cls_name or "WaterBrush" in label or "WaterZone" in cls_name or "WaterBody" in cls_name:
            actor_subsystem.destroy_actor(actor)
            removed += 1

    print(f"Removed {removed} editor-only water actors")
    level_editor.save_current_level()
    print("Level saved cleanly")

if __name__ == "__main__":
    run()
