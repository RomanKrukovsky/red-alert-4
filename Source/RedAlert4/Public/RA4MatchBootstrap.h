// Copyright (c) Red Alert 4 project. Temporary skirmish seeding.
//
// Until the lobby exists, something has to decide the map, the factions and the
// starting bases. Keeping that here rather than inside the subsystem means the
// lobby can replace one function call later instead of unpicking initialisation.
#pragma once

#include "RA4Recon/ReconConfig.h"

#include "CoreMinimal.h"
#include "RA4Content/ContentTypes.h"

namespace RA4
{
class ContentDatabase;
class SimWorld;
}

struct FRA4SkirmishSlotConfig
{
    bool bActive = false;
    RA4::FactionId Faction = RA4::FactionId::Soviet;
    uint8 Team = 0;
    int32 StartSpot = 0;
};

struct REDALERT4_API FRA4MatchBootstrap
{
    // Fills Content, initialises World, and seeds every active lobby slot.
    // ReconSettings may be null (classic perfect information). The pointer must
    // outlive the match: SimWorld keeps borrowing it for Restart().
    static void BuildSkirmish(RA4::ContentDatabase& Content, RA4::SimWorld& World, uint64 Seed,
                              const TArray<FRA4SkirmishSlotConfig>& PlayerSlots,
                              int32 StartingCredits,
                              const RA4::Recon::ReconSettings* ReconSettings = nullptr);
};
