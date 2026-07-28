// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RA4MapBaker.generated.h"

UCLASS(BlueprintType)
class RA4EDITOR_API URA4MapBaker : public UObject
{
    GENERATED_BODY()

public:
    // Bake current level static geometry into deterministic NavGrid binary data
    UFUNCTION(BlueprintCallable, Category = "RA4 Editor")
    static bool BakeLevelNavGrid(const FString& OutputPath, int32 GridWidth, int32 GridHeight, float CellSize);

    // Validate level for RTS rules (e.g. at least 2 spawn locations, valid Ore nodes)
    UFUNCTION(BlueprintCallable, Category = "RA4 Editor")
    static bool ValidateLevelSetup(FString& OutErrorMessage);
};
