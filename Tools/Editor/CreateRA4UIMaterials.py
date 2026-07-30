"""Creates M_UI_AngularPanel and theme material instances in /Game/RA4UI/Materials."""

import unreal

MATERIAL_ROOT = "/Game/RA4UI/Materials"


def ensure_directory():
    if not unreal.EditorAssetLibrary.does_directory_exist(MATERIAL_ROOT):
        unreal.EditorAssetLibrary.make_directory(MATERIAL_ROOT)


def create_ui_material():
    ensure_directory()
    path = "{}/M_UI_AngularPanel".format(MATERIAL_ROOT)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_UI_AngularPanel",
        MATERIAL_ROOT,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    # Accent color parameter
    accent_param = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -300, 0
    )
    accent_param.set_editor_property("parameter_name", "AccentColor")
    accent_param.set_editor_property("default_value", unreal.LinearColor(0.85, 0.15, 0.15, 1.0))

    # Connect to Base Color and Emissive
    unreal.MaterialEditingLibrary.connect_material_property(
        accent_param, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("[RA4 UI Materials] Created M_UI_AngularPanel")
    return material


def create_material_instances(master_material):
    ensure_directory()
    themes = [
        ("MI_UI_USSR", unreal.LinearColor(0.85, 0.15, 0.15, 1.0)),
        ("MI_UI_Allies", unreal.LinearColor(0.24, 0.61, 0.91, 1.0)),
        ("MI_UI_Eastern", unreal.LinearColor(0.84, 0.69, 0.31, 1.0)),
        ("MI_UI_Chrono", unreal.LinearColor(0.66, 0.36, 1.0, 1.0)),
    ]

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    for instance_name, color in themes:
        path = "{}/{}".format(MATERIAL_ROOT, instance_name)
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            mi = unreal.load_asset(path)
        else:
            factory = unreal.MaterialInstanceConstantFactoryNew()
            mi = asset_tools.create_asset(
                instance_name, MATERIAL_ROOT, unreal.MaterialInstanceConstant, factory
            )

        mi.set_editor_property("parent", master_material)
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            mi, "AccentColor", color
        )
        unreal.EditorAssetLibrary.save_loaded_asset(mi)
        unreal.log("[RA4 UI Materials] Created {}".format(instance_name))


def main():
    master = create_ui_material()
    create_material_instances(master)
    unreal.EditorAssetLibrary.save_directory(MATERIAL_ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log("[RA4 UI Materials] Setup complete.")


if __name__ == "__main__":
    main()
