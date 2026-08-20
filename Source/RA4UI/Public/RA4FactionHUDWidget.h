// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4HUDWidget.h"
#include "RA4FactionHUDWidget.generated.h"

/** Data-driven combat HUD variant. The shell stays shared; only theme and slots change. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4FactionHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()

public:
    URA4FactionHUDWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "RA4|HUD")
    bool ConfigureReference(int32 InReferenceNumber);

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    int32 GetReferenceNumber() const { return ReferenceNumber; }

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    FName GetSpecializedPanelId() const { return SpecializedPanelId; }

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    const TArray<FText>& GetProductionTabs() const { return ProductionTabs; }

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    bool ShouldShowSuperweaponPanel() const { return bShowSuperweaponPanel; }

protected:
    virtual void NativePreConstruct() override;

private:
    void SetVariantData(
        int32 InReferenceNumber,
        ERA4FactionTheme Theme,
        ERA4UIScreenVariant Variant,
        int32 ActiveTab,
        FName InSpecializedPanelId);
    void RebuildTabs();

    UPROPERTY(Transient)
    int32 ReferenceNumber = 13;

    UPROPERTY(Transient)
    FName SpecializedPanelId = TEXT("SovietProduction");

    UPROPERTY(Transient)
    TArray<FText> ProductionTabs;

    UPROPERTY(Transient)
    bool bShowSuperweaponPanel = false;

    UPROPERTY(EditDefaultsOnly, Category = "RA4|HUD", meta = (ClampMin = "13", ClampMax = "24"))
    int32 InitialReferenceNumber = 13;
};
