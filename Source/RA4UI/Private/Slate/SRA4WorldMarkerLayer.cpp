// Copyright (c) Red Alert 4 project.

#include "Slate/SRA4WorldMarkerLayer.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void SRA4WorldMarkerLayer::Construct(const FArguments& InArgs)
{
    SetCanTick(false);
}

void SRA4WorldMarkerLayer::SetSnapshot(const TArray<FRA4WorldMarkerView>& InMarkers)
{
    Markers = InMarkers;
    Invalidate(EInvalidateWidgetReason::Paint);
}

ERA4MarkerGlyph SRA4WorldMarkerLayer::ResolveGlyph(const FRA4WorldMarkerView& Marker)
{
    if (Marker.Intel == ERA4MarkerIntel::Hidden)
    {
        return ERA4MarkerGlyph::Hidden;
    }
    switch (Marker.Team)
    {
    case ERA4MarkerTeam::Friendly:
        return Marker.bSelected ? ERA4MarkerGlyph::FriendlySelected : ERA4MarkerGlyph::Friendly;
    case ERA4MarkerTeam::Allied:
        return ERA4MarkerGlyph::Allied;
    case ERA4MarkerTeam::Neutral:
        return ERA4MarkerGlyph::Neutral;
    case ERA4MarkerTeam::Enemy:
        return ERA4MarkerGlyph::Enemy;
    default:
        checkNoEntry();
        return ERA4MarkerGlyph::Hidden;
    }
}

ERA4HealthBand SRA4WorldMarkerLayer::ResolveHealthBand(const float HealthRatio)
{
    if (HealthRatio <= 0.30f)
    {
        return ERA4HealthBand::Critical;
    }
    if (HealthRatio <= 0.65f)
    {
        return ERA4HealthBand::Damaged;
    }
    return ERA4HealthBand::Healthy;
}

FVector2D SRA4WorldMarkerLayer::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D::ZeroVector;
}

int32 SRA4WorldMarkerLayer::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    const int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    const bool bParentEnabled) const
{
    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    for (const FRA4WorldMarkerView& Marker : Markers)
    {
        const ERA4MarkerGlyph Glyph = ResolveGlyph(Marker);
        if (Glyph == ERA4MarkerGlyph::Hidden)
        {
            continue;
        }

        const FVector2D BarSize(FMath::Max(Marker.ScreenSize.X, 12.0), 5.0);
        const FVector2D BarPosition = Marker.ScreenPosition - FVector2D(BarSize.X * 0.5, Marker.ScreenSize.Y + 10.0);
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(
                FVector2f(BarSize), FSlateLayoutTransform(FVector2f(BarPosition))),
            WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.015f, 0.015f, 0.015f, 0.88f));

        FLinearColor HealthColor;
        switch (ResolveHealthBand(Marker.HealthRatio))
        {
        case ERA4HealthBand::Critical:
            HealthColor = FLinearColor(0.95f, 0.08f, 0.04f, 1.0f);
            break;
        case ERA4HealthBand::Damaged:
            HealthColor = FLinearColor(0.95f, 0.64f, 0.06f, 1.0f);
            break;
        case ERA4HealthBand::Healthy:
            HealthColor = FLinearColor(0.12f, 0.88f, 0.24f, 1.0f);
            break;
        default:
            checkNoEntry();
            HealthColor = FLinearColor::White;
            break;
        }
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(
                FVector2f(BarSize.X * FMath::Clamp(Marker.HealthRatio, 0.0f, 1.0f), BarSize.Y),
                FSlateLayoutTransform(FVector2f(BarPosition))),
            WhiteBrush, ESlateDrawEffect::None, HealthColor);

        if (Marker.bSelected)
        {
            const FVector2D SelectionSize = Marker.ScreenSize + FVector2D(10.0f, 10.0f);
            const FVector2D SelectionPosition = Marker.ScreenPosition - SelectionSize * 0.5f;
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 2,
                AllottedGeometry.ToPaintGeometry(
                    FVector2f(SelectionSize), FSlateLayoutTransform(FVector2f(SelectionPosition))),
                WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.18f, 1.0f, 0.30f, 0.22f));
        }
    }
    return LayerId + 2;
}
