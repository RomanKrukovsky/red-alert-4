"""Read-only diagnostic for the production skirmish map.

Answers, with measured numbers rather than assumptions, why the map renders the
way it does: what lights exist, what the landscape is made of, where foliage
actually sits, and which meshes carry which materials.

Run headless:
    UnrealEditor-Cmd RedAlert4.uproject \
        -ExecutePythonScript="Tools/Editor/diagnose_map_visuals.py"
"""

import unreal

MAP = "/Game/Maps/RA4_Skirmish_Production"


def out(line):
    unreal.log("RA4DIAG| %s" % line)


def load():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    out("loaded %s" % MAP)


def actors():
    return unreal.EditorLevelLibrary.get_all_level_actors()


def report_classes(all_actors):
    counts = {}
    for a in all_actors:
        name = a.get_class().get_name()
        counts[name] = counts.get(name, 0) + 1
    out("--- actor classes (%d actors total) ---" % len(all_actors))
    for name in sorted(counts, key=lambda k: -counts[k]):
        out("  %-44s %d" % (name, counts[name]))


def report_lighting(all_actors):
    out("--- lighting ---")
    found = False
    for a in all_actors:
        cls = a.get_class().get_name()
        if cls not in ("DirectionalLight", "SkyLight", "SkyAtmosphere",
                       "ExponentialHeightFog", "PostProcessVolume",
                       "VolumetricCloud", "AtmosphericFog", "SphereReflectionCapture"):
            continue
        found = True
        rot = a.get_actor_rotation()
        line = "  %-26s loc=%s rot=(p=%.1f y=%.1f r=%.1f)" % (
            cls, a.get_actor_location(), rot.pitch, rot.yaw, rot.roll)
        comp = a.get_component_by_class(unreal.LightComponent)
        if comp:
            line += " intensity=%.3f colour=%s affects_world=%s mobility=%s" % (
                comp.get_editor_property("intensity"),
                comp.get_editor_property("light_color"),
                comp.get_editor_property("affects_world"),
                comp.get_editor_property("mobility"))
        out(line)
    if not found:
        out("  NONE -- no lights, no sky, no atmosphere in this level")


def report_landscape(all_actors):
    out("--- landscape ---")
    found = False
    for a in all_actors:
        if "Landscape" not in a.get_class().get_name():
            continue
        found = True
        out("  %s (%s)" % (a.get_actor_label(), a.get_class().get_name()))
        out("    loc=%s scale=%s" % (a.get_actor_location(), a.get_actor_scale3d()))
        try:
            mat = a.get_editor_property("landscape_material")
            out("    material=%s" % (mat.get_path_name() if mat else "NONE (renders default grey/black)"))
        except Exception as exc:
            out("    material read failed: %s" % exc)
        comps = a.get_components_by_class(unreal.LandscapeComponent)
        out("    landscape components=%d" % len(comps))
    if not found:
        out("  NONE -- there is no ALandscape in this level")


def report_foliage(all_actors):
    out("--- foliage ---")
    total = 0
    for a in all_actors:
        if "InstancedFoliageActor" not in a.get_class().get_name():
            continue
        for comp in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            mesh = comp.get_editor_property("static_mesh")
            n = comp.get_instance_count()
            total += n
            if n:
                out("  %-34s instances=%d" % (
                    mesh.get_name() if mesh else "<no mesh>", n))
    out("  total foliage instances = %d" % total)


def report_meshes(all_actors):
    out("--- static mesh actors: mesh -> materials ---")
    seen = {}
    for a in all_actors:
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        comp = a.get_component_by_class(unreal.StaticMeshComponent)
        if not comp:
            continue
        mesh = comp.get_editor_property("static_mesh")
        key = mesh.get_path_name() if mesh else "<none>"
        if key in seen:
            seen[key][0] += 1
            continue
        mats = []
        for i in range(comp.get_num_materials()):
            m = comp.get_material(i)
            mats.append(m.get_path_name() if m else "<null>")
        seen[key] = [1, mats, a.get_actor_location().z]
    for key in sorted(seen):
        count, mats, z = seen[key]
        out("  x%-4d %s  z=%.0f" % (count, key, z))
        for m in mats:
            out("          mat: %s" % m)


def report_bounds(all_actors):
    out("--- world extent of visible geometry ---")
    xs, ys, zs = [], [], []
    for a in all_actors:
        if isinstance(a, (unreal.StaticMeshActor,)) or "Landscape" in a.get_class().get_name():
            loc = a.get_actor_location()
            xs.append(loc.x); ys.append(loc.y); zs.append(loc.z)
    if xs:
        out("  x %.0f .. %.0f" % (min(xs), max(xs)))
        out("  y %.0f .. %.0f" % (min(ys), max(ys)))
        out("  z %.0f .. %.0f" % (min(zs), max(zs)))


def main():
    load()
    a = actors()
    report_classes(a)
    report_lighting(a)
    report_landscape(a)
    report_foliage(a)
    report_meshes(a)
    report_bounds(a)
    out("=== diagnostic complete ===")


main()
