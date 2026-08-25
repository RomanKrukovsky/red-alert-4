# Contributing to Red Alert 4

Thanks for contributing. This repository is a clean-room RTS project: do not add Electronic Arts assets, source code, data, or protected names to the project.

## Set up

Clone the repository, enter the `Scarlet-Horizon` workspace, and install:

- CMake 3.20 or later and a C++17 compiler for the headless simulation build.
- Unreal Engine **5.8** for Unreal modules, assets, and editor testing.
- Git LFS if your checkout requires LFS-managed assets.

The project file is `RedAlert4.uproject`. It enables the native CommonUI, UMG, Slate, ModelViewViewModel, and Enhanced Input path. React/Vite and Noesis are not supported UI dependencies.

## Build and test

From the workspace root:

```bash
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless --parallel
ctest --test-dir build/headless --output-on-failure
```

Use the CMake/CTest commands above for the engine-free suite; retired script and Python-helper paths are not supported.

For Unreal changes, open the project with UE 5.8 and test the affected editor or PIE workflow:

```bash
UnrealEditor RedAlert4.uproject
```

## Workflow

1. Create a focused branch using the team’s branch convention.
2. Make the smallest complete change.
3. Add or update tests for behaviour changes.
4. Build and run CTest for headless-code changes.
5. Test the relevant Unreal Editor or PIE path for Unreal/UI changes.
6. Update documentation when commands, behaviour, or supported tools change.
7. In the pull request, describe the change, its risks, and the commands or editor steps you ran.

## C++ and architecture rules

- Keep the simulation deterministic and engine-independent. Do not add Unreal types, floating-point state, unseeded randomness, or pointer-address hashing to simulation modules.
- Route simulation state changes through `SimWorld::ApplyCommand`.
- Keep imports at the top of a module. For TypeScript switches over discriminated unions or enums, include a `never` check in the default case.
- Use the existing module and naming patterns; avoid unrelated refactors.

## Pull request checklist

- [ ] The headless build and CTest suite pass for changes that affect engine-free code.
- [ ] New or changed behaviour has appropriate tests.
- [ ] Unreal/UI changes were checked in UE 5.8 where applicable.
- [ ] No unsupported React/Vite or Noesis dependency was introduced.
- [ ] Documentation reflects any changed setup, command, or supported path.
- [ ] The change respects the clean-room/IP rules.

## Reporting issues

Include the Unreal Engine version (use 5.8 for supported editor work), operating system, branch or revision, clear reproduction steps, expected result, actual result, and relevant logs or screenshots. Do not include proprietary assets or data in an issue.
