"""Imports the original visual scenes used by the native RA4 showcase UI."""

import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "Assets", "RA4UI", "Generated")
DESTINATION = "/Game/RA4UI/Art"

ARTWORK = (
    ("UnitIcons/Soviet/SU_Conscript.png", "T_RA4_SU_Conscript"),
    ("UnitIcons/Soviet/SU_FlakTrooper.png", "T_RA4_SU_FlakTrooper"),
    ("UnitIcons/Soviet/SU_HammerTank.png", "T_RA4_SU_HammerTank"),
    ("UnitIcons/Soviet/SU_MiG41.png", "T_RA4_SU_MiG41"),
    ("UnitIcons/Soviet/SU_SickleScout.png", "T_RA4_SU_SickleScout"),
    ("UnitIcons/Soviet/SU_TyphoonSub.png", "T_RA4_SU_TyphoonSub"),
    ("UnitIcons/Alliance/AL_Guardian.png", "T_RA4_AL_Guardian"),
    ("UnitIcons/Alliance/AL_Harrier.png", "T_RA4_AL_Harrier"),
    ("UnitIcons/Alliance/AL_Javelin.png", "T_RA4_AL_Javelin"),
    ("UnitIcons/Alliance/AL_Mirage.png", "T_RA4_AL_Mirage"),
    ("UnitIcons/Alliance/AL_Peacekeeper.png", "T_RA4_AL_Peacekeeper"),
    ("UnitIcons/Alliance/AL_Poseidon.png", "T_RA4_AL_Poseidon"),
    ("ra4_ussr_campaign_commander_v2.png", "T_RA4_USSR_CampaignCommander"),
    ("ra4_allies_campaign_commander_v2.png", "T_RA4_Allies_CampaignCommander"),
    ("ra4_eastern_campaign_commander_v2.png", "T_RA4_Eastern_CampaignCommander"),
    ("ra4_chrono_campaign_commander_v2.png", "T_RA4_Chrono_CampaignCommander"),
    ("ra4_ussr_loading_kyiv_v2.png", "T_RA4_USSR_LoadingKyiv"),
    ("ra4_ussr_mission_map_v2.png", "T_RA4_USSR_MissionMap"),
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
