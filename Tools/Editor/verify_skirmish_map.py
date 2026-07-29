# Copyright (c) Red Alert 4 project.
# Verifies the generated skirmish level actually contains what the build script claims.
import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish"
GROUND_MATERIAL_PATH = (
    "/Game/RA4/Materials/M_RA4Ground_Lit.M_RA4Ground_Lit"
)
SUN_INTENSITY_LUX = 75000.0
SUN_PITCH = -48.0
SUN_YAW = 145.0

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(LEVEL_PATH)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
unreal.log_warning("[VERIFY] level={} actors={}".format(LEVEL_PATH, len(actors)))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world_settings = world.get_world_settings()
if not world_settings.get_editor_property("force_no_precomputed_lighting"):
    raise RuntimeError(
        "[VERIFY] skirmish level must disable precomputed lighting"
    )

labels = {actor.get_actor_label() for actor in actors}
required_labels = {
    "RA4_Ground",
    "RA4_PlayerStart",
    "RA4_Sun",
    "RA4_SkyLight",
    "RA4_SkyAtmosphere",
}
missing_labels = sorted(required_labels - labels)
if missing_labels:
    raise RuntimeError(
        "[VERIFY] skirmish level is missing required actors: {}".format(
            ", ".join(missing_labels)
        )
    )

atmospheres = [
    actor for actor in actors if actor.get_actor_label() == "RA4_SkyAtmosphere"
]
if len(atmospheres) != 1:
    raise RuntimeError(
        "[VERIFY] skirmish level must contain exactly one RA4_SkyAtmosphere"
    )

actors_by_label = {actor.get_actor_label(): actor for actor in actors}
sun_component = actors_by_label["RA4_Sun"].get_component_by_class(
    unreal.DirectionalLightComponent
)
sky_component = actors_by_label["RA4_SkyLight"].get_component_by_class(
    unreal.SkyLightComponent
)
ground_component = actors_by_label["RA4_Ground"].get_component_by_class(
    unreal.StaticMeshComponent
)
for label, component in (
    ("RA4_Sun", sun_component),
    ("RA4_SkyLight", sky_component),
):
    if (
        component is None
        or component.get_editor_property("mobility")
        != unreal.ComponentMobility.MOVABLE
    ):
        raise RuntimeError(
            "[VERIFY] {} must use movable lighting".format(label)
        )

sun_intensity = sun_component.get_editor_property("intensity")
if abs(sun_intensity - SUN_INTENSITY_LUX) > 1.0:
    raise RuntimeError(
        "[VERIFY] RA4_Sun intensity is {}, expected {} lux".format(
            sun_intensity, SUN_INTENSITY_LUX
        )
    )

sun_rotation = actors_by_label["RA4_Sun"].get_actor_rotation()
if (
    abs(sun_rotation.pitch - SUN_PITCH) > 0.1
    or abs(sun_rotation.yaw - SUN_YAW) > 0.1
):
    raise RuntimeError(
        "[VERIFY] RA4_Sun rotation is ({}, {}), expected ({}, {})".format(
            sun_rotation.pitch, sun_rotation.yaw, SUN_PITCH, SUN_YAW
        )
    )

ground_material = (
    ground_component.get_material(0) if ground_component is not None else None
)
if (
    ground_material is None
    or ground_material.get_path_name()
    != GROUND_MATERIAL_PATH
):
    raise RuntimeError(
        "[VERIFY] RA4_Ground material is {}, expected {}".format(
            ground_material.get_path_name()
            if ground_material is not None
            else "None",
            GROUND_MATERIAL_PATH,
        )
    )

for a in actors:
    loc = a.get_actor_location()
    scale = a.get_actor_scale3d()
    unreal.log_warning("[VERIFY] {} :: {} pos=({:.0f},{:.0f},{:.0f}) scale=({:.1f},{:.1f},{:.1f})".format(
        a.get_actor_label(), a.get_class().get_name(), loc.x, loc.y, loc.z, scale.x, scale.y, scale.z))
