# Copyright (c) Red Alert 4 project.
import math
import random
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
MAP_EXTENT = 12800.0
MAP_CENTRE = 6400.0
SEA_LEVEL = 0.0

def sample_terrain_z(world_x, world_y):
    """Matches the mathematical terrain formula used by RA4LandscapeCommandlet."""
    Seed = 20260730
    SeedPhase = float(Seed % 1000) * 0.01
    v = 0.0
    v += math.sin(world_x * 0.00028 + SeedPhase) * math.cos(world_y * 0.00024 + SeedPhase * 1.7)
    v += 0.5 * math.sin(world_x * 0.0006 - SeedPhase * 0.5) * math.cos(world_y * 0.0005 + SeedPhase)
    v += 0.25 * math.sin((world_x + world_y) * 0.0009 + SeedPhase * 2.0)
    height_norm = v / 1.75
    amplitude_units = 220.0
    return height_norm * amplitude_units

def safe_set(obj, name, val):
    try:
        obj.set_editor_property(name, val)
    except Exception:
        try:
            setattr(obj, name, val)
        except Exception:
            pass

def run():
    print("=== [RA4] Starting Master Skirmish Map Population ===")
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError(f"Failed to load level {LEVEL_PATH}")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("force_no_precomputed_lighting", True)

    # 1. Clean up old/redundant lighting and decor actors
    tags_to_clear = [
        "RA4_Sun", "RA4_SkyLight", "RA4_SkyAtmosphere", "RA4_ExponentialHeightFog",
        "RA4_PostProcessVolume", "RA4_SkySphere", "RA4_Ocean_Horizon", "RA4_Water_OceanPlane",
        "RA4_Decor_", "RA4_Tree_", "RA4_Rock_", "RA4_Bush_", "RA4_Grass_", "RA4_BasePaving_", "RA4_Prop_"
    ]
    
    destroyed_count = 0
    for actor in list(actor_subsystem.get_all_level_actors()):
        label = actor.get_actor_label()
        for tag in tags_to_clear:
            if label.startswith(tag) or label == tag:
                actor_subsystem.destroy_actor(actor)
                destroyed_count += 1
                break

    print(f"Cleaned up {destroyed_count} previous environment/decor actors.")

    # 2. Lighting & Atmosphere Setup (Crisp RTS Daylight)
    # 2.1 Sun Directional Light
    sun = actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 9000.0),
        unreal.Rotator(-54.0, 52.0, 0.0)
    )
    sun.set_actor_label("RA4_Sun")
    sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
    if sun_comp:
        safe_set(sun_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set(sun_comp, "intensity", 95000.0)
        safe_set(sun_comp, "light_color", unreal.Color(255, 250, 240, 255))
        safe_set(sun_comp, "atmosphere_sun_light", True)
        safe_set(sun_comp, "atmosphere_sun_light_index", 0)
        safe_set(sun_comp, "cast_shadows", True)
        safe_set(sun_comp, "dynamic_shadow_distance_movable_light", 38000.0)
        safe_set(sun_comp, "shadow_bias", 0.2)
        safe_set(sun_comp, "cascade_distribution_exponent", 2.2)
    print("Created pristine RA4_Sun DirectionalLight")

    # 2.2 Sky Atmosphere
    sky_atm = actor_subsystem.spawn_actor_from_class(
        unreal.SkyAtmosphere,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    sky_atm.set_actor_label("RA4_SkyAtmosphere")

    # 2.3 SkyLight with Natural Greenish-Warm Ground Bounce
    skylight = actor_subsystem.spawn_actor_from_class(
        unreal.SkyLight,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 3500.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    skylight.set_actor_label("RA4_SkyLight")
    skylight_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
    if skylight_comp:
        safe_set(skylight_comp, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set(skylight_comp, "intensity", 4.2)
        safe_set(skylight_comp, "light_color", unreal.Color(230, 242, 255, 255))
        safe_set(skylight_comp, "real_time_capture", True)
        safe_set(skylight_comp, "lower_hemisphere_is_black", False)
        safe_set(skylight_comp, "lower_hemisphere_color", unreal.LinearColor(0.24, 0.35, 0.16, 1.0))
        safe_set(skylight_comp, "cast_shadows", True)
    print("Created bright RA4_SkyLight with Ground Ambient Bounce")

    # 2.4 Exponential Height Fog
    fog = actor_subsystem.spawn_actor_from_class(
        unreal.ExponentialHeightFog,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, -100.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    fog.set_actor_label("RA4_ExponentialHeightFog")
    fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    if fog_comp:
        safe_set(fog_comp, "fog_density", 0.0008)
        safe_set(fog_comp, "start_distance", 2500.0)
        safe_set(fog_comp, "fog_inscattering_luminance", unreal.LinearColor(0.72, 0.84, 0.98, 1.0))
        safe_set(fog_comp, "b_enable_volumetric_fog", True)

    # 2.5 Post Process Volume
    pp = actor_subsystem.spawn_actor_from_class(
        unreal.PostProcessVolume,
        unreal.Vector(MAP_CENTRE, MAP_CENTRE, 500.0),
        unreal.Rotator(0.0, 0.0, 0.0)
    )
    pp.set_actor_label("RA4_PostProcessVolume")
    safe_set(pp, "unbound", True)
    safe_set(pp, "priority", 10.0)
    pp_settings = pp.settings
    safe_set(pp_settings, "auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    safe_set(pp_settings, "auto_exposure_min_brightness", 0.3)
    safe_set(pp_settings, "auto_exposure_max_brightness", 4.0)
    safe_set(pp_settings, "auto_exposure_bias", 0.8)
    safe_set(pp_settings, "bloom_intensity", 0.3)
    safe_set(pp_settings, "vignette_intensity", 0.12)

    # 2.6 Horizon Ocean Plane
    plane_mesh = unreal.load_asset("/Engine/BasicShapes/Plane.Plane")
    mat_water = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/BaseMaterial/M_WaterShader_V_01")
    if mat_water is None:
        mat_water = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Water1")

    if plane_mesh:
        ocean_actor = actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(MAP_CENTRE, MAP_CENTRE, SEA_LEVEL - 15.0),
            unreal.Rotator(0.0, 0.0, 0.0)
        )
        ocean_actor.set_actor_label("RA4_Ocean_Horizon")
        oc_comp = ocean_actor.static_mesh_component
        oc_comp.set_static_mesh(plane_mesh)
        if mat_water:
            oc_comp.set_material(0, mat_water)
        ocean_actor.set_actor_scale3d(unreal.Vector(2500.0, 2500.0, 1.0))
        ocean_actor.set_mobility(unreal.ComponentMobility.STATIC)
        print("Created RA4_Ocean_Horizon water plane")

    # 3. Ensure Landscape has rich PBR Grass/Ground Material
    mat_landscape = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Landscape")
    if mat_landscape is None:
        mat_landscape = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01")

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.LandscapeProxy):
            if mat_landscape:
                actor.set_editor_property("landscape_material", mat_landscape)
                print(f"Assigned landscape material: {mat_landscape.get_path_name()}")
            break

    # 4. Asset Library Setup
    tree_assets = [
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Elm01"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_AmurCork01"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Trees/SM_Maple01"),
    ]
    tree_assets = [t for t in tree_assets if t is not None]

    bush_assets = [
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Bushs/SM_Bush1_01"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Bushs/SM_Bush1_03"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Bushs/SM_Bush2_01"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Bushs/SM_Bush4_01"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Bushs/SM_Bush5_01"),
    ]
    bush_assets = [b for b in bush_assets if b is not None]

    rock_assets = [
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock01"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock02"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock03"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock04"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock05"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock06"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Ground/SM_Rock07"),
    ]
    rock_assets = [r for r in rock_assets if r is not None]

    grass_assets = [
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass10_3"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass11_1"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass12_1"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Flora/Grass/SM_Grass13_1"),
    ]
    grass_assets = [g for g in grass_assets if g is not None]

    square_assets = [
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/ParkSquare/SM_ParkSquare01_1"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/ParkSquare/SM_ParkSquare02_1"),
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/ParkSquare/SM_ParkSquare13_1"),
    ]
    square_assets = [s for s in square_assets if s is not None]

    lamp_mesh = unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Props/SM_LampPost01")
    fence_mesh = unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Props/SM_ParkFence01")
    fountain_mesh = unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/ParkSquare/SM_Fountain01")
    statue_mesh = (
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Props/SM_Statue01") or
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Props/SM_Statue02") or
        unreal.load_asset("/Game/ThirdParty/CityPark/Meshes/Props/SM_Statue03")
    )

    rng = random.Random(42)

    def spawn_static_mesh(mesh, pos, rot=None, scale=None, label_prefix="RA4_Decor_"):
        if mesh is None:
            return None
        if rot is None:
            rot = unreal.Rotator(0.0, rng.uniform(0.0, 360.0), 0.0)
        if scale is None:
            scale = unreal.Vector(1.0, 1.0, 1.0)
        
        actor = actor_subsystem.spawn_actor_from_class(unreal.StaticMeshActor, pos, rot)
        if actor:
            actor.set_actor_label(f"{label_prefix}{rng.randint(1000, 9999)}")
            sm_comp = actor.static_mesh_component
            sm_comp.set_static_mesh(mesh)
            actor.set_actor_scale3d(scale)
            actor.set_mobility(unreal.ComponentMobility.STATIC)
        return actor

    # 5. Populate Natural Decor Elements
    spawned_props = 0

    # 5.1 Forest Groves (Perimeter & Flanks)
    forest_clusters = [
        (2000.0, 10500.0, 1400.0, 45),
        (10800.0, 2200.0, 1400.0, 45),
        (10500.0, 10500.0, 1200.0, 35),
        (2200.0, 2200.0, 1200.0, 35),
        (2500.0, 6400.0, 900.0, 25),
        (10300.0, 6400.0, 900.0, 25),
        (6400.0, 11000.0, 1100.0, 30),
        (6400.0, 1800.0, 1100.0, 30),
    ]

    for cx, cy, radius, count in forest_clusters:
        for _ in range(count):
            angle = rng.uniform(0.0, 2.0 * math.pi)
            dist = math.sqrt(rng.uniform(0.0, 1.0)) * radius
            x = cx + math.cos(angle) * dist
            y = cy + math.sin(angle) * dist
            z = sample_terrain_z(x, y)

            if tree_assets:
                t_mesh = rng.choice(tree_assets)
                scale_val = rng.uniform(0.85, 1.35)
                spawn_static_mesh(
                    t_mesh,
                    unreal.Vector(x, y, z - 10.0),
                    unreal.Rotator(rng.uniform(-3.0, 3.0), rng.uniform(0.0, 360.0), rng.uniform(-3.0, 3.0)),
                    unreal.Vector(scale_val, scale_val, scale_val),
                    "RA4_Tree_"
                )
                spawned_props += 1

            if bush_assets and rng.random() > 0.4:
                b_mesh = rng.choice(bush_assets)
                bx = x + rng.uniform(-180.0, 180.0)
                by = y + rng.uniform(-180.0, 180.0)
                bz = sample_terrain_z(bx, by)
                b_scale = rng.uniform(0.9, 1.6)
                spawn_static_mesh(
                    b_mesh,
                    unreal.Vector(bx, by, bz - 5.0),
                    unreal.Rotator(0.0, rng.uniform(0.0, 360.0), 0.0),
                    unreal.Vector(b_scale, b_scale, b_scale),
                    "RA4_Bush_"
                )
                spawned_props += 1

            if grass_assets and rng.random() > 0.5:
                g_mesh = rng.choice(grass_assets)
                gx = x + rng.uniform(-120.0, 120.0)
                gy = y + rng.uniform(-120.0, 120.0)
                gz = sample_terrain_z(gx, gy)
                spawn_static_mesh(
                    g_mesh,
                    unreal.Vector(gx, gy, gz),
                    unreal.Rotator(0.0, rng.uniform(0.0, 360.0), 0.0),
                    unreal.Vector(1.4, 1.4, 1.4),
                    "RA4_Grass_"
                )
                spawned_props += 1

    # 5.2 Dramatic Rock Formations (Choke points & Hills)
    rock_clusters = [
        (5600.0, 7400.0, 800.0, 18),
        (7200.0, 5400.0, 800.0, 18),
        (3800.0, 4800.0, 700.0, 14),
        (9000.0, 8000.0, 700.0, 14),
        (4500.0, 9200.0, 600.0, 12),
        (8300.0, 3600.0, 600.0, 12),
    ]

    for cx, cy, radius, count in rock_clusters:
        for _ in range(count):
            if not rock_assets:
                break
            angle = rng.uniform(0.0, 2.0 * math.pi)
            dist = math.sqrt(rng.uniform(0.0, 1.0)) * radius
            x = cx + math.cos(angle) * dist
            y = cy + math.sin(angle) * dist
            z = sample_terrain_z(x, y)

            r_mesh = rng.choice(rock_assets)
            r_scale_xy = rng.uniform(1.2, 2.8)
            r_scale_z = rng.uniform(1.0, 2.4)
            spawn_static_mesh(
                r_mesh,
                unreal.Vector(x, y, z - 25.0),
                unreal.Rotator(rng.uniform(-15.0, 15.0), rng.uniform(0.0, 360.0), rng.uniform(-15.0, 15.0)),
                unreal.Vector(r_scale_xy, r_scale_xy, r_scale_z),
                "RA4_Rock_"
            )
            spawned_props += 1

    # 5.3 Central Plaza & Monument
    center_z = sample_terrain_z(MAP_CENTRE, MAP_CENTRE)
    if square_assets:
        sq_mesh = square_assets[0]
        for ox in [-400.0, 0.0, 400.0]:
            for oy in [-400.0, 0.0, 400.0]:
                pz = sample_terrain_z(MAP_CENTRE + ox, MAP_CENTRE + oy)
                spawn_static_mesh(
                    sq_mesh,
                    unreal.Vector(MAP_CENTRE + ox, MAP_CENTRE + oy, pz + 2.0),
                    unreal.Rotator(0.0, 0.0, 0.0),
                    unreal.Vector(1.0, 1.0, 1.0),
                    "RA4_BasePaving_"
                )
                spawned_props += 1

    if fountain_mesh:
        spawn_static_mesh(
            fountain_mesh,
            unreal.Vector(MAP_CENTRE, MAP_CENTRE, center_z + 4.0),
            unreal.Rotator(0.0, 0.0, 0.0),
            unreal.Vector(1.2, 1.2, 1.2),
            "RA4_Prop_Monument_"
        )
        spawned_props += 1
    elif statue_mesh:
        spawn_static_mesh(
            statue_mesh,
            unreal.Vector(MAP_CENTRE, MAP_CENTRE, center_z + 4.0),
            unreal.Rotator(0.0, 45.0, 0.0),
            unreal.Vector(1.5, 1.5, 1.5),
            "RA4_Prop_Monument_"
        )
        spawned_props += 1

    # 5.4 Base Zone Paving & Perimeter Lighting
    base_locations = [
        (3200.0, 3200.0, "Soviet"),
        (9600.0, 9600.0, "Alliance"),
    ]

    for bx, by, faction_name in base_locations:
        bz = sample_terrain_z(bx, by)
        if lamp_mesh:
            for angle_deg in [0, 90, 180, 270]:
                rad = math.radians(angle_deg)
                lx = bx + math.cos(rad) * 650.0
                ly = by + math.sin(rad) * 650.0
                lz = sample_terrain_z(lx, ly)
                spawn_static_mesh(
                    lamp_mesh,
                    unreal.Vector(lx, ly, lz),
                    unreal.Rotator(0.0, angle_deg + 180.0, 0.0),
                    unreal.Vector(1.0, 1.0, 1.0),
                    f"RA4_Prop_Lamp_{faction_name}_"
                )
                spawned_props += 1

        if fence_mesh:
            for angle_deg in [45, 135, 225, 315]:
                rad = math.radians(angle_deg)
                fx = bx + math.cos(rad) * 750.0
                fy = by + math.sin(rad) * 750.0
                fz = sample_terrain_z(fx, fy)
                spawn_static_mesh(
                    fence_mesh,
                    unreal.Vector(fx, fy, fz),
                    unreal.Rotator(0.0, angle_deg + 90.0, 0.0),
                    unreal.Vector(1.2, 1.2, 1.2),
                    f"RA4_Prop_Fence_{faction_name}_"
                )
                spawned_props += 1

    print(f"=== Successfully populated map with {spawned_props} high-quality environmental props! ===")

    # 6. Save Level Cleanly
    level_editor.save_current_level()
    print(f"=== Level saved cleanly at {LEVEL_PATH} ===")

if __name__ == "__main__":
    run()
