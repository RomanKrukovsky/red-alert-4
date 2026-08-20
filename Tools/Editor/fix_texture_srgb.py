#!/usr/bin/env python
"""Turn sRGB off on the data textures that must be linear, and report the rest.

Run: UnrealEditor-Cmd RedAlert4.uproject -run=pythonscript \
         -script="<abs path>/Tools/Editor/fix_texture_srgb.py" \
         -unattended -nopause -nosplash

Why this exists: a roughness, normal or mask map carries measurements, not
colour. Imported with sRGB on, the value the material reads is the value on disk
run through a gamma curve -- and Unreal refuses to compile a material that
samples an sRGB texture through a Linear sampler at all. One such node took down
M_RA4_Ground_Bulletproof, so the skirmish map rendered on Unreal's default
material: a white floor with no terrain. The compile failure is logged as a
warning, the game starts anyway, and nothing else says a word.

Two notes on running this as a commandlet, both learned the hard way:
a relative -script path is silently ignored (the editor starts and exits without
running anything), and the asset registry is not scanned unless asked, so a
search returns nothing and the script "succeeds" having looked at zero assets.
"""
import traceback

import unreal

# Suffixes that mean "this texture is data". Colour maps are left alone: sRGB is
# correct for them and clearing it would wash the terrain out.
LINEAR_SUFFIXES = ("_Roughness", "_Normal", "_Mask", "_Height", "_AO",
                   "_Metallic", "_Specular")

SEARCH_PATHS = ["/Game/RA4", "/Game/Maps", "/Game/ThirdParty"]


REPORT_PATH = "/tmp/ra4_srgb_report.txt"
_lines = []


def say(msg):
    """Log and record. unreal.log output does not reliably reach the commandlet
    log under -unattended, so the report is also written to a file the caller can
    read -- a check whose result you cannot see is not a check."""
    _lines.append(str(msg))
    try:
        unreal.log(str(msg))
    except Exception:
        pass


def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    # Without this the registry is empty in a commandlet and every search below
    # returns nothing, which looks exactly like "there was nothing to fix".
    say("scanning asset registry: %s" % ", ".join(SEARCH_PATHS))
    registry.scan_paths_synchronous(SEARCH_PATHS, force_rescan=True)
    registry.wait_for_completion()

    fixed, already_ok, failed = [], [], []
    examined = 0

    for path in SEARCH_PATHS:
        for data in registry.get_assets_by_path(path, recursive=True):
            if str(data.asset_class_path.asset_name) != "Texture2D":
                continue
            name = str(data.asset_name)
            if not name.endswith(LINEAR_SUFFIXES):
                continue
            examined += 1

            texture = data.get_asset()
            if texture is None:
                failed.append("%s: could not load" % name)
                continue

            if not texture.get_editor_property("srgb"):
                already_ok.append(name)
                continue

            texture.set_editor_property("srgb", False)
            if unreal.EditorAssetLibrary.save_loaded_asset(texture, False):
                fixed.append(name)
            else:
                failed.append("%s: sRGB cleared but save failed" % name)

    say("=" * 66)
    say("sRGB audit of data textures")
    say("=" * 66)
    say("  data textures examined : %d" % examined)
    say("  already linear         : %d" % len(already_ok))
    say("  fixed now              : %d" % len(fixed))
    for n in sorted(fixed):
        say("      fixed %s" % n)
    for n in sorted(failed):
        say("      FAILED %s" % n)

    if examined == 0:
        say("RESULT: FAILED -- examined zero textures, the search found nothing")
    elif failed:
        say("RESULT: FAILED -- %d texture(s) could not be corrected" % len(failed))
    else:
        say("RESULT: OK -- %d data texture(s) are linear" % (len(fixed) + len(already_ok)))


try:
    main()
except Exception:
    _lines.append("EXCEPTION:\n" + traceback.format_exc())

with open(REPORT_PATH, "w") as fh:
    fh.write("\n".join(_lines) + "\n")
