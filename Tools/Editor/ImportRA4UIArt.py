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
    ("ra4_frame_button_normal_v2.png", "T_RA4_Frame_ButtonNormalV2"),
    ("ra4_frame_button_hovered_v2.png", "T_RA4_Frame_ButtonHoveredV2"),
    ("ra4_frame_panel_v2.png", "T_RA4_Frame_PanelV2"),
    ("ra4_frame_minimap_v2.png", "T_RA4_Frame_MinimapV2"),
    ("ra4_frame_resource_bar_v2.png", "T_RA4_Frame_ResourceBarV2"),
    ("ra4_frame_tabs_v2.png", "T_RA4_Frame_TabsV2"),
    ("ra4_frame_progress_v2.png", "T_RA4_Frame_ProgressV2"),
    ("ra4_frame_unit_card_v2.png", "T_RA4_Frame_UnitCardV2"),
    ("ra4_ui_button_pressed_v1.png", "T_RA4_UI_ButtonPressed"),
    ("ra4_ui_button_disabled_v1.png", "T_RA4_UI_ButtonDisabled"),
    ("ra4_ui_button_back_v1.png", "T_RA4_UI_ButtonBack"),
    ("ra4_ui_button_primary_v1.png", "T_RA4_UI_ButtonPrimary"),
    ("ra4_ui_button_secondary_v1.png", "T_RA4_UI_ButtonSecondary"),
    ("ra4_ui_button_icon_square_v1.png", "T_RA4_UI_ButtonIconSquare"),
    ("ra4_ui_panel_compact_v1.png", "T_RA4_UI_PanelCompact"),
    ("ra4_ui_panel_tall_v1.png", "T_RA4_UI_PanelTall"),
    ("ra4_ui_panel_hero_v1.png", "T_RA4_UI_PanelHero"),
    ("ra4_ui_modal_frame_v1.png", "T_RA4_UI_ModalFrame"),
    ("ra4_ui_tooltip_frame_v1.png", "T_RA4_UI_TooltipFrame"),
    ("ra4_ui_notification_frame_v1.png", "T_RA4_UI_NotificationFrame"),
    ("ra4_ui_objectives_frame_v1.png", "T_RA4_UI_ObjectivesFrame"),
    ("ra4_ui_queue_frame_v1.png", "T_RA4_UI_QueueFrame"),
    ("ra4_ui_chat_frame_v1.png", "T_RA4_UI_ChatFrame"),
    ("ra4_ui_lobby_slot_v1.png", "T_RA4_UI_LobbySlot"),
    ("ra4_ui_map_preview_frame_v1.png", "T_RA4_UI_MapPreviewFrame"),
    ("ra4_ui_portrait_frame_v1.png", "T_RA4_UI_PortraitFrame"),
    ("ra4_ui_faction_badge_frame_v1.png", "T_RA4_UI_FactionBadgeFrame"),
    ("ra4_ui_build_card_v1.png", "T_RA4_UI_BuildCard"),
    ("ra4_ui_ability_slot_v1.png", "T_RA4_UI_AbilitySlot"),
    ("ra4_ui_command_bar_v1.png", "T_RA4_UI_CommandBar"),
    ("ra4_ui_scrollbar_v1.png", "T_RA4_UI_Scrollbar"),
    ("ra4_ui_divider_v1.png", "T_RA4_UI_Divider"),
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
