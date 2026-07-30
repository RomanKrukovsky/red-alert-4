// Copyright (c) Red Alert 4 project. Commandlet that builds a real, sculpted
// Landscape for the skirmish map, replacing the flat placeholder ground plane.
//
// Run with the map already specified on the command line, e.g.:
//   UnrealEditor RedAlert4.uproject /Game/Maps/RA4_Skirmish -run=RA4Landscape
// so the commandlet's default world is the level to modify -- landscape creation
// is an editor-world operation and cannot run headless without one loaded.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RA4LandscapeCommandlet.generated.h"

UCLASS()
class RA4EDITOR_API URA4LandscapeCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URA4LandscapeCommandlet();

    virtual int32 Main(const FString& Params) override;
};
