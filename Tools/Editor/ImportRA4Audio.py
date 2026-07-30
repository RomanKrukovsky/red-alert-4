"""Import the trimmed main-menu theme as a SoundWave asset."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
SOURCE_FILE = PROJECT_ROOT / "Assets" / "RA4UI" / "Audio" / "RA4_MainMenu_Theme.wav"
DESTINATION = "/Game/RA4UI/Audio"


def import_main_menu_theme() -> None:
    task = unreal.AssetImportTask()
    task.filename = str(SOURCE_FILE)
    task.destination_path = DESTINATION
    task.destination_name = "S_RA4_MainMenu_Theme"
    task.automated = True
    task.replace_existing = True
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError(f"Audio import failed: {SOURCE_FILE}")

    unreal.log(f"Imported RA4 main-menu theme: {task.imported_object_paths[0]}")


import_main_menu_theme()
