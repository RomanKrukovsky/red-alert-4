// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4HUDTypes.h"
#include "Widgets/SLeafWidget.h"

/** Batched selection, team and health overlays drawn over the 3D world. */
class RA4UI_API SRA4WorldMarkerLayer : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRA4WorldMarkerLayer) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetSnapshot(const TArray<FRA4WorldMarkerView>& InMarkers);
    int32 GetMarkerCount() const { return Markers.Num(); }

    static ERA4MarkerGlyph ResolveGlyph(const FRA4WorldMarkerView& Marker);
    static ERA4HealthBand ResolveHealthBand(float HealthRatio);

    virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

private:
    TArray<FRA4WorldMarkerView> Markers;
};
