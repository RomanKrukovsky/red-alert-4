// Copyright (c) Red Alert 4 project. Imports the ambientCG ground textures, builds a
// PBR terrain material from them, assigns it to the landscape, and gives the map a
// lighting rig worth looking at.
//
// Same story as the audio pack: the textures were already in the repository as raw
// JPGs and had never been imported, so the terrain fell back to a flat grey shape
// material. The map also shipped with only a directional light and a skylight -- no
// fog, no post process -- which is what made the lighting look washed out and dead.
//
// Run with: UnrealEditor RedAlert4.uproject -run=RA4TerrainSetup
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RA4TerrainSetupCommandlet.generated.h"

UCLASS()
class RA4EDITOR_API URA4TerrainSetupCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URA4TerrainSetupCommandlet();

    virtual int32 Main(const FString& Params) override;
};
