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

private:
    void DrawMarquee(const class ARA4PlayerController* Controller);
    void DrawSelectionRings(const class ARA4PlayerController* Controller);
};
