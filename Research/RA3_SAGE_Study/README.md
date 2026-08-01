# Red Alert 4 - Research Base Stage 1
Date of fixation: July 30, 2026.
## Target
Extract RTS architectural principles from officially published EA materials, without transferring protected code, art materials, titles, story or ready-made balance of Command & Conquer to the commercial Unreal-Project.
## Main Conclusion
The materials need to be divided into two different sources of knowledge:
1. **Generals / Zero Hour source code** - SAGE device reference: separation of general infrastructure, client view, game simulation and network.
2. **Red Alert 3 Modding Support** - data schema reference: objects, dependencies, commands, locomotion, Weapons, Armor, AI, UI, missions and visual states.
The game's production code must be created independently. The game repository cannot automatically import source EA XML, shaders, models, textures, audio or GPL code.
## Package contents
- `01_SOURCE_MATRIX.md` - matrix of official and secondary sources.
- `02_RA3_DATA_MODEL.md` - RA3 XML device and its useful abstractions.
- `03_SAGE_TO_UNREAL_ARCHITECTURE.md` - proposed Unreal code split.
- `04_CLEAN_ROOM_POLICY.md` - rules of legal and technical isolation.
- `05_IMPORTER_BLUEPRINT.md` - secure research pipeline.
- `06_RESEARCH_BACKLOG.md` - next research passes.
- `research_sources.json` - machine-readable registry of sources.
- `install_into_repo.sh` — installing documentation into the user’s local repository.
## Recommended location in the project
```text
red-alert-4/
  Research/
    RA3_SAGE_Study/
...files of this package...```

Do not place EA materials inside `Source/`, `Content/`, `Plugins/` or in a directory that falls within the packaged build.