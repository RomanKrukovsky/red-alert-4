// Copyright (c) Red Alert 4 project. Name card shown after resting the cursor over a
// building or unit.
//
// Built in C++ like the rest of the HUD so the game stays playable from a fresh clone
// with no editor-authored assets.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "RA4HoverTooltipWidget.generated.h"

class UTextBlock;

UCLASS()
class RA4UI_API URA4HoverTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;

    /** Sets the displayed name and optional second line (health, cost, owner...). */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI")
    void SetContent(const FText& Title, const FText& Subtitle);

private:
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SubtitleText;
};
