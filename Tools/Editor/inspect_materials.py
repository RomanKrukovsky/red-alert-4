# Copyright (c) Red Alert 4 project.
import unreal

for path in [
    "/Game/ThirdParty/CityPark/Materials/Ground/MI_Landscape",
    "/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01",
    "/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground02",
    "/Game/RA4/Generated/Terrain/M_RA4_Terrain"
]:
    mat = unreal.load_asset(path)
    if mat:
        print(f"Material: {path} ({mat.get_class().get_name()})")
        if isinstance(mat, unreal.MaterialInstance):
            print(f"  Parent: {mat.parent.get_path_name() if mat.parent else 'None'}")
            for tp in mat.texture_parameter_values:
                print(f"  TextureParam: {tp.parameter_info.name} = {tp.parameter_value.get_path_name() if tp.parameter_value else 'None'}")
            for sp in mat.scalar_parameter_values:
                print(f"  ScalarParam: {sp.parameter_info.name} = {sp.parameter_value}")
            for vp in mat.vector_parameter_values:
                print(f"  VectorParam: {vp.parameter_info.name} = {vp.parameter_value}")
        elif isinstance(mat, unreal.Material):
            print(f"  Material Domain: {mat.material_domain}, BlendMode: {mat.blend_mode}")
