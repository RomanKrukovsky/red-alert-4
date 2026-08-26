// Copyright (c) Red Alert 4 project.

#include "Slate/SRA4Minimap.h"

#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "HAL/PlatformTime.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/Texture2D.h"

void SRA4Minimap::Construct(const FArguments& InArgs)
{
    OnMapCommand = InArgs._OnMapCommand;
    OnCameraJump = InArgs._OnCameraJump;
    SetCanTick(false);
}

void SRA4Minimap::SetSnapshot(
    const TArray<FRA4RadarMarker>& InMarkers,
    const FVector2D InWorldSize,
    const int32 InLocalPlayer)
{
    Markers = InMarkers;
    WorldSize = FVector2D(
        FMath::Max(InWorldSize.X, 1.0),
        FMath::Max(InWorldSize.Y, 1.0));
    LocalPlayer = InLocalPlayer;
    bPaintTimingLogged = false;
    Invalidate(EInvalidateWidgetReason::Paint);
}

void SRA4Minimap::SetViewportWorldBounds(const FBox2D& InBounds)
{
    ViewportWorldBounds = InBounds;
    Invalidate(EInvalidateWidgetReason::Paint);
}

void SRA4Minimap::SetBackgroundTexture(UTexture2D* InTexture)
{
    if (BackgroundTexture == InTexture)
    {
        return;
    }
    BackgroundTexture = InTexture;
    BackgroundBrush.SetResourceObject(InTexture);
    BackgroundBrush.ImageSize = InTexture
        ? FVector2f(InTexture->GetSurfaceWidth(), InTexture->GetSurfaceHeight())
        : FVector2f(96.0f, 96.0f);
    BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
    Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SRA4Minimap::WorldToMap(
    const FVector2D WorldPoint,
    const FVector2D InWorldSize,
    const FVector2D MapSize)
{
    const double SafeWorldX = FMath::Max(InWorldSize.X, 1.0);
    const double SafeWorldY = FMath::Max(InWorldSize.Y, 1.0);
    return FVector2D(
        FMath::Clamp(WorldPoint.X / SafeWorldX, 0.0, 1.0) * FMath::Max(MapSize.X, 0.0),
        (1.0 - FMath::Clamp(WorldPoint.Y / SafeWorldY, 0.0, 1.0)) * FMath::Max(MapSize.Y, 0.0));
}

FVector2D SRA4Minimap::MapToWorld(
    const FVector2D MapPoint,
    const FVector2D MapSize,
    const FVector2D InWorldSize)
{
    const double SafeMapX = FMath::Max(MapSize.X, 1.0);
    const double SafeMapY = FMath::Max(MapSize.Y, 1.0);
    return FVector2D(
        FMath::Clamp(MapPoint.X / SafeMapX, 0.0, 1.0) * FMath::Max(InWorldSize.X, 0.0),
        (1.0 - FMath::Clamp(MapPoint.Y / SafeMapY, 0.0, 1.0)) * FMath::Max(InWorldSize.Y, 0.0));
}

FVector2D SRA4Minimap::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
    return FVector2D(300.0f, 260.0f);
}

int32 SRA4Minimap::OnPaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    const int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    const bool bParentEnabled) const
{
    const double PaintStartSeconds = FPlatformTime::Seconds();
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    FSlateDrawElement::MakeBox(
        OutDrawElements, LayerId,
        AllottedGeometry.ToPaintGeometry(), WhiteBrush,
        ESlateDrawEffect::None, FLinearColor(0.008f, 0.018f, 0.022f, 0.96f));

    // The explored-map texture (terrain x shroud), stretched over the whole
    // panel: one textured element instead of up to 9216 cell boxes.
    if (BackgroundTexture != nullptr)
    {
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(), &BackgroundBrush,
            ESlateDrawEffect::None, FLinearColor::White);
    }

    for (int32 LineIndex = 1; LineIndex < 6; ++LineIndex)
    {
        const float Fraction = float(LineIndex) / 6.0f;
        TArray<FVector2D> Vertical;
        Vertical.Add(FVector2D(Size.X * Fraction, 0.0f));
        Vertical.Add(FVector2D(Size.X * Fraction, Size.Y));
        FSlateDrawElement::MakeLines(
            OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
            Vertical, ESlateDrawEffect::None, FLinearColor(0.08f, 0.22f, 0.22f, 0.34f), true, 1.0f);
        TArray<FVector2D> Horizontal;
        Horizontal.Add(FVector2D(0.0f, Size.Y * Fraction));
        Horizontal.Add(FVector2D(Size.X, Size.Y * Fraction));
        FSlateDrawElement::MakeLines(
            OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
            Horizontal, ESlateDrawEffect::None, FLinearColor(0.08f, 0.22f, 0.22f, 0.34f), true, 1.0f);
    }

    for (const FRA4RadarMarker& Marker : Markers)
    {
        const FVector2D Point = WorldToMap(Marker.WorldPosition, WorldSize, Size);
        const FVector2D MarkerSize = Marker.Kind == ERA4RadarMarkerKind::Building
            ? FVector2D(7.0f, 7.0f)
            : FVector2D(4.0f, 4.0f);
        FLinearColor Color;
        if (Marker.Kind == ERA4RadarMarkerKind::Resource)
        {
            Color = FLinearColor(0.95f, 0.78f, 0.15f, 1.0f);
        }
        else if (Marker.Owner == LocalPlayer)
        {
            Color = Marker.bSelected
                ? FLinearColor(0.55f, 1.0f, 0.55f, 1.0f)
                : FLinearColor(0.15f, 0.78f, 1.0f, 1.0f);
        }
        else
        {
            Color = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);
        }
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 2,
            AllottedGeometry.ToPaintGeometry(
                FVector2f(MarkerSize),
                FSlateLayoutTransform(FVector2f(Point - MarkerSize * 0.5f))),
            WhiteBrush, ESlateDrawEffect::None, Color);
    }

    if (ViewportWorldBounds.bIsValid)
    {
        const FVector2D Min = WorldToMap(ViewportWorldBounds.Min, WorldSize, Size);
        const FVector2D Max = WorldToMap(ViewportWorldBounds.Max, WorldSize, Size);
        TArray<FVector2D> ViewportLines;
        ViewportLines.Add(Min);
        ViewportLines.Add(FVector2D(Max.X, Min.Y));
        ViewportLines.Add(Max);
        ViewportLines.Add(FVector2D(Min.X, Max.Y));
        ViewportLines.Add(Min);
        FSlateDrawElement::MakeLines(
            OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
            ViewportLines, ESlateDrawEffect::None,
            FLinearColor(0.30f, 1.0f, 0.35f, 1.0f), true, 2.0f);
    }
    if (!bPaintTimingLogged)
    {
        bPaintTimingLogged = true;
        const double PaintMicroseconds = (FPlatformTime::Seconds() - PaintStartSeconds) * 1000000.0;
        UE_LOG(
            LogTemp, Display,
            TEXT("RA4MinimapPerf Markers=%d PaintUs=%.2f DrawElements=%d MarkerWidgets=0"),
            Markers.Num(), PaintMicroseconds, 14 + Markers.Num());
    }
    return LayerId + 3;
}

FReply SRA4Minimap::OnMouseButtonDown(
    const FGeometry& MyGeometry,
    const FPointerEvent& MouseEvent)
{
    const FVector2D LocalPoint = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    const FVector2D WorldPoint = MapToWorld(LocalPoint, MyGeometry.GetLocalSize(), WorldSize);
    if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OnMapCommand.IsBound())
    {
        OnMapCommand.Execute(WorldPoint);
        return FReply::Handled();
    }
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnCameraJump.IsBound())
    {
        OnCameraJump.Execute(WorldPoint);
        return FReply::Handled();
    }
    return FReply::Unhandled();
}
