// Copyright (c) Red Alert 4 project.

#include "RA4MinimapWidget.h"

#include "Slate/SRA4Minimap.h"
#include "Engine/Texture2D.h"
#include "RA4HUDTypes.h"
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

void URA4MinimapWidget::SetBackground(
    const TArray<uint8>& Terrain,
    const TArray<uint8>& Shroud,
    const int32 Width,
    const int32 Height)
{
    if (Width <= 0 || Height <= 0 ||
        Terrain.Num() < Width * Height || Shroud.Num() < Width * Height)
    {
        return;
    }

    if (BackgroundTexture == nullptr ||
        BackgroundTexture->GetSurfaceWidth() != Width ||
        BackgroundTexture->GetSurfaceHeight() != Height)
    {
        BackgroundTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
        if (BackgroundTexture == nullptr)
        {
            return;
        }
        BackgroundTexture->SRGB = false;
        BackgroundTexture->Filter = TF_Nearest;
        BackgroundTexture->NeverStream = true;
    }

    // Cell -> colour: readable terrain hues, shroud as brightness.
    auto TerrainColour = [](const uint8 T) -> FLinearColor
    {
        switch (static_cast<ERA4MinimapTerrain>(T))
        {
        case ERA4MinimapTerrain::Ground:    return FLinearColor(0.22f, 0.42f, 0.16f);
        case ERA4MinimapTerrain::Water:     return FLinearColor(0.08f, 0.28f, 0.52f);
        case ERA4MinimapTerrain::Cliff:     return FLinearColor(0.36f, 0.34f, 0.30f);
        case ERA4MinimapTerrain::Ore:       return FLinearColor(0.85f, 0.68f, 0.16f);
        case ERA4MinimapTerrain::Structure: return FLinearColor(0.52f, 0.55f, 0.60f);
        default:                            return FLinearColor(0.0f, 0.0f, 0.0f);
        }
    };

    if (FTexturePlatformData* PlatformData = BackgroundTexture->GetPlatformData())
    {
        if (PlatformData->Mips.Num() > 0)
        {
            FTexture2DMipMap& Mip = PlatformData->Mips[0];
            if (void* Dest = Mip.BulkData.Lock(LOCK_READ_WRITE))
            {
                uint8* Pixels = static_cast<uint8*>(Dest);
                for (int32 Y = 0; Y < Height; ++Y)
                {
                    for (int32 X = 0; X < Width; ++X)
                    {
                        // Dest Y=0 is top (north), source Cell Y=0 is south -> flip
                        const int32 SrcCell = (Height - 1 - Y) * Width + X;
                        const uint8 ShroudByte = Shroud[SrcCell];
                        float Brightness = 1.0f;
                        if (ShroudByte == uint8(ERA4MinimapShroud::NeverSeen))
                        {
                            Brightness = 0.04f;
                        }
                        else if (ShroudByte == uint8(ERA4MinimapShroud::Remembered))
                        {
                            Brightness = 0.45f;
                        }
                        const FLinearColor C = TerrainColour(Terrain[SrcCell]) * Brightness;
                        uint8* Px = Pixels + (static_cast<int64>(Y) * Width + X) * 4;
                        Px[0] = uint8(FMath::Clamp(C.B, 0.0f, 1.0f) * 255.0f);
                        Px[1] = uint8(FMath::Clamp(C.G, 0.0f, 1.0f) * 255.0f);
                        Px[2] = uint8(FMath::Clamp(C.R, 0.0f, 1.0f) * 255.0f);
                        Px[3] = 255;
                    }
                }
            }
            Mip.BulkData.Unlock();
            BackgroundTexture->UpdateResource();
        }
    }

    if (SlateMinimap.IsValid())
    {
        SlateMinimap->SetBackgroundTexture(BackgroundTexture);
    }
}
