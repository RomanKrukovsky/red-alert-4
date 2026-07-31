# Copyright (c) Red Alert 4 project. Art Mapping & ArtLab Map Generator.
import os
import json
import unreal

def log(msg):
    print(f"[RA4 ArtLab Generator] {msg}")

def create_art_mappings_and_map():
    log("Starting Art Mappings and RA4_ArtLab map generation...")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

    # 1. Ensure Generated Folders Exist & Scan Asset Registry
    folders = [
        "/Game/RA4/Art/Generated",
        "/Game/RA4/Animation/Generated",
        "/Game/RA4/VFX/Generated",
        "/Game/RA4/Audio/Generated",
        "/Game/Maps"
    ]
    for folder in folders:
        if not editor_asset_subsystem.does_directory_exist(folder):
            editor_asset_subsystem.make_directory(folder)
            log(f"Created directory: {folder}")

    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    asset_registry.scan_paths_synchronous(["/Game/RA4/Art/Blockout"], True)
    log("AssetRegistry scanned /Game/RA4/Art/Blockout synchronously.")

    # 2. Create Art Mapping Data Asset
    da_path = "/Game/RA4/Art/Generated/DA_RA4_ArtMappings"
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.RA4ArtMappingDataAsset)

    if editor_asset_subsystem.does_asset_exist(da_path):
        art_da = editor_asset_subsystem.load_asset(da_path)
        log("Loaded existing DA_RA4_ArtMappings")
    else:
        art_da = asset_tools.create_asset("DA_RA4_ArtMappings", "/Game/RA4/Art/Generated", unreal.RA4ArtMappingDataAsset, factory)
        log("Created new DA_RA4_ArtMappings")

    if not art_da:
        log("ERROR: Failed to create or load URA4ArtMappingDataAsset")
        return

    # 3. Populate USSR & Alliance Mappings
    roles_data = [
        # USSR Buildings
        ("SU_ConYard", "Building", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ConYard_Blockout"),
        ("SU_PowerPlant", "Building", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_PowerPlant_Blockout"),
        ("SU_Refinery", "Building", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Refinery_Blockout"),
        ("SU_Barracks", "Building", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_Barracks_Blockout"),
        ("SU_WarFactory", "Building", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_WarFactory_Blockout"),
        ("SU_SentryTurret", "Building", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GunTurret_Blockout"),
        
        # Alliance Buildings
        ("AL_ConYard", "Building", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_ConYard_Blockout"),
        ("AL_PowerPlant", "Building", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_PowerPlant_Blockout"),
        ("AL_Refinery", "Building", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Refinery_Blockout"),
        ("AL_Barracks", "Building", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Barracks_Blockout"),
        ("AL_WarFactory", "Building", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_WarFactory_Blockout"),
        ("AL_MultigunTurret", "Building", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_GunTurret_Blockout"),

        # USSR Units
        ("SU_Conscript", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RubezhRifleman_Blockout"),
        ("SU_ShockTrooper", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RazryadTrooper_Blockout"),
        ("SU_Commissar", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_VektorOfficer_Blockout"),
        ("SU_Sickle", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_RysScout_Blockout"),
        ("SU_HammerTank", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_GranitMBT_Blockout"),
        ("SU_Harvester", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_BogatyrOreCarrier_Blockout"),
        ("SU_Flak", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZaslonAATeam_Blockout"),
        ("SU_Buratino", "Unit", "/Game/RA4/Art/Blockout/Soviet/SM_Soviet_SU_ZarevoMLRS_Blockout"),

        # Alliance Units
        ("AL_Peacekeeper", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Peacekeeper_Blockout"),
        ("AL_Javelin", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Javelin_Blockout"),
        ("AL_Medic", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Medic_Blockout"),
        ("AL_Jackal", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Jackal_Blockout"),
        ("AL_GuardianTank", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Guardian_Blockout"),
        ("AL_Prospector", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Prospector_Blockout"),
        ("AL_Aegis", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_AegisShield_Blockout"),
        ("AL_Athena", "Unit", "/Game/RA4/Art/Blockout/Alliance/SM_Alliance_AL_Athena_Blockout")
    ]

    unit_map = art_da.get_editor_property("units")
    building_map = art_da.get_editor_property("buildings")

    for bible_id, entity_kind, mesh_path in roles_data:
        mesh_obj = None
        if editor_asset_subsystem.does_asset_exist(mesh_path):
            mesh_obj = editor_asset_subsystem.load_asset(mesh_path)

        if entity_kind == "Unit":
            unit_def = unreal.RA4UnitArtDefinition()
            unit_def.set_editor_property("unit_id", bible_id)
            if mesh_obj and isinstance(mesh_obj, unreal.StaticMesh):
                unit_def.set_editor_property("static_mesh", mesh_obj)
            elif mesh_obj and isinstance(mesh_obj, unreal.SkeletalMesh):
                unit_def.set_editor_property("skeletal_mesh", mesh_obj)

            # Map SkeletalMesh and Animations for Infantry Units
            if bible_id in ["SU_Conscript", "SU_ShockTrooper", "SU_Commissar", "SU_Flak", "AL_Peacekeeper", "AL_Javelin", "AL_Medic"]:
                skel_path = "/Game/ThirdParty/QuantumCharacter/Mesh/SKM_QuantumCharacter"
                idle_path = "/Game/ThirdParty/QuantumCharacter/Demo/Animations/A_MM_Idle"
                run_path = "/Game/ThirdParty/QuantumCharacter/Demo/Animations/A_MM_Run_Fwd"
                walk_path = "/Game/ThirdParty/QuantumCharacter/Demo/Animations/A_MM_Walk_Fwd"

                if editor_asset_subsystem.does_asset_exist(skel_path):
                    unit_def.set_editor_property("skeletal_mesh", editor_asset_subsystem.load_asset(skel_path))
                if editor_asset_subsystem.does_asset_exist(idle_path):
                    unit_def.set_editor_property("idle_anim", editor_asset_subsystem.load_asset(idle_path))
                if editor_asset_subsystem.does_asset_exist(run_path):
                    unit_def.set_editor_property("run_anim", editor_asset_subsystem.load_asset(run_path))
                if editor_asset_subsystem.does_asset_exist(walk_path):
                    unit_def.set_editor_property("walk_anim", editor_asset_subsystem.load_asset(walk_path))

            unit_def.set_editor_property("turret_socket_name", "Socket_Turret")
            unit_def.set_editor_property("muzzle_socket_name", "Socket_Muzzle")
            unit_def.set_editor_property("engine_socket_name", "Socket_Engine")
            unit_def.set_editor_property("cargo_socket_name", "Socket_Cargo")
            
            if bible_id.startswith("SU_"):
                unit_def.set_editor_property("team_color_override", unreal.LinearColor(0.8, 0.15, 0.1, 1.0))
            else:
                unit_def.set_editor_property("team_color_override", unreal.LinearColor(0.1, 0.4, 0.8, 1.0))
                
            unit_map[bible_id] = unit_def
        else:
            bldg_def = unreal.RA4BuildingArtDefinition()
            bldg_def.set_editor_property("building_id", bible_id)
            if mesh_obj and isinstance(mesh_obj, unreal.StaticMesh):
                bldg_def.set_editor_property("stage0_delivery_mesh", mesh_obj)
                bldg_def.set_editor_property("stage1_foundation_mesh", mesh_obj)
                bldg_def.set_editor_property("stage2_structure_mesh", mesh_obj)
                bldg_def.set_editor_property("stage3_wiring_mesh", mesh_obj)
                bldg_def.set_editor_property("stage4_active_mesh", mesh_obj)
            building_map[bible_id] = bldg_def

    art_da.set_editor_property("units", unit_map)
    art_da.set_editor_property("buildings", building_map)
    editor_asset_subsystem.save_asset(da_path)
    log("Saved DA_RA4_ArtMappings with 14 unit definitions and 12 building definitions.")

    # 4. Create Map /Game/Maps/RA4_ArtLab
    map_path = "/Game/Maps/RA4_ArtLab"
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    unreal.EditorLoadingAndSavingUtils.save_map(world, map_path)
    log(f"Created map: {map_path}")

    # 5. Populate RA4_ArtLab with Showcase Layout
    # Spawn directional light and sky light
    subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    
    # Spawn directional light
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500))
    if sun:
        sun.set_actor_label("ArtLab_SunLight")
        sun.set_actor_rotation(unreal.Rotator(-45, 45, 0), False)

    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 600))
    if sky:
        sky.set_actor_label("ArtLab_SkyLight")

    # Spawn showcases for USSR and Alliance
    x_offset = -2000
    y_offset = -2000
    
    for bible_id, entity_kind, mesh_path in roles_data:
        mesh_obj = None
        if editor_asset_subsystem.does_asset_exist(mesh_path):
            mesh_obj = editor_asset_subsystem.load_asset(mesh_path)
        if mesh_obj and isinstance(mesh_obj, unreal.StaticMesh):
            actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x_offset, y_offset, 0))
            if actor:
                actor.set_actor_label(f"ArtLab_Showcase_{bible_id}")
                actor.static_mesh_component.set_static_mesh(mesh_obj)
                
            x_offset += 600
            if x_offset > 2000:
                x_offset = -2000
                y_offset += 800

    unreal.EditorLoadingAndSavingUtils.save_map(world, map_path)
    log("Successfully populated and saved /Game/Maps/RA4_ArtLab.")

if __name__ == "__main__":
    create_art_mappings_and_map()
