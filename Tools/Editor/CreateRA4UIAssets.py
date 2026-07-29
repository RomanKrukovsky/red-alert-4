import unreal


ASSET_PATH = "/Game/RA4UI/Widgets"
SHOWCASE_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4ShowcaseWidget")


def create_widget_blueprint(asset_name, screen_index):
    asset_path = f"{ASSET_PATH}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    else:
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", SHOWCASE_CLASS)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, ASSET_PATH, unreal.WidgetBlueprint, factory
        )

    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    generated_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
    default_widget = unreal.get_default_object(generated_class)
    default_widget.set_editor_property("initial_screen_index", screen_index)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def main():
    if not SHOWCASE_CLASS:
        raise RuntimeError("URA4ShowcaseWidget was not loaded; compile the RA4UI module first.")

    screens = [
        ("WBP_RA4_Showcase", 0),
        ("WBP_RA4_Splash", 5),
        ("WBP_RA4_MainMenu", 0),
        ("WBP_RA4_FactionSelect", 6),
        ("WBP_RA4_Campaign_USSR", 1),
        ("WBP_RA4_Campaign_Allies", 7),
        ("WBP_RA4_Campaign_Eastern", 8),
        ("WBP_RA4_Campaign_Chrono", 9),
        ("WBP_RA4_MissionMap_USSR", 10),
        ("WBP_RA4_Briefing_USSR", 11),
        ("WBP_RA4_VideoComms", 12),
        ("WBP_RA4_Loading_USSR", 13),
        ("WBP_RA4_HUD_USSR", 2),
        ("WBP_RA4_HUD_Allies", 14),
        ("WBP_RA4_HUD_Eastern", 15),
        ("WBP_RA4_HUD_Chrono", 16),
        ("WBP_RA4_MultiplayerLobby", 3),
        ("WBP_RA4_Pause", 17),
        ("WBP_RA4_Victory", 18),
        ("WBP_RA4_Encyclopedia", 19),
        ("WBP_RA4_TechTree", 20),
        ("WBP_RA4_Mods", 21),
        ("WBP_RA4_Settings", 4),
        ("WBP_RA4_Campaign_EasternDetail", 22),
        ("WBP_RA4_HUD_USSR_Battle", 23),
        ("WBP_RA4_HUD_USSR_Alert", 24),
        ("WBP_RA4_HUD_Allies_Naval", 25),
        ("WBP_RA4_HUD_Allies_Air", 26),
        ("WBP_RA4_HUD_Chrono_Superweapon", 27),
    ]

    created = [create_widget_blueprint(name, screen_index) for name, screen_index in screens]
    unreal.EditorAssetLibrary.save_directory(ASSET_PATH, only_if_is_dirty=False, recursive=True)
    unreal.log(f"RA4 UI assets ready: {len(created)}")


if __name__ == "__main__":
    main()
