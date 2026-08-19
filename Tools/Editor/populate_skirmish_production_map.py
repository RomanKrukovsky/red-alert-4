# Copyright (c) Red Alert 4 project.
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
MAP_EXTENT = 12800.0
MAP_CENTRE = 6400.0

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

def log(msg):
    unreal.log(f"[RA4 Foliage] {msg}")

def run():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError(f"Failed to load {LEVEL_PATH}")

    # 1. Clear old foliage
    unreal.FoliageService.clear_all_foliage()
    log("Cleared previous foliage")

    # 2. Scatter Grass Meshes across the map
    total_grass = 0
    for idx, mesh_path in enumerate(GRASS_MESHES):
        if unreal.load_asset(mesh_path) is None:
            continue
        res = unreal.FoliageService.scatter_foliage_rect(
            mesh_path,
            1000.0,
            1000.0,
            11800.0,
            11800.0,
            1500,
            1.2,
            2.5,
            True,
            True,
            100 + idx,
            ""
        )
        total_grass += res.instances_added
        log(f"Scattered grass mesh {mesh_path.split('/')[-1]}: {res.instances_added} instances")

    log(f"Total grass instances placed: {total_grass}")

    # 3. Scatter Trees
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
            80,
            0.8,
            1.4,
            False,
            True,
            200 + idx,
            ""
        )
        total_trees += res.instances_added
        log(f"Scattered tree mesh {mesh_path.split('/')[-1]}: {res.instances_added} instances")

    log(f"Total tree instances placed: {total_trees}")

    # Save level
    level_editor.save_current_level()
    log("Level with rich foliage saved successfully!")

if __name__ == "__main__":
    run()
