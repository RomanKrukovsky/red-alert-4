// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ActivatableWidget.h"
#include "RA4HUDTypes.h"
#include "RA4MinimapWidget.generated.h"

class SRA4Minimap;
class SRA4WorldMarkerLayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRA4MapPointEvent, FVector2D, WorldPoint);

/** UMG integration point for the Slate tactical map. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4MinimapWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "RA4|Minimap")
    void SetSnapshot(
        const TArray<FRA4RadarMarker>& InMarkers,
        FVector2D InWorldSize,
        int32 InLocalPlayer);

    UFUNCTION(BlueprintCallable, Category = "RA4|Minimap")
    void SetViewportWorldBounds(FVector2D Min, FVector2D Max);

    UFUNCTION(BlueprintPure, Category = "RA4|Minimap")
    int32 GetMarkerCount() const { return Markers.Num(); }

    UFUNCTION(BlueprintPure, Category = "RA4|Minimap")
    int32 GetMarkerWidgetCount() const { return 0; }

    UFUNCTION(BlueprintPure, Category = "RA4|Minimap")
    FVector2D ConvertMapClickToWorld(FVector2D MapPoint, FVector2D MapSize) const;

    UPROPERTY(BlueprintAssignable, Category = "RA4|Minimap")
    FRA4MapPointEvent OnMapCommand;

    UPROPERTY(BlueprintAssignable, Category = "RA4|Minimap")
    FRA4MapPointEvent OnCameraJump;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    void HandleMapCommand(FVector2D WorldPoint);
    void HandleCameraJump(FVector2D WorldPoint);

    UPROPERTY(Transient)
    TArray<FRA4RadarMarker> Markers;

    FVector2D WorldSize = FVector2D(1.0f, 1.0f);
    FBox2D ViewportWorldBounds = FBox2D(EForceInit::ForceInit);
    int32 LocalPlayer = 0;
    TSharedPtr<SRA4Minimap> SlateMinimap;
};

/** UMG integration point for the batched world-space marker overlay. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4WorldMarkerLayerWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "RA4|World Markers")
    void SetSnapshot(const TArray<FRA4WorldMarkerView>& InMarkers);

    UFUNCTION(BlueprintPure, Category = "RA4|World Markers")
    int32 GetMarkerCount() const { return Markers.Num(); }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    UPROPERTY(Transient)
    TArray<FRA4WorldMarkerView> Markers;

    TSharedPtr<SRA4WorldMarkerLayer> SlateMarkerLayer;
};
