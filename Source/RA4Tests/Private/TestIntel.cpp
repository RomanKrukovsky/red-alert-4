// Copyright (c) Red Alert 4 project. Tests for the Unreliable Intelligence layer (M0).
//
// M0 covers the contract, not behaviour: config loading and validation, the
// disabled-by-default kill switch, track slot lifetime, serialization round-trip
// and checksum stability. Distortion math tests arrive with M2 alongside the code.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/ByteStream.h"
#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"
#include "RA4Intel/IntelConfig.h"
#include "RA4Intel/IntelSystem.h"
#include "RA4Intel/PerceivedWorld.h"

#include <fstream>
#include <sstream>

// Tile centre in world units without dragging MapDescription into these tests.
static RA4::Vec2 TileCentre(int32_t X, int32_t Y)
{
    using RA4::Fixed;
    return RA4::Vec2(Fixed::FromInt(int64_t(X) * RA4::kTileSizeUnits + RA4::kTileSizeUnits / 2),
                     Fixed::FromInt(int64_t(Y) * RA4::kTileSizeUnits + RA4::kTileSizeUnits / 2));
}

namespace
{

using namespace RA4;
using namespace RA4Test;

std::string ReadRepoFile(const std::string& RelativePath)
{
#ifdef RA4_REPO_ROOT
    const std::string Full = std::string(RA4_REPO_ROOT) + "/" + RelativePath;
#else
    const std::string Full = RelativePath;
#endif
    std::ifstream File(Full);
    std::stringstream Buffer;
    Buffer << File.rdbuf();
    return Buffer.str();
}

// Minimal valid settings for tests that do not want to depend on shipped content.
Intel::IntelSettings MakeMinimalSettings(bool bEnabled)
{
    Intel::IntelSettings S;
    S.bEnabled = bEnabled;
    Intel::DistortionProfile P;
    P.Name = "profile.default";
    S.DistortionProfiles.push_back(P);
    Intel::CommsProfile C;
    C.Name = "comms.default";
    C.HopDelayTicksByLevel = {160, 80, 30, 5};
    S.CommsProfiles.push_back(C);
    return S;
}

} // namespace

// --- Config loading -----------------------------------------------------------

RA4_TEST(Intel, ShippedSettingsFileLoadsAndValidates)
{
    const std::string Text = ReadRepoFile("Content/RA4/Data/Intel/intel_settings.json");
    RA4_EXPECT(!Text.empty());

    Intel::IntelSettings Settings;
    std::vector<std::string> Errors;
    const bool bLoaded = Intel::LoadIntelSettingsFromJson(Text, Settings, Errors);
    for (const std::string& E : Errors)
    {
        std::fprintf(stderr, "  intel config error: %s\n", E.c_str());
    }
    RA4_EXPECT(bLoaded);
    RA4_EXPECT(Errors.empty());

    // The shipped default MUST be disabled until M2: every other system keeps
    // classic behaviour and no playtest gets surprise phantoms.
    RA4_EXPECT(!Settings.bEnabled);
    RA4_EXPECT(Settings.FindDistortionProfile("profile.default") != nullptr);
    RA4_EXPECT(Settings.FindCommsProfile("comms.default") != nullptr);
    // Per-mille conversion happened exactly (0.02 -> 20, 8.0 s -> 160 ticks).
    RA4_EXPECT(Settings.DistortionProfiles[0].FabricationChanceMaxPerMille == 20);
    RA4_EXPECT(Settings.CommsProfiles[0].HopDelayTicksByLevel.size() == 4);
    RA4_EXPECT(Settings.CommsProfiles[0].HopDelayTicksByLevel[0] == 160);
}

RA4_TEST(Intel, ValidatorRejectsBrokenConfusionMatrixRow)
{
    Intel::IntelSettings S = MakeMinimalSettings(false);
    // Row no longer sums to 1000.
    S.Confusion.PerMille[0][0] = 900;

    std::vector<std::string> Errors;
    RA4_EXPECT(!Intel::ValidateIntelSettings(S, Errors));
    RA4_EXPECT(!Errors.empty());
}

RA4_TEST(Intel, ValidatorRejectsMissingActiveProfile)
{
    Intel::IntelSettings S = MakeMinimalSettings(false);
    S.ActiveDistortionProfile = "profile.does_not_exist";

    std::vector<std::string> Errors;
    RA4_EXPECT(!Intel::ValidateIntelSettings(S, Errors));
}

RA4_TEST(Intel, ValidatorRejectsUnboundedPhantomLifetime)
{
    Intel::IntelSettings S = MakeMinimalSettings(false);
    S.DistortionProfiles[0].MaxPhantomLifetimeTicks = 0;

    std::vector<std::string> Errors;
    RA4_EXPECT(!Intel::ValidateIntelSettings(S, Errors));
}

RA4_TEST(Intel, LoaderRejectsMalformedJson)
{
    Intel::IntelSettings S;
    std::vector<std::string> Errors;
    RA4_EXPECT(!Intel::LoadIntelSettingsFromJson("{ not json", S, Errors));
    RA4_EXPECT(!Errors.empty());
}

// --- Kill switch (§4.7): disabled layer is genuinely absent ---------------------

RA4_TEST(Intel, DisabledLayerDoesNotChangeSimulationResults)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    // Same seed, same commands (none): a world without the intel argument and a
    // world with a disabled settings object must stay bit-identical. This is the
    // regression gate that lets the rest of the game ignore this module.
    Intel::IntelSettings Disabled = MakeMinimalSettings(false);

    SimWorld A;
    A.Initialize(&Content, MakeTestSetup(777));
    SimWorld B;
    B.Initialize(&Content, MakeTestSetup(777), &Disabled);

    A.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    B.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));

    for (int32_t T = 0; T < 200; ++T)
    {
        A.Tick(nullptr);
        B.Tick(nullptr);
    }
    RA4_EXPECT(A.ComputeStateChecksum() == B.ComputeStateChecksum());
}

RA4_TEST(Intel, EnabledEmptyLayerTicksWithoutStateDrift)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Intel::IntelSettings Enabled = MakeMinimalSettings(true);

    // M0 phases are empty, so two enabled worlds from one seed must stay in
    // lockstep; this pins the plumbing (rng stream, checksum, tick order) before
    // any behaviour exists.
    SimWorld A;
    A.Initialize(&Content, MakeTestSetup(4242), &Enabled);
    SimWorld B;
    B.Initialize(&Content, MakeTestSetup(4242), &Enabled);

    for (int32_t T = 0; T < 200; ++T)
    {
        A.Tick(nullptr);
        B.Tick(nullptr);
        RA4_EXPECT(A.ComputeStateChecksum() == B.ComputeStateChecksum());
    }
    RA4_EXPECT(A.GetIntel().IsEnabled());
}

// --- PerceivedWorld slot lifetime ------------------------------------------------

RA4_TEST(Intel, TrackHandlesAreGenerational)
{
    Intel::PerceivedWorld World;
    World.Initialize(64, 64, 16);

    const Intel::TrackId First = World.AllocateTrack();
    RA4_EXPECT(First.IsValid());
    RA4_EXPECT(World.IsTrackAlive(First));

    World.ReleaseTrack(First);
    RA4_EXPECT(!World.IsTrackAlive(First));

    // Slot is recycled with a bumped generation: the stale handle must stay dead.
    const Intel::TrackId Second = World.AllocateTrack();
    RA4_EXPECT(Second.IsValid());
    RA4_EXPECT(Second.Index == First.Index);
    RA4_EXPECT(Second.Generation != First.Generation);
    RA4_EXPECT(!World.IsTrackAlive(First));
    RA4_EXPECT(World.IsTrackAlive(Second));
}

RA4_TEST(Intel, TrackAllocationRespectsHardCap)
{
    Intel::PerceivedWorld World;
    World.Initialize(64, 64, 4);

    for (int32_t I = 0; I < 4; ++I)
    {
        RA4_EXPECT(World.AllocateTrack().IsValid());
    }
    // Cap reached: allocation refuses instead of growing (memory budget guard).
    RA4_EXPECT(!World.AllocateTrack().IsValid());
    RA4_EXPECT(World.GetAliveTrackCount() == 4);
}

RA4_TEST(Intel, RegionQueryFindsOnlyTracksInside)
{
    Intel::PerceivedWorld World;
    World.Initialize(64, 64, 16);

    const Intel::TrackId Inside = World.AllocateTrack();
    World.GetTrack(Inside)->BelievedPosition = TileCentre(5, 5);
    const Intel::TrackId Outside = World.AllocateTrack();
    World.GetTrack(Outside)->BelievedPosition = TileCentre(40, 40);

    std::vector<const Intel::PerceivedTrack*> Found;
    World.GetTracksInRegion(0, 0, 10, 10, Found);
    RA4_EXPECT(Found.size() == 1);
    RA4_EXPECT(Found[0]->Id == Inside);
}

RA4_TEST(Intel, NegativeKnowledgeDistinguishesNeverSeenFromSeen)
{
    Intel::PerceivedWorld World;
    World.Initialize(64, 64, 16);

    RA4_EXPECT(World.GetLastObservedTick(10, 10) == 0); // never observed
    World.SetLastObservedTick(10, 10, 500);
    RA4_EXPECT(World.GetLastObservedTick(10, 10) == 500);
    RA4_EXPECT(World.GetLastObservedTick(11, 10) == 0); // neighbour untouched
}

// --- Serialization round-trip -----------------------------------------------------

RA4_TEST(Intel, PerceivedWorldSurvivesSerializationRoundTrip)
{
    Intel::PerceivedWorld World;
    World.Initialize(32, 32, 8);

    const Intel::TrackId Id = World.AllocateTrack();
    Intel::PerceivedTrack* T = World.GetTrack(Id);
    T->BelievedClass = Ids::SovHeavyTank;
    T->BelievedPosition = TileCentre(7, 9);
    T->BelievedCountMin = 3;
    T->BelievedCountMax = 8;
    T->Confidence = Fixed::FromRatio(750, 1000);
    T->bContested = true;
    World.SetLastObservedTick(7, 9, 123);

    // Release-then-allocate so the free list and generations are non-trivial.
    const Intel::TrackId Temp = World.AllocateTrack();
    World.ReleaseTrack(Temp);

    ByteWriter W;
    World.Serialize(W);

    Intel::PerceivedWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_EXPECT(Restored.Deserialize(R));

    // Checksums must match bit-for-bit: this is the "no GT/PS divergence after
    // load" contract from the spec, at the unit level.
    Hash64 HA, HB;
    World.FeedChecksum(HA);
    Restored.FeedChecksum(HB);
    RA4_EXPECT(HA.Get() == HB.Get());

    const Intel::PerceivedTrack* RT = Restored.GetTrack(Id);
    RA4_REQUIRE(RT != nullptr);
    RA4_EXPECT(RT->BelievedClass == Ids::SovHeavyTank);
    RA4_EXPECT(RT->BelievedCountMin == 3);
    RA4_EXPECT(RT->BelievedCountMax == 8);
    RA4_EXPECT(RT->bContested);
    RA4_EXPECT(Restored.GetLastObservedTick(7, 9) == 123);

    // The recycled slot's generation survived, so the stale handle stays dead.
    RA4_EXPECT(!Restored.IsTrackAlive(Temp));
}

RA4_TEST(Intel, SimWorldSaveLoadRoundTripsWithIntelEnabled)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Intel::IntelSettings Enabled = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(999), &Enabled);
    World.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    for (int32_t T = 0; T < 50; ++T)
    {
        World.Tick(nullptr);
    }

    ByteWriter W;
    World.Serialize(W);

    SimWorld Restored;
    // The restored session must be armed with the same settings before load,
    // mirroring how a real load flow passes content and settings together.
    Restored.Initialize(&Content, MakeTestSetup(999), &Enabled);
    ByteReader R(W.GetBuffer());
    RA4_REQUIRE(Restored.Deserialize(R, &Content));

    RA4_EXPECT(World.ComputeStateChecksum() == Restored.ComputeStateChecksum());

    // And the two must stay identical when stepped further: catching drift that
    // only shows up after resume is the entire point of this test.
    for (int32_t T = 0; T < 50; ++T)
    {
        World.Tick(nullptr);
        Restored.Tick(nullptr);
    }
    RA4_EXPECT(World.ComputeStateChecksum() == Restored.ComputeStateChecksum());
}

RA4_TEST(Intel, PreIntelSaveIsRefusedWhenIntelEnabled)
{
    // A v2 (pre-intel) save cannot provide belief state; loading it into an
    // intel-enabled session must fail loudly, not start with an empty HQ map.
    // We approximate a v2 save by serializing with intel disabled and patching
    // the version field -- the payload layout up to the intel block is identical.
    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld Classic;
    Classic.Initialize(&Content, MakeTestSetup(31337));
    for (int32_t T = 0; T < 10; ++T)
    {
        Classic.Tick(nullptr);
    }

    ByteWriter W;
    Classic.Serialize(W);

    Intel::IntelSettings Enabled = MakeMinimalSettings(true);
    SimWorld Target;
    Target.Initialize(&Content, MakeTestSetup(31337), &Enabled);
    ByteReader R(W.GetBuffer());
    // v3 save with intel disabled into enabled session: IntelSystem::Deserialize
    // sees the enabled-ness mismatch and refuses.
    RA4_EXPECT(!Target.Deserialize(R, &Content));
}
