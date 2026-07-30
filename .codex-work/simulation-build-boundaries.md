# Simulation Build Boundaries Fix

Date: 2026-07-30
Owner scope: build-boundary only

## Goal

1. `RedAlert4` game target must not include `RA4Editor`
2. `RedAlert4Editor` target must still include `RA4Editor`
3. headless modules must not depend on `Engine` / `CoreUObject` unless code really needs it

## Root cause

The immediate non-editor build break was this dependency chain:

```text
RedAlert4Target
  -> RA4Editor
  -> UnrealEd
```

`UnrealEd` cannot be instantiated for non-editor targets, so `RedAlert4 Mac Development` failed during UBT rules evaluation before compile.

At the same time, several modules that already compile headlessly through CMake still declared unnecessary Unreal-side private dependencies in `.Build.cs`. That weakened the intended architectural wall even though the code itself was mostly clean.

## What I changed

### Target fix

- Removed `RA4Editor` from [Source/RedAlert4.Target.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RedAlert4.Target.cs:11)
- Kept `RA4Editor` in [Source/RedAlert4Editor.Target.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RedAlert4Editor.Target.cs:11)

### Engine-free module cleanup

Removed unused `CoreUObject` / `Engine` private dependencies from:

- [Source/RA4Input/RA4Input.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Input/RA4Input.Build.cs:18)
- [Source/RA4Navigation/RA4Navigation.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Navigation/RA4Navigation.Build.cs:16)
- [Source/RA4FogOfWar/RA4FogOfWar.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4FogOfWar/RA4FogOfWar.Build.cs:11)
- [Source/RA4Campaign/RA4Campaign.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Campaign/RA4Campaign.Build.cs:11)
- [Source/RA4Combat/RA4Combat.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Combat/RA4Combat.Build.cs:11)

### Intentionally not changed

- `RA4Network.Build.cs`

Reason:

- `RA4Network` contains `UWorldSubsystem`, `UCLASS`, `CoreMinimal.h`, `TMap`, and `UE_LOG`, so its Unreal dependencies are real, not accidental.

## Reproduction before fix

Command:

```bash
"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4 Mac Development -project="$PWD/RedAlert4.uproject"
```

Observed failure:

```text
Unable to instantiate module 'UnrealEd': Unable to instantiate UnrealEd module for non-editor targets.
(referenced via Target -> RA4Editor.Build.cs)
Result: Failed (RulesError)
```

## Validation after fix

### 1. Headless build and tests

Command:

```bash
cmake -S Tools/HeadlessBuild -B build/sim-boundaries -DCMAKE_BUILD_TYPE=Release
cmake --build build/sim-boundaries -j8
./build/sim-boundaries/RA4Tests
```

Result:

```text
208 passed, 0 failed
```

### 2. UBT game target

Command:

```bash
"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4 Mac Development -project="$PWD/RedAlert4.uproject"
```

Result after fix:

- The original `RA4Editor -> UnrealEd` non-editor failure is gone.
- Build now advances into UHT and stops on an unrelated header problem:

```text
Source/RA4UI/Public/RA4SidebarWidget.h(1): Error: #include found after .generated.h file - the .generated.h file should always be the last #include in a header
Result: Failed (OtherCompilationError)
```

Interpretation:

- build-boundary fix worked
- full `Game` target is still blocked by a separate `RA4UI` header issue outside the owned scope

### 3. UBT editor target

Command:

```bash
"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4Editor Mac Development -project="$PWD/RedAlert4.uproject"
```

Result:

- `RA4Editor` remained wired into the editor target as intended
- build reaches UHT and stops on the same unrelated `RA4SidebarWidget.h` include-order error

## Outcome

Status: `DONE_WITH_CONCERNS`

### Done

- `RedAlert4` game target no longer includes `RA4Editor`
- `RedAlert4Editor` target still includes `RA4Editor`
- truly headless modules in my scope no longer declare unnecessary `Engine` / `CoreUObject` deps
- headless core still builds and passes all tests

### Concerns still blocking full UBT green

- unrelated UHT error in [Source/RA4UI/Public/RA4SidebarWidget.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4UI/Public/RA4SidebarWidget.h:1)
- because of that, neither `RedAlert4` nor `RedAlert4Editor` completed a full compile, even though both moved past the original target-boundary failure

## Follow-up: RA4Sidebar include-order fix

Scope for this round:

- `Source/RA4UI/Public/RA4SidebarWidget.h` only

### Root cause

`RA4SidebarWidget.generated.h` was not the last include in the header. UnrealHeaderTool requires `.generated.h` to be last.

Before:

```text
#include "RA4HUDTypes.h"
#include "RA4SidebarWidget.generated.h"
#include "Components/Button.h"
```

### Minimal fix applied

Moved `#include "Components/Button.h"` above `.generated.h` in:

- [Source/RA4UI/Public/RA4SidebarWidget.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4UI/Public/RA4SidebarWidget.h:16)

No behavior changes. No `.cpp` changes were needed.

### Commands rerun

```bash
"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4Editor Mac Development -project="$PWD/RedAlert4.uproject"

"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4 Mac Development -project="$PWD/RedAlert4.uproject"
```

### Results after include-order fix

#### RedAlert4Editor Mac Development

- UHT error in `RA4SidebarWidget.h` is gone
- reflection completed successfully
- `RA4SidebarWidget.cpp` compiled
- build then failed later on an unrelated compile error in:
  - `Source/RedAlert4/Private/RA4PlayerController.cpp:766`

Observed error:

```text
error: parentheses were disambiguated as a function declaration [-Werror,-Wvexing-parse]
const ContentId Content(uint32(ContentIdValue));
```

#### RedAlert4 Mac Development

- UHT error in `RA4SidebarWidget.h` is gone
- reflection completed successfully
- `RA4SidebarWidget.cpp` compiled
- build then failed later on the same unrelated compile error in:
  - `Source/RedAlert4/Private/RA4PlayerController.cpp:766`

### Updated interpretation

- The `RA4SidebarWidget` include-order issue is fixed.
- Both `Editor` and `Game` targets now get past UHT and compile `RA4SidebarWidget`.
- The remaining UBT blocker is no longer in `RA4UI`; it is an unrelated compile error in `RA4PlayerController.cpp`.

## Follow-up: RA4PlayerController vexing-parse check

Scope for this round:

- `Source/RedAlert4/Private/RA4PlayerController.cpp` around line 766 only

### Current source state

Current code at line 766 is:

```cpp
const ContentId Content = ContentId(uint32(ContentIdValue));
```

This is already the non-vexing form. So in the current `HEAD` there was no remaining local source fix to apply in my scope.

### Commands run

```bash
"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4 Mac Development -project="$PWD/RedAlert4.uproject"

"/Users/Shared/Epic Games/UE_5.6/Engine/Build/BatchFiles/Mac/Build.sh" \
  RedAlert4Editor Mac Development -project="$PWD/RedAlert4.uproject"
```

### Results

#### RedAlert4 Mac Development

- `RA4PlayerController.cpp` compiled successfully
- target linked successfully
- final result: `Succeeded`

Note:

- post-build staging printed a warning about missing staged cooked data:
  - `Saved/StagedBuilds/Mac`
- this did not fail the build

#### RedAlert4Editor Mac Development

- target reported `up to date`
- final result: `Succeeded`

### Updated interpretation

- The previous `-Wvexing-parse` blocker at `RA4PlayerController.cpp:766` is no longer active in the current workspace state.
- No additional source patch was required in this round inside `RA4PlayerController.cpp`.
- Both requested UBT targets are now succeeding on the current tree.
