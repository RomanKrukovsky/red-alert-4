// Copyright (c) Red Alert 4 project. AI vs AI Self-Play Tournament League implementation.
#include "RA4AI/AISelfPlayLeague.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Simulation/SimWorld.h"

#include <cmath>
#include <algorithm>

namespace RA4
{
namespace AI
{

namespace
{
const ContentId kSovYard = MakeContentId("building.sov.construction_yard");
const ContentId kAllYard = MakeContentId("building.all.construction_yard");
const ContentId kOreField = MakeContentId("resource.ore_field");

MatchSetup MakeTournamentMatchSetup(uint64_t Seed, bool bSwapFactions)
{
    MatchSetup Setup;
    Setup.Seed = Seed;
    Setup.Map.Name = "tournament.arena";
    Setup.Map.Resize(64, 64, Tile_GroundPassable);

    Setup.Players[0].bActive = true;
    Setup.Players[0].Faction = bSwapFactions ? FactionId::Alliance : FactionId::Soviet;
    Setup.Players[0].StartingCredits = 10000;

    Setup.Players[1].bActive = true;
    Setup.Players[1].Faction = bSwapFactions ? FactionId::Soviet : FactionId::Alliance;
    Setup.Players[1].StartingCredits = 10000;

    return Setup;
}
} // namespace

void AISelfPlayLeague::UpdateElo(float& InOutEloA, float& InOutEloB, float ScoreA, float KFactor)
{
    // Standard Elo expected outcome: E_A = 1 / (1 + 10^((R_B - R_A) / 400))
    const float ExpectedA = 1.0f / (1.0f + std::pow(10.0f, (InOutEloB - InOutEloA) / 400.0f));
    const float ExpectedB = 1.0f - ExpectedA;
    const float ScoreB = 1.0f - ScoreA;

    InOutEloA += KFactor * (ScoreA - ExpectedA);
    InOutEloB += KFactor * (ScoreB - ExpectedB);
}

LeagueSummary AISelfPlayLeague::RunTournament(uint32_t MatchCount, AIProfile ProfileA, AIProfile ProfileB,
                                             uint64_t BaseSeed, uint32_t MaxTicksPerMatch)
{
    LeagueSummary Summary;
    Summary.TotalMatchesRun = MatchCount;
    Summary.EloRatingP0 = 1500.0f;
    Summary.EloRatingP1 = 1500.0f;

    if (MatchCount == 0)
    {
        return Summary;
    }

    ContentDatabase Content;
    BuildDefaultContent(Content);

    uint64_t TotalDurationTicks = 0;

    for (uint32_t M = 0; M < MatchCount; ++M)
    {
        const uint64_t Seed = BaseSeed + M * 10007ull;
        const bool bSwap = (M % 2 == 1);

        MatchSetup Setup = MakeTournamentMatchSetup(Seed, bSwap);

        SimWorld World;
        World.Initialize(&Content, Setup);

        World.SpawnBuilding(bSwap ? kAllYard : kSovYard, 0, TileCoord(10, 10), true);
        // Point-mirror of player 0's base about the map centre, matching the league
        // and viewer bootstraps so neither start spot is structurally advantaged.
        World.SpawnBuilding(bSwap ? kSovYard : kAllYard, 1, TileCoord(53, 53), true);

        for (int32_t X = 0; X < 3; ++X)
        {
            for (int32_t Y = 0; Y < 3; ++Y)
            {
                World.SpawnResourceNode(kOreField, TileCoord(6 + X, 15 + Y), 4000);
                World.SpawnResourceNode(kOreField, TileCoord(57 - X, 48 - Y), 4000);
            }
        }
        World.ClearEvents();

        AICommander Cmd0;
        AICommander Cmd1;
        Cmd0.Initialize(0, ProfileA, Seed);
        Cmd1.Initialize(1, ProfileB, Seed ^ 0xFEEDFACEull);

        uint32_t Tick = 0;
        for (; Tick < MaxTicksPerMatch && World.GetPhase() == MatchPhase::Running; ++Tick)
        {
            CommandFrame Frame;
            Frame.Tick = World.GetTick();

            // Alternate the deciding order each tick: a fixed order hands one slot a
            // permanent first-strike advantage at every engagement boundary.
            const bool bFirst = (Tick % 2) == 0;
            if (bFirst)
            {
                Cmd0.Tick(World, Frame.Commands);
                Cmd1.Tick(World, Frame.Commands);
            }
            else
            {
                Cmd1.Tick(World, Frame.Commands);
                Cmd0.Tick(World, Frame.Commands);
            }

            World.ClearEvents();
            World.Tick(Frame.Commands.empty() ? nullptr : &Frame);
        }

        MatchRecord Rec;
        Rec.MatchId = M + 1;
        Rec.ProfilePlayer0 = ProfileA;
        Rec.ProfilePlayer1 = ProfileB;
        Rec.DurationTicks = Tick;
        Rec.TotalHarvestedP0 = World.GetPlayer(0).TotalHarvested;
        Rec.TotalHarvestedP1 = World.GetPlayer(1).TotalHarvested;
        Rec.FinalStateChecksum = World.ComputeStateChecksum();

        TotalDurationTicks += Tick;

        if (World.GetPhase() == MatchPhase::Finished)
        {
            Rec.Winner = World.GetWinner();
            if (Rec.Winner == 0)
            {
                Summary.Player0Wins++;
                UpdateElo(Summary.EloRatingP0, Summary.EloRatingP1, 1.0f);
            }
            else if (Rec.Winner == 1)
            {
                Summary.Player1Wins++;
                UpdateElo(Summary.EloRatingP0, Summary.EloRatingP1, 0.0f);
            }
            else
            {
                Summary.Draws++;
                UpdateElo(Summary.EloRatingP0, Summary.EloRatingP1, 0.5f);
            }
        }
        else
        {
            Rec.Winner = kInvalidPlayer;
            Summary.Draws++;
            UpdateElo(Summary.EloRatingP0, Summary.EloRatingP1, 0.5f);
        }

        Summary.Matches.push_back(Rec);
    }

    Summary.AverageDurationSeconds = static_cast<float>(TotalDurationTicks) / (static_cast<float>(MatchCount) * 20.0f);
    return Summary;
}

} // namespace AI
} // namespace RA4
