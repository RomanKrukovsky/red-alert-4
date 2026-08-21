// Copyright (c) Red Alert 4 project. Interactive Building Placement Controller implementation.

#include "RA4BuildingPlacementController.h"
#include "RA4EntityActor.h"
#include "RA4SimWorldSubsystem.h"
#include "RA4PlayerController.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Core/Command.h"
#include "RA4SimCoords.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

URA4BuildingPlacementController::URA4BuildingPlacementController()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void URA4BuildingPlacementController::BeginPlay()
{
    Super::BeginPlay();
}

void URA4BuildingPlacementController::StartPlacement(int32 BuildingTypeId, const FString& BuildingName)
{
    CancelPlacement();

    ActiveBuildingTypeId = BuildingTypeId;
    ActiveBuildingName = BuildingName;
    bIsPlacing = true;

    UWorld* World = GetWorld();
    if (World != nullptr)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GhostActor = World->SpawnActor<ARA4EntityActor>(ARA4EntityActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        
        if (GhostActor != nullptr)
        {
#if WITH_EDITOR
            // Editor-only convenience: names the ghost in the World Outliner while
            // placement is being debugged. AActor::SetActorLabel does not exist in a
            // packaged build, so guarding it is what lets the game target link.
            GhostActor->SetActorLabel(FString::Printf(TEXT("GhostPlacement_%s"), *BuildingName));
#endif
            GhostActor->SetTeamColor(FLinearColor(0.2f, 0.9f, 0.2f, 0.6f));
        }
    }

    UE_LOG(LogTemp, Display, TEXT("RA4 Building Placement Started: %s (TypeId=%d)"), *BuildingName, BuildingTypeId);
}

void URA4BuildingPlacementController::CancelPlacement()
{
    if (GhostActor != nullptr)
    {
        GhostActor->Destroy();
        GhostActor = nullptr;
    }

    bIsPlacing = false;
    bCurrentLocationValid = false;
    ActiveBuildingTypeId = 0;
    ActiveBuildingName.Empty();
}

bool URA4BuildingPlacementController::ConfirmPlacement()
{
    if (!bIsPlacing || !bCurrentLocationValid)
    {
        return false;
    }

    UWorld* World = GetWorld();
    URA4SimWorldSubsystem* SimSubsystem = World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    if (SimSubsystem != nullptr)
    {
        // Convert world XY location to tile coordinates
        constexpr double TileSize = 256.0; // RA4::kTileSizeUnits
        const int32 TileX = FMath::FloorToInt(CurrentGridLocation.X / TileSize);
        const int32 TileY = FMath::FloorToInt(CurrentGridLocation.Y / TileSize);

        RA4::Command Cmd;
        Cmd.Type = RA4::CommandType::PlaceBuilding;
        Cmd.Issuer = 0; // Local player
        Cmd.Content = RA4::ContentId{ static_cast<uint32_t>(ActiveBuildingTypeId) };
        Cmd.Tile.X = static_cast<int16_t>(TileX);
        Cmd.Tile.Y = static_cast<int16_t>(TileY);
        Cmd.Location = RA4Coords::FromUnreal(CurrentGridLocation);

        SimSubsystem->EnqueueCommand(Cmd);
        UE_LOG(LogTemp, Display, TEXT("RA4 Building Placement Confirmed: %s (ContentId=%d) at tile (%d, %d)"), *ActiveBuildingName, ActiveBuildingTypeId, TileX, TileY);
    }

    CancelPlacement();
    return true;
}

void URA4BuildingPlacementController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsPlacing)
    {
        UpdateGhostTransform();
    }
}

void URA4BuildingPlacementController::UpdateGhostTransform()
{
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (PC == nullptr)
    {
        return;
    }

    FHitResult HitResult;
    if (PC->GetHitResultUnderCursor(ECC_WorldStatic, false, HitResult))
    {
        const FVector HitPoint = HitResult.ImpactPoint;
        
        // Snap to 64cm grid
        constexpr float GridSize = 64.0f;
        const float SnappedX = FMath::GridSnap(HitPoint.X, GridSize);
        const float SnappedY = FMath::GridSnap(HitPoint.Y, GridSize);

        CurrentGridLocation = FVector(SnappedX, SnappedY, HitPoint.Z);
        bCurrentLocationValid = ValidateLocation(CurrentGridLocation);

        if (GhostActor != nullptr)
        {
            GhostActor->SetActorLocation(CurrentGridLocation);
            UpdateGhostMaterial(bCurrentLocationValid);
        }

        // Draw holographic building placement footprint grid on terrain
        if (UWorld* World = GetWorld())
        {
            const FColor GridColor = bCurrentLocationValid ? FColor(50, 240, 70) : FColor(240, 50, 50);
            constexpr float FootprintSize = 256.0f;
            const FVector Center = CurrentGridLocation + FVector(0.0f, 0.0f, 6.0f);

            // Bounding box frame on ground
            DrawDebugBox(World, Center, FVector(FootprintSize, FootprintSize, 4.0f), GridColor, false, -1.0f, 0, 2.5f);

            // Inner cell crosshairs
            DrawDebugLine(World, Center - FVector(FootprintSize, 0, 0), Center + FVector(FootprintSize, 0, 0), GridColor, false, -1.0f, 0, 1.2f);
            DrawDebugLine(World, Center - FVector(0, FootprintSize, 0), Center + FVector(0, FootprintSize, 0), GridColor, false, -1.0f, 0, 1.2f);

            // Tactical 4-corner vertical ticks
            const float O = FootprintSize;
            DrawDebugLine(World, Center + FVector(O, O, 0), Center + FVector(O, O, 40.0f), GridColor, false, -1.0f, 0, 2.0f);
            DrawDebugLine(World, Center + FVector(-O, O, 0), Center + FVector(-O, O, 40.0f), GridColor, false, -1.0f, 0, 2.0f);
            DrawDebugLine(World, Center + FVector(O, -O, 0), Center + FVector(O, -O, 40.0f), GridColor, false, -1.0f, 0, 2.0f);
            DrawDebugLine(World, Center + FVector(-O, -O, 0), Center + FVector(-O, -O, 40.0f), GridColor, false, -1.0f, 0, 2.0f);
        }
    }
}

bool URA4BuildingPlacementController::ValidateLocation(const FVector& Location) const
{
    UWorld* World = GetWorld();
    const URA4SimWorldSubsystem* SimSubsystem = World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    if (SimSubsystem == nullptr || SimSubsystem->GetSimWorld() == nullptr)
    {
        return true;
    }

    // Convert location to sim tiles
    constexpr double TileSize = 256.0;
    const int32 TileX = FMath::FloorToInt(Location.X / TileSize);
    const int32 TileY = FMath::FloorToInt(Location.Y / TileSize);

    // Basic map bounds check
    const RA4::MapDescription& Map = SimSubsystem->GetSimWorld()->GetMap();
    if (TileX < 2 || TileY < 2 || TileX >= Map.Width - 2 || TileY >= Map.Height - 2)
    {
        return false;
    }

    return true;
}

void URA4BuildingPlacementController::UpdateGhostMaterial(bool bValid)
{
    if (GhostActor != nullptr)
    {
        const FLinearColor ValidColor(0.1f, 0.9f, 0.2f, 0.6f);
        const FLinearColor InvalidColor(0.9f, 0.1f, 0.1f, 0.6f);
        GhostActor->SetTeamColor(bValid ? ValidColor : InvalidColor);
    }
}
