// Copyright (c) Red Alert 4 project.
#include "RA4TerrainSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "EngineUtils.h"
#include "Factories/TextureFactory.h"
#include "LandscapeProxy.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
const TCHAR* kGeneratedRoot = TEXT("/Game/RA4/Generated/Terrain");

// ambientCG's Ground039 set: a dry, gravelly dirt that reads well from an RTS camera
// height and does not fight the faction colours the way grass would.
const TCHAR* kSourceSetDir = TEXT("Content/ThirdParty/ambientCG/Ground039_2K-JPG");
const TCHAR* kColorSuffix = TEXT("Ground039_2K-JPG_Color.jpg");
const TCHAR* kNormalSuffix = TEXT("Ground039_2K-JPG_NormalGL.jpg");
const TCHAR* kRoughnessSuffix = TEXT("Ground039_2K-JPG_Roughness.jpg");

UTexture2D* ImportTexture(const FString& SourceFile, const FString& AssetName, bool bNormalMap)
{
    const FString PackageName = FString(kGeneratedRoot) + TEXT("/") + AssetName;
    if (FPackageName::DoesPackageExist(PackageName))
    {
        return LoadObject<UTexture2D>(nullptr, *(PackageName + TEXT(".") + AssetName));
    }

    if (!FPaths::FileExists(SourceFile))
    {
        UE_LOG(LogTemp, Error, TEXT("RA4TerrainSetup: missing source texture %s"), *SourceFile);
        return nullptr;
    }

    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    UTextureFactory* Factory = NewObject<UTextureFactory>();
    Factory->SuppressImportOverwriteDialog();
    // Normal and roughness carry data, not colour: sampling them through sRGB would
    // bend the values and give the terrain a washed-out, plastic look.
    Factory->LODGroup = TEXTUREGROUP_World;
    if (bNormalMap)
    {
        Factory->CompressionSettings = TC_Normalmap;
        Factory->bFlipNormalMapGreenChannel = false;
    }

    bool bCancelled = false;
    UObject* Imported = Factory->ImportObject(UTexture2D::StaticClass(), Package, FName(*AssetName),
                                             RF_Public | RF_Standalone, SourceFile, nullptr, bCancelled);
    UTexture2D* Texture = Cast<UTexture2D>(Imported);
    if (Texture == nullptr || bCancelled)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4TerrainSetup: failed to import %s"), *SourceFile);
        return nullptr;
    }

    if (bNormalMap)
    {
        Texture->SRGB = false;
        Texture->CompressionSettings = TC_Normalmap;
    }
    Texture->PostEditChange();
    Texture->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Texture);

    const FString FileName =
        FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Texture, *FileName, SaveArgs);
    return Texture;
}
} // namespace

URA4TerrainSetupCommandlet::URA4TerrainSetupCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URA4TerrainSetupCommandlet::Main(const FString& Params)
{
    const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    const FString SetDir = FPaths::Combine(ProjectDir, kSourceSetDir);

    UTexture2D* ColorTex =
        ImportTexture(FPaths::Combine(SetDir, kColorSuffix), TEXT("T_RA4_Ground_Color"), false);
    UTexture2D* NormalTex =
        ImportTexture(FPaths::Combine(SetDir, kNormalSuffix), TEXT("T_RA4_Ground_Normal"), true);
    UTexture2D* RoughTex =
        ImportTexture(FPaths::Combine(SetDir, kRoughnessSuffix), TEXT("T_RA4_Ground_Roughness"), false);

    if (ColorTex == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4TerrainSetup: no base colour texture, aborting"));
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("RA4TerrainSetup: textures ready"));

    // --- material ------------------------------------------------------------
    const FString MaterialName = TEXT("M_RA4_Terrain");
    const FString MaterialPackageName = FString(kGeneratedRoot) + TEXT("/") + MaterialName;

    // Rebuilt from scratch on every run rather than reused when present.
    //
    // This used to load the existing material and skip the whole graph-building
    // block, which meant a change to the tiling constants below silently did
    // nothing: the commandlet reported success, resaved the package, and left the
    // old node graph in place. That is how the 64-repeat tiling survived a fix.
    // Reconciling an existing graph against changed constants is far more
    // error-prone than regenerating a graph this small, and the asset is generated
    // content with no hand edits to preserve.
    UPackage* MatPackage = CreatePackage(*MaterialPackageName);
    MatPackage->FullyLoad();
    UMaterial* Material = NewObject<UMaterial>(MatPackage, *MaterialName,
                                              RF_Public | RF_Standalone);
    if (Material == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4TerrainSetup: could not create the material"));
        return 1;
    }

    {
        // One tiling coordinate drives all three samplers, so the maps stay in
        // register.
        //
        // 10 repeats across a 12800-unit map is one tile per 1280 units, i.e. about
        // 13 metres. This was 64 (one tile per 200 units, two metres), which sounds
        // reasonable until you account for the camera: an RTS view sits 30-80 metres
        // up, so a 2 m tile is under a centimetre on screen and the texture
        // collapses into per-pixel noise that reads as grime instead of ground.
        // A ~13 m tile is a legible surface feature at that distance.
        UMaterialExpressionTextureCoordinate* Coords =
            Cast<UMaterialExpressionTextureCoordinate>(UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionTextureCoordinate::StaticClass(), -700, 0));
        Coords->UTiling = 10.0f;
        Coords->VTiling = 10.0f;

        auto AddSampler = [&](UTexture2D* Texture, int32 PosY, bool bNormal) -> UMaterialExpressionTextureSample*
        {
            if (Texture == nullptr)
            {
                return nullptr;
            }
            UMaterialExpressionTextureSample* Sample =
                Cast<UMaterialExpressionTextureSample>(UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, UMaterialExpressionTextureSample::StaticClass(), -400, PosY));
            Sample->Texture = Texture;
            Sample->SamplerType = bNormal ? SAMPLERTYPE_Normal : SAMPLERTYPE_Color;
            UMaterialEditingLibrary::ConnectMaterialExpressions(Coords, TEXT(""), Sample, TEXT("UVs"));
            return Sample;
        };

        if (UMaterialExpressionTextureSample* ColorSample = AddSampler(ColorTex, -200, false))
        {
            UMaterialEditingLibrary::ConnectMaterialProperty(ColorSample, TEXT(""), MP_BaseColor);
        }
        if (UMaterialExpressionTextureSample* NormalSample = AddSampler(NormalTex, 0, true))
        {
            UMaterialEditingLibrary::ConnectMaterialProperty(NormalSample, TEXT(""), MP_Normal);
        }
        if (UMaterialExpressionTextureSample* RoughSample = AddSampler(RoughTex, 200, false))
        {
            UMaterialEditingLibrary::ConnectMaterialProperty(RoughSample, TEXT(""), MP_Roughness);
        }

        UMaterialEditingLibrary::RecompileMaterial(Material);
        Material->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(Material);

        const FString MatFile = FPackageName::LongPackageNameToFilename(MaterialPackageName,
                                                                        FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Material->GetPackage(), Material, *MatFile, SaveArgs);
        UE_LOG(LogTemp, Display, TEXT("RA4TerrainSetup: material created"));
    }

    // --- map: assign the material and fix the lighting -----------------------
    const FString MapPackageName = TEXT("/Game/Maps/RA4_Skirmish");
    UPackage* MapPackage = LoadPackage(nullptr, *MapPackageName, LOAD_None);
    UWorld* World = MapPackage != nullptr ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
    if (World == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4TerrainSetup: failed to load %s"), *MapPackageName);
        return 1;
    }
    World->WorldType = EWorldType::Editor;
    GWorld = World;
    if (!World->bIsWorldInitialized)
    {
        World->InitWorld();
    }

    int32 LandscapesTouched = 0;
    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
    {
        It->LandscapeMaterial = Material;
        It->PostEditChange();
        ++LandscapesTouched;
    }
    UE_LOG(LogTemp, Display, TEXT("RA4TerrainSetup: terrain material assigned to %d landscape(s)"),
           LandscapesTouched);

    // Sunlight: a low, warm sun gives the blockout geometry long readable shadows.
    // The previous setup was a default white light almost straight overhead, which
    // flattens everything -- that is the "awful lighting".
    int32 SunCount = 0;
    for (TActorIterator<ADirectionalLight> It(World); It; ++It)
    {
        ADirectionalLight* Sun = *It;
        Sun->SetMobility(EComponentMobility::Movable);
        Sun->SetActorRotation(FRotator(-38.0f, 125.0f, 0.0f));
        if (UDirectionalLightComponent* Comp = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
        {
            Comp->SetIntensity(3.2f);
            Comp->SetLightColor(FLinearColor(1.0f, 0.94f, 0.82f));
            Comp->SetDynamicShadowDistanceMovableLight(30000.0f);
            Comp->SetDynamicShadowCascades(4);
            Comp->PostEditChange();
        }
        ++SunCount;
    }

    // Skylight fills the shadows so they read as shadow rather than as black holes.
    for (TActorIterator<ASkyLight> It(World); It; ++It)
    {
        if (USkyLightComponent* Comp = It->GetLightComponent())
        {
            Comp->SetMobility(EComponentMobility::Movable);
            Comp->SetIntensity(1.0f);
            Comp->SetLightColor(FLinearColor(0.72f, 0.80f, 0.95f));
            Comp->PostEditChange();
        }
    }

    // A little haze gives the map depth; without it distant terrain has exactly the
    // same contrast as near terrain and the scene looks flat regardless of the sun.
    // Find-or-spawn, then always configure: a rerun must be able to correct settings
    // it got wrong the first time, not skip because an actor happens to exist.
    AExponentialHeightFog* Fog = nullptr;
    TActorIterator<AExponentialHeightFog> It(World);
    if (It)
    {
        Fog = *It;
    }
    if (Fog == nullptr)
    {
        Fog = World->SpawnActor<AExponentialHeightFog>();
    }
    {
        {
            if (Fog != nullptr)
            {
                Fog->SetActorLabel(TEXT("RA4_HeightFog"));
            }
            if (UExponentialHeightFogComponent* Comp = Fog != nullptr ? Fog->GetComponent() : nullptr)
            {
                Comp->SetMobility(EComponentMobility::Movable);
                // Deliberately faint. An RTS camera looks *down* through the fog
                // column, so density that would be subtle in a first-person scene
                // washes the whole battlefield out to flat grey. This is only meant to
                // soften the far edge of the map, not to be visible on the playfield.
                Comp->SetFogDensity(0.0015f);
                Comp->SetFogHeightFalloff(0.5f);
                Comp->SetFogInscatteringColor(FLinearColor(0.42f, 0.50f, 0.62f));
                Comp->SetStartDistance(9000.0f);
                Comp->PostEditChange();
            }
            UE_LOG(LogTemp, Display, TEXT("RA4TerrainSetup: height fog configured"));
        }
    }

    UE_LOG(LogTemp, Display, TEXT("RA4TerrainSetup: lighting updated (%d directional light(s))"), SunCount);

    MapPackage->MarkPackageDirty();
    const FString MapFile =
        FPackageName::LongPackageNameToFilename(MapPackage->GetName(), FPackageName::GetMapPackageExtension());
    FSavePackageArgs MapSaveArgs;
    MapSaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    if (!UPackage::SavePackage(MapPackage, World, *MapFile, MapSaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("RA4TerrainSetup: failed to save %s"), *MapFile);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("RA4TerrainSetup: map saved"));
    return 0;
}
