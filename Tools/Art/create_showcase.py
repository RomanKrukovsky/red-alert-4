#!/usr/bin/env python3
"""Create reproducible day/night UE showcase maps for the 36-model art slice."""

from __future__ import annotations

import json
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
MANIFEST_PATH = PROJECT_ROOT / "Tools/Art/model_manifest.json"


def spawn_actor(actor_class, location: unreal.Vector, label: str):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location)
    if actor is not None:
        actor.set_actor_label(label)
    return actor


def build_map(map_path: str, *, night: bool) -> None:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)

    ground_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    ground_material = unreal.load_asset("/Game/RA4/Materials/M_RA4Ground_Lit")
    ground = spawn_actor(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, -55.0), "Showcase_Ground")
    if ground is not None:
        ground.static_mesh_component.set_static_mesh(ground_mesh)
        ground.static_mesh_component.set_world_scale3d(unreal.Vector(120.0, 82.0, 1.0))
        if ground_material is not None:
            ground.static_mesh_component.set_material(0, ground_material)

    sun = spawn_actor(unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 6000.0), "Showcase_Moon" if night else "Showcase_Sun")
    if sun is not None:
        # Rotator is Pitch/Yaw/Roll. The previous values put pitch at zero, so
        # the directional light travelled parallel to the ground and left the
        # regenerated atmosphere showcase effectively black.
        light_origin = unreal.Vector(0.0, -10000.0, 10000.0 if not night else 3200.0)
        sun.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(light_origin, unreal.Vector()), False)
        sun.light_component.set_editor_property("intensity", 0.12 if night else 1.6)
        sun.light_component.set_editor_property("atmosphere_sun_light", True)
        sun.light_component.set_editor_property(
            "light_color",
            unreal.Color(92, 128, 210, 255) if night else unreal.Color(255, 228, 190, 255),
        )

    sky = spawn_actor(unreal.SkyLight, unreal.Vector(0.0, 0.0, 5500.0), "Showcase_SkyLight")
    if sky is not None:
        sky.light_component.set_editor_property("intensity", 0.25 if night else 0.6)
        sky.light_component.set_editor_property("real_time_capture", True)

    spawn_actor(unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0), "Showcase_SkyAtmosphere")
    fog = spawn_actor(unreal.ExponentialHeightFog, unreal.Vector(0.0, 0.0, 0.0), "Showcase_HeightFog")
    if fog is not None:
        fog.component.set_editor_property("fog_density", 0.0015 if not night else 0.0025)
        fog.component.set_editor_property("start_distance", 3500.0)

    if night:
        fill = spawn_actor(unreal.PointLight, unreal.Vector(0.0, 0.0, 2200.0), "Showcase_NightFill")
        if fill is not None:
            fill.light_component.set_editor_property("intensity", 42000.0)
            fill.light_component.set_editor_property("attenuation_radius", 9000.0)
            fill.light_component.set_editor_property("light_color", unreal.Color(55, 110, 255, 255))

    rows = {"Soviet": -3000.0, "Alliance": -1000.0, "Coalition": 1000.0, "Chronolegion": 3000.0}
    indices = {faction: 0 for faction in rows}
    for entry in manifest["assets"]:
        mesh = unreal.load_asset(entry["target"])
        if not isinstance(mesh, unreal.StaticMesh):
            unreal.log_error(f"RA4Showcase: missing {entry['target']}")
            continue
        index = indices[entry["faction"]]
        indices[entry["faction"]] += 1
        x = (index - 4) * 1250.0
        y = rows[entry["faction"]]
        actor = spawn_actor(unreal.StaticMeshActor, unreal.Vector(x, y, 0.0), f"Showcase_{entry['stable_id']}")
        if actor is not None:
            actor.static_mesh_component.set_static_mesh(mesh)

    camera_location = unreal.Vector(0.0, -8000.0, 6500.0)
    camera_rotation = unreal.MathLibrary.find_look_at_rotation(camera_location, unreal.Vector())
    camera = spawn_actor(unreal.CameraActor, camera_location, "Showcase_Camera")
    if camera is not None:
        camera.set_actor_rotation(camera_rotation, False)
        camera.camera_component.set_editor_property("field_of_view", 58.0)

    # Save a useful editor viewport as well as a CameraActor. A blank map keeps
    # whichever construction view happened to be active otherwise, so opening
    # the showcase could appear to contain only the ground plane.
    unreal.EditorLevelLibrary.set_level_viewport_camera_info(camera_location, camera_rotation)

    unreal.EditorLoadingAndSavingUtils.save_map(world, map_path)
    unreal.log(f"RA4Showcase: saved {map_path} with 36 production-draft meshes")


def main() -> None:
    build_map("/Game/Maps/Art/RA4_ArtShowcase_Day", night=False)
    build_map("/Game/Maps/Art/RA4_ArtShowcase_Night", night=True)


if __name__ == "__main__":
    main()
