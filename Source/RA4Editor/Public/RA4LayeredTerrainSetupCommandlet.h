// Copyright (c) Red Alert 4 project. Builds a four-layer paintable landscape material
// from CC0 ambientCG texture sets.
//
// RA4TerrainSetupCommandlet imports a single set (Ground039, dry gravel) and wires it
// straight into BaseColor/Normal/Roughness, so the whole 12800-unit map wears one
// texture: no beaches on the shorelines, no grass on the islands, no rock on the
// cliffs. The archipelago needs surfaces an artist can paint, which means a
// LandscapeLayerBlend plus a LandscapeLayerInfoObject per layer.
//
// Additive rather than a rewrite: M_RA4_Terrain is left untouched and this produces
// M_RA4_TerrainLayered beside it, so the existing single-texture path keeps working
// and the two can be compared.
//
// Layers, in blend order: Dirt (base), Sand, Grass, Rock.
//
// Run with: UnrealEditor-Cmd RedAlert4.uproject -run=RA4LayeredTerrainSetup
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RA4LayeredTerrainSetupCommandlet.generated.h"

UCLASS()
class RA4EDITOR_API URA4LayeredTerrainSetupCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URA4LayeredTerrainSetupCommandlet();

    virtual int32 Main(const FString& Params) override;
};
