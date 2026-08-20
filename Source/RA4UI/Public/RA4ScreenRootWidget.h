// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ActivatableWidget.h"
#include "RA4ScreenRootWidget.generated.h"

class UImage;
class UOverlay;
class USafeZone;
class USizeBox;
class URA4UIScreenData;

/** Shared responsive root used by every full-screen RA4 menu and HUD. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4ScreenRootWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "RA4|UI|Screen")
    void ApplyScreenData(const URA4UIScreenData* ScreenData);

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Screen")
    USafeZone* GetSafeZone() const { return SafeZone; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Screen")
    UImage* GetBackgroundLayer() const { return BackgroundLayer; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Screen")
    USizeBox* GetReferenceFrame() const { return ReferenceFrame; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Screen")
    UOverlay* GetChromeLayer() const { return ChromeLayer; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Screen")
    UOverlay* GetContentLayer() const { return ContentLayer; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI|Validation")
    FText GetValidationError() const { return ValidationError; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void ApplyNeutralBackground();

    UPROPERTY(Transient)
    TObjectPtr<USafeZone> SafeZone;

    UPROPERTY(Transient)
    TObjectPtr<UImage> BackgroundLayer;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ReferenceFrame;

    UPROPERTY(Transient)
    TObjectPtr<UOverlay> ChromeLayer;

    UPROPERTY(Transient)
    TObjectPtr<UOverlay> ContentLayer;

    UPROPERTY(Transient)
    FText ValidationError;
};
