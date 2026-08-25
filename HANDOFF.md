# Red Alert 4 project handoff

## Identity and environment

- Workspace: `Scarlet-Horizon`
- Unreal project: `RedAlert4.uproject`
- Supported engine: **Unreal Engine 5.8**
- Project status: pre-alpha RTS prototype using a deterministic C++ simulation with an Unreal presentation layer.
- Clean-room rule: use original content and neutral identifiers; do not add Electronic Arts assets, code, data, or protected names.

## Architecture boundary

`RA4Core`, `RA4Content`, `RA4Simulation`, `RA4Navigation`, `RA4Input`, `RA4Presentation`, `RA4Replay`, and related headless modules are designed to compile without Unreal Engine. The simulation uses fixed-point math and mutates authoritative state through `SimWorld::ApplyCommand`.

`RedAlert4` and `RA4UI` provide the Unreal-facing layer. The supported production UI is native **CommonUI + UMG + Slate**, fed by C++ view models and presentation snapshots. The earlier React/Vite and Noesis prototypes were removed and must not be restored as supported build paths.

## Verification commands

Run from the workspace root:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
```

Use the CMake/CTest workflow above for the engine-free suite; retired script and Python-helper paths are not supported.

For editor work, use Unreal Engine 5.8 to open `RedAlert4.uproject`; if the editor is on `PATH`:

```bash
UnrealEditor RedAlert4.uproject
```

## Key checks before handoff

1. Keep deterministic code free of Unreal dependencies and non-deterministic state.
2. Run the headless CMake/CTest sequence after headless changes.
3. Check the affected UE 5.8 editor or PIE path after Unreal/UI changes.
4. Keep product documentation aligned with CommonUI + UMG + Slate as the only production UI stack.
5. Do not claim a fixed total number of tests; report the current command output instead.

## Useful references

- [Current project state](Docs/Agent/PROJECT_STATE.md)
- [Current project status](Docs/Agent/PROJECT_STATUS.md)
- [Quick start](QUICK_START.md)
- [Contributing](CONTRIBUTING.md)
- [Headless build configuration](Tools/HeadlessBuild/CMakeLists.txt)
