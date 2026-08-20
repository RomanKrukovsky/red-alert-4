"""Release-gate validation for the 24 screenshot-driven RA4 UI screens."""

import unreal


WIDGET_ROOT = "/Game/RA4UI/Widgets"
THEME_ROOT = "/Game/RA4UI/Themes"


SCREEN_ASSETS = (
    (1, "WBP_RA4_Splash", "/Script/RA4UI.RA4SplashScreenWidget", {}),
    (2, "WBP_RA4_MainMenu", "/Script/RA4UI.RA4MainMenuScreenWidget", {}),
    (3, "WBP_RA4_FactionSelect", "/Script/RA4UI.RA4CampaignSelectWidget", {}),
    (4, "WBP_RA4_Campaign_USSR", "/Script/RA4UI.RA4CampaignScreenWidget", {"faction_theme": unreal.RA4FactionTheme.USSR}),
    (5, "WBP_RA4_Campaign_Allies", "/Script/RA4UI.RA4CampaignScreenWidget", {"faction_theme": unreal.RA4FactionTheme.ALLIES}),
    (6, "WBP_RA4_Campaign_Eastern", "/Script/RA4UI.RA4CampaignScreenWidget", {"faction_theme": unreal.RA4FactionTheme.EASTERN_COALITION}),
    (7, "WBP_RA4_Campaign_Chrono", "/Script/RA4UI.RA4CampaignScreenWidget", {"faction_theme": unreal.RA4FactionTheme.CHRONOLEGION}),
    (8, "WBP_RA4_MissionMap_USSR", "/Script/RA4UI.RA4MissionMapScreenWidget", {}),
    (9, "WBP_RA4_Briefing_USSR", "/Script/RA4UI.RA4BriefingScreenWidget", {}),
    (10, "WBP_RA4_VideoComms", "/Script/RA4UI.RA4VideoCommsScreenWidget", {}),
    (11, "WBP_RA4_Campaign_AlliesAlternate", "/Script/RA4UI.RA4CampaignScreenWidget", {
        "faction_theme": unreal.RA4FactionTheme.ALLIES,
        "campaign_variant": unreal.RA4UIScreenVariant.ALLIES_ALTERNATE,
    }),
    (12, "WBP_RA4_Loading_USSR", "/Script/RA4UI.RA4LoadingScreenWidget", {
        "loading_variant": unreal.RA4UIScreenVariant.DEFAULT,
    }),
    (13, "WBP_RA4_HUD_USSR", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 13}),
    (14, "WBP_RA4_HUD_Allies", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 14}),
    (15, "WBP_RA4_HUD_Eastern", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 15}),
    (16, "WBP_RA4_HUD_Chrono", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 16}),
    (17, "WBP_RA4_MultiplayerLobby", "/Script/RA4UI.RA4LobbyScreenWidget", {}),
    (18, "WBP_RA4_Campaign_EasternDetail", "/Script/RA4UI.RA4CampaignScreenWidget", {
        "faction_theme": unreal.RA4FactionTheme.EASTERN_COALITION,
        "campaign_variant": unreal.RA4UIScreenVariant.EASTERN_DETAIL,
    }),
    (19, "WBP_RA4_Loading_USSR_Briefing", "/Script/RA4UI.RA4LoadingScreenWidget", {
        "loading_variant": unreal.RA4UIScreenVariant.LOADING_BRIEFING,
    }),
    (20, "WBP_RA4_HUD_USSR_Battle", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 20}),
    (21, "WBP_RA4_HUD_USSR_Alert", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 21}),
    (22, "WBP_RA4_HUD_Allies_Naval", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 22}),
    (23, "WBP_RA4_HUD_Allies_Air", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 23}),
    (24, "WBP_RA4_HUD_Chrono_Superweapon", "/Script/RA4UI.RA4FactionHUDWidget", {"initial_reference_number": 24}),
)


THEME_ASSETS = (
    ("DA_RA4_Theme_USSR", unreal.RA4FactionTheme.USSR),
    ("DA_RA4_Theme_Allies", unreal.RA4FactionTheme.ALLIES),
    ("DA_RA4_Theme_Eastern", unreal.RA4FactionTheme.EASTERN_COALITION),
    ("DA_RA4_Theme_Chrono", unreal.RA4FactionTheme.CHRONOLEGION),
)


def failure(asset_path, property_name, expected, actual):
    return "{}: property '{}' expected {!r}, got {!r}".format(
        asset_path, property_name, expected, actual
    )


def validate_property(errors, asset_path, default_object, property_name, expected):
    actual = default_object.get_editor_property(property_name)
    if actual != expected:
        errors.append(failure(asset_path, property_name, expected, actual))


def validate_self_test():
    message = failure(
        "/Game/RA4UI/Widgets/WBP_RA4_InvalidFixture",
        "initial_reference_number",
        24,
        0,
    )
    expected = (
        "/Game/RA4UI/Widgets/WBP_RA4_InvalidFixture: property "
        "'initial_reference_number' expected 24, got 0"
    )
    if message != expected:
        raise RuntimeError("Validator fixture did not report the exact asset and property")


def validate_screen_assets(errors):
    references = [contract[0] for contract in SCREEN_ASSETS]
    if references != list(range(1, 25)):
        errors.append("screen_contracts: references must be exactly 1..24")

    asset_names = [contract[1] for contract in SCREEN_ASSETS]
    if len(asset_names) != len(set(asset_names)):
        errors.append("screen_contracts: Widget Blueprint asset names must be unique")

    for reference, asset_name, parent_path, properties in SCREEN_ASSETS:
        asset_path = "{}/{}".format(WIDGET_ROOT, asset_name)
        if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            errors.append("{}: required asset does not exist".format(asset_path))
            continue

        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        parent_class = unreal.load_class(None, parent_path)
        if not parent_class:
            errors.append("{}: parent class '{}' did not load".format(asset_path, parent_path))
            continue
        actual_parent = asset.get_blueprint_parent_class()
        if actual_parent != parent_class:
            errors.append(failure(asset_path, "parent_class", parent_class, actual_parent))

        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        generated_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        if not generated_class:
            errors.append("{}: generated_class is missing after compilation".format(asset_path))
            continue
        default_object = unreal.get_default_object(generated_class)
        for property_name, expected in properties.items():
            validate_property(errors, asset_path, default_object, property_name, expected)

        if reference >= 13 and reference != 17 and reference != 18 and reference != 19:
            validate_property(
                errors, asset_path, default_object, "initial_reference_number", reference
            )


def validate_themes(errors):
    for asset_name, expected_faction in THEME_ASSETS:
        asset_path = "{}/{}".format(THEME_ROOT, asset_name)
        if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            errors.append("{}: required theme does not exist".format(asset_path))
            continue
        theme = unreal.EditorAssetLibrary.load_asset(asset_path)
        validate_property(errors, asset_path, theme, "faction", expected_faction)
        if not theme.get_editor_property("menu_background"):
            errors.append("{}: property 'menu_background' must be assigned".format(asset_path))
        if theme.get_editor_property("glow_strength") <= 0.0:
            errors.append("{}: property 'glow_strength' must be positive".format(asset_path))


def main():
    validate_self_test()
    errors = []
    validate_screen_assets(errors)
    validate_themes(errors)
    if errors:
        for error in errors:
            unreal.log_error("RA4 UI VALIDATION: {}".format(error))
        raise RuntimeError("RA4 UI validation failed with {} error(s)".format(len(errors)))
    unreal.log(
        "RA4 UI VALIDATION PASSED: 24 reference contracts, 24 Widget Blueprints, "
        "4 themes"
    )


if __name__ == "__main__":
    main()
