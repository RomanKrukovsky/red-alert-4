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

struct REDALERT4_API FRA4MatchBootstrap
{
    // Fills Content, initialises World, and seeds two opposing bases plus an ore
    // field for each. Content must outlive World: the simulation holds a raw
    // pointer to it for the whole match. NumAIPlayers seeds additional opposing
    // bases (slots 1..NumAIPlayers); defaults keep the classic 1v1 unchanged.
    // ReconSettings may be null (classic perfect information). The pointer must
    // outlive the match: SimWorld keeps borrowing it for Restart().
    static void BuildSkirmish(RA4::ContentDatabase& Content, RA4::SimWorld& World, uint64 Seed,
                              RA4::FactionId PlayerFaction = RA4::FactionId::Soviet,
                              RA4::FactionId EnemyFaction = RA4::FactionId::Alliance,
                              int32 NumAIPlayers = 1, int32 AISpot = -1,
                              const RA4::Recon::ReconSettings* ReconSettings = nullptr);
};
