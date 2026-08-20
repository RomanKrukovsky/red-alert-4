// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ScreenRootWidget.h"
#include "RA4SplashScreenWidget.generated.h"

class UImage;
class UTextBlock;

/** Interactive title screen matching reference 1. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4SplashScreenWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4SplashScreenWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Splash")
    bool ContinueToMainMenu();

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Splash")
    UImage* GetLogoImage() const { return LogoImage; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UImage> LogoImage;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ContinuePrompt;

    bool bContinueRequested = false;
    float PromptTime = 0.0f;
};
