"""Creates stable Widget Blueprint integration points for Fab UI packages."""

import unreal


ASSET_PATH = "/Game/RA4UI/Components"

COMPONENTS = (
    ("WBP_RTSHUD", "/Script/RA4UI.RA4HUDWidget"),
    ("WBP_Minimap", "/Script/RA4UI.RA4MinimapWidget"),
    ("WBP_ResourceBar", "/Script/RA4UI.RA4ResourceBarWidget"),
    ("WBP_ProductionTabs", "/Script/RA4UI.RA4ProductionTabsWidget"),
    ("WBP_ProductionCard", "/Script/RA4UI.RA4ProductionCardWidget"),
    ("WBP_SelectionPanel", "/Script/RA4UI.RA4SelectionPanelWidget"),
    ("WBP_CommandGrid", "/Script/RA4UI.RA4CommandGridWidget"),
    ("WBP_NotificationFeed", "/Script/RA4UI.RA4NotificationFeedWidget"),
    ("WBP_Objectives", "/Script/RA4UI.RA4ObjectivesWidget"),
    ("WBP_EVAAlert", "/Script/RA4UI.RA4EVAAlertWidget"),
)


def create_widget_blueprint(asset_name, class_path):
    parent_class = unreal.load_class(None, class_path)
    if not parent_class:
        raise RuntimeError("Widget class was not loaded: {}".format(class_path))

    asset_path = "{}/{}".format(ASSET_PATH, asset_name)
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    else:
        factory = unreal.WidgetBlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, ASSET_PATH, unreal.WidgetBlueprint, factory
        )

    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


created = [create_widget_blueprint(name, class_path) for name, class_path in COMPONENTS]
unreal.EditorAssetLibrary.save_directory(ASSET_PATH, only_if_is_dirty=False, recursive=True)
unreal.log("RA4 HUD component assets ready: {}".format(len(created)))
