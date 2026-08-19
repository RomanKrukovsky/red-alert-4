# Copyright (c) Red Alert 4 project.
import math
import random
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
MAP_EXTENT = 12800.0
MAP_CENTRE = 6400.0
SEA_LEVEL = 0.0

def sample_terrain_z(world_x, world_y):
    """Deterministic mathematical height formula matching RA4LandscapeCommandlet rolling hills."""
    Seed = 20260730
    SeedPhase = float(Seed % 1000) * 0.01
    v = 0.0
    v += math.sin(world_x * 0.00028 + SeedPhase) * math.cos(world_y * 0.00024 + SeedPhase * 1.7)
    v += 0.5 * math.sin(world_x * 0.0006 - SeedPhase * 0.5) * math.cos(world_y * 0.0005 + SeedPhase)
    v += 0.25 * math.sin((world_x + world_y) * 0.0009 + SeedPhase * 2.0)
    height_norm = v / 1.75
    amplitude_units = 220.0
    return height_norm * amplitude_units

def run():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError(f"Failed to load {LEVEL_PATH}")

    # 1. Assign MI_Ground01 / MI_Ground02 directly to Landscape so ground is textured (NOT black)
    mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01.MI_Ground01")
    if mat_ground is None:
        mat_ground = unreal.load_asset("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground02.MI_Ground02")

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.LandscapeProxy):
            if mat_ground:
                actor.set_editor_property("landscape_material", mat_ground)
                print(f"Set landscape material to {mat_ground.get_path_name()} on {actor.get_actor_label()}")

    # 2. Snap ALL actors to the terrain surface
    for actor in actor_subsystem.get_all_level_actors():
        label = actor.get_actor_label()
        loc = actor.get_actor_location()

        # Buildings: Soviet and Alliance structures
        if label.startswith("Soviet_") or label.startswith("Alliance_") or "Building" in label:
            target_z = sample_terrain_z(loc.x, loc.y)
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)
            print(f"Snapped building {label} from Z={loc.z:.1f} -> Z={target_z:.1f}")

        # Player starts
        elif label.startswith("RA4_PlayerStart_"):
            target_z = sample_terrain_z(loc.x, loc.y) + 50.0 # 50 units above ground
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)
            print(f"Snapped player start {label} from Z={loc.z:.1f} -> Z={target_z:.1f}")

        # Roads and Bridges
        elif "Road" in label or "Bridge" in label:
            target_z = sample_terrain_z(loc.x, loc.y)
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)
            print(f"Snapped road/bridge {label} from Z={loc.z:.1f} -> Z={target_z:.1f}")

        # Ore rocks
        elif label.startswith("RA4_Ore_"):
            target_z = sample_terrain_z(loc.x, loc.y)
            actor.set_actor_location(unreal.Vector(loc.x, loc.y, target_z), False, True)

    # Save level
    level_editor.save_current_level()
    print("Level with snapped actors and textured ground saved successfully!")

if __name__ == "__main__":
    run()
