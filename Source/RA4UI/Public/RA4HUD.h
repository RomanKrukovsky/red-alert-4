// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RA4HUD.generated.h"

UCLASS()
class RA4UI_API ARA4HUD : public AHUD
{
    GENERATED_BODY()

public:
    ARA4HUD();

    virtual void DrawHUD() override;

    // Set selection rectangle coordinates for box-selection (rubberbanding)
    void SetSelectionRect(const FVector2D& InStart, const FVector2D& InEnd, bool bDraw);

    // Direct Control HUD presentation state
    void UpdateDirectControlDisplay(bool bActive, int32 Health, int32 MaxHealth,
                                   const FText& InPrimaryName, const FText& InSecondaryName,
                                   float InPrimaryCd, float InSecondaryCd, float InSpeedKph,
                                   bool bInOpticsZoomed);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
    FLinearColor SelectionRectColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.25f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
    FLinearColor SelectionRectBorderColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

private:
    FVector2D SelectionStart;
    FVector2D SelectionEnd;
    bool bDrawSelectionRect = false;

    // Direct control HUD data
    bool bDirectControlActive = false;
    int32 DirectControlHealth = 100;
    int32 DirectControlMaxHealth = 100;
    FText DirectControlPrimaryWeapon;
    FText DirectControlSecondaryWeapon;
    float DirectControlPrimaryCd = 0.0f;
    float DirectControlSecondaryCd = 0.0f;
    float DirectControlSpeedKph = 0.0f;
    bool bDirectControlOpticsZoomed = false;
};
