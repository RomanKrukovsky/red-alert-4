// Copyright (c) Red Alert 4 project.
#include "RA4SkirmishGameMode.h"

#include "RA4CameraPawn.h"
#include "RA4PlayerController.h"
#include "RA4RtsHud.h"
#include "RA4SimCoords.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include "RA4GameState.h"
#include "RA4PlayerState.h"
#include "RA4SimWorldSubsystem.h"
#include "Kismet/GameplayStatics.h"

ARA4SkirmishGameMode::ARA4SkirmishGameMode()
{
    PlayerControllerClass = ARA4PlayerController::StaticClass();
    DefaultPawnClass = ARA4CameraPawn::StaticClass();
    HUDClass = ARA4RtsHud::StaticClass();
    GameStateClass = ARA4GameState::StaticClass();
    PlayerStateClass = ARA4PlayerState::StaticClass();
    // The camera pawn is spawned at the origin and then driven entirely by the
    // camera controller, so no PlayerStart is required to make the map playable.
    bStartPlayersAsSpectators = false;
}

void ARA4SkirmishGameMode::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Display, TEXT("RA4 skirmish game mode started"));

    // Extract options from the URL (Main Menu passes them here)
    FString Options = OptionsString;
    uint8 PlayerFaction = UGameplayStatics::GetIntOption(Options, TEXT("PlayerFaction"), 0);
    uint8 EnemyFaction = UGameplayStatics::GetIntOption(Options, TEXT("EnemyFaction"), 1);
    int32 Difficulty = UGameplayStatics::GetIntOption(Options, TEXT("Difficulty"), 1);

    // Start the simulation match
    if (UWorld* World = GetWorld())
    {
        if (URA4SimWorldSubsystem* SimSub = World->GetSubsystem<URA4SimWorldSubsystem>())
        {
            SimSub->StartSkirmishMatch(PlayerFaction, EnemyFaction, Difficulty);
        }
    }

    AStaticMeshActor* LargestFloorActor = nullptr;
    double LargestFloorArea = 0.0;

    // This project uses the default exposure range rather than the extended
    // physical-luminance range. A physical 75,000-lux value clips this simple
    // skirmish map to solid white, while the old authored value 6 is too dark.
    // Normalize either legacy extreme to the level generator's tested value.
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        if (UDirectionalLightComponent* Light =
                Cast<UDirectionalLightComponent>(It->GetLightComponent()))
        {
            Light->SetMobility(EComponentMobility::Movable);
            // Non-physical exposure is used by this prototype level. A low value
            // preserves saturated team colours instead of washing every placeholder
            // mesh to white.
            Light->SetIntensity(5.0f);
        }
    }

    for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
    {
        if (USkyLightComponent* Sky = It->GetLightComponent())
        {
            Sky->SetMobility(EComponentMobility::Movable);
            Sky->SetIntensity(0.5f);
        }
    }

    // Keep old copies of RA4_Skirmish playable after the material assets evolve.
    // The level contains a single authored static-mesh actor: the map-sized floor.
    // Runtime entities are ARA4EntityActor instances and are therefore unaffected.
    UMaterialInterface* GroundMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (GroundMaterial != nullptr)
    {
        for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
        {
            if (UStaticMeshComponent* Mesh = It->GetStaticMeshComponent())
            {
                const FVector Extent = Mesh->Bounds.BoxExtent;
                const double FloorArea = double(Extent.X) * double(Extent.Y);
                if (Extent.Z > 1.0f && Extent.X > 1000.0f && Extent.Y > 1000.0f &&
                    FloorArea > LargestFloorArea)
                {
                    LargestFloorArea = FloorArea;
                    LargestFloorActor = *It;
                }

                if (UMaterialInstanceDynamic* Material =
                        UMaterialInstanceDynamic::Create(GroundMaterial, Mesh))
                {
                    Material->SetVectorParameterValue(
                        TEXT("Color"),
                        FLinearColor(0.025f, 0.055f, 0.065f));
                    Mesh->SetMaterial(0, Material);
                }
            }
        }
    }

    if (LargestFloorActor != nullptr)
    {
        if (UStaticMeshComponent* Mesh = LargestFloorActor->GetStaticMeshComponent())
        {
            const float HalfThickness = Mesh->Bounds.BoxExtent.Z;
            Mesh->SetMobility(EComponentMobility::Movable);
            FVector Location = LargestFloorActor->GetActorLocation();
            // The simulation and picking treat GroundZ as the walkable surface. Many
            // placeholder floor meshes are authored around their centre, so align the
            // mesh top to GroundZ instead of leaving the camera to look through it.
            Location.Z = float(RA4Coords::GroundZ) - HalfThickness;
            LargestFloorActor->SetActorLocation(Location);
        }
    }
}
