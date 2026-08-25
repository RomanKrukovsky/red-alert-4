# Project status

**Updated:** August 25, 2026

**Workspace:** `Scarlet-Horizon`

**Unreal project:** `RedAlert4.uproject`
**Supported engine:** Unreal Engine 5.8

## Current supported paths

- The deterministic C++ simulation is validated through the CMake/CTest harness in `Tools/HeadlessBuild`.
- The production Unreal UI is **CommonUI + UMG + Slate**, supplied by C++ view models and presentation snapshots.
- React/Vite and Noesis prototypes have been removed and are not supported build or UI paths.

## Validation

From the workspace root:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
```

The current command output is the source of truth for test results; this document intentionally does not keep a fixed test total.

For Unreal Editor work, open `RedAlert4.uproject` in Unreal Engine 5.8 and validate the affected editor or PIE workflow. Packaging is a separate validation step and is not implied by headless CTest success.

## Known boundaries

- The simulation must remain deterministic and free of Unreal Engine dependencies.
- Do not add unsupported React/Vite or Noesis dependencies.
- Historical audit documents may describe removed or superseded paths; use this file, [PROJECT_STATE.md](PROJECT_STATE.md), and the contributor guides for current operating guidance.
