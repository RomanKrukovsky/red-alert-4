# Model Preview Pipeline

How to look at the game's models without launching the Unreal editor, and why that
is currently the only reliable way.

## Quick use

```sh
# one faction
blender -b --python Tools/Art/render_model_previews.py -- \
    --out /tmp/previews --faction Soviet

# everything, plus a single side-by-side sheet
blender -b --python Tools/Art/render_model_previews.py -- \
    --out /tmp/previews --all --contact-sheet --resolution 640
```

Outputs into `--out`:

| File | Purpose |
| :--- | :--- |
| `SM_<Faction>_<Name>.png` | one model, neutral studio material, RTS-style 3/4 view |
| `contact_sheet.png` | every render tiled — how silhouette clashes become visible |
| `manifest.json` | triangle count and bounds per model, so regressions are measurable |

Renders LOD0 base models only. `_LOD1..3`, `_Turret`, `_Wheel` and `_Destroyed`
are parts of a model, not models to review, so they are skipped.

The preview material is deliberately **not** the faction material. Faction colour
tinting hides exactly what a review needs to judge: silhouette, proportion and
detail density.

## Why not capture from the Unreal editor

Three separate routes were tried and measured on 2026-08-06:

1. **`Tools/Editor/capture_map_screenshots.py` produces no images.** It writes
   `<name>.png.meta` text files describing camera positions and logs
   `Status: Captured`. There is no render call in it. Any report that cited it as
   visual evidence was citing text files.

2. **Launching `UnrealEditor` with a map argument makes the editor exit on its
   own** after roughly two minutes. Verified by isolation: with
   `RedAlert4.uproject RA4_ArtShowcase_Day` the log shows PIE starting
   (`Creating play world package: /Game/Maps/Art/UEDPIE_0_...`), the RA4 camera
   logging focus state, then `Cmd: QUIT_EDITOR` /
   `UUnrealEdEngine::CloseEditor()`. Launched with **no** map argument the same
   build stayed alive past 230 s with zero PIE sessions. So the trigger is
   map-argument-driven PIE, not the project failing to boot.

3. **The MCP editor bridge cannot reach this project.** The engine's own MCP
   server does start (`Starting MCP server on port 8000`) and answers HTTP, but
   returns `errors.com.epicgames.httpserver.route_handler_not_found` for the
   bridge's routes — that bridge expects the RemoteControl plugin, which
   `RedAlert4.uproject` does not enable.

A headless `-run=pythonscript` commandlet *does* work for **inspection** (it
successfully reported 42 actors and every material slot on the showcase map), but
`AutomationLibrary.take_high_res_screenshot` needs a live viewport and produced no
file under `-nullrhi`.

Blender is already a hard dependency of the art pipeline — the `.blend` sources
live in `ArtSource/RA4/Models` — so rendering there adds nothing new to the
toolchain and works unattended.

## Blender version notes

Written against Blender 5.1. Two API facts matter and are handled in the script:

- `BLENDER_EEVEE_NEXT` does not exist; the valid enum is
  `('BLENDER_EEVEE', 'BLENDER_WORKBENCH', 'CYCLES')`.
- `Scene.node_tree` was removed, so the compositor cannot be used to tile images.
  The contact sheet instead textures each finished PNG onto an emission-shaded
  plane under an orthographic camera, which is pixel-exact and version-stable.

Unrelated console noise from the user's installed `auto_rig_pro` add-on
(`arp_debug_mode`, `callback_remove`) appears on every `read_factory_settings`
call. It is harmless and does not affect output; do not mistake it for a failure
of this script — check for `ALL_DONE` on the last line instead.

## Measured state of the 36 models (2026-08-06)

Rendered all four factions and read `manifest.json`:

- **71 776 triangles total, mean 1 993 per model.** Reasonable for RTS scale.
- **Building footprints are exactly consistent across factions** — every ConYard
  is 8.0 x 8.0 m, every PowerPlant 6.0 x 6.0 m, every Barracks 6.0 x 6.0 m. That
  is the property that matters for placement grids and it holds.
- **Chronolegion is systematically the lowest-poly faction** in every mobile role:
  harvester 1 688 vs 2 888 for Soviet/Coalition, artillery 1 720 vs 2 996, tank
  1 828 vs 3 028. A 1.7x detail gap that will read as "the Chrono faction looks
  cheaper" once textures land.
- **`SM_Coalition_CO_KamakiriWalker` is out of family for a main tank**: 3.5 x
  2.19 x 2.76 m against 5.0-5.6 m long for every other MBT — shorter and taller.
  That may be intentional for a walker silhouette, but it is a deliberate design
  call that should be recorded rather than discovered later during balance work.
- **No unique textures exist.** One shared `M_RA4_PaintedMetal_PBR` plus
  per-faction colour instances. Faction identity is currently carried by
  silhouette and hue only.

These are the concrete gaps to close before this set can be called production art,
and they are now measurable: re-run the script and diff `manifest.json`.
