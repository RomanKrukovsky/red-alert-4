// Copyright (c) Red Alert 4 project. Shared scaffolding for simulation tests.
#pragma once

#include "RA4Content/ContentDatabase.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4Test
{

using namespace RA4;

// Content ids the tests drive. Duplicated from DefaultContent.cpp on purpose: if a
// name changes, the tests must fail rather than silently exercise nothing.
namespace Ids
{
constexpr ContentId SovConYard = MakeContentId("building.sov.construction_yard");
constexpr ContentId SovPower = MakeContentId("building.sov.tesla_reactor");
constexpr ContentId SovRefinery = MakeContentId("building.sov.ore_refinery");
constexpr ContentId SovBarracks = MakeContentId("building.sov.barracks");
constexpr ContentId SovWarFactory = MakeContentId("building.sov.war_factory");
constexpr ContentId SovTurret = MakeContentId("building.sov.gun_turret");
constexpr ContentId SovHarvester = MakeContentId("unit.sov.ore_harvester");
constexpr ContentId SovConscript = MakeContentId("unit.sov.conscript");
constexpr ContentId SovHeavyTank = MakeContentId("unit.sov.heavy_tank");

constexpr ContentId AllConYard = MakeContentId("building.all.construction_yard");
constexpr ContentId AllRifleman = MakeContentId("unit.all.rifleman");
constexpr ContentId AllLightTank = MakeContentId("unit.all.light_tank");

constexpr ContentId OreField = MakeContentId("resource.ore_field");
} // namespace Ids

// 64x64 tiles == 128x128 metres. Large enough that the two bases are a real march
// apart, small enough that a full match test runs in milliseconds.
inline MatchSetup MakeTestSetup(uint64_t Seed = 12345)
{
    MatchSetup Setup;
    Setup.Seed = Seed;
    Setup.Map.Name = "test.plains";
    Setup.Map.Resize(64, 64, Tile_GroundPassable);

    Setup.Players[0].bActive = true;
    Setup.Players[0].Faction = FactionId::Soviet;
    Setup.Players[0].StartingCredits = 10000;

    Setup.Players[1].bActive = true;
    Setup.Players[1].Faction = FactionId::Alliance;
    Setup.Players[1].StartingCredits = 10000;

    return Setup;
}

inline Command MakeCommand(CommandType Type, PlayerId Issuer)
{
    Command C;
    C.Type = Type;
    C.Issuer = Issuer;
    return C;
}

// Steps the world N ticks with no player input.
inline void RunTicks(SimWorld& World, int32_t Count)
{
    for (int32_t I = 0; I < Count; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
}

// Steps until Predicate is true or the budget runs out. Returns the tick count
// consumed, or -1 on timeout, so tests can assert on how long something took
// rather than only that it eventually happened.
//
// The predicate is evaluated after each tick, never before the first one: many
// interesting conditions ("has arrived", "queue is empty") are trivially true on a
// unit that has not been given a chance to act yet, and checking first turned those
// assertions into no-ops.
template <typename Predicate>
int32_t RunUntil(SimWorld& World, int32_t MaxTicks, Predicate&& Pred)
{
    for (int32_t I = 0; I < MaxTicks; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        if (Pred())
        {
            return I + 1;
        }
    }
    return -1;
}

// The match ends the moment a player owns nothing at all. Tests that only populate
// one side would therefore finish on tick zero and silently stop simulating, so
// they plant a token enemy base in the far corner instead.
inline EntityId SpawnEnemyOutpost(SimWorld& World, PlayerId Owner = 1)
{
    return World.SpawnBuilding(Ids::AllConYard, Owner, TileCoord(58, 58), true);
}

inline int32_t CountEntities(const SimWorld& World, PlayerId Owner, EntityKind Kind)
{
    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < Cores.size(); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == Owner && Cores[I].Kind == Kind)
        {
            ++Count;
        }
    }
    return Count;
}

inline int32_t CountEntitiesOfType(const SimWorld& World, PlayerId Owner, ContentId Def)
{
    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < Cores.size(); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == Owner && Cores[I].Def == Def)
        {
            ++Count;
        }
    }
    return Count;
}

inline EntityId FindFirstOfType(const SimWorld& World, PlayerId Owner, ContentId Def)
{
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < Cores.size(); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == Owner && Cores[I].Def == Def)
        {
            return World.MakeId(I);
        }
    }
    return EntityId::Invalid();
}

} // namespace RA4Test
