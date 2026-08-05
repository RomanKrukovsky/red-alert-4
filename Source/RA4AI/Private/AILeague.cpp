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

MatchSetup MakeLeagueSetup(uint64_t Seed, bool bSwapFactions)
{
    MatchSetup Setup;
    Setup.Seed = Seed;
    Setup.Map.Name = "league.plains";
    Setup.Map.Resize(64, 64, Tile_GroundPassable);
    Setup.Players[0].bActive = true;
    Setup.Players[0].Faction = bSwapFactions ? FactionId::Alliance : FactionId::Soviet;
    Setup.Players[0].StartingCredits = 10000;
    Setup.Players[1].bActive = true;
    Setup.Players[1].Faction = bSwapFactions ? FactionId::Soviet : FactionId::Alliance;
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

// Folds one tick's events into the record. Event semantics (from SimWorld):
//   DamageApplied:  Entity=victim, Other=source, Player=ATTACKER, Value=damage
//   EntityDestroyed: Entity=victim, Other=killer, Player=VICTIM owner, Content=def
// The victim entity is still readable during its destruction tick (destruction is
// deferred), so kind/role lookups here are safe.
void AccumulateCombatTelemetry(const SimWorld& World, LeagueMatchRecord& Record)
{
    const ContentDatabase* Content = World.GetContent();

    // Deferred destruction erases the killer: SystemDeaths destroys with
    // Killer=Invalid, so EntityDestroyed.Other is Invalid for every combat death
    // too (verified in SimWorld.cpp:2781). But the fatal DamageApplied and the
    // EntityDestroyed always land in the SAME tick's batch, so a victim that took
    // enemy damage in this batch and then died in this batch died in combat.
    // First pass: who was hit by an enemy this tick.
    std::vector<uint64_t> DamagedByEnemy;
    for (const SimEvent& Ev : World.GetEvents())
    {
        if (Ev.Type == SimEventType::DamageApplied && Ev.Player <= 1)
        {
            DamagedByEnemy.push_back(
                (uint64_t(Ev.Entity.Index) << 32) | Ev.Entity.Generation);
        }
    }
    auto WasHitThisTick = [&DamagedByEnemy](const EntityId& Id)
    {
        const uint64_t Key = (uint64_t(Id.Index) << 32) | Id.Generation;
        for (uint64_t K : DamagedByEnemy)
        {
            if (K == Key)
            {
                return true;
            }
        }
        return false;
    };

    for (const SimEvent& Ev : World.GetEvents())
    {
        if (Ev.Type == SimEventType::DamageApplied)
        {
            if (Ev.Player > 1)
            {
                continue;   // neutral / invalid attacker
            }
            Record.DamageDealt[Ev.Player] += Ev.Value;
            const EntityCore* Victim = World.GetCore(Ev.Entity);
            if (Victim != nullptr && Victim->Kind == EntityKind::Building)
            {
                Record.DamageToBuildings[Ev.Player] += Ev.Value;
            }
            if (Record.FirstBloodTick == 0)
            {
                const EntityCore* V = World.GetCore(Ev.Entity);
                // Only combat between the two players counts as first blood.
                if (V != nullptr && V->Owner <= 1 && V->Owner != Ev.Player)
                {
                    Record.FirstBloodTick = uint32_t(Ev.Tick);
                    Record.FirstBloodBy = Ev.Player;
                }
            }
        }
        else if (Ev.Type == SimEventType::EntityDestroyed)
        {
            const PlayerId VictimOwner = Ev.Player;
            if (VictimOwner > 1)
            {
                continue;   // projectiles, ore nodes, neutrals
            }
            // A combat kill is a death preceded by combat damage in the same
            // batch. Sold buildings and exhausted ore nodes die without a
            // DamageApplied, so they are excluded; crediting the enemy for our
            // own demolitions would corrupt the aggression metrics.
            if (!Ev.Other.IsValid() && !WasHitThisTick(Ev.Entity))
            {
                continue;
            }
            Record.KillsByPlayer[1 - VictimOwner] += 1;
            if (Content != nullptr)
            {
                const EntityDef* Def = Content->FindEntity(Ev.Content);
                if (Def != nullptr)
                {
                    if (Def->Kind == EntityKind::Unit && Def->Unit.bIsHarvester)
                    {
                        Record.HarvestersLost[VictimOwner] += 1;
                    }
                    if (Def->Kind == EntityKind::Building && Def->Weapon.IsValid())
                    {
                        Record.DefencesLost[VictimOwner] += 1;
                    }
                }
            }
        }
    }
}

} // namespace

LeagueMatchRecord AILeague::PlayMatch(AIProfile ProfileA, AIProfile ProfileB,
                                      AIDifficulty Difficulty, uint64_t Seed,
                                      int32_t MaxTicks, bool bSwapFactions)
{
    LeagueMatchRecord Record;
    Record.Seed = Seed;
    Record.Profiles[0] = ProfileA;
    Record.Profiles[1] = ProfileB;
    Record.Difficulty = Difficulty;
    Record.bSwappedFactions = bSwapFactions;

    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld World;
    World.Initialize(&Content, MakeLeagueSetup(Seed, bSwapFactions));
    World.SpawnBuilding(bSwapFactions ? kAllYard : kSovYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(bSwapFactions ? kSovYard : kAllYard, 1, TileCoord(48, 48), true);
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
        // telemetry reads the same batch, then it is cleared. Same contract as
        // AIMatch, with the league as one more read-only observer.
        AccumulateCombatTelemetry(World, Record);
        World.ClearEvents();
        World.Tick(Frame.Commands.empty() ? nullptr : &Frame);
    }

    // The final tick's events (the killing blow, MatchEnded) are emitted inside
    // the last World.Tick, after the in-loop accumulation ran -- fold them in now
    // or every match would under-report exactly its decisive moment.
    AccumulateCombatTelemetry(World, Record);

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
                // Odd repeats swap factions so half of every pairing is played
                // from each side of the faction matchup.
                const bool bSwap = Config.bAlternateFactions && (I % 2u) == 1u;
                LeagueMatchRecord Record = PlayMatch(A, B, Config.Difficulty, Seed,
                                                     Config.MaxTicksPerMatch, bSwap);
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
