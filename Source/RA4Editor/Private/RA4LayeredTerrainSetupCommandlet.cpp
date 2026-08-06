// Copyright (c) Red Alert 4 project.
//
// Builds the layered landscape material for the archipelago map.
//
// WHY THIS EXISTS ALONGSIDE RA4TerrainSetupCommandlet
// ---------------------------------------------------
// RA4TerrainSetupCommandlet imports one ambientCG set (Ground039, a dry gravel)
// and wires it straight into BaseColor/Normal/Roughness. That gives the whole
// 12800-unit map a single texture: no beaches, no grass, no rock on the cliffs.
// The archipelago needs four surfaces the artist can paint, which means a
// LandscapeLayerBlend node and a LandscapeLayerInfoObject per layer, not three
// samplers.
//
// This commandlet is therefore additive rather than a rewrite: it leaves
// M_RA4_Terrain alone and produces M_RA4_TerrainLayered next to it, so the
// existing single-texture path keeps working and can be compared against.
//
// Run:
//   UnrealEditor-Cmd RedAlert4.uproject -run=RA4LayeredTerrainSetup
//
// LICENSING
// ---------
// Every texture here is from ambientCG and CC0 1.0 (public domain), already
// recorded as permitted in Docs/Production/LEGAL_AND_LICENSES.md. No attribution
// is required and nothing here is derived from a licensed third-party pack.
#include "RA4LayeredTerrainSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "Factories/TextureFactory.h"
#include "LandscapeLayerInfoObject.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
const TCHAR* kGeneratedRoot = TEXT("/Game/RA4/Generated/Terrain");
const TCHAR* kLayerInfoRoot = TEXT("/Game/RA4/Generated/Terrain/Layers");

// Tiling is per layer on purpose, and the numbers are far lower than they look.
//
// WHY THE OLD VALUES (64-80) LOOKED LIKE DIRT AND NOISE
// ----------------------------------------------------
// The map is 12800 units across, so 64 repeats put one tile every 200 units - two
// metres. An RTS camera sits 30-80 metres up, which means a 2 m tile is well under
// a centimetre on screen: the texture collapses into per-pixel noise and reads as
// grime rather than ground. That is the "ugly ground texture" complaint, and it was
// a resolution problem, not a colour problem. Measured: Ground039 is only 2.8%
// rust-hued pixels, the most neutral of five ambientCG ground sets tested, so the
// colour was never the fault.
//
// 8-12 repeats put one tile every 1000-1600 units (10-16 m), which is a legible
// surface feature at that camera height. Rock stays lowest because its features are
// largest; grass is highest because fine blades tolerate repetition.
struct FLayerSpec
{
    const TCHAR* LayerName;
    const TCHAR* SourceSet;
    const TCHAR* AssetPrefix;
    float Tiling;
};

const FLayerSpec kLayers[] = {
    // Dirt first: it is the base the others paint over, so it is the layer whose
    // weight the landscape falls back to when nothing else is painted.
    {TEXT("Dirt"), TEXT("Ground039_2K-JPG"), TEXT("Dirt"), 10.0f},
    {TEXT("Sand"), TEXT("Ground054_2K-JPG"), TEXT("Sand"), 10.0f},
    {TEXT("Grass"), TEXT("Grass004_2K-JPG"), TEXT("Grass"), 12.0f},
    {TEXT("Rock"), TEXT("Rock030_2K-JPG"), TEXT("Rock"), 8.0f},
};

constexpr int32 kLayerCount = UE_ARRAY_COUNT(kLayers);

FString SourceFileFor(const FLayerSpec& Layer, const TCHAR* MapSuffix)
{
    // ambientCG names files "<Set>_<Map>.jpg", e.g. "Grass004_2K-JPG_Color.jpg".
    const FString FileName = FString(Layer.SourceSet) + TEXT("_") + MapSuffix + TEXT(".jpg");
    return FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),
                           TEXT("Content/ThirdParty/ambientCG"), Layer.SourceSet, FileName);
}

// Imports one JPG as a Texture2D under kGeneratedRoot, or returns the existing
// asset. Normal maps need their compression and sRGB set before the first build
// or they are treated as colour and the lighting comes out wrong.
UTexture2D* ImportOrLoadTexture(const FString& SourceFile, const FString& AssetName, bool bNormalMap)
{
    const FString PackageName = FString(kGeneratedRoot) + TEXT("/") + AssetName;
    if (FPackageName::DoesPackageExist(PackageName))
    {
        if (UTexture2D* Existing =
                LoadObject<UTexture2D>(nullptr, *(PackageName + TEXT(".") + AssetName)))
        {
            return Existing;
        }
    }

    if (!FPaths::FileExists(SourceFile))
    {
        UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: source texture missing: %s"), *SourceFile);
        return nullptr;
    }

    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    UTextureFactory* Factory = NewObject<UTextureFactory>();
    Factory->AddToRoot();
    Factory->SuppressImportOverwriteDialog();

    bool bCancelled = false;
    UObject* Imported = Factory->ImportObject(UTexture2D::StaticClass(), Package, FName(*AssetName),
                                              RF_Public | RF_Standalone, SourceFile, nullptr, bCancelled);
    Factory->RemoveFromRoot();

    UTexture2D* Texture = Cast<UTexture2D>(Imported);
    if (Texture == nullptr || bCancelled)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: import failed for %s"), *SourceFile);
        return nullptr;
    }

    if (bNormalMap)
    {
        Texture->CompressionSettings = TC_Normalmap;
        Texture->SRGB = false;
        Texture->LODGroup = TEXTUREGROUP_WorldNormalMap;
    }
    else
    {
        Texture->LODGroup = TEXTUREGROUP_World;
    }
    Texture->PostEditChange();
    Texture->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Texture);

    return Texture;
}

// A landscape needs a LayerInfoObject per paintable layer; without one the layer
// exists in the material but cannot be painted or weighted.
ULandscapeLayerInfoObject* CreateOrLoadLayerInfo(const FString& LayerName)
{
    const FString AssetName = FString(TEXT("LI_")) + LayerName;
    const FString PackageName = FString(kLayerInfoRoot) + TEXT("/") + AssetName;

    if (FPackageName::DoesPackageExist(PackageName))
    {
        if (ULandscapeLayerInfoObject* Existing =
                LoadObject<ULandscapeLayerInfoObject>(nullptr, *(PackageName + TEXT(".") + AssetName)))
        {
            return Existing;
        }
    }

    UPackage* Package = CreatePackage(*PackageName);
    Package->FullyLoad();

    ULandscapeLayerInfoObject* Info = NewObject<ULandscapeLayerInfoObject>(
        Package, *AssetName, RF_Public | RF_Standalone);
    if (Info == nullptr)
    {
        return nullptr;
    }
    Info->LayerName = FName(*LayerName);
    Info->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Info);
    return Info;
}

bool SavePackageFor(UObject* Object)
{
    if (Object == nullptr)
    {
        return false;
    }
    UPackage* Package = Object->GetOutermost();
    const FString FileName =
        FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, nullptr, *FileName, Args);
}

}  // namespace

URA4LayeredTerrainSetupCommandlet::URA4LayeredTerrainSetupCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URA4LayeredTerrainSetupCommandlet::Main(const FString& Params)
{
    // --- textures ------------------------------------------------------------
    UTexture2D* Colour[kLayerCount] = {};
    UTexture2D* Normal[kLayerCount] = {};
    UTexture2D* Rough[kLayerCount] = {};

    for (int32 Index = 0; Index < kLayerCount; ++Index)
    {
        const FLayerSpec& Layer = kLayers[Index];
        Colour[Index] = ImportOrLoadTexture(SourceFileFor(Layer, TEXT("Color")),
                                            FString(TEXT("T_RA4_")) + Layer.AssetPrefix + TEXT("_Color"),
                                            /*bNormalMap*/ false);
        Normal[Index] = ImportOrLoadTexture(SourceFileFor(Layer, TEXT("NormalGL")),
                                            FString(TEXT("T_RA4_")) + Layer.AssetPrefix + TEXT("_Normal"),
                                            /*bNormalMap*/ true);
        Rough[Index] = ImportOrLoadTexture(SourceFileFor(Layer, TEXT("Roughness")),
                                           FString(TEXT("T_RA4_")) + Layer.AssetPrefix + TEXT("_Roughness"),
                                           /*bNormalMap*/ false);

        // Colour is the only map the material cannot do without. A missing normal
        // or roughness degrades the look; a missing base colour means the layer
        // would render untextured, which is worse than failing loudly.
        if (Colour[Index] == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: layer '%s' has no base colour, aborting"),
                   Layer.LayerName);
            return 1;
        }
    }
    UE_LOG(LogTemp, Display, TEXT("RA4LayeredTerrain: %d layer texture sets ready"), kLayerCount);

    // --- layer infos ---------------------------------------------------------
    for (const FLayerSpec& Layer : kLayers)
    {
        ULandscapeLayerInfoObject* Info = CreateOrLoadLayerInfo(Layer.LayerName);
        if (Info == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: could not create layer info for '%s'"),
                   Layer.LayerName);
            return 1;
        }
        SavePackageFor(Info);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4LayeredTerrain: layer info objects saved"));

    // --- material ------------------------------------------------------------
    const FString MaterialName = TEXT("M_RA4_TerrainLayered");
    const FString MaterialPackageName = FString(kGeneratedRoot) + TEXT("/") + MaterialName;

    // Rebuilt from scratch each run rather than patched. Reconciling an existing
    // node graph against a changed layer table is far more error-prone than
    // regenerating a graph this small, and the asset is generated content.
    UPackage* MatPackage = CreatePackage(*MaterialPackageName);
    MatPackage->FullyLoad();
    UMaterial* Material = NewObject<UMaterial>(MatPackage, *MaterialName, RF_Public | RF_Standalone);
    if (Material == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: could not create the material"));
        return 1;
    }

    // Three blends: base colour, normal and roughness each need their own, because
    // a LandscapeLayerBlend carries one value per layer and not a material attribute.
    UMaterialExpressionLandscapeLayerBlend* ColourBlend =
        Cast<UMaterialExpressionLandscapeLayerBlend>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionLandscapeLayerBlend::StaticClass(), -200, -300));
    UMaterialExpressionLandscapeLayerBlend* NormalBlend =
        Cast<UMaterialExpressionLandscapeLayerBlend>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionLandscapeLayerBlend::StaticClass(), -200, 100));
    UMaterialExpressionLandscapeLayerBlend* RoughBlend =
        Cast<UMaterialExpressionLandscapeLayerBlend>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionLandscapeLayerBlend::StaticClass(), -200, 500));

    if (ColourBlend == nullptr || NormalBlend == nullptr || RoughBlend == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: could not create the layer blend nodes"));
        return 1;
    }

    for (int32 Index = 0; Index < kLayerCount; ++Index)
    {
        const FLayerSpec& Layer = kLayers[Index];
        const int32 Row = Index * 260;

        // Each layer gets its own coordinate node so per-layer tiling is possible;
        // sharing one would force rock to repeat as often as grass.
        UMaterialExpressionTextureCoordinate* Coords =
            Cast<UMaterialExpressionTextureCoordinate>(UMaterialEditingLibrary::CreateMaterialExpression(
                Material, UMaterialExpressionTextureCoordinate::StaticClass(), -1200, Row - 400));
        Coords->UTiling = Layer.Tiling;
        Coords->VTiling = Layer.Tiling;

        auto AddSampler = [&](UTexture2D* Texture, int32 OffsetY, bool bNormal)
            -> UMaterialExpressionTextureSample*
        {
            if (Texture == nullptr)
            {
                return nullptr;
            }
            UMaterialExpressionTextureSample* Sample =
                Cast<UMaterialExpressionTextureSample>(UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, UMaterialExpressionTextureSample::StaticClass(), -800, Row + OffsetY - 400));
            Sample->Texture = Texture;
            Sample->SamplerType = bNormal ? SAMPLERTYPE_Normal : SAMPLERTYPE_Color;
            UMaterialEditingLibrary::ConnectMaterialExpressions(Coords, TEXT(""), Sample, TEXT("UVs"));
            return Sample;
        };

        UMaterialExpressionTextureSample* ColourSample = AddSampler(Colour[Index], 0, false);
        UMaterialExpressionTextureSample* NormalSample = AddSampler(Normal[Index], 80, true);
        UMaterialExpressionTextureSample* RoughSample = AddSampler(Rough[Index], 160, false);

        // LB_WeightBlend for every layer, including the first. LB_AlphaBlend would
        // make the dirt an opaque base that later layers cannot fully replace.
        auto AddLayerEntry = [&](UMaterialExpressionLandscapeLayerBlend* Blend,
                                 UMaterialExpressionTextureSample* Sample)
        {
            FLayerBlendInput Input;
            Input.LayerName = FName(Layer.LayerName);
            Input.BlendType = LB_WeightBlend;
            // Preview weight only; the real weights come from the painted
            // weightmaps at runtime. Giving the base layer 1 and the rest 0 makes
            // an unpainted landscape read as dirt rather than black.
            Input.PreviewWeight = (Index == 0) ? 1.0f : 0.0f;
            Blend->Layers.Add(Input);

            const int32 Slot = Blend->Layers.Num() - 1;
            if (Sample != nullptr)
            {
                UMaterialEditingLibrary::ConnectMaterialExpressions(
                    Sample, TEXT(""), Blend, FString::Printf(TEXT("Layer %d"), Slot));
            }
        };

        AddLayerEntry(ColourBlend, ColourSample);
        AddLayerEntry(NormalBlend, NormalSample);
        AddLayerEntry(RoughBlend, RoughSample);
    }

    UMaterialEditingLibrary::ConnectMaterialProperty(ColourBlend, TEXT(""), MP_BaseColor);
    UMaterialEditingLibrary::ConnectMaterialProperty(NormalBlend, TEXT(""), MP_Normal);
    UMaterialEditingLibrary::ConnectMaterialProperty(RoughBlend, TEXT(""), MP_Roughness);

    Material->PostEditChange();
    Material->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Material);
    UMaterialEditingLibrary::RecompileMaterial(Material);

    // Save the textures too: they were imported into memory above and are lost
    // otherwise, leaving a material that references packages which do not exist.
    for (int32 Index = 0; Index < kLayerCount; ++Index)
    {
        SavePackageFor(Colour[Index]);
        SavePackageFor(Normal[Index]);
        SavePackageFor(Rough[Index]);
    }

    if (!SavePackageFor(Material))
    {
        UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: failed to save %s"), *MaterialPackageName);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("RA4LayeredTerrain: %s saved with %d layers"),
           *MaterialPackageName, kLayerCount);
    return 0;
}
