#!/usr/bin/env python
"""Verify the generated fog materials expose every parameter the code sets.

Run: UnrealEditor-Cmd RedAlert4.uproject -run=pythonscript \
         -script=Tools/Editor/verify_fog_materials.py -unattended -nopause -nosplash

Why this exists: Unreal silently ignores SetScalarParameterValue for a parameter
the material does not have. When ADR-0030's fog first landed, the code set four
parameters and the generated materials declared one -- fog strength and the
high-contrast accessibility mode did nothing at all, with no error anywhere.
Nothing in a compile or a headless test can catch that, because the contract is
between a C++ string literal and a generated asset. This check closes that gap
and belongs in CI once the editor runs there.

Exits non-zero on mismatch so it can gate a build.
"""
import sys
import unreal

# Must match URA4SimWorldSubsystem::PublishFogParametersToTerrain /
# PublishFogParametersToCamera. If you add a parameter there, add it here.
EXPECTED_SCALARS = {
    "RA4FogWorldWidth",
    "RA4FogWorldHeight",
    "RA4FogStrength",
    "RA4FogHighContrast",
}
EXPECTED_TEXTURES = {"RA4FogVisibility"}

MATERIALS = [
    "/Game/RA4/Generated/Terrain/M_RA4_TerrainLayered",
    "/Game/RA4/Generated/Terrain/M_RA4_FogPostProcess",
]

EXPECTED_DOMAIN = {
    "/Game/RA4/Generated/Terrain/M_RA4_TerrainLayered": unreal.MaterialDomain.MD_SURFACE,
    "/Game/RA4/Generated/Terrain/M_RA4_FogPostProcess": unreal.MaterialDomain.MD_POST_PROCESS,
}

failures = []

for path in MATERIALS:
    mat = unreal.EditorAssetLibrary.load_asset(path)
    if mat is None:
        failures.append(
            "%s does not exist -- run the RA4LayeredTerrainSetup commandlet" % path
        )
        continue

    domain = mat.get_editor_property("material_domain")
    if domain != EXPECTED_DOMAIN[path]:
        failures.append(
            "%s has domain %s, expected %s" % (path, domain, EXPECTED_DOMAIN[path])
        )

    scalars = {str(n) for n in unreal.MaterialEditingLibrary.get_scalar_parameter_names(mat)}
    textures = {str(n) for n in unreal.MaterialEditingLibrary.get_texture_parameter_names(mat)}

    missing_s = sorted(EXPECTED_SCALARS - scalars)
    missing_t = sorted(EXPECTED_TEXTURES - textures)
    if missing_s:
        failures.append("%s missing scalar parameters: %s" % (path, ", ".join(missing_s)))
    if missing_t:
        failures.append("%s missing texture parameters: %s" % (path, ", ".join(missing_t)))

    print("CHECKED %s: %d scalars, %d textures" % (path, len(scalars), len(textures)))

if failures:
    for f in failures:
        unreal.log_error("FOG MATERIAL CHECK FAILED: %s" % f)
    print("FOG_MATERIAL_CHECK: FAILED (%d problems)" % len(failures))
    sys.exit(1)

print("FOG_MATERIAL_CHECK: PASSED")
