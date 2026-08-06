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
// Fog nodes (ADR-0028). These live here rather than being hand-wired in the
// editor because this commandlet rebuilds the material from scratch on every run
// -- an editor-side edit would be silently reverted the next time it runs, which
// is exactly how the ground-tiling fix was lost before.
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionDesaturation.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionWorldPosition.h"
// Post-process fog (ADR-0028 / V-7): needed so props, water and buildings are
// fogged too. The landscape material only tints the ground, which leaves lit
// objects floating over unexplored black -- worse than no fog, because it points
// at exactly what the player is not supposed to know is there.
#include "Materials/MaterialExpressionSceneTexture.h"
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

    // --- fog of war (ADR-0028) ----------------------------------------------
    // The layer blend result is tinted by the local player's visibility before it
    // reaches BaseColor. Two channels carry the fog, never one: brightness AND
    // desaturation, because UI_UX_BIBLE section 1.1 forbids conveying state by
    // brightness or colour alone -- a player who cannot see the dimming still
    // sees the colour drain.
    //
    // The fog texture is one texel per tile, sampled bilinearly, so the value
    // arrives as a continuous 0..1 ramp with the four VisibilityStates as
    // anchors (0 unexplored, 1/3 remembered, 2/3 radar, 1 visible). The material
    // therefore does arithmetic on it rather than comparing it to an enum.
    UMaterialExpressionWorldPosition* WorldPos =
        Cast<UMaterialExpressionWorldPosition>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionWorldPosition::StaticClass(), -1600, 1100));

    // Map world XY into fog UV by dividing by the map extent, which the subsystem
    // publishes. Parameter names are the contract with
    // URA4SimWorldSubsystem::PublishFogParametersToTerrain -- if they drift, Unreal
    // silently ignores the sets and fog never appears.
    UMaterialExpressionScalarParameter* FogWidth =
        Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionScalarParameter::StaticClass(), -1600, 1250));
    FogWidth->ParameterName = TEXT("RA4FogWorldWidth");
    FogWidth->DefaultValue = 12800.0f;   // 64 tiles * 200 units, the default test map

    // Height as well as width: a non-square map divided by width alone stretches
    // the fog on one axis, and the subsystem publishes both. Verified the hard
    // way -- a parameter the code sets but the material lacks is silently
    // ignored by Unreal, so fog strength and contrast did nothing at all until
    // this was checked with MaterialEditingLibrary.get_scalar_parameter_names.
    UMaterialExpressionScalarParameter* FogHeight =
        Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionScalarParameter::StaticClass(), -1600, 1400));

    // Accessibility parameters from ADR-0028 section 4. Defaults match the
    // intended look so a material used without the subsystem still renders
    // correct fog rather than none.
    UMaterialExpressionScalarParameter* FogStrengthParam =
        Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionScalarParameter::StaticClass(), -1600, 1550));
    UMaterialExpressionScalarParameter* FogContrastParam =
        Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionScalarParameter::StaticClass(), -1600, 1700));

    // Fog UV needs both axes, so the divide takes a two-component B built from
    // the width and height parameters.
    UMaterialExpressionAppendVector* FogExtent =
        Cast<UMaterialExpressionAppendVector>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionAppendVector::StaticClass(), -1450, 1300));

    UMaterialExpressionDivide* FogUV =
        Cast<UMaterialExpressionDivide>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionDivide::StaticClass(), -1350, 1150));

    // High contrast turns the smooth ramp into a hard edge: visibility is pushed
    // toward 0 or 1 around the midpoint. Implemented as a lerp between the raw
    // sample and a stepped version so one parameter blends between the two modes
    // rather than needing a second material.
    UMaterialExpressionMultiply* ContrastBoost =
        Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionMultiply::StaticClass(), -950, 1700));
    UMaterialExpressionConstant* ContrastGain =
        Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionConstant::StaticClass(), -1100, 1800));
    UMaterialExpressionLinearInterpolate* ContrastMix =
        Cast<UMaterialExpressionLinearInterpolate>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionLinearInterpolate::StaticClass(), -800, 1750));
    // Strength scales how far brightness is allowed to fall: at strength 1 fog
    // reaches the floor, at the clamped minimum it stays much brighter. The
    // floor itself is enforced in code (kMinFogStrength), not here.
    UMaterialExpressionLinearInterpolate* StrengthMix =
        Cast<UMaterialExpressionLinearInterpolate>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionLinearInterpolate::StaticClass(), -700, 1500));

    UMaterialExpressionTextureSampleParameter2D* FogSample =
        Cast<UMaterialExpressionTextureSampleParameter2D>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -1100, 1150));

    // Floor on darkness: unexplored ground is very dark but not pure black, so a
    // player can still read terrain silhouette enough to navigate the camera.
    // ADR-0028 forbids the opposite extreme -- fog strength may not be lowered to
    // where unexplored and visible are indistinguishable -- so this is a constant
    // in generated content, not a user setting.
    UMaterialExpressionConstant* FogFloor =
        Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionConstant::StaticClass(), -1100, 1400));
    FogFloor->R = 0.08f;

    UMaterialExpressionLinearInterpolate* FogBrightness =
        Cast<UMaterialExpressionLinearInterpolate>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionLinearInterpolate::StaticClass(), -850, 1250));

    UMaterialExpressionConstant* FullBright =
        Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionConstant::StaticClass(), -1100, 1550));
    FullBright->R = 1.0f;

    // Desaturation is driven by the inverse of visibility: fully seen ground keeps
    // its colour, remembered ground drains toward grey.
    UMaterialExpressionOneMinus* FogInverse =
        Cast<UMaterialExpressionOneMinus>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionOneMinus::StaticClass(), -850, 1000));

    UMaterialExpressionDesaturation* FogDesat =
        Cast<UMaterialExpressionDesaturation>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionDesaturation::StaticClass(), -600, 900));

    UMaterialExpressionMultiply* FogTint =
        Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionMultiply::StaticClass(), -350, 1000));

    const bool bFogNodesCreated = WorldPos != nullptr && FogWidth != nullptr && FogUV != nullptr &&
                                  FogSample != nullptr && FogFloor != nullptr &&
                                  FogBrightness != nullptr && FullBright != nullptr &&
                                  FogInverse != nullptr && FogDesat != nullptr && FogTint != nullptr &&
                                  FogHeight != nullptr && FogStrengthParam != nullptr &&
                                  FogContrastParam != nullptr && FogExtent != nullptr &&
                                  ContrastBoost != nullptr && ContrastGain != nullptr &&
                                  ContrastMix != nullptr && StrengthMix != nullptr;

    if (bFogNodesCreated)
    {
        FogSample->ParameterName = TEXT("RA4FogVisibility");
        FogSample->SamplerType = SAMPLERTYPE_LinearGrayscale;   // data, not sRGB colour

        FogHeight->ParameterName = TEXT("RA4FogWorldHeight");
        FogHeight->DefaultValue = 12800.0f;
        FogStrengthParam->ParameterName = TEXT("RA4FogStrength");
        FogStrengthParam->DefaultValue = 1.0f;
        FogContrastParam->ParameterName = TEXT("RA4FogHighContrast");
        FogContrastParam->DefaultValue = 0.0f;
        ContrastGain->R = 4.0f;   // steepens the ramp around the midpoint

        // UV = worldXY / (width, height)
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogWidth, TEXT(""), FogExtent, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogHeight, TEXT(""), FogExtent, TEXT("B"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(WorldPos, TEXT(""), FogUV, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogExtent, TEXT(""), FogUV, TEXT("B"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogUV, TEXT(""), FogSample, TEXT("UVs"));

        // High contrast: steepen the sample, then blend between raw and steepened
        // by the contrast parameter, so one graph serves both modes.
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogSample, TEXT("R"), ContrastBoost, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(ContrastGain, TEXT(""), ContrastBoost, TEXT("B"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogSample, TEXT("R"), ContrastMix, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(ContrastBoost, TEXT(""), ContrastMix, TEXT("B"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogContrastParam, TEXT(""), ContrastMix, TEXT("Alpha"));

        // brightness = lerp(floor, 1, visibility), then strength decides how much
        // of that darkening is applied: lerp(1, brightness, strength).
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogFloor, TEXT(""), FogBrightness, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FullBright, TEXT(""), FogBrightness, TEXT("B"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(ContrastMix, TEXT(""), FogBrightness, TEXT("Alpha"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FullBright, TEXT(""), StrengthMix, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogBrightness, TEXT(""), StrengthMix, TEXT("B"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogStrengthParam, TEXT(""), StrengthMix, TEXT("Alpha"));

        // desaturate(colour, 1 - visibility), then multiply by brightness
        UMaterialEditingLibrary::ConnectMaterialExpressions(ContrastMix, TEXT(""), FogInverse, TEXT(""));
        UMaterialEditingLibrary::ConnectMaterialExpressions(ColourBlend, TEXT(""), FogDesat, TEXT(""));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogInverse, TEXT(""), FogDesat, TEXT("Fraction"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(FogDesat, TEXT(""), FogTint, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(StrengthMix, TEXT(""), FogTint, TEXT("B"));

        UMaterialEditingLibrary::ConnectMaterialProperty(FogTint, TEXT(""), MP_BaseColor);
        UE_LOG(LogTemp, Display, TEXT("RA4LayeredTerrain: fog-of-war nodes wired into BaseColor"));
    }
    else
    {
        // Fail loud but still produce a usable material: terrain without fog is a
        // visible bug, terrain that does not compile is a broken level.
        UE_LOG(LogTemp, Error,
               TEXT("RA4LayeredTerrain: could not create the fog nodes -- material generated WITHOUT fog"));
        UMaterialEditingLibrary::ConnectMaterialProperty(ColourBlend, TEXT(""), MP_BaseColor);
    }

    UMaterialEditingLibrary::ConnectMaterialProperty(NormalBlend, TEXT(""), MP_Normal);
    UMaterialEditingLibrary::ConnectMaterialProperty(RoughBlend, TEXT(""), MP_Roughness);

    Material->PostEditChange();
    Material->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(Material);
    UMaterialEditingLibrary::RecompileMaterial(Material);

    // --- post-process fog (ADR-0028 / V-7) -----------------------------------
    // A second material, applied to the camera, fogs EVERYTHING the landscape
    // material cannot reach: props, water, buildings, particles. It reconstructs
    // the same fog UV from world position, so both surfaces read the same texture
    // and cannot disagree about where the boundary is.
    //
    // Generated here rather than authored by hand for the same reason as the
    // terrain nodes: this commandlet is the only writer of generated art, and an
    // editor-side asset would drift from the terrain material it must match.
    {
        const FString FogPPName = TEXT("M_RA4_FogPostProcess");
        const FString FogPPPackageName = FString(kGeneratedRoot) + TEXT("/") + FogPPName;
        UPackage* FogPPPackage = CreatePackage(*FogPPPackageName);
        FogPPPackage->FullyLoad();
        UMaterial* FogPP =
            NewObject<UMaterial>(FogPPPackage, *FogPPName, RF_Public | RF_Standalone);
        if (FogPP == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4LayeredTerrain: could not create the fog post-process material"));
        }
        else
        {
            FogPP->MaterialDomain = MD_PostProcess;
            // BeforeTonemapping: fog is part of the scene, so it must be graded
            // with the scene. Applied after tonemapping it would sit on top of
            // colour grading and read as a UI overlay rather than as distance.
            FogPP->BlendableLocation = BL_SceneColorBeforeDOF;

            UMaterialExpressionSceneTexture* SceneColour =
                Cast<UMaterialExpressionSceneTexture>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionSceneTexture::StaticClass(), -900, -100));
            // AbsoluteWorldPosition in a post-process material reads the depth
            // buffer, so every pixel -- ground, prop or unit -- resolves to the
            // world position actually visible there. That is what lets one fog
            // texture cover geometry the landscape material never sees.
            UMaterialExpressionWorldPosition* PPWorldPos =
                Cast<UMaterialExpressionWorldPosition>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionWorldPosition::StaticClass(), -1400, 200));
            UMaterialExpressionScalarParameter* PPFogWidth =
                Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionScalarParameter::StaticClass(), -1400, 350));
            UMaterialExpressionScalarParameter* PPFogHeight =
                Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionScalarParameter::StaticClass(), -1400, 500));
            UMaterialExpressionScalarParameter* PPStrength =
                Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionScalarParameter::StaticClass(), -1400, 650));
            UMaterialExpressionScalarParameter* PPContrast =
                Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionScalarParameter::StaticClass(), -1400, 800));
            UMaterialExpressionAppendVector* PPExtent =
                Cast<UMaterialExpressionAppendVector>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionAppendVector::StaticClass(), -1250, 420));
            UMaterialExpressionMultiply* PPContrastBoost =
                Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionMultiply::StaticClass(), -750, 800));
            UMaterialExpressionConstant* PPContrastGain =
                Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionConstant::StaticClass(), -900, 900));
            UMaterialExpressionLinearInterpolate* PPContrastMix =
                Cast<UMaterialExpressionLinearInterpolate>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionLinearInterpolate::StaticClass(), -600, 850));
            UMaterialExpressionLinearInterpolate* PPStrengthMix =
                Cast<UMaterialExpressionLinearInterpolate>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionLinearInterpolate::StaticClass(), -500, 600));

            UMaterialExpressionDivide* PPFogUV =
                Cast<UMaterialExpressionDivide>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionDivide::StaticClass(), -1150, 250));
            UMaterialExpressionTextureSampleParameter2D* PPFogSample =
                Cast<UMaterialExpressionTextureSampleParameter2D>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -900, 250));
            UMaterialExpressionConstant* PPFloor =
                Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionConstant::StaticClass(), -900, 500));
            UMaterialExpressionConstant* PPFull =
                Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionConstant::StaticClass(), -900, 620));
            UMaterialExpressionLinearInterpolate* PPBrightness =
                Cast<UMaterialExpressionLinearInterpolate>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionLinearInterpolate::StaticClass(), -650, 400));
            UMaterialExpressionOneMinus* PPInverse =
                Cast<UMaterialExpressionOneMinus>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionOneMinus::StaticClass(), -650, 150));
            UMaterialExpressionDesaturation* PPDesat =
                Cast<UMaterialExpressionDesaturation>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionDesaturation::StaticClass(), -400, 0));
            UMaterialExpressionMultiply* PPTint =
                Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(
                    FogPP, UMaterialExpressionMultiply::StaticClass(), -150, 200));

            const bool bPPCreated = SceneColour != nullptr && PPWorldPos != nullptr &&
                                    PPFogWidth != nullptr && PPFogUV != nullptr &&
                                    PPFogSample != nullptr && PPFloor != nullptr &&
                                    PPFull != nullptr && PPBrightness != nullptr &&
                                    PPInverse != nullptr && PPDesat != nullptr && PPTint != nullptr &&
                                    PPFogHeight != nullptr && PPStrength != nullptr &&
                                    PPContrast != nullptr && PPExtent != nullptr &&
                                    PPContrastBoost != nullptr && PPContrastGain != nullptr &&
                                    PPContrastMix != nullptr && PPStrengthMix != nullptr;
            if (bPPCreated)
            {
                SceneColour->SceneTextureId = PPI_PostProcessInput0;
                // Same parameter names as the terrain material: the subsystem sets
                // both from one place, so a rename breaks both loudly instead of
                // leaving one surface silently unfogged.
                PPFogWidth->ParameterName = TEXT("RA4FogWorldWidth");
                PPFogWidth->DefaultValue = 12800.0f;
                PPFogSample->ParameterName = TEXT("RA4FogVisibility");
                PPFogSample->SamplerType = SAMPLERTYPE_LinearGrayscale;
                // Same floor as the terrain: if the two differed, the boundary
                // would be visible as a step between ground and props.
                PPFloor->R = 0.08f;
                PPFull->R = 1.0f;

                PPFogHeight->ParameterName = TEXT("RA4FogWorldHeight");
                PPFogHeight->DefaultValue = 12800.0f;
                PPStrength->ParameterName = TEXT("RA4FogStrength");
                PPStrength->DefaultValue = 1.0f;
                PPContrast->ParameterName = TEXT("RA4FogHighContrast");
                PPContrast->DefaultValue = 0.0f;
                PPContrastGain->R = 4.0f;

                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFogWidth, TEXT(""), PPExtent, TEXT("A"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFogHeight, TEXT(""), PPExtent, TEXT("B"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPWorldPos, TEXT(""), PPFogUV, TEXT("A"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPExtent, TEXT(""), PPFogUV, TEXT("B"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFogUV, TEXT(""), PPFogSample, TEXT("UVs"));

                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFogSample, TEXT("R"), PPContrastBoost, TEXT("A"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPContrastGain, TEXT(""), PPContrastBoost, TEXT("B"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFogSample, TEXT("R"), PPContrastMix, TEXT("A"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPContrastBoost, TEXT(""), PPContrastMix, TEXT("B"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPContrast, TEXT(""), PPContrastMix, TEXT("Alpha"));

                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFloor, TEXT(""), PPBrightness, TEXT("A"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFull, TEXT(""), PPBrightness, TEXT("B"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPContrastMix, TEXT(""), PPBrightness, TEXT("Alpha"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPFull, TEXT(""), PPStrengthMix, TEXT("A"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPBrightness, TEXT(""), PPStrengthMix, TEXT("B"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPStrength, TEXT(""), PPStrengthMix, TEXT("Alpha"));

                UMaterialEditingLibrary::ConnectMaterialExpressions(PPContrastMix, TEXT(""), PPInverse, TEXT(""));
                UMaterialEditingLibrary::ConnectMaterialExpressions(SceneColour, TEXT(""), PPDesat, TEXT(""));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPInverse, TEXT(""), PPDesat, TEXT("Fraction"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPDesat, TEXT(""), PPTint, TEXT("A"));
                UMaterialEditingLibrary::ConnectMaterialExpressions(PPStrengthMix, TEXT(""), PPTint, TEXT("B"));

                UMaterialEditingLibrary::ConnectMaterialProperty(PPTint, TEXT(""), MP_EmissiveColor);

                FogPP->PostEditChange();
                FogPP->MarkPackageDirty();
                FAssetRegistryModule::AssetCreated(FogPP);
                UMaterialEditingLibrary::RecompileMaterial(FogPP);
                SavePackageFor(FogPP);
                UE_LOG(LogTemp, Display,
                       TEXT("RA4LayeredTerrain: fog post-process material generated at %s"),
                       *FogPPPackageName);
            }
            else
            {
                UE_LOG(LogTemp, Error,
                       TEXT("RA4LayeredTerrain: could not create the fog post-process nodes -- props will not be fogged"));
            }
        }
    }

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
