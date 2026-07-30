# Copyright (c) Red Alert 4 project.
#
# Creates deterministic, clearly readable materials for the native skirmish slice.
# They use the normal lit surface path so the ground and faction colours behave the
# same in the editor viewport, PIE, and standalone game.

import unreal


MATERIAL_ROOT = "/Game/RA4/Materials"


def create_new(name):
    path = "{}/{}".format(MATERIAL_ROOT, name)
    existing = unreal.load_asset(path)
    if existing is not None:
        unreal.log("[RA4 materials] reusing {}".format(existing.get_path_name()))
        return existing

    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name,
        MATERIAL_ROOT,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
def make_lit_vector_material(name, parameter_name, color):
    material = create_new(name)
    material.set_editor_property(
        "shading_model",
        unreal.MaterialShadingModel.MSM_DEFAULT_LIT,
    )

    vector = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionVectorParameter,
        -250,
        0,
    )
    vector.set_editor_property("parameter_name", parameter_name)
    vector.set_editor_property("default_value", color)
    unreal.MaterialEditingLibrary.connect_material_property(
        vector,
        "RGB",
        unreal.MaterialProperty.MP_BASE_COLOR,
    )
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant,
        -250,
        120,
    )
    roughness.set_editor_property("r", 0.82)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness,
        "",
        unreal.MaterialProperty.MP_ROUGHNESS,
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("[RA4 materials] saved {}".format(material.get_path_name()))


make_lit_vector_material(
    "M_RA4Ground_Lit",
    "GroundColor",
    unreal.LinearColor(0.055, 0.10, 0.12, 1.0),
)
make_lit_vector_material(
    "M_RA4EntityPlaceholder_Lit",
    "TeamColor",
    unreal.LinearColor(0.85, 0.12, 0.10, 1.0),
)
