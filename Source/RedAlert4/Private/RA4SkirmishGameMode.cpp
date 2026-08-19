// Copyright (c) Red Alert 4 project.
#include "RA4SkirmishGameMode.h"

#include "RA4CameraPawn.h"
#include "RA4PlayerController.h"
#include "RA4RtsHud.h"
#include "RA4SimCoords.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include "RA4GameState.h"
#include "RA4PlayerState.h"
#include "RA4SimWorldSubsystem.h"
#include "RA4NetworkManager.h"
#include "RA4NetworkChannel.h"
#include "Kismet/GameplayStatics.h"

#include "RA4Content/ContentTypes.h"

namespace
{
constexpr uint8 SovietFaction = static_cast<uint8>(RA4::FactionId::Soviet);
constexpr uint8 AllianceFaction = static_cast<uint8>(RA4::FactionId::Alliance);
}

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

    StartSimulationMatch();
    ApplySceneDressing();
}

void ARA4SkirmishGameMode::StartSimulationMatch()
{
    UE_LOG(LogTemp, Display, TEXT("RA4 skirmish game mode started"));

    // Extract options from the URL (Main Menu passes them here)
    FString Options = OptionsString;
    uint8 PlayerFaction = UGameplayStatics::GetIntOption(Options, TEXT("PlayerFaction"), SovietFaction);
    uint8 EnemyFaction = UGameplayStatics::GetIntOption(Options, TEXT("EnemyFaction"), AllianceFaction);
    int32 Difficulty = UGameplayStatics::GetIntOption(Options, TEXT("Difficulty"), 1);
    int32 AISpot = UGameplayStatics::GetIntOption(Options, TEXT("AISpot"), -1);
    int32 NumAI = UGameplayStatics::GetIntOption(Options, TEXT("NumAI"), 1);
    // Unreliable-recon skirmish options (ADR-0026). Open options, not cheat codes:
    // ?Recon=1 makes belief drive the HUD, ?ShowTruth=1 also raises the two-maps
    // overlay so a player can watch how wrong their staff map is.
    const bool bReconEnabled = UGameplayStatics::GetIntOption(Options, TEXT("Recon"), 0) != 0;
    const bool bReconShowTruth = UGameplayStatics::GetIntOption(Options, TEXT("ShowTruth"), 0) != 0;
    if (NumAI < 1)
    {
        NumAI = 1;
    }

    // ?NumPlayers>1 turns this into a lockstep match. At 1 nothing below runs and the
    // simulation ticks locally exactly as it always has, which is what keeps the
    // single-player path free of any dependency on the network being healthy.
    ExpectedPlayers = UGameplayStatics::GetIntOption(Options, TEXT("NumPlayers"), 1);
    if (ExpectedPlayers < 1)
    {
        ExpectedPlayers = 1;
    }

    const auto IsPlayableFaction = [](uint8 Faction)
    {
        return Faction == SovietFaction || Faction == AllianceFaction;
    };
    if (!IsPlayableFaction(PlayerFaction))
    {
        UE_LOG(LogTemp, Warning, TEXT("RA4 invalid PlayerFaction=%u; using Soviet"), PlayerFaction);
        PlayerFaction = SovietFaction;
    }
    if (!IsPlayableFaction(EnemyFaction) || EnemyFaction == PlayerFaction)
    {
        EnemyFaction = PlayerFaction == SovietFaction ? AllianceFaction : SovietFaction;
        UE_LOG(LogTemp, Warning, TEXT("RA4 invalid EnemyFaction option; using %u"), EnemyFaction);
    }

    UE_LOG(LogTemp, Display, TEXT("RA4 skirmish factions: player=%u enemy=%u difficulty=%d ai=%d aispot=%d"),
           PlayerFaction, EnemyFaction, Difficulty, NumAI, AISpot);

    // Start the simulation match
    if (UWorld* World = GetWorld())
    {
        if (URA4SimWorldSubsystem* SimSub = World->GetSubsystem<URA4SimWorldSubsystem>())
        {
            SimSub->ConfigureRecon(bReconEnabled, bReconShowTruth);
            SimSub->StartSkirmishMatch(PlayerFaction, EnemyFaction, Difficulty, NumAI, AISpot);
        }
    }
}

void ARA4SkirmishGameMode::ApplySceneDressing()
{
    AStaticMeshActor* LargestFloorActor = nullptr;
    double LargestFloorArea = 0.0;

    // Directional light tuned for RTS sunlight
    bool bFoundSun = false;
    for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
    {
        bFoundSun = true;
        if (UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent()))
        {
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetIntensity(85000.0f);
            Light->SetLightColor(FLinearColor(1.0f, 0.98f, 0.94f));
            Light->bAtmosphereSunLight = true;
            Light->DynamicShadowDistanceMovableLight = 30000.0f;
        }
    }
    if (!bFoundSun && GetWorld() != nullptr)
    {
        ADirectionalLight* SunActor = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0,0,500), FRotator(-52.0f, 38.0f, 0.0f));
        if (SunActor && SunActor->GetLightComponent())
        {
            SunActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            SunActor->GetLightComponent()->SetIntensity(85000.0f);
            SunActor->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.98f, 0.94f));
        }
    }

    // Sky light for ambient shadow fill
    bool bFoundSky = false;
    for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
    {
        bFoundSky = true;
        if (USkyLightComponent* Sky = It->GetLightComponent())
        {
            Sky->SetMobility(EComponentMobility::Movable);
            Sky->SetIntensity(3.5f);
            Sky->SetLightColor(FLinearColor(0.85f, 0.93f, 1.0f));
            Sky->bLowerHemisphereIsBlack = false;
            Sky->LowerHemisphereColor = FLinearColor(0.25f, 0.32f, 0.18f);
            Sky->bRealTimeCapture = true;
        }
    }
    if (!bFoundSky && GetWorld() != nullptr)
    {
        ASkyLight* SkyActor = GetWorld()->SpawnActor<ASkyLight>(ASkyLight::StaticClass());
        if (SkyActor && SkyActor->GetLightComponent())
        {
            SkyActor->GetLightComponent()->SetMobility(EComponentMobility::Movable);
            SkyActor->GetLightComponent()->SetIntensity(3.5f);
            SkyActor->GetLightComponent()->SetLightColor(FLinearColor(0.85f, 0.93f, 1.0f));
            SkyActor->GetLightComponent()->bLowerHemisphereIsBlack = false;
            SkyActor->GetLightComponent()->LowerHemisphereColor = FLinearColor(0.25f, 0.32f, 0.18f);
            SkyActor->GetLightComponent()->bRealTimeCapture = true;
        }
    }

    // Configure ExponentialHeightFog
    bool bFoundFog = false;
    for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
    {
        bFoundFog = true;
        if (UExponentialHeightFogComponent* Fog = It->GetComponent())
        {
            Fog->SetFogDensity(0.0012f);
            Fog->SetStartDistance(6000.0f);
        }
    }
    if (!bFoundFog && GetWorld() != nullptr)
    {
        AExponentialHeightFog* FogActor = GetWorld()->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass());
        if (FogActor && FogActor->GetComponent())
        {
            FogActor->GetComponent()->SetFogDensity(0.0012f);
            FogActor->GetComponent()->SetStartDistance(6000.0f);
        }
    }

    // Load PBR Ground material for terrain floor
    UMaterialInterface* GroundMaterial = LoadObject<UMaterialInterface>(
        nullptr,
        TEXT("/Game/RA4/Presentation/Materials/Environment/Ground039.Ground039"));
    if (GroundMaterial == nullptr)
    {
        GroundMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Game/RA4/Presentation/Materials/Blockout/M_RA4_BlockoutGround.M_RA4_BlockoutGround"));
    }
    if (GroundMaterial == nullptr)
    {
        GroundMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Game/ThirdParty/CityPark/Materials/Ground/MI_Landscape.MI_Landscape"));
    }
    if (GroundMaterial == nullptr)
    {
        GroundMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Game/ThirdParty/CityPark/Materials/Ground/MI_Ground01.MI_Ground01"));
    }

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

            if (GroundMaterial != nullptr)
            {
                Mesh->SetMaterial(0, GroundMaterial);
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
            Location.Z = float(RA4Coords::GroundZ) - HalfThickness;
            LargestFloorActor->SetActorLocation(Location);
        }
    }

    // Scatter industrial props around the map for visual detail
    static const TCHAR* PropPaths[] = {
        TEXT("/Game/ThirdParty/IndustryPropsPack6/Meshes/SM_Barrel01.SM_Barrel01"),
        TEXT("/Game/ThirdParty/IndustryPropsPack6/Meshes/SM_Barrel02.SM_Barrel02"),
        TEXT("/Game/ThirdParty/IndustryPropsPack6/Meshes/SM_Pallet01.SM_Pallet01"),
        TEXT("/Game/ThirdParty/IndustryPropsPack6/Meshes/SM_WaterTank01.SM_WaterTank01"),
        TEXT("/Game/ThirdParty/IndustryPropsPack6/Meshes/SM_Box01.SM_Box01"),
        TEXT("/Game/ThirdParty/IndustryPropsPack6/Meshes/SM_TrafficBarrel01.SM_TrafficBarrel01")
    };
    TArray<UStaticMesh*> PropMeshes;
    for (const TCHAR* Path : PropPaths)
    {
        if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path))
        {
            PropMeshes.Add(Mesh);
        }
    }
    if (PropMeshes.Num() > 0 && GetWorld() != nullptr)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const FVector ScatterSpots[] = {
            FVector(400.0, 400.0, RA4Coords::GroundZ),
            FVector(450.0, 380.0, RA4Coords::GroundZ),
            FVector(380.0, 480.0, RA4Coords::GroundZ),
            FVector(-400.0, -400.0, RA4Coords::GroundZ),
            FVector(-420.0, -360.0, RA4Coords::GroundZ),
            FVector(1200.0, 800.0, RA4Coords::GroundZ),
            FVector(-1200.0, -800.0, RA4Coords::GroundZ)
        };
        for (int32 i = 0; i < 7; ++i)
        {
            AStaticMeshActor* PropActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), ScatterSpots[i], FRotator(0.0f, float(i * 45), 0.0f), SpawnParams);
            if (PropActor && PropActor->GetStaticMeshComponent())
            {
                PropActor->GetStaticMeshComponent()->SetStaticMesh(PropMeshes[i % PropMeshes.Num()]);
                PropActor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            }
        }
    }
}

void ARA4SkirmishGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (ExpectedPlayers <= 1 || NewPlayer == nullptr)
    {
        return;
    }

    if (NextPlayerSlot >= ExpectedPlayers)
    {
        // Slots are the lockstep protocol's addressing; there is no slot for this
        // player, so admitting them would mean their orders had nowhere to go.
        UE_LOG(LogTemp, Warning, TEXT("RA4 refusing extra player: match is full at %d."),
               ExpectedPlayers);
        return;
    }

    // The channel is the player's RPC endpoint. It lives on the PlayerController
    // because that is the only actor with an owning connection per player, which is
    // what lets the server address one client and attribute one client's submission.
    URA4NetworkChannel* Channel = NewObject<URA4NetworkChannel>(NewPlayer);
    Channel->RegisterComponent();

    const int32 Slot = NextPlayerSlot++;
    Channel->ConfigureAsServer(Slot, ExpectedPlayers, int32(RA4::Net::kDefaultInputDelay));

    UE_LOG(LogTemp, Display, TEXT("RA4 player joined in slot %d of %d."), Slot, ExpectedPlayers);

    TryStartNetworkedMatch();
}

void ARA4SkirmishGameMode::TryStartNetworkedMatch()
{
    if (bMatchStarted || NextPlayerSlot < ExpectedPlayers)
    {
        return;
    }

    UWorld* World = GetWorld();
    URA4NetworkManager* Network = World ? World->GetSubsystem<URA4NetworkManager>() : nullptr;
    if (Network == nullptr)
    {
        return;
    }

    bMatchStarted = true;

    // Slot 0 is the host. BeginMatch runs after every channel is registered, because
    // it primes the first input-delay ticks with empty frames and those have to be
    // able to reach the clients that are already waiting on them.
    Network->BeginMatch(/*LocalPlayerIndex*/ 0, uint8(ExpectedPlayers), /*bIsServer*/ true,
                        RA4::Net::kDefaultInputDelay);

    UE_LOG(LogTemp, Display, TEXT("RA4 lockstep match started with %d players."), ExpectedPlayers);
}
