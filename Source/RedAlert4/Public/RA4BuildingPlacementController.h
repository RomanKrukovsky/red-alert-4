// Copyright (c) Red Alert 4 project. Interactive Building Placement Controller.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RA4BuildingPlacementController.generated.h"

class ARA4EntityActor;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class REDALERT4_API URA4BuildingPlacementController : public UActorComponent
{
    GENERATED_BODY()

public:
    URA4BuildingPlacementController();

    // Starts building placement mode for a specific Content ID / Building Type
    UFUNCTION(BlueprintCallable, Category = "Building Placement")
    void StartPlacement(int32 BuildingTypeId, const FString& BuildingName);

    // Cancels active placement mode
    UFUNCTION(BlueprintCallable, Category = "Building Placement")
    void CancelPlacement();

    // Confirms placement at current cursor location and queues build command to simulation
    UFUNCTION(BlueprintCallable, Category = "Building Placement")
    bool ConfirmPlacement();

    // Returns true if placement mode is active
    UFUNCTION(BlueprintPure, Category = "Building Placement")
    bool IsPlacing() const { return bIsPlacing; }

    // Returns current validity of ghost location
    UFUNCTION(BlueprintPure, Category = "Building Placement")
    bool IsLocationValid() const { return bCurrentLocationValid; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    void UpdateGhostTransform();
    bool ValidateLocation(const FVector& Location) const;
    void UpdateGhostMaterial(bool bValid);

    UPROPERTY(Transient)
    TObjectPtr<ARA4EntityActor> GhostActor;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> GhostDynamicMaterial;

    int32 ActiveBuildingTypeId = 0;
    FString ActiveBuildingName;
    bool bIsPlacing = false;
    bool bCurrentLocationValid = false;
    FVector CurrentGridLocation = FVector::ZeroVector;
};
