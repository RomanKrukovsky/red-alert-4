// Copyright (c) Red Alert 4 project. Selection feedback drawn in code.
//
// Deliberately assetless: the marquee and selection rings are drawn with primitives
// so the game is playable and legible before any UI art exists. The CommonUI HUD
// replaces this; the drawing here is scaffolding, not the shipping presentation.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "RA4RtsHud.generated.h"

UCLASS()
class REDALERT4_API ARA4RtsHud : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

protected:
    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor MarqueeColor = FLinearColor(0.35f, 0.9f, 0.4f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor SelectionColor = FLinearColor(0.35f, 0.9f, 0.4f, 1.0f);

    // Health bars use the same three bands as the originals, because players read
    // colour long before they read a number.
    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor HealthHighColor = FLinearColor(0.30f, 0.88f, 0.36f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor HealthMediumColor = FLinearColor(0.95f, 0.80f, 0.20f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor HealthLowColor = FLinearColor(0.90f, 0.25f, 0.20f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor PlacementValidColor = FLinearColor(0.2f, 0.9f, 0.2f, 0.5f);

    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor PlacementInvalidColor = FLinearColor(0.9f, 0.1f, 0.1f, 0.5f);

    // Outline only, per the request: a filled disc hides the ground the player is
    // trying to aim at.
    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    FLinearColor MoveTargetRingColor = FLinearColor(0.25f, 0.95f, 0.35f, 0.9f);

    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    float MoveTargetRingRadiusUnits = 90.0f;

    // How long the confirmation ping lives after the order is given.
    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    float MoveTargetRingDurationSeconds = 0.55f;

private:
    void DrawMarquee(const class ARA4PlayerController* Controller);
    void DrawSelectionBrackets(const class ARA4PlayerController* Controller);
    void DrawPlacementFootprint(const class ARA4PlayerController* Controller);
    // One-shot ping at the spot a move order was just issued.
    void DrawMoveTargetRing(const class ARA4PlayerController* Controller);
    void DrawDirectControlHUD(const class ARA4PlayerController* Controller);
};
