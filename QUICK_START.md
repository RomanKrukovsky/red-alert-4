# Red Alert 4 quick start

The workspace folder is `Scarlet-Horizon`; the Unreal project file is `RedAlert4.uproject`. The headless C++ simulation is the fastest way to validate a checkout. It needs CMake 3.20+ and a C++17 compiler, but not Unreal Engine.

## Headless build and tests

Run these commands from the workspace root:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
```

CTest runs the test targets registered by `Tools/HeadlessBuild/CMakeLists.txt`. Do not rely on a fixed test count; the suite changes with the project.

To validate a fresh build without disturbing your normal build directory, use another empty directory:

```bash
cmake -S Tools/HeadlessBuild -B build/headless-fresh -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless-fresh --parallel
ctest --test-dir build/headless-fresh --output-on-failure
```

## Open the Unreal project

Install Unreal Engine **5.8**. Open `RedAlert4.uproject` in the editor, or run this command when `UnrealEditor` is on your `PATH`:

```bash
UnrealEditor RedAlert4.uproject
```

In Unreal, load `/Game/Maps/RA4_Skirmish_Production` and use Play in Editor. The project uses the native Unreal UI stack: CommonUI for routing, UMG for widget layout, Slate for the minimap and world markers, and C++ presentation snapshots for data. React/Vite and Noesis are not part of the production build.

## Everyday workflow

```bash
git pull --ff-only
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
```

If no build directory exists yet, start with the configuration command in “Headless build and tests.”

## Common problems

**CMake cannot configure the project.** Check that CMake is version 3.20 or later and that a C++17 compiler is installed.

**CTest cannot find tests.** Configure the build with `-S Tools/HeadlessBuild`; the repository root does not provide a separate top-level CMake project.

**UnrealEditor is not found.** Launch `RedAlert4.uproject` through your Unreal Engine 5.8 installation, or add that installation’s editor executable to your `PATH` using the method appropriate to your operating system.

**A headless test fails because content cannot be found.** Run CTest as shown above. The CMake configuration supplies the repository-root working directory needed by content-loading tests.
