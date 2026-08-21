#!/usr/bin/env python3
"""Create the RA4 painted-metal PBR master material and faction instances."""

from __future__ import annotations

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
MATERIAL_ROOT = "/Game/RA4/Art/Materials"
TEXTURE_ROOT = f"{MATERIAL_ROOT}/Textures"

TEXTURES = {
    "T_RA4_PaintedMetal_BaseColor": "Content/ThirdParty/ambientCG/Metal032_2K-JPG/Metal032_2K-JPG_Color.jpg",
    "T_RA4_PaintedMetal_Normal": "Content/ThirdParty/ambientCG/Metal032_2K-JPG/Metal032_2K-JPG_NormalGL.jpg",
    "T_RA4_PaintedMetal_Roughness": "Content/ThirdParty/ambientCG/Metal032_2K-JPG/Metal032_2K-JPG_Roughness.jpg",
    "T_RA4_PaintedMetal_Metallic": "Content/ThirdParty/ambientCG/Metal032_2K-JPG/Metal032_2K-JPG_Metalness.jpg",
}

PALETTES = {
    "Soviet": unreal.LinearColor(0.85, 0.12, 0.08, 1.0),
    "Alliance": unreal.LinearColor(0.12, 0.48, 0.95, 1.0),
    "Coalition": unreal.LinearColor(0.15, 0.72, 0.32, 1.0),
    "Chronolegion": unreal.LinearColor(0.68, 0.20, 0.95, 1.0),
}


def ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_texture(name: str, relative_source: str) -> unreal.Texture2D:
    asset_path = f"{TEXTURE_ROOT}/{name}"
    existing = unreal.load_asset(asset_path)
    if isinstance(existing, unreal.Texture2D):
        return existing
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(PROJECT_ROOT / relative_source))
    task.set_editor_property("destination_path", TEXTURE_ROOT)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Failed to import {name}")
    if "Normal" in name:
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        texture.set_editor_property("srgb", False)
    elif "Roughness" in name or "Metallic" in name:
        texture.set_editor_property("srgb", False)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
    return texture


def expression(material: unreal.Material, expression_type: type, x: int, y: int):
    return unreal.MaterialEditingLibrary.create_material_expression(material, expression_type, x, y)


def texture_parameter(material: unreal.Material, name: str, texture: unreal.Texture2D, x: int, y: int, sampler) -> object:
    node = expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", texture)
    node.set_editor_property("sampler_type", sampler)
    return node


def create_master(textures: dict[str, unreal.Texture2D]) -> unreal.Material:
    asset_path = f"{MATERIAL_ROOT}/M_RA4_PaintedMetal_PBR"
    existing = unreal.load_asset(asset_path)
    if isinstance(existing, unreal.Material):
        unreal.EditorAssetLibrary.delete_asset(asset_path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_RA4_PaintedMetal_PBR",
        MATERIAL_ROOT,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    base = texture_parameter(material, "SurfaceColor", textures["T_RA4_PaintedMetal_BaseColor"], -700, -220, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    team = expression(material, unreal.MaterialExpressionVectorParameter, -700, -60)
    team.set_editor_property("parameter_name", "FactionColor")
    team.set_editor_property("default_value", unreal.LinearColor.WHITE)
    tint = expression(material, unreal.MaterialExpressionMultiply, -410, -160)
    unreal.MaterialEditingLibrary.connect_material_expressions(base, "RGB", tint, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(team, "RGB", tint, "B")
    unreal.MaterialEditingLibrary.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)

    normal = texture_parameter(material, "SurfaceNormal", textures["T_RA4_PaintedMetal_Normal"], -700, 140, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    unreal.MaterialEditingLibrary.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    roughness = texture_parameter(material, "SurfaceRoughness", textures["T_RA4_PaintedMetal_Roughness"], -700, 310, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    roughness_scale = expression(material, unreal.MaterialExpressionScalarParameter, -430, 360)
    roughness_scale.set_editor_property("parameter_name", "RoughnessScale")
    roughness_scale.set_editor_property("default_value", 1.0)
    roughness_multiply = expression(material, unreal.MaterialExpressionMultiply, -170, 310)
    unreal.MaterialEditingLibrary.connect_material_expressions(roughness, "R", roughness_multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(roughness_scale, "", roughness_multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_property(roughness_multiply, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metallic = texture_parameter(material, "SurfaceMetallic", textures["T_RA4_PaintedMetal_Metallic"], -700, 500, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    metallic_scale = expression(material, unreal.MaterialExpressionScalarParameter, -430, 550)
    metallic_scale.set_editor_property("parameter_name", "MetallicScale")
    metallic_scale.set_editor_property("default_value", 1.0)
    metallic_multiply = expression(material, unreal.MaterialExpressionMultiply, -170, 500)
    unreal.MaterialEditingLibrary.connect_material_expressions(metallic, "R", metallic_multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(metallic_scale, "", metallic_multiply, "B")
    unreal.MaterialEditingLibrary.connect_material_property(metallic_multiply, "", unreal.MaterialProperty.MP_METALLIC)

    emissive_strength = expression(material, unreal.MaterialExpressionScalarParameter, -410, 40)
    emissive_strength.set_editor_property("parameter_name", "EmissiveStrength")
    emissive_strength.set_editor_property("default_value", 0.0)
    emissive = expression(material, unreal.MaterialExpressionMultiply, -150, 20)
    unreal.MaterialEditingLibrary.connect_material_expressions(team, "RGB", emissive, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(emissive_strength, "", emissive, "B")
    unreal.MaterialEditingLibrary.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    return material


def create_instance(name: str, parent: unreal.Material, color: unreal.LinearColor, roughness: float, metallic: float, emissive: float = 0.0) -> unreal.MaterialInstanceConstant:
    path = f"{MATERIAL_ROOT}/{name}"
    instance = unreal.load_asset(path)
    if not isinstance(instance, unreal.MaterialInstanceConstant):
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            MATERIAL_ROOT,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
    instance.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(instance, "FactionColor", color)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "RoughnessScale", roughness)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "MetallicScale", metallic)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(instance, "EmissiveStrength", emissive)
    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    return instance


def main() -> None:
    ensure_directory(MATERIAL_ROOT)
    ensure_directory(TEXTURE_ROOT)
    textures = {name: import_texture(name, source) for name, source in TEXTURES.items()}
    master = create_master(textures)
    for faction, color in PALETTES.items():
        create_instance(f"MI_RA4_Surface_{faction}", master, color, 0.9, 1.0)
        create_instance(f"MI_RA4_Emissive_{faction}", master, color, 0.45, 0.7, 1.5)
    create_instance("MI_RA4_Surface_Dark", master, unreal.LinearColor(0.045, 0.05, 0.055, 1.0), 1.35, 0.2)
    create_instance("MI_RA4_Surface_Concrete", master, unreal.LinearColor(0.48, 0.50, 0.52, 1.0), 1.5, 0.0)
    create_instance("MI_RA4_Surface_Glass", master, unreal.LinearColor(0.05, 0.32, 0.42, 1.0), 0.28, 0.35)
    unreal.EditorAssetLibrary.save_directory(MATERIAL_ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log("RA4ArtMaterials: PBR master, 4 textures and 11 instances ready")


if __name__ == "__main__":
    main()
