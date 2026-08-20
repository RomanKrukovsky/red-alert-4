# Copyright (c) Red Alert 4 project.
"""Dump what the skirmish map is actually made of, as facts rather than guesses.

Run: UnrealEditor-Cmd RedAlert4.uproject -run=pythonscript \
         -script="<abs path>/Tools/Editor/audit_scene.py" \
         -unattended -nopause -nosplash
     then read /tmp/ra4_scene_audit.txt

Why this exists: the map rendered white, then black, and every fix was aimed at a
guess. Two things made that possible. A commandlet python script is not run as
"__main__", so a file guarded that way does nothing while reporting success; and
unreal.log output does not reach the commandlet's captured stdout, so a script
that does run still looks silent. Both are worked around here -- run at import,
report to a file -- because the point of this file is to be believed.

It changes nothing. It only reports.
"""
import traceback

import unreal

LEVEL_PATH = "/Game/Maps/RA4_Skirmish_Production"
REPORT_PATH = "/tmp/ra4_scene_audit.txt"

_lines = []


def say(msg=""):
    _lines.append(str(msg))
    try:
        unreal.log(f"[RA4 Scene Audit] {msg}")
    except Exception:
        pass


def mat_name(m):
    if m is None:
        return "<none>"
    try:
        return m.get_path_name()
    except Exception:
        return "<unreadable>"


def audit_landscape(actor):
    say("  LANDSCAPE %s" % actor.get_actor_label())
    try:
        m = actor.get_editor_property("landscape_material")
        say("    material         : %s" % mat_name(m))
    except Exception as exc:
        say("    material         : could not read (%s)" % exc)
    try:
        say("    location         : %s" % actor.get_actor_location())
        say("    scale            : %s" % actor.get_actor_scale3d())
    except Exception:
        pass


def audit_static_mesh(actor):
    comp = actor.static_mesh_component
    mesh = comp.static_mesh if comp else None
    mesh_name = mesh.get_path_name() if mesh else "<none>"
    mats = []
    if comp:
        try:
            for i in range(comp.get_num_materials()):
                mats.append(mat_name(comp.get_material(i)))
        except Exception:
            pass
    say("  MESH %-28s z=%-9.1f mesh=%s" % (
        actor.get_actor_label(), actor.get_actor_location().z, mesh_name))
    for m in mats:
        say("        material: %s" % m)


def audit_light(actor, comp_class, label):
    comp = actor.get_component_by_class(comp_class)
    if comp is None:
        say("  %s %s: no component" % (label, actor.get_actor_label()))
        return
    bits = []
    for prop in ("intensity", "mobility", "cast_shadows", "atmosphere_sun_light",
                 "real_time_capture"):
        try:
            bits.append("%s=%s" % (prop, comp.get_editor_property(prop)))
        except Exception:
            pass
    say("  %s %-22s %s" % (label, actor.get_actor_label(), "  ".join(bits)))


def audit_postprocess(actor):
    say("  POSTPROCESS %s" % actor.get_actor_label())
    try:
        say("    unbound          : %s" % actor.get_editor_property("unbound"))
        pp = actor.get_editor_property("settings")
        for prop in ("auto_exposure_method", "auto_exposure_bias",
                     "auto_exposure_min_brightness", "auto_exposure_max_brightness",
                     "override_auto_exposure_method",
                     "override_auto_exposure_min_brightness",
                     "override_auto_exposure_max_brightness"):
            try:
                say("    %-30s = %s" % (prop, pp.get_editor_property(prop)))
            except Exception:
                say("    %-30s = <unreadable>" % prop)
        try:
            blendables = pp.get_editor_property("weighted_blendables")
            say("    weighted_blendables entries: %d" % len(blendables.array))
            for b in blendables.array:
                say("        %s (weight %s)" % (mat_name(b.object), b.weight))
        except Exception as exc:
            say("    weighted_blendables: <unreadable> (%s)" % exc)
    except Exception as exc:
        say("    settings unreadable: %s" % exc)


def audit_material_params(path):
    m = unreal.load_asset(path)
    if m is None:
        say("  %s : MISSING" % path)
        return
    say("  %s" % path)
    try:
        names = unreal.MaterialEditingLibrary.get_scalar_parameter_names(m)
        say("    scalar params  : %s" % ", ".join(str(n) for n in names))
    except Exception as exc:
        say("    scalar params  : <unreadable> (%s)" % exc)
    try:
        tnames = unreal.MaterialEditingLibrary.get_texture_parameter_names(m)
        say("    texture params : %s" % ", ".join(str(n) for n in tnames))
    except Exception as exc:
        say("    texture params : <unreadable> (%s)" % exc)


def audit_texture(path):
    t = unreal.load_asset(path)
    if t is None:
        say("  %-64s MISSING" % path)
        return
    bits = []
    for prop in ("srgb", "compression_settings"):
        try:
            bits.append("%s=%s" % (prop, t.get_editor_property(prop)))
        except Exception:
            pass
    try:
        bits.append("size=%dx%d" % (t.blueprint_get_size_x(), t.blueprint_get_size_y()))
    except Exception:
        pass
    say("  %-64s %s" % (path.split("/")[-1], "  ".join(bits)))


def run():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_editor.load_level(LEVEL_PATH):
        say("FAILED to load %s" % LEVEL_PATH)
        return

    actors = actor_subsystem.get_all_level_actors()
    say("=" * 78)
    say("SCENE AUDIT  %s" % LEVEL_PATH)
    say("=" * 78)
    say("actors in level: %d" % len(actors))
    say()

    say("--- lights and environment ---")
    for a in actors:
        if isinstance(a, unreal.DirectionalLight):
            audit_light(a, unreal.DirectionalLightComponent, "SUN     ")
        elif isinstance(a, unreal.SkyLight):
            audit_light(a, unreal.SkyLightComponent, "SKYLIGHT")
        elif isinstance(a, unreal.SkyAtmosphere):
            comp = a.get_component_by_class(unreal.SkyAtmosphereComponent)
            tm = "<unreadable>"
            if comp:
                try:
                    tm = comp.get_editor_property("transform_mode")
                except Exception:
                    pass
            say("  ATMOSPHERE %-20s transform_mode=%s  z=%.1f"
                % (a.get_actor_label(), tm, a.get_actor_location().z))
        elif isinstance(a, unreal.ExponentialHeightFog):
            comp = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
            d = comp.get_editor_property("fog_density") if comp else "?"
            say("  HEIGHTFOG  %-20s density=%s  z=%.1f"
                % (a.get_actor_label(), d, a.get_actor_location().z))
        elif isinstance(a, unreal.PostProcessVolume):
            audit_postprocess(a)
    say()

    say("--- landscape ---")
    found_landscape = False
    for a in actors:
        if isinstance(a, unreal.LandscapeProxy):
            found_landscape = True
            audit_landscape(a)
    if not found_landscape:
        say("  NO LANDSCAPE ACTOR IN THIS LEVEL")
        say("  (so the ground is static meshes, and a landscape material has")
        say("   nowhere to apply -- which would explain fog never reaching it)")
    say()

    say("--- large static meshes (the ground candidates) ---")
    count = 0
    for a in actors:
        if isinstance(a, unreal.StaticMeshActor):
            s = a.get_actor_scale3d()
            if max(s.x, s.y) >= 50.0:
                audit_static_mesh(a)
                count += 1
    say("  large meshes: %d" % count)
    say()

    say("--- generated materials ---")
    for p in ("/Game/RA4/Generated/Terrain/M_RA4_TerrainLayered",
              "/Game/RA4/Generated/Terrain/M_RA4_FogPostProcess",
              "/Game/RA4/Generated/Terrain/M_RA4_Ground_Bulletproof"):
        audit_material_params(p)
    say()

    say("--- terrain textures ---")
    for n in ("Grass", "Dirt", "Rock", "Sand"):
        for kind in ("Color", "Normal", "Roughness"):
            audit_texture("/Game/RA4/Generated/Terrain/T_RA4_%s_%s" % (n, kind))
    audit_texture("/Game/RA4/Generated/Terrain/T_RA4_FogDefault")
    say()
    say("audit complete")


try:
    run()
except Exception:
    say("EXCEPTION:\n" + traceback.format_exc())

try:
    with open(REPORT_PATH, "w") as fh:
        fh.write("\n".join(_lines) + "\n")
except Exception:
    pass
