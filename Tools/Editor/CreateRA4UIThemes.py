import unreal


ASSET_PATH = "/Game/RA4UI/Themes"
THEME_CLASS = unreal.load_class(None, "/Script/RA4UI.RA4UITheme")


def create_theme(name, faction, primary, secondary, background, menu_background, glow_strength):
    path = f"{ASSET_PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.EditorAssetLibrary.load_asset(path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", THEME_CLASS)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, ASSET_PATH, THEME_CLASS, factory
        )

    asset.set_editor_property("faction", faction)
    asset.set_editor_property("primary_color", unreal.LinearColor(*primary))
    asset.set_editor_property("secondary_color", unreal.LinearColor(*secondary))
    asset.set_editor_property("background_color", unreal.LinearColor(*background))
    asset.set_editor_property("text_color", unreal.LinearColor(0.87, 0.91, 0.95, 1.0))
    asset.set_editor_property("menu_background", unreal.load_asset(menu_background))
    asset.set_editor_property("frame_stroke", 2.0)
    asset.set_editor_property("glow_strength", glow_strength)
    asset.set_editor_property("transition_duration", 0.28)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def main():
    if not THEME_CLASS:
        raise RuntimeError("URA4UITheme was not loaded; compile the RA4UI module first.")

    themes = [
        ("DA_RA4_Theme_USSR", unreal.RA4FactionTheme.USSR, (0.90, 0.07, 0.08, 1.0), (0.30, 0.02, 0.02, 1.0), (0.012, 0.016, 0.023, 0.96), "/Game/RA4UI/Art/T_RA4_USSR_CommandCenter", 1.35),
        ("DA_RA4_Theme_Allies", unreal.RA4FactionTheme.ALLIES, (0.10, 0.48, 0.95, 1.0), (0.02, 0.14, 0.30, 1.0), (0.012, 0.016, 0.023, 0.96), "/Game/RA4UI/Art/T_RA4_Allies_ArcticFleet", 1.15),
        ("DA_RA4_Theme_Eastern", unreal.RA4FactionTheme.EASTERN_COALITION, (0.26, 0.92, 0.24, 1.0), (0.20, 0.15, 0.03, 1.0), (0.006, 0.030, 0.012, 0.96), "/Game/RA4UI/Art/T_RA4_Eastern_CommandFortress", 1.10),
        ("DA_RA4_Theme_Chrono", unreal.RA4FactionTheme.CHRONOLEGION, (0.61, 0.25, 0.94, 1.0), (0.15, 0.03, 0.28, 1.0), (0.012, 0.016, 0.023, 0.96), "/Game/RA4UI/Art/T_RA4_Chrono_TemporalCitadel", 1.45),
    ]

    created = [create_theme(*theme) for theme in themes]
    unreal.EditorAssetLibrary.save_directory(ASSET_PATH, only_if_is_dirty=False, recursive=True)
    unreal.log(f"RA4 UI theme assets ready: {len(created)}")


if __name__ == "__main__":
    main()
