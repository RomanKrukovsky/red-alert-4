# Copyright (c) Red Alert 4 project. Verifies production skirmish map validity.

import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"

def verify():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError("[VERIFY] Failed to load production level {}".format(LEVEL_PATH))

    actors = actor_subsystem.get_all_level_actors()
    unreal.log_warning("[VERIFY] Loaded {} with {} actors".format(LEVEL_PATH, len(actors)))

    labels = {actor.get_actor_label() for actor in actors}

    required_labels = {
        "RA4_Landscape_MainGround",
        "RA4_Base1_Plateau",
        "RA4_Base2_Plateau",
        "RA4_Road_MainArterial",
        "RA4_Road_OuterFlank",
        "RA4_Cliff_NorthWestRidge",
        "RA4_Cliff_SouthEastRidge",
        "RA4_OreField_Safe_Base1",
        "RA4_OreField_Safe_Base2",
        "RA4_OreField_Contested_A",
        "RA4_OreField_Contested_B",
        "RA4_Sun",
        "RA4_SkyLight",
        "RA4_SkyAtmosphere",
        "RA4_ExponentialHeightFog",
        "RA4_PostProcessVolume",
        "RA4_PlayerStart_P0",
        "RA4_PlayerStart_P1",
        "RA4_NavMeshBoundsVolume",
    }

    missing = sorted(required_labels - labels)
    if missing:
        raise RuntimeError("[VERIFY] Map missing required actors: {}".format(", ".join(missing)))

    # Verify no objects placed under ground (Z < -100 uu)
    underground_actors = []
    for actor in actors:
        loc = actor.get_actor_location()
        if loc.z < -100.0:
            underground_actors.append(actor.get_actor_label())

    if underground_actors:
        raise RuntimeError("[VERIFY] Underground actors found (Z < -100): {}".format(", ".join(underground_actors)))

    # Verify Base 1 and Base 2 player starts
    p0 = next((a for a in actors if a.get_actor_label() == "RA4_PlayerStart_P0"), None)
    p1 = next((a for a in actors if a.get_actor_label() == "RA4_PlayerStart_P1"), None)
    if p0 is None or p1 is None:
        raise RuntimeError("[VERIFY] Player starts P0/P1 missing or invalid")

    # Verify sun intensity
    sun = next((a for a in actors if a.get_actor_label() == "RA4_Sun"), None)
    if sun is not None:
        sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp is not None:
            intensity = sun_comp.get_editor_property("intensity")
            if abs(intensity - 75000.0) > 1.0:
                raise RuntimeError("[VERIFY] Sun intensity is {}, expected 75000.0".format(intensity))

    unreal.log_warning("[VERIFY] SUCCESS: All production skirmish map checks passed for {}!".format(LEVEL_PATH))


if __name__ == "__main__":
    verify()
