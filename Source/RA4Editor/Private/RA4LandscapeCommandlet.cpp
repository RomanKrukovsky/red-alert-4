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
    // Deliberately not GEditor->GetEditorWorldContext().World(): a commandlet's
    // ambient editor world is whatever EditorStartupMap says (Entry, in this
    // project), never the map passed on the command line. Loading the target level's
    // package directly is the reliable way a commandlet gets hold of a specific map.
    const FString MapPackageName = TEXT("/Game/Maps/RA4_Skirmish");
    UPackage* MapPackage = LoadPackage(nullptr, *MapPackageName, LOAD_None);
    UWorld* World = MapPackage != nullptr ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4Landscape: failed to load %s"), *MapPackageName);
        return 1;
    }

    World->WorldType = EWorldType::Editor;
    GWorld = World;
    if (!World->bIsWorldInitialized)
    {
        World->InitWorld();
    }
    World->PersistentLevel->UpdateModelComponents();
    World->UpdateWorldComponents(/*bRerunConstructionScripts*/ true, /*bCurrentLevelOnly*/ false);

    // Mirrors FRA4MatchBootstrap::BuildSkirmish: 64 tiles at 200 units each. Kept in
    // sync manually rather than linked against RA4Core, since a commandlet has no
    // reason to depend on the simulation module just to read two constants.
    constexpr int32 MapTiles = 64;
    constexpr int32 TileSizeUnits = 200;
    constexpr double SpanUnits = double(MapTiles) * double(TileSizeUnits);   // 12800

    constexpr int32 SectionsPerComponent = 1;
    constexpr int32 QuadsPerSection = 63;   // one of the landscape's fixed valid sizes
    constexpr int32 ComponentCount = 4;
    constexpr int32 QuadsPerComponent = SectionsPerComponent * QuadsPerSection;
    const int32 SizeX = ComponentCount * QuadsPerComponent + 1;   // 253 verts/side
    const int32 SizeY = SizeX;

    const double ScaleXY = SpanUnits / double(SizeX - 1);
    constexpr double ScaleZ = 100.0;
    // How many world units one step of raw heightmap data is worth at ScaleZ: fixed
    // by the landscape format (LANDSCAPE_ZSCALE = 1/128), not something to tune.
    constexpr double LandscapeZScale = 1.0 / 128.0;

    // Gentle rolling terrain, not mountains: the simulation is flat and 2D, and units
    // only get their visual Z lifted to follow the surface (a separate, later piece
    // of work) -- a steep slope would still let a tank drive through what looks like
    // a cliff face, so the relief here is kept modest on purpose.
    constexpr float AmplitudeUnits = 220.0f;
    const float AmplitudeRaw = AmplitudeUnits / float(ScaleZ * LandscapeZScale);

    TArray<uint16> HeightData;
    HeightData.SetNumUninitialized(SizeX * SizeY);
    for (int32 Y = 0; Y < SizeY; ++Y)
    {
        for (int32 X = 0; X < SizeX; ++X)
        {
            const float WorldX = float(double(X) * ScaleXY);
            const float WorldY = float(double(Y) * ScaleXY);
            const float Height = RollingHills(WorldX, WorldY, 20260730u);
            const int32 Raw = FMath::Clamp(int32(32768.0f + Height * AmplitudeRaw), 0, 65535);
            HeightData[Y * SizeX + X] = uint16(Raw);
        }
    }

    RemovePlaceholderGroundPlane(World);

    // The landscape's own origin is its corner, not its centre, so it is offset back
    // by half its footprint to land exactly on the map's (0,0)-(Span,Span) rectangle
    // -- the same rectangle every other system (camera bounds, ground picking,
    // minimap) already agrees on.
    const FVector Centre(SpanUnits * 0.5, SpanUnits * 0.5, 0.0);
    const FVector Origin =
        Centre - FVector(double(SizeX - 1) * ScaleXY * 0.5, double(SizeY - 1) * ScaleXY * 0.5, 0.0);

    ALandscape* Landscape = World->SpawnActor<ALandscape>(Origin, FRotator::ZeroRotator);
    if (Landscape == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4Landscape: SpawnActor<ALandscape> failed"));
        return 1;
    }
    Landscape->SetActorRelativeScale3D(FVector(ScaleXY, ScaleXY, ScaleZ));
    Landscape->SetActorLabel(TEXT("RA4_Landscape"));

    // Reuses the project's own ground material rather than inventing a new one blind:
    // it already exposes a GroundColor parameter and was clearly authored for this
    // purpose, just never assigned to anything before now.
    UMaterialInterface* GroundMaterial =
        LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RA4/Materials/M_RA4Ground_Lit.M_RA4Ground_Lit"));
    if (GroundMaterial == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RA4Landscape: M_RA4Ground_Lit not found, landscape will use the engine default material"));
    }
    Landscape->LandscapeMaterial = GroundMaterial;

    TMap<FGuid, TArray<uint16>> HeightDataPerLayers;
    HeightDataPerLayers.Add(FGuid(), HeightData);
    // No paintable material layers: the ground material colours by parameter, not by
    // landscape weightmap, so there is nothing to import here.
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayers;
    MaterialLayerDataPerLayers.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());

    Landscape->Import(FGuid::NewGuid(), 0, 0, SizeX - 1, SizeY - 1, SectionsPerComponent, QuadsPerSection,
                      HeightDataPerLayers, nullptr, MaterialLayerDataPerLayers,
                      ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());

    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    if (LandscapeInfo == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4Landscape: Import() did not produce a LandscapeInfo"));
        return 1;
    }
    LandscapeInfo->UpdateLayerInfoMap(Landscape);
    Landscape->PostEditChange();

    UE_LOG(LogTemp, Display,
           TEXT("RA4Landscape: created a %d x %d landscape covering %.0f x %.0f units, centred at (%.0f, %.0f)"),
           SizeX, SizeY, SpanUnits, SpanUnits, Centre.X, Centre.Y);

    MapPackage->MarkPackageDirty();
    const FString PackageFileName =
        FPackageName::LongPackageNameToFilename(MapPackage->GetName(), FPackageName::GetMapPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    if (!UPackage::SavePackage(MapPackage, World, *PackageFileName, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("RA4Landscape: failed to save %s"), *PackageFileName);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("RA4Landscape: level saved to %s"), *PackageFileName);
    return 0;
}
