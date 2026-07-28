# Red Alert 4 (internal working title)

A real-time strategy game built on a deterministic, engine-independent C++ core,
with Unreal Engine 5.6 as the presentation layer.

> **Naming.** "Red Alert 4" is an internal working title only. The project holds no
> Electronic Arts licence and contains no Command & Conquer assets, code or data.
> All names, factions and terminology live in localization keys and data assets so
> that the shipping Clean-Room profile is a data swap, not a code change. See
> `Docs/ADR/0004-content-lives-in-data-not-code.md`.

## Current state

See **`Docs/Roadmap.md`** for the honest breakdown of what is built, what is
scaffolded, and what has not been started. Short version: the deterministic
simulation core and a full playable match loop exist and are tested; nothing
graphical, networked or campaign-related exists yet.

## Building and testing the core

The simulation has no Unreal dependency, so it builds and runs in seconds:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless -j8
./build/headless/RA4Tests
```

With sanitizers:

```bash
cmake -S Tools/HeadlessBuild -B build/asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan -j8 && ./build/asan/RA4Tests
```

Filter to one suite with `--filter=VerticalSlice`, or list tests with `--list`.

## Building the Unreal project

`RedAlert4.uproject` targets UE 5.6. The `.Build.cs` and `.Target.cs` files exist but
**have not been run through UnrealBuildTool yet** -- treat the Unreal side as
unverified until `Docs/Roadmap.md` says otherwise.

## Layout

```
Source/RA4Core/         fixed-point math, RNG, ids, commands, serialization
Source/RA4Content/      data definitions, damage table, validation, content hash
Source/RA4Simulation/   match state, SoA storage, gameplay systems
Source/RA4Replay/       replay record, playback, checksum verification
Source/RA4Tests/        headless test suite
Source/RedAlert4/       Unreal game module (presentation, input, UI)
Tools/HeadlessBuild/    CMake harness for the engine-free modules
Docs/                   architecture, ADRs, threat model, roadmap
```
