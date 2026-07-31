// Copyright (c) Red Alert 4 project. Data-driven mapping implementation.

#include "RA4Presentation/RA4ArtMapping.h"

bool URA4ArtMappingDataAsset::FindUnitArt(FName UnitId, FRA4UnitArtDefinition& OutDefinition) const
{
    if (const FRA4UnitArtDefinition* Found = Units.Find(UnitId))
    {
        OutDefinition = *Found;
        return true;
    }
    return false;
}

bool URA4ArtMappingDataAsset::FindBuildingArt(FName BuildingId, FRA4BuildingArtDefinition& OutDefinition) const
{
    if (const FRA4BuildingArtDefinition* Found = Buildings.Find(BuildingId))
    {
        OutDefinition = *Found;
        return true;
    }
    return false;
}
