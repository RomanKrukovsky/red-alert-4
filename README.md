# Red Alert 4 (working title)

Red Alert 4 is an RTS prototype with a deterministic, engine-independent C++ simulation and an Unreal Engine 5.8 presentation layer. `RedAlert4.uproject` is the Unreal project; the workspace directory is `Scarlet-Horizon`.

> **Clean-room project.** “Red Alert 4” is an internal working title. The repository contains no Electronic Arts assets, source code, or data.

## Highlights

- Deterministic simulation core using fixed-point math (`Fixed48.16`).
- Headless C++ build and test harness under `Tools/HeadlessBuild`; it does not require Unreal Engine.
- RTS economy, production, combat, navigation, AI, replay, campaign, and presentation systems.
- Native Unreal UI: **CommonUI + UMG + Slate** with C++ view models and presentation snapshots.
- The former React/Vite and Noesis prototypes were removed. They are not supported production UI paths.

## Quick start

From the workspace root, configure, build, and run the headless test suite:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
```

For a clean build, choose a new build directory rather than deleting an existing one:

```bash
cmake -S Tools/HeadlessBuild -B build/headless-clean -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless-clean --parallel
ctest --test-dir build/headless-clean --output-on-failure
```

## Unreal Engine 5.8

Install Unreal Engine **5.8** and open `RedAlert4.uproject` from the workspace root. If `UnrealEditor` is available on your `PATH`, use:

```bash
UnrealEditor RedAlert4.uproject
```

The project descriptor enables CommonUI, ModelViewViewModel, Enhanced Input, and Gameplay Abilities. In the editor, open `/Game/Maps/RA4_Skirmish_Production` and start Play in Editor to exercise the RTS controls and native HUD.

## Repository layout

```
Source/RA4Core/         Fixed-point math, commands, identifiers
Source/RA4Content/      Content definitions and validation
Source/RA4Simulation/   Match state and gameplay systems
Source/RA4Navigation/   Navigation, flow fields, reservations, formations
Source/RA4AI/           AI commander and tactical systems
Source/RA4Replay/       Replay recording and verification
Source/RedAlert4/       Unreal presentation module
Source/RA4UI/           CommonUI, UMG, Slate, and UI view models
Tools/HeadlessBuild/    CMake/CTest harness for engine-free modules
Docs/                   Architecture, production, and agent documentation
```

## Documentation

- [Quick start](QUICK_START.md)
- [Contributing](CONTRIBUTING.md)
- [Project handoff](HANDOFF.md)
- [Current project state](Docs/Agent/PROJECT_STATE.md)
- [Current project status](Docs/Agent/PROJECT_STATUS.md)
