// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4HUDTypes.h"
#include "Widgets/SLeafWidget.h"

DECLARE_DELEGATE_OneParam(FOnRA4MapCommand, FVector2D);
DECLARE_DELEGATE_OneParam(FOnRA4CameraJump, FVector2D);

/** One-pass tactical map. Contacts are paint data, never child widgets. */
class RA4UI_API SRA4Minimap : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRA4Minimap) {}
        SLATE_EVENT(FOnRA4MapCommand, OnMapCommand)
        SLATE_EVENT(FOnRA4CameraJump, OnCameraJump)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void SetSnapshot(
        const TArray<FRA4RadarMarker>& InMarkers,
        FVector2D InWorldSize,
        int32 InLocalPlayer);
    void SetViewportWorldBounds(const FBox2D& InBounds);

    int32 GetMarkerCount() const { return Markers.Num(); }

    static FVector2D WorldToMap(FVector2D WorldPoint, FVector2D WorldSize, FVector2D MapSize);
    static FVector2D MapToWorld(FVector2D MapPoint, FVector2D MapSize, FVector2D WorldSize);

    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;
    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
    TArray<FRA4RadarMarker> Markers;
    FVector2D WorldSize = FVector2D(1.0f, 1.0f);
    FBox2D ViewportWorldBounds = FBox2D(EForceInit::ForceInit);
    int32 LocalPlayer = 0;
    FOnRA4MapCommand OnMapCommand;
    FOnRA4CameraJump OnCameraJump;
    mutable bool bPaintTimingLogged = false;
};
