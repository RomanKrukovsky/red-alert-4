// Copyright (c) Red Alert 4 project. AI-versus-AI self-play league.
#include "RA4AI/AILeague.h"

#include <cstdio>

#include "RA4AI/AICommander.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4
{
namespace AI
{

namespace
{

// The same mirrored bootstrap the AI regression tests and the match viewer use:
// two construction yards on opposite corners, a 3x3 ore field near each. Keeping
// all three harnesses on one bootstrap means a league anomaly can be replayed in
// a unit test with nothing but the seed.
const ContentId kSovYard = MakeContentId("building.sov.construction_yard");
const ContentId kAllYard = MakeContentId("building.all.construction_yard");
const ContentId kOreField = MakeContentId("resource.ore_field");

MatchSetup MakeLeagueSetup(uint64_t Seed)
{
    MatchSetup Setup;
    Setup.Seed = Seed;
    Setup.Map.Name = "league.plains";
    Setup.Map.Resize(64, 64, Tile_GroundPassable);
    Setup.Players[0].bActive = true;
    Setup.Players[0].Faction = FactionId::Soviet;
    Setup.Players[0].StartingCredits = 10000;
    Setup.Players[1].bActive = true;
    Setup.Players[1].Faction = FactionId::Alliance;
    Setup.Players[1].StartingCredits = 10000;
    return Setup;
}

// Splits a 64-bit base seed and a match index into a well-spread per-match seed.
// SplitMix64: deterministic, integer-only, and avalanche-complete, so seed 5 and
// seed 6 produce unrelated matches instead of near-identical ones.
uint64_t DeriveMatchSeed(uint64_t BaseSeed, uint32_t MatchIndex)
{
    uint64_t Z = BaseSeed + 0x9E3779B97F4A7C15ULL * (uint64_t(MatchIndex) + 1);
    Z = (Z ^ (Z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    Z = (Z ^ (Z >> 27)) * 0x94D049BB133111EBULL;
    return Z ^ (Z >> 31);
}

const char* ShortName(AIProfile P)
{
    return ToString(P);
}

} // namespace

LeagueMatchRecord AILeague::PlayMatch(AIProfile ProfileA, AIProfile ProfileB,
                                      AIDifficulty Difficulty, uint64_t Seed,
                                      int32_t MaxTicks)
{
    LeagueMatchRecord Record;
    Record.Seed = Seed;
    Record.Profiles[0] = ProfileA;
    Record.Profiles[1] = ProfileB;
    Record.Difficulty = Difficulty;

    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld World;
    World.Initialize(&Content, MakeLeagueSetup(Seed));
    World.SpawnBuilding(kSovYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(kAllYard, 1, TileCoord(48, 48), true);
    for (int32_t X = 0; X < 3; ++X)
    {
        for (int32_t Y = 0; Y < 3; ++Y)
        {
            World.SpawnResourceNode(kOreField, TileCoord(6 + X, 15 + Y), 4000);
            World.SpawnResourceNode(kOreField, TileCoord(53 + X, 43 + Y), 4000);
        }
    }
    World.ClearEvents();

    AICommander Commanders[2];
    Commanders[0].Initialize(0, ProfileA, Seed);
    Commanders[1].Initialize(1, ProfileB, Seed);
    Commanders[0].SetConfig(MakeProfileConfig(ProfileA, Difficulty));
    Commanders[1].SetConfig(MakeProfileConfig(ProfileB, Difficulty));

    int32_t Tick = 0;
    for (; Tick < MaxTicks && World.GetPhase() == MatchPhase::Running; ++Tick)
    {
        CommandFrame Frame;
        Frame.Tick = World.GetTick();
        for (PlayerId P = 0; P < 2; ++P)
        {
            Commanders[P].Tick(World, Frame.Commands);
        }
        // Commanders consume the previous tick's events (opponent modelling);
        // clear only after both have observed them. Same contract as AIMatch.
        World.ClearEvents();
        World.Tick(Frame.Commands.empty() ? nullptr : &Frame);
    }

    Record.DurationTicks = uint32_t(Tick);
    Record.bTimedOut = World.GetPhase() == MatchPhase::Running;
    Record.Winner = Record.bTimedOut ? kInvalidPlayer : World.GetWinner();
    Record.FinalChecksum = World.ComputeStateChecksum();

    for (PlayerId P = 0; P < 2; ++P)
    {
        const PlayerState& State = World.GetPlayer(P);
        Record.TotalHarvested[P] = State.TotalHarvested;
        Record.UnitsBuilt[P] = State.UnitsBuilt;
        Record.UnitsLost[P] = State.UnitsLost;
        Record.BuildingsBuilt[P] = State.BuildingsBuilt;
        Record.BuildingsLost[P] = State.BuildingsLost;
    }
    return Record;
}

LeagueResult AILeague::RunRoundRobin(const LeagueConfig& Config)
{
    LeagueResult Result;

    std::vector<AIProfile> Roster = Config.Roster;
    if (Roster.empty())
    {
        Roster = {AIProfile::Balanced,       AIProfile::Aggressive,
                  AIProfile::Defensive,      AIProfile::Economic,
                  AIProfile::Rush,           AIProfile::Turtle,
                  AIProfile::AirSuperiority, AIProfile::Guerrilla};
    }

    uint32_t MatchIndex = 0;
    for (AIProfile A : Roster)
    {
        for (AIProfile B : Roster)
        {
            if (A == B)
            {
                continue;   // mirrors add cost without ranking signal
            }

            LeaguePairingStats Stats;
            Stats.ProfileA = A;
            Stats.ProfileB = B;

            for (uint32_t I = 0; I < Config.MatchesPerPairing; ++I)
            {
                const uint64_t Seed = DeriveMatchSeed(Config.BaseSeed, MatchIndex);
                LeagueMatchRecord Record = PlayMatch(A, B, Config.Difficulty, Seed,
                                                     Config.MaxTicksPerMatch);
                Record.MatchIndex = MatchIndex;
                ++MatchIndex;

                Stats.Matches += 1;
                Stats.TotalDurationTicks += Record.DurationTicks;
                if (Record.bTimedOut)
                {
                    Stats.TimedOut += 1;
                    Stats.Draws += 1;
                }
                else if (Record.Winner == 0)
                {
                    Stats.WinsA += 1;
                }
                else if (Record.Winner == 1)
                {
                    Stats.WinsB += 1;
                }
                else
                {
                    Stats.Draws += 1;
                }
                Result.Matches.push_back(Record);
            }
            Result.Pairings.push_back(Stats);
        }
    }
    return Result;
}

const LeaguePairingStats* LeagueResult::FindPairing(AIProfile A, AIProfile B) const
{
    for (const LeaguePairingStats& P : Pairings)
    {
        if (P.ProfileA == A && P.ProfileB == B)
        {
            return &P;
        }
    }
    return nullptr;
}

std::string LeagueResult::FormatTable() const
{
    std::string Out;
    char Line[192];
    std::snprintf(Line, sizeof(Line), "%-16s %-16s %7s %6s %6s %6s %8s %10s\n",
                  "profile A", "profile B", "matches", "winsA", "winsB", "draws",
                  "A win%", "avg ticks");
    Out += Line;
    for (const LeaguePairingStats& P : Pairings)
    {
        std::snprintf(Line, sizeof(Line), "%-16s %-16s %7u %6u %6u %6u %7d%% %10u\n",
                      ShortName(P.ProfileA), ShortName(P.ProfileB), P.Matches,
                      P.WinsA, P.WinsB, P.Draws, P.WinRatePercentA(),
                      P.AverageDurationTicks());
        Out += Line;
    }
    return Out;
}

} // namespace AI
} // namespace RA4
