"""
Import all RA4 VoxCPM unit voice lines, EVA lines, and music tracks as SoundWave assets in Unreal Engine.
"""

from pathlib import Path
import os
import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())


def make_asset_name(base_filename: str) -> str:
    """Sanitize asset name for Unreal Engine naming conventions."""
    name = "".join(c if c.isalnum() or c == '_' else '_' for c in base_filename)
    if name and name[0].isdigit():
        name = "SW_" + name
    return name


def import_directory_trees() -> None:
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    import_specs = [
        (PROJECT_ROOT / "Audio" / "Voice" / "Mastered", "/Game/RA4/Audio/Generated/Voice/Mastered"),
        (PROJECT_ROOT / "Audio" / "Voice" / "RU" / "Soviet" / "Runtime", "/Game/RA4/Audio/Generated/Voice/Soviet"),
        (PROJECT_ROOT / "Audio" / "Voice" / "RU" / "Alliance" / "Runtime", "/Game/RA4/Audio/Generated/Voice/Alliance"),
        (PROJECT_ROOT / "Audio" / "Voice" / "RU" / "Coalition" / "Runtime", "/Game/RA4/Audio/Generated/Voice/Coalition"),
        (PROJECT_ROOT / "Audio" / "Voice" / "RU" / "Chrono" / "Runtime", "/Game/RA4/Audio/Generated/Voice/Chrono"),
        (PROJECT_ROOT / "Content" / "RA4" / "Audio" / "EVA" / "Processed", "/Game/RA4/Audio/Generated/EVA"),
        (PROJECT_ROOT / "Audio" / "Music", "/Game/RA4/Audio/Generated/Music"),
        (PROJECT_ROOT / "Assets" / "RA4UI" / "Audio", "/Game/RA4UI/Audio"),
    ]

    total_imported = 0
    total_skipped = 0
    total_failed = 0

    for source_dir, dest_package_root in import_specs:
        if not source_dir.exists():
            continue

        wav_files = list(source_dir.glob("**/*.wav")) + list(source_dir.glob("**/*.mp3"))
        if not wav_files:
            continue

        unreal.log(f"Importing {len(wav_files)} audio files from {source_dir} -> {dest_package_root}...")

        tasks = []
        for wav_path in wav_files:
            rel_path = wav_path.relative_to(source_dir)
            rel_dir = rel_path.parent
            asset_name = make_asset_name(wav_path.stem)

            package_dir = dest_package_root
            if str(rel_dir) != ".":
                package_dir = f"{dest_package_root}/{rel_dir}"

            full_package_path = f"{package_dir}/{asset_name}"
            if unreal.EditorAssetLibrary.does_asset_exist(full_package_path):
                total_skipped += 1
                continue

            task = unreal.AssetImportTask()
            task.filename = str(wav_path)
            task.destination_path = package_dir
            task.destination_name = asset_name
            task.automated = True
            task.replace_existing = True
            task.save = True
            tasks.append(task)

        if tasks:
            asset_tools.import_asset_tasks(tasks)
            for t in tasks:
                if t.imported_object_paths:
                    total_imported += 1
                else:
                    total_failed += 1

    unreal.log(f"RA4 Audio Import Complete: {total_imported} imported, {total_skipped} skipped (already present), {total_failed} failed.")


if __name__ == "__main__":
    import_directory_trees()

