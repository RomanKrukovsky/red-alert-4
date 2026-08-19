# Copyright (c) Red Alert 4 project.
import unreal

print("=== Checking Available Ground & Grass Materials ===")
for p in [
    "/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01",
    "/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground02",
    "/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground03",
    "/Game/ThirdParty/CityPark/Materials/Flora/Grass/MI_Grass01",
    "/Game/ThirdParty/CityPark/Materials/Flora/Grass/MI_Grass02",
    "/Game/ThirdParty/CityPark/Materials/Flora/Grass/MI_GrassMaster",
    "/Game/RA4/Materials/M_RA4Ground_Lit",
]:
    mat = unreal.load_asset(p)
    print(f"Asset: {p} -> {mat.get_class().get_name() if mat else 'NOT FOUND'}")

print("=== Checking Available Ground & Grass Textures ===")
for p in [
    "/Game/ThirdParty/CityPark/Textures/Flora/Grass/T_Grass01_D",
    "/Game/ThirdParty/CityPark/Textures/Flora/Grass/T_Grass10_D",
    "/Game/ThirdParty/CityPark/Textures/Flora/Grass/T_Grass11_D",
    "/Game/ThirdParty/CityPark/Textures/Ground/T_Ground01_D",
    "/Game/ThirdParty/CityPark/Textures/Ground/T_Ground02_D",
    "/Game/ThirdParty/CityPark/Textures/Ground/T_Ground05_D",
]:
    tex = unreal.load_asset(p)
    print(f"Texture: {p} -> {tex.get_class().get_name() if tex else 'NOT FOUND'}")
