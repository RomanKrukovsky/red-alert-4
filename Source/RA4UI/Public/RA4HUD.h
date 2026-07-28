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

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
    FLinearColor SelectionRectColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.25f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
    FLinearColor SelectionRectBorderColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

private:
    FVector2D SelectionStart;
    FVector2D SelectionEnd;
    bool bDrawSelectionRect = false;
};
