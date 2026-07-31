// Copyright (c) Red Alert 4 project.
#include "RA4LandscapeCommandlet.h"

#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "FileHelpers.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

namespace
{
// Smooth, deterministic, dependency-free "hills" -- three sine layers at different
// frequencies and phases. No noise library needed, and it is C0-continuous
// everywhere, which is what keeps the relief gentle enough for an RTS: real slopes
// with no cliffs, since the simulation underneath has no idea the ground undulates.
float RollingHills(float WorldX, float WorldY, uint32 Seed)
{
    const float SeedPhase = float(Seed % 1000) * 0.01f;
    float Value = 0.0f;
    Value += FMath::Sin(WorldX * 0.00028f + SeedPhase) * FMath::Cos(WorldY * 0.00024f + SeedPhase * 1.7f);
    Value += 0.5f * FMath::Sin(WorldX * 0.0006f - SeedPhase * 0.5f) * FMath::Cos(WorldY * 0.0005f + SeedPhase);
    Value += 0.25f * FMath::Sin((WorldX + WorldY) * 0.0009f + SeedPhase * 2.0f);
    return Value / 1.75f;   // roughly [-1, 1]
}

// The existing ground plane is a scaled engine cube (see
// URA4SimWorldSubsystem::FitGroundPlaneToMap); once a real landscape covers the same
// area it would only z-fight underneath it, so it is removed here.
void RemovePlaceholderGroundPlane(UWorld* World)
{
    TArray<AStaticMeshActor*> ToDestroy;
    for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        const UStaticMeshComponent* MeshComp = Actor != nullptr ? Actor->GetStaticMeshComponent() : nullptr;
        const UStaticMesh* Asset = MeshComp != nullptr ? MeshComp->GetStaticMesh() : nullptr;
        if (Asset != nullptr && Asset->GetPathName().Contains(TEXT("BasicShapes/Cube")))
        {
            ToDestroy.Add(Actor);
        }
    }
    for (AStaticMeshActor* Actor : ToDestroy)
    {
        UE_LOG(LogTemp, Display, TEXT("RA4Landscape: removing placeholder ground plane actor %s"), *Actor->GetName());
        Actor->Destroy();
    }
}
} // namespace

URA4LandscapeCommandlet::URA4LandscapeCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URA4LandscapeCommandlet::Main(const FString& Params)
{
    const TArray<FString> MapPackagesToBuild = {
        TEXT("/Game/Maps/RA4_Skirmish_Production"),
        TEXT("/Game/Maps/RA4_Skirmish"),
        TEXT("/Game/Maps/RA4_Skirmish_Hills"),
        TEXT("/Game/Maps/RA4_Skirmish_Canyon")
    };

    UMaterialInterface* GroundMaterial =
        LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Presentation/Materials/Environment/Ground039.Ground039"));
    if (GroundMaterial == nullptr)
    {
        GroundMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Materials/M_RA4Ground_Lit.M_RA4Ground_Lit"));
    }

    for (int32 MapIndex = 0; MapIndex < MapPackagesToBuild.Num(); ++MapIndex)
    {
        const FString& MapPackageName = MapPackagesToBuild[MapIndex];
        UPackage* MapPackage = LoadPackage(nullptr, *MapPackageName, LOAD_None);
        if (MapPackage == nullptr)
        {
            MapPackage = CreatePackage(*MapPackageName);
        }

        UWorld* World = MapPackage != nullptr ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
        if (World == nullptr && MapPackage != nullptr)
        {
            World = UWorld::CreateWorld(EWorldType::Editor, false, FName(*FPaths::GetBaseFilename(MapPackageName)), MapPackage);
        }

        if (World == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4Landscape: failed to load or create %s"), *MapPackageName);
            continue;
        }

        World->WorldType = EWorldType::Editor;
        GWorld = World;
        if (!World->bIsWorldInitialized)
        {
            World->InitWorld();
        }
        World->PersistentLevel->UpdateModelComponents();
        World->UpdateWorldComponents(/*bRerunConstructionScripts*/ true, /*bCurrentLevelOnly*/ false);

        constexpr int32 MapTiles = 64;
        constexpr int32 TileSizeUnits = 200;
        constexpr double SpanUnits = double(MapTiles) * double(TileSizeUnits);   // 12800

        constexpr int32 SectionsPerComponent = 1;
        constexpr int32 QuadsPerSection = 63;
        constexpr int32 ComponentCount = 4;
        constexpr int32 QuadsPerComponent = SectionsPerComponent * QuadsPerSection;
        const int32 SizeX = ComponentCount * QuadsPerComponent + 1;   // 253 verts/side
        const int32 SizeY = SizeX;

        const double ScaleXY = SpanUnits / double(SizeX - 1);
        constexpr double ScaleZ = 100.0;
        constexpr double LandscapeZScale = 1.0 / 128.0;

        constexpr float AmplitudeUnits = 220.0f;
        const float AmplitudeRaw = AmplitudeUnits / float(ScaleZ * LandscapeZScale);

        TArray<uint16> HeightData;
        HeightData.SetNumUninitialized(SizeX * SizeY);
        const uint32 Seed = 20260730u + uint32(MapIndex) * 7777u;
        for (int32 Y = 0; Y < SizeY; ++Y)
        {
            for (int32 X = 0; X < SizeX; ++X)
            {
                const float WorldX = float(double(X) * ScaleXY);
                const float WorldY = float(double(Y) * ScaleXY);
                const float Height = RollingHills(WorldX, WorldY, Seed);
                const int32 Raw = FMath::Clamp(int32(32768.0f + Height * AmplitudeRaw), 0, 65535);
                HeightData[Y * SizeX + X] = uint16(Raw);
            }
        }

        RemovePlaceholderGroundPlane(World);

        const FVector Centre(SpanUnits * 0.5, SpanUnits * 0.5, 0.0);
        const FVector Origin =
            Centre - FVector(double(SizeX - 1) * ScaleXY * 0.5, double(SizeY - 1) * ScaleXY * 0.5, 0.0);

        ALandscape* Landscape = nullptr;
        TActorIterator<ALandscape> LandscapeIt(World);
        if (LandscapeIt)
        {
            Landscape = *LandscapeIt;
        }

        if (Landscape == nullptr)
        {
            FActorSpawnParameters ActorSpawnParams;
            ActorSpawnParams.Name = FName(*FString::Printf(TEXT("RA4_Landscape_%d"), MapIndex));
            Landscape = World->SpawnActor<ALandscape>(Origin, FRotator::ZeroRotator, ActorSpawnParams);
        }

        if (Landscape == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4Landscape: failed to spawn ALandscape for %s"), *MapPackageName);
            continue;
        }

        Landscape->SetActorLocation(Origin);
        Landscape->SetActorScale3D(FVector(ScaleXY, ScaleXY, ScaleZ));

        if (GroundMaterial != nullptr)
        {
            Landscape->LandscapeMaterial = GroundMaterial;
        }

        TMap<FGuid, TArray<uint16>> HeightmapDataPerLayers;
        HeightmapDataPerLayers.Add(FGuid(), HeightData);
        TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
        MaterialLayerDataPerLayers.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

        Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1, SectionsPerComponent, QuadsPerSection,
                          HeightmapDataPerLayers, nullptr, MaterialLayerDataPerLayers,
                          ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());

        Landscape->PostEditChange();

        UE_LOG(LogTemp, Display,
               TEXT("RA4Landscape: created landscape covering %.0f x %.0f units for %s"),
               SpanUnits, SpanUnits, *MapPackageName);

        World->SetFlags(RF_Standalone | RF_Public);
        MapPackage->MarkPackageDirty();
        const FString PackageFileName =
            FPackageName::LongPackageNameToFilename(MapPackage->GetName(), FPackageName::GetMapPackageExtension());

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(MapPackage, World, *PackageFileName, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("RA4Landscape: failed to save %s"), *PackageFileName);
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("RA4Landscape: level saved to %s"), *PackageFileName);
        }
    }
    return 0;
}
