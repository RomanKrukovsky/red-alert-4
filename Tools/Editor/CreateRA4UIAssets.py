import unreal


ASSET_PATH = "/Game/RA4UI/Widgets"
SHOWCASE_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4ShowcaseWidget")
SPLASH_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4SplashScreenWidget")
MAIN_MENU_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4MainMenuScreenWidget")
CAMPAIGN_SELECT_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4CampaignSelectWidget")
CAMPAIGN_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4CampaignScreenWidget")
MISSION_MAP_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4MissionMapScreenWidget")
BRIEFING_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4BriefingScreenWidget")
VIDEO_COMMS_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4VideoCommsScreenWidget")
LOADING_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4LoadingScreenWidget")
LOBBY_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4LobbyScreenWidget")
LOBBY_ROW_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4LobbyPlayerRowWidget")


def create_widget_blueprint(asset_name, parent_class, screen_index=None, default_values=None):
    asset_path = f"{ASSET_PATH}/{asset_name}"
    changed = False
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if asset.get_blueprint_parent_class() != parent_class:
            unreal.BlueprintEditorLibrary.reparent_blueprint(asset, parent_class)
            changed = True
    else:
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, ASSET_PATH, unreal.WidgetBlueprint, factory
        )
        changed = True

    if changed:
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)

    if screen_index is not None or default_values:
        generated_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        default_widget = unreal.get_default_object(generated_class)
        values = dict(default_values or {})
        if screen_index is not None:
            values["initial_screen_index"] = screen_index
        for property_name, value in values.items():
            if default_widget.get_editor_property(property_name) != value:
                default_widget.set_editor_property(property_name, value)
                changed = True

    if changed:
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def main():
    required_classes = [
        SHOWCASE_CLASS,
        SPLASH_CLASS,
        MAIN_MENU_CLASS,
        CAMPAIGN_SELECT_CLASS,
        CAMPAIGN_CLASS,
        MISSION_MAP_CLASS,
        BRIEFING_CLASS,
        VIDEO_COMMS_CLASS,
        LOADING_CLASS,
        LOBBY_CLASS,
        LOBBY_ROW_CLASS,
    ]
    if not all(required_classes):
        raise RuntimeError("Required RA4 UI classes were not loaded; compile the RA4UI module first.")

    screens = [
        ("WBP_RA4_Showcase", 0),
        ("WBP_RA4_HUD_USSR", 2),
        ("WBP_RA4_HUD_Allies", 14),
        ("WBP_RA4_HUD_Eastern", 15),
        ("WBP_RA4_HUD_Chrono", 16),
        ("WBP_RA4_Pause", 17),
        ("WBP_RA4_Victory", 18),
        ("WBP_RA4_Encyclopedia", 19),
        ("WBP_RA4_TechTree", 20),
        ("WBP_RA4_Mods", 21),
        ("WBP_RA4_Settings", 4),
        ("WBP_RA4_HUD_USSR_Battle", 23),
        ("WBP_RA4_HUD_USSR_Alert", 24),
        ("WBP_RA4_HUD_Allies_Naval", 25),
        ("WBP_RA4_HUD_Allies_Air", 26),
        ("WBP_RA4_HUD_Chrono_Superweapon", 27),
    ]

    created = [
        create_widget_blueprint("WBP_RA4_Splash", SPLASH_CLASS),
        create_widget_blueprint("WBP_RA4_MainMenu", MAIN_MENU_CLASS),
        create_widget_blueprint("WBP_RA4_FactionSelect", CAMPAIGN_SELECT_CLASS),
        create_widget_blueprint("WBP_RA4_Campaign_USSR", CAMPAIGN_CLASS),
        create_widget_blueprint(
            "WBP_RA4_Campaign_Allies",
            CAMPAIGN_CLASS,
            default_values={"faction_theme": unreal.RA4FactionTheme.ALLIES},
        ),
        create_widget_blueprint(
            "WBP_RA4_Campaign_AlliesAlternate",
            CAMPAIGN_CLASS,
            default_values={
                "faction_theme": unreal.RA4FactionTheme.ALLIES,
                "campaign_variant": unreal.RA4UIScreenVariant.ALLIES_ALTERNATE,
            },
        ),
        create_widget_blueprint(
            "WBP_RA4_Campaign_Eastern",
            CAMPAIGN_CLASS,
            default_values={"faction_theme": unreal.RA4FactionTheme.EASTERN_COALITION},
        ),
        create_widget_blueprint(
            "WBP_RA4_Campaign_EasternDetail",
            CAMPAIGN_CLASS,
            default_values={
                "faction_theme": unreal.RA4FactionTheme.EASTERN_COALITION,
                "campaign_variant": unreal.RA4UIScreenVariant.EASTERN_DETAIL,
            },
        ),
        create_widget_blueprint(
            "WBP_RA4_Campaign_Chrono",
            CAMPAIGN_CLASS,
            default_values={"faction_theme": unreal.RA4FactionTheme.CHRONOLEGION},
        ),
        create_widget_blueprint("WBP_RA4_MissionMap_USSR", MISSION_MAP_CLASS),
        create_widget_blueprint("WBP_RA4_Briefing_USSR", BRIEFING_CLASS),
        create_widget_blueprint("WBP_RA4_VideoComms", VIDEO_COMMS_CLASS),
        create_widget_blueprint("WBP_RA4_Loading_USSR", LOADING_CLASS),
        create_widget_blueprint(
            "WBP_RA4_Loading_USSR_Briefing",
            LOADING_CLASS,
            default_values={"loading_variant": unreal.RA4UIScreenVariant.LOADING_BRIEFING},
        ),
        create_widget_blueprint("WBP_RA4_MultiplayerLobby", LOBBY_CLASS),
        create_widget_blueprint("WBP_RA4_LobbyPlayerRow", LOBBY_ROW_CLASS),
    ]
    created.extend(
        create_widget_blueprint(name, SHOWCASE_CLASS, screen_index)
        for name, screen_index in screens
    )
    unreal.EditorAssetLibrary.save_directory(ASSET_PATH, only_if_is_dirty=True, recursive=True)
    unreal.log(f"RA4 UI assets ready: {len(created)}")


if __name__ == "__main__":
    main()
