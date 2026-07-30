"""Imports the original visual scenes used by the native RA4 showcase UI."""

import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "Assets", "RA4UI", "Generated")
DESTINATION = "/Game/RA4UI/Art"

ARTWORK = (
    ("ra4_ussr_command_center_v1.png", "T_RA4_USSR_CommandCenter"),
    ("ra4_allies_arctic_fleet_v1.png", "T_RA4_Allies_ArcticFleet"),
    ("ra4_eastern_command_fortress_v1.png", "T_RA4_Eastern_CommandFortress"),
    ("ra4_chrono_temporal_citadel_v1.png", "T_RA4_Chrono_TemporalCitadel"),
    ("ra4_ussr_main_menu_chrome_v1.png", "T_RA4_USSR_MainMenuChrome"),
    ("ra4_logo_v1.png", "T_RA4_Logo"),
    ("ra4_ussr_main_menu_background_v1.png", "T_RA4_USSR_MainMenuBackground"),
    ("ra4_button_normal_v1.png", "T_RA4_Button_Normal"),
    ("ra4_button_hovered_v1.png", "T_RA4_Button_Hovered"),
    ("ra4_button_pressed_v1.png", "T_RA4_Button_Pressed"),
    ("ra4_button_disabled_v1.png", "T_RA4_Button_Disabled"),
    ("ra4_menu_icons_v2.png", "T_RA4_MenuIcons"),
    ("ra4_panel_gradient_ussr_v1.png", "T_RA4_PanelGradient_USSR"),
    ("ra4_campaign_reference_v1.png", "T_RA4_CampaignReference"),
)


def import_texture(filename, destination_name):
    source_file = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source_file):
        raise RuntimeError("Artwork source is missing: {}".format(source_file))

    task = unreal.AssetImportTask()
    task.filename = source_file
    task.destination_path = DESTINATION
    task.destination_name = destination_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    if not task.imported_object_paths:
        raise RuntimeError("Unreal did not import: {}".format(source_file))
    unreal.log("Imported UI artwork: {}".format(task.imported_object_paths[0]))


for source, asset_name in ARTWORK:
    import_texture(source, asset_name)

unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
unreal.log("RA4 UI artwork import complete.")
