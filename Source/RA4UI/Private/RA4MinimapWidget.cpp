// Copyright (c) Red Alert 4 project.

#include "RA4MinimapWidget.h"

#include "Slate/SRA4Minimap.h"
#include "Slate/SRA4WorldMarkerLayer.h"

void URA4MinimapWidget::SetSnapshot(
    const TArray<FRA4RadarMarker>& InMarkers,
    const FVector2D InWorldSize,
    const int32 InLocalPlayer)
{
    Markers = InMarkers;
    WorldSize = FVector2D(
        FMath::Max(InWorldSize.X, 1.0),
        FMath::Max(InWorldSize.Y, 1.0));
    LocalPlayer = InLocalPlayer;
    if (SlateMinimap.IsValid())
    {
        SlateMinimap->SetSnapshot(Markers, WorldSize, LocalPlayer);
    }
}

void URA4MinimapWidget::SetViewportWorldBounds(const FVector2D Min, const FVector2D Max)
{
    ViewportWorldBounds = FBox2D(Min, Max);
    if (SlateMinimap.IsValid())
    {
        SlateMinimap->SetViewportWorldBounds(ViewportWorldBounds);
    }
}

FVector2D URA4MinimapWidget::ConvertMapClickToWorld(
    const FVector2D MapPoint,
    const FVector2D MapSize) const
{
    return SRA4Minimap::MapToWorld(MapPoint, MapSize, WorldSize);
}

TSharedRef<SWidget> URA4MinimapWidget::RebuildWidget()
{
    SAssignNew(SlateMinimap, SRA4Minimap)
        .OnMapCommand(FOnRA4MapCommand::CreateUObject(this, &URA4MinimapWidget::HandleMapCommand))
        .OnCameraJump(FOnRA4CameraJump::CreateUObject(this, &URA4MinimapWidget::HandleCameraJump));
    SlateMinimap->SetSnapshot(Markers, WorldSize, LocalPlayer);
    SlateMinimap->SetViewportWorldBounds(ViewportWorldBounds);
    return SlateMinimap.ToSharedRef();
}

void URA4MinimapWidget::ReleaseSlateResources(const bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    SlateMinimap.Reset();
}

void URA4MinimapWidget::HandleMapCommand(const FVector2D WorldPoint)
{
    OnMapCommand.Broadcast(WorldPoint);
}

void URA4MinimapWidget::HandleCameraJump(const FVector2D WorldPoint)
{
    OnCameraJump.Broadcast(WorldPoint);
}

void URA4WorldMarkerLayerWidget::SetSnapshot(const TArray<FRA4WorldMarkerView>& InMarkers)
{
    Markers = InMarkers;
    if (SlateMarkerLayer.IsValid())
    {
        SlateMarkerLayer->SetSnapshot(Markers);
    }
}

TSharedRef<SWidget> URA4WorldMarkerLayerWidget::RebuildWidget()
{
    SAssignNew(SlateMarkerLayer, SRA4WorldMarkerLayer);
    SlateMarkerLayer->SetSnapshot(Markers);
    return SlateMarkerLayer.ToSharedRef();
}

void URA4WorldMarkerLayerWidget::ReleaseSlateResources(const bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    SlateMarkerLayer.Reset();
}
