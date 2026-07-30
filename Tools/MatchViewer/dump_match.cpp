// Copyright (c) Red Alert 4 project.
//
// Plays a full AI-versus-AI match headlessly and writes the whole thing out as JSON,
// one snapshot every few ticks. Feeds Tools/MatchViewer/render.py, which turns it
// into a scrubbable page.
//
// This exists because the simulation is the part of the game that already works, and
// until the editor renders it there is no other way to actually look at a match.
#include "RA4AI/AICommander.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace RA4;
using namespace RA4::AI;

namespace
{
constexpr int32_t kSampleEveryTicks = 10;   // 0.5 s at 20 Hz
constexpr int32_t kMaxTicks = 20 * 60 * 12; // 12 minutes of simulated time

const ContentId SovYard = MakeContentId("building.sov.construction_yard");
const ContentId AllYard = MakeContentId("building.all.construction_yard");
const ContentId Ore = MakeContentId("resource.ore_field");

// Short label for the viewer legend. Derived from the definition name so it stays
// correct when content is renamed.
std::string ShortLabel(const std::string& Name)
{
    const size_t Last = Name.find_last_of('.');
    return Last == std::string::npos ? Name : Name.substr(Last + 1);
}
} // namespace

int main(int argc, char** argv)
{
    const char* OutPath = argc > 1 ? argv[1] : "match.json";
    const uint64_t Seed = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 20260728ull;

    ContentDatabase Content;
    BuildDefaultContent(Content);

    MatchSetup Setup;
    Setup.Seed = Seed;
    Setup.Map.Name = "viewer.plains";
    Setup.Map.Resize(64, 64, Tile_GroundPassable);
    Setup.Players[0].bActive = true;
    Setup.Players[0].Faction = FactionId::Soviet;
    Setup.Players[0].StartingCredits = 10000;
    Setup.Players[1].bActive = true;
    Setup.Players[1].Faction = FactionId::Alliance;
    Setup.Players[1].StartingCredits = 10000;

    SimWorld World;
    World.Initialize(&Content, Setup);
    World.SpawnBuilding(SovYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(AllYard, 1, TileCoord(48, 48), true);
    for (int32_t X = 0; X < 3; ++X)
    {
        for (int32_t Y = 0; Y < 3; ++Y)
        {
            World.SpawnResourceNode(Ore, TileCoord(6 + X, 15 + Y), 4000);
            World.SpawnResourceNode(Ore, TileCoord(53 + X, 43 + Y), 4000);
        }
    }
    World.ClearEvents();

    AICommander Red;
    AICommander Blue;
    Red.Initialize(0, AIProfile::Aggressive, Seed);
    Blue.Initialize(1, AIProfile::Balanced, Seed ^ 0xABCDEF);

    std::FILE* Out = std::fopen(OutPath, "wb");
    if (Out == nullptr)
    {
        std::fprintf(stderr, "cannot open %s\n", OutPath);
        return 1;
    }

    std::fprintf(Out, "{\n  \"mapTiles\": 64,\n  \"tileUnits\": %d,\n  \"frames\": [\n",
                 int32_t(kTileSizeUnits));

    bool bFirstFrame = true;
    for (int32_t Tick = 0; Tick < kMaxTicks && World.GetPhase() == MatchPhase::Running; ++Tick)
    {
        CommandFrame Frame;
        Frame.Tick = World.GetTick();
        Red.Tick(World, Frame.Commands);
        Blue.Tick(World, Frame.Commands);
        World.ClearEvents();
        World.Tick(Frame.Commands.empty() ? nullptr : &Frame);

        if (Tick % kSampleEveryTicks != 0)
        {
            continue;
        }

        if (!bFirstFrame)
        {
            std::fprintf(Out, ",\n");
        }
        bFirstFrame = false;

        std::fprintf(Out,
                     "    {\"t\": %u, \"credits\": [%d, %d], \"power\": [[%d,%d],[%d,%d]], \"e\": [",
                     World.GetTick(), World.GetPlayer(0).Credits, World.GetPlayer(1).Credits,
                     World.GetPlayer(0).PowerProduced, World.GetPlayer(0).PowerConsumed,
                     World.GetPlayer(1).PowerProduced, World.GetPlayer(1).PowerConsumed);

        const std::vector<EntityCore>& Cores = World.GetAllCores();
        const std::vector<TransformComp>& Xf = World.GetAllTransforms();
        bool bFirstEntity = true;
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Kind == EntityKind::Projectile)
            {
                continue;
            }
            const EntityDef* Def = Content.FindEntity(Cores[I].Def);
            const HealthComp* Health = World.GetHealth(World.MakeId(I));

            int32_t Footprint = 1;
            std::string Label = "ore";
            if (Def != nullptr)
            {
                Label = ShortLabel(Def->Name);
                Footprint = Def->Kind == EntityKind::Building
                                ? std::max(Def->Building.FootprintX, Def->Building.FootprintY)
                                : 0;
            }

            if (!bFirstEntity)
            {
                std::fprintf(Out, ",");
            }
            bFirstEntity = false;
            std::fprintf(Out, "[%d,%d,%.0f,%.0f,%d,%d,%d,\"%s\"]", int32_t(Cores[I].Owner),
                         int32_t(Cores[I].Kind), Xf[I].Position.X.ToDoubleUnsafe(),
                         Xf[I].Position.Y.ToDoubleUnsafe(),
                         Health != nullptr ? Health->Current : 0,
                         Health != nullptr ? Health->Max : 1, Footprint, Label.c_str());
        }
        std::fprintf(Out, "]}");
    }

    std::fprintf(Out, "\n  ],\n  \"winner\": %d,\n  \"finalTick\": %u\n}\n",
                 int32_t(World.GetWinner()), World.GetTick());
    std::fclose(Out);

    std::printf("match: %u ticks (%.0f s), winner=player %d -> %s\n", World.GetTick(),
                double(World.GetTick()) / double(kTicksPerSecond), int32_t(World.GetWinner()), OutPath);
    return 0;
}
