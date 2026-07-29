// Copyright (c) Red Alert 4 project. Tests for deterministic save & restore.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/ByteStream.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;
using namespace RA4Test;

RA4_TEST(SaveSystem, MidMatchSaveAndRestorePreservesStateAndChecksum)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld WorldA;
    WorldA.Initialize(&Content, MakeTestSetup(12345));

    // Spawn initial base structure & units
    WorldA.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    WorldA.SpawnBuilding(Ids::AllConYard, 1, TileCoord(48, 48), true);
    const EntityId HarvesterA = WorldA.SpawnUnit(Ids::SovHarvester, 0, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));
    const EntityId TankA = WorldA.SpawnUnit(Ids::SovHeavyTank, 0, Vec2(Fixed::FromInt(2400), Fixed::FromInt(2400)));
    RA4_REQUIRE(HarvesterA.IsValid());
    RA4_REQUIRE(TankA.IsValid());

    // Step 100 ticks
    RunTicks(WorldA, 100);

    const uint64_t ChecksumA_BeforeSave = WorldA.ComputeStateChecksum();
    RA4_EXPECT(ChecksumA_BeforeSave != 0);

    // Serialize WorldA
    ByteWriter Writer;
    WorldA.Serialize(Writer);
    RA4_REQUIRE(Writer.Size() > 0);

    // Deserialize into WorldB
    SimWorld WorldB;
    ByteReader Reader(Writer.GetBuffer());
    const bool bLoaded = WorldB.Deserialize(Reader, &Content);
    RA4_REQUIRE(bLoaded);
    RA4_REQUIRE(!Reader.HasError());

    const uint64_t ChecksumB_ImmediatelyAfterLoad = WorldB.ComputeStateChecksum();
    RA4_EXPECT_EQ(ChecksumA_BeforeSave, ChecksumB_ImmediatelyAfterLoad);

    // Step both worlds for another 100 ticks with identical commands
    RunTicks(WorldA, 100);
    RunTicks(WorldB, 100);

    const uint64_t ChecksumA_AfterTicks = WorldA.ComputeStateChecksum();
    const uint64_t ChecksumB_AfterTicks = WorldB.ComputeStateChecksum();

    RA4_EXPECT_EQ(ChecksumA_AfterTicks, ChecksumB_AfterTicks);
}
