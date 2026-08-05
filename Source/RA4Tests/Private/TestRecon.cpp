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
#include "RA4Recon/ReconConfig.h"
#include "RA4Recon/ReconSystem.h"
#include "RA4Recon/PerceivedWorld.h"

#include <fstream>
#include <sstream>

namespace RA4
{
namespace Recon
{
// The friend bridge declared in PerceivedWorld.h. Belief writes are structural
// (INVARIANT 9): production code cannot reach these, the deterministic test
// harness can -- through this one named door, not through a public accessor.
struct PerceivedWorldTestAccess
{
    static void Initialize(PerceivedWorld& W, int32_t Width, int32_t Height, int32_t MaxTracks)
    {
        W.Initialize(Width, Height, MaxTracks);
    }
    static TrackId AllocateTrack(PerceivedWorld& W) { return W.AllocateTrack(); }
    static void ReleaseTrack(PerceivedWorld& W, TrackId Id) { W.ReleaseTrack(Id); }
    static PerceivedTrack* GetTrackMutable(PerceivedWorld& W, TrackId Id) { return W.GetTrackMutable(Id); }
    static void SetLastObservedTick(PerceivedWorld& W, int32_t X, int32_t Y, TickIndex T)
    {
        W.SetLastObservedTick(X, Y, T);
    }
    static bool Deserialize(PerceivedWorld& W, ByteReader& R) { return W.Deserialize(R); }
    static bool IsPhantom(const PerceivedWorld& W, TrackId Id) { return W.IsTrackPhantomInternal(Id); }
    static void SetPhantom(PerceivedWorld& W, TrackId Id, bool bP) { W.SetTrackPhantomInternal(Id, bP); }
    static void SetDecayCursor(PerceivedWorld& W, uint32_t C) { W.DecayCursor = C; }
    static uint32_t GetDecayCursor(const PerceivedWorld& W) { return W.DecayCursor; }
};
} // namespace Recon
} // namespace RA4

using RA4::Recon::PerceivedWorldTestAccess;

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
Recon::ReconSettings MakeMinimalSettings(bool bEnabled)
{
    Recon::ReconSettings S;
    S.bEnabled = bEnabled;
    Recon::DistortionProfile P;
    P.Name = "profile.default";
    S.DistortionProfiles.push_back(P);
    Recon::CommsProfile C;
    C.Name = "comms.default";
    C.HopDelayTicksByLevel = {160, 80, 30, 5};
    S.CommsProfiles.push_back(C);
    return S;
}

} // namespace

// --- Config loading -----------------------------------------------------------

RA4_TEST(Recon, ShippedSettingsFileLoadsAndValidates)
{
    const std::string Text = ReadRepoFile("Content/RA4/Data/Recon/recon_settings.json");
    RA4_EXPECT(!Text.empty());

    Recon::ReconSettings Settings;
    std::vector<std::string> Errors;
    const bool bLoaded = Recon::LoadReconSettingsFromJson(Text, Settings, Errors);
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

RA4_TEST(Recon, ValidatorRejectsBrokenConfusionMatrixRow)
{
    Recon::ReconSettings S = MakeMinimalSettings(false);
    // Row no longer sums to 1000.
    S.Confusion.PerMille[0][0] = 900;

    std::vector<std::string> Errors;
    RA4_EXPECT(!Recon::ValidateReconSettings(S, Errors));
    RA4_EXPECT(!Errors.empty());
}

RA4_TEST(Recon, ValidatorRejectsMissingActiveProfile)
{
    Recon::ReconSettings S = MakeMinimalSettings(false);
    S.ActiveDistortionProfile = "profile.does_not_exist";

    std::vector<std::string> Errors;
    RA4_EXPECT(!Recon::ValidateReconSettings(S, Errors));
}

RA4_TEST(Recon, ValidatorRejectsUnboundedPhantomLifetime)
{
    Recon::ReconSettings S = MakeMinimalSettings(false);
    S.DistortionProfiles[0].MaxPhantomLifetimeTicks = 0;

    std::vector<std::string> Errors;
    RA4_EXPECT(!Recon::ValidateReconSettings(S, Errors));
}

RA4_TEST(Recon, LoaderRejectsMalformedJson)
{
    Recon::ReconSettings S;
    std::vector<std::string> Errors;
    RA4_EXPECT(!Recon::LoadReconSettingsFromJson("{ not json", S, Errors));
    RA4_EXPECT(!Errors.empty());
}

// --- Kill switch (§4.7): disabled layer is genuinely absent ---------------------

RA4_TEST(Recon, DisabledLayerDoesNotChangeSimulationResults)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    // Same seed, same commands (none): a world without the intel argument and a
    // world with a disabled settings object must stay bit-identical. This is the
    // regression gate that lets the rest of the game ignore this module.
    Recon::ReconSettings Disabled = MakeMinimalSettings(false);

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

RA4_TEST(Recon, EnabledEmptyLayerTicksWithoutStateDrift)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Enabled = MakeMinimalSettings(true);

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
    RA4_EXPECT(A.GetRecon().IsEnabled());
}

// --- PerceivedWorld slot lifetime ------------------------------------------------

RA4_TEST(Recon, TrackHandlesAreGenerational)
{
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 64, 64, 16);

    const Recon::TrackId First = PerceivedWorldTestAccess::AllocateTrack(World);
    RA4_EXPECT(First.IsValid());
    RA4_EXPECT(World.IsTrackAlive(First));

    PerceivedWorldTestAccess::ReleaseTrack(World, First);
    RA4_EXPECT(!World.IsTrackAlive(First));

    // Slot is recycled with a bumped generation: the stale handle must stay dead.
    const Recon::TrackId Second = PerceivedWorldTestAccess::AllocateTrack(World);
    RA4_EXPECT(Second.IsValid());
    RA4_EXPECT(Second.Index == First.Index);
    RA4_EXPECT(Second.Generation != First.Generation);
    RA4_EXPECT(!World.IsTrackAlive(First));
    RA4_EXPECT(World.IsTrackAlive(Second));
}

RA4_TEST(Recon, TrackAllocationRespectsHardCap)
{
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 64, 64, 4);

    for (int32_t I = 0; I < 4; ++I)
    {
        RA4_EXPECT(PerceivedWorldTestAccess::AllocateTrack(World).IsValid());
    }
    // Cap reached: allocation refuses instead of growing (memory budget guard).
    RA4_EXPECT(!PerceivedWorldTestAccess::AllocateTrack(World).IsValid());
    RA4_EXPECT(World.GetAliveTrackCount() == 4);
}

RA4_TEST(Recon, RegionQueryFindsOnlyTracksInside)
{
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 64, 64, 16);

    const Recon::TrackId Inside = PerceivedWorldTestAccess::AllocateTrack(World);
    PerceivedWorldTestAccess::GetTrackMutable(World, Inside)->BelievedPosition = TileCentre(5, 5);
    const Recon::TrackId Outside = PerceivedWorldTestAccess::AllocateTrack(World);
    PerceivedWorldTestAccess::GetTrackMutable(World, Outside)->BelievedPosition = TileCentre(40, 40);

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetTracksInRegion(0, 0, 10, 10, Found);
    RA4_EXPECT(Found.size() == 1);
    RA4_EXPECT(Found[0]->Id == Inside);
}

RA4_TEST(Recon, NegativeKnowledgeDistinguishesNeverSeenFromSeen)
{
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 64, 64, 16);

    RA4_EXPECT(World.GetLastObservedTick(10, 10) == 0); // never observed
    PerceivedWorldTestAccess::SetLastObservedTick(World, 10, 10, 500);
    RA4_EXPECT(World.GetLastObservedTick(10, 10) == 500);
    RA4_EXPECT(World.GetLastObservedTick(11, 10) == 0); // neighbour untouched
}

RA4_TEST(Recon, PhantomTruthLivesOutsideTheReadSurface)
{
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 64, 64, 16);

    const Recon::TrackId Id = PerceivedWorldTestAccess::AllocateTrack(World);
    PerceivedWorldTestAccess::SetPhantom(World, Id, true);
    RA4_EXPECT(PerceivedWorldTestAccess::IsPhantom(World, Id));

    // The instrumented leak detector for INVARIANT 10: the read-surface struct
    // must be exactly its documented fields. If someone adds a truth flag (or an
    // EntityId) back into PerceivedTrack, the size changes and this fails the
    // build review immediately instead of leaking quietly.
    struct ExpectedReadSurface
    {
        Recon::TrackId Id;
        ContentId BelievedClass;
        Vec2 BelievedPosition;
        Fixed PositionErrorRadius;
        int32_t BelievedCountMin;
        int32_t BelievedCountMax;
        TickIndex LastUpdateTick;
        Fixed Confidence;
        uint8_t IndependentSourceCount;
        bool bStale;
        bool bContested;
        uint32_t ProvenanceReportIds[Recon::kTrackProvenanceSize];
        uint8_t ProvenanceCount;
        bool bAlive;
    };
    static_assert(sizeof(Recon::PerceivedTrack) == sizeof(ExpectedReadSurface),
                  "PerceivedTrack layout changed: verify no ground-truth field was added "
                  "to the belief read surface (INVARIANT 10) before updating this mirror");

    // Recycling the slot must clear the internal phantom flag with it.
    PerceivedWorldTestAccess::ReleaseTrack(World, Id);
    const Recon::TrackId Reused = PerceivedWorldTestAccess::AllocateTrack(World);
    RA4_EXPECT(Reused.Index == Id.Index);
    RA4_EXPECT(!PerceivedWorldTestAccess::IsPhantom(World, Reused));
}

// --- Serialization round-trip -----------------------------------------------------

RA4_TEST(Recon, PerceivedWorldSurvivesSerializationRoundTrip)
{
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 32, 32, 8);

    const Recon::TrackId Id = PerceivedWorldTestAccess::AllocateTrack(World);
    Recon::PerceivedTrack* T = PerceivedWorldTestAccess::GetTrackMutable(World, Id);
    T->BelievedClass = Ids::SovHeavyTank;
    T->BelievedPosition = TileCentre(7, 9);
    T->BelievedCountMin = 3;
    T->BelievedCountMax = 8;
    T->Confidence = Fixed::FromRatio(750, 1000);
    T->bContested = true;
    PerceivedWorldTestAccess::SetPhantom(World, Id, true);
    PerceivedWorldTestAccess::SetLastObservedTick(World, 7, 9, 123);

    // Release-then-allocate so the free list and generations are non-trivial.
    const Recon::TrackId Temp = PerceivedWorldTestAccess::AllocateTrack(World);
    PerceivedWorldTestAccess::ReleaseTrack(World, Temp);

    ByteWriter W;
    World.Serialize(W);

    Recon::PerceivedWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_EXPECT(PerceivedWorldTestAccess::Deserialize(Restored, R));

    // Checksums must match bit-for-bit: this is the "no GT/PS divergence after
    // load" contract from the spec, at the unit level.
    Hash64 HA, HB;
    World.FeedChecksum(HA);
    Restored.FeedChecksum(HB);
    RA4_EXPECT(HA.Get() == HB.Get());

    const Recon::PerceivedTrack* RT = Restored.GetTrack(Id);
    RA4_REQUIRE(RT != nullptr);
    RA4_EXPECT(RT->BelievedClass == Ids::SovHeavyTank);
    RA4_EXPECT(RT->BelievedCountMin == 3);
    RA4_EXPECT(RT->BelievedCountMax == 8);
    RA4_EXPECT(RT->bContested);
    RA4_EXPECT(PerceivedWorldTestAccess::IsPhantom(Restored, Id));
    RA4_EXPECT(Restored.GetLastObservedTick(7, 9) == 123);

    // The recycled slot's generation survived, so the stale handle stays dead.
    RA4_EXPECT(!Restored.IsTrackAlive(Temp));
}

RA4_TEST(Recon, DecayCursorIsSimStateNotScratch)
{
    // I-B4: the amortized-sweep cursor decides WHICH TICK each track's
    // confidence drops once decay math lands (M2). If it were scratch state,
    // a save/load or a late-join would silently shift every subsequent decay
    // event on one peer only -- a delayed-fuse desync. Pin all three
    // properties now, while the phase is still empty.
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 32, 32, 8);
    // Cursor must stay within [0, HighWaterMark): allocate enough slots that 5
    // is a legal position, or the load-time corruption clamp rewrites it.
    for (int I = 0; I < 6; ++I)
    {
        (void)PerceivedWorldTestAccess::AllocateTrack(World);
    }
    PerceivedWorldTestAccess::SetDecayCursor(World, 5);

    // 1. Survives the round trip.
    ByteWriter W;
    World.Serialize(W);
    Recon::PerceivedWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_REQUIRE(PerceivedWorldTestAccess::Deserialize(Restored, R));
    RA4_EXPECT(PerceivedWorldTestAccess::GetDecayCursor(Restored) == 5u);

    // 1b. The corruption clamp: an out-of-range cursor in the byte stream is
    // wrapped deterministically on load, never trusted.
    {
        Recon::PerceivedWorld Tiny;
        PerceivedWorldTestAccess::Initialize(Tiny, 16, 16, 4);
        (void)PerceivedWorldTestAccess::AllocateTrack(Tiny); // HighWaterMark = 1
        PerceivedWorldTestAccess::SetDecayCursor(Tiny, 3);   // out of range on purpose
        ByteWriter TW;
        Tiny.Serialize(TW);
        Recon::PerceivedWorld TinyRestored;
        ByteReader TR(TW.GetBuffer());
        RA4_REQUIRE(PerceivedWorldTestAccess::Deserialize(TinyRestored, TR));
        RA4_EXPECT(PerceivedWorldTestAccess::GetDecayCursor(TinyRestored) == 0u); // 3 % 1
    }

    // 2. Feeds the checksum: two worlds equal except for the cursor must hash
    //    differently, or a cursor divergence would hide until it moved a track.
    Recon::PerceivedWorld Other;
    ByteReader R2(W.GetBuffer());
    RA4_REQUIRE(PerceivedWorldTestAccess::Deserialize(Other, R2));
    PerceivedWorldTestAccess::SetDecayCursor(Other, 6);
    Hash64 HA, HB;
    Restored.FeedChecksum(HA);
    Other.FeedChecksum(HB);
    RA4_EXPECT(HA.Get() != HB.Get());

    // 3. Reset clears it with the rest of the world.
    PerceivedWorldTestAccess::Initialize(Other, 16, 16, 4);
    RA4_EXPECT(PerceivedWorldTestAccess::GetDecayCursor(Other) == 0u);
}

RA4_TEST(Recon, ValidatorRejectsBadTracksPerTickBudget)
{
    // I-B4: budget 0 stalls the sweep forever -- tracks never decay and never
    // GC, which reads as "intel works" until the track cap fills. Above the cap
    // is meaningless. The validator must catch both at load, not at minute 40.
    {
        Recon::ReconSettings S = MakeMinimalSettings(false);
        S.Tracks.TracksPerTickBudget = 0;
        std::vector<std::string> Errors;
        RA4_EXPECT(!Recon::ValidateReconSettings(S, Errors));
    }
    {
        Recon::ReconSettings S = MakeMinimalSettings(false);
        S.Tracks.TracksPerTickBudget = S.Tracks.MaxTracksPerPlayer + 1;
        std::vector<std::string> Errors;
        RA4_EXPECT(!Recon::ValidateReconSettings(S, Errors));
    }
    {
        // Sanity: the default passes.
        Recon::ReconSettings S = MakeMinimalSettings(false);
        std::vector<std::string> Errors;
        RA4_EXPECT(Recon::ValidateReconSettings(S, Errors));
    }
    {
        // Boundary accept-case: budget == cap is legal (a full sweep every
        // tick). Pins the validator's > against an accidental >=.
        Recon::ReconSettings S = MakeMinimalSettings(false);
        S.Tracks.TracksPerTickBudget = S.Tracks.MaxTracksPerPlayer;
        std::vector<std::string> Errors;
        RA4_EXPECT(Recon::ValidateReconSettings(S, Errors));
    }
}

RA4_TEST(Recon, SimWorldSaveLoadRoundTripsWithReconEnabled)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Enabled = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(999), &Enabled);
    World.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    // A live enemy contact makes the save carry real belief state: a track AND
    // its association-table entry. Without it this test round-trips only empty
    // containers and would miss a forgotten field in either.
    World.SpawnUnit(Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    for (int32_t T = 0; T < 50; ++T)
    {
        World.Tick(nullptr);
    }
    RA4_EXPECT(World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() == 1);

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
    // The association survived the load: the ongoing contact kept updating ONE
    // track, it did not fork a duplicate blip after resume.
    RA4_EXPECT(Restored.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() == 1);
    RA4_EXPECT(World.ComputeStateChecksum() == Restored.ComputeStateChecksum());
}

RA4_TEST(Recon, PreReconSaveIsRefusedWhenReconEnabled)
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

    Recon::ReconSettings Enabled = MakeMinimalSettings(true);
    SimWorld Target;
    Target.Initialize(&Content, MakeTestSetup(31337), &Enabled);
    ByteReader R(W.GetBuffer());
    // v3 save with intel disabled into enabled session: ReconSystem::Deserialize
    // sees the enabled-ness mismatch and refuses.
    RA4_EXPECT(!Target.Deserialize(R, &Content));
}

// --- M1: truthful pipeline (PS == GT while nothing distorts) -----------------------

namespace
{

// Both bases plus one hostile scout parked inside player 0's vision. Fog reveal
// radius comes from unit VisionRange (600 units = 3 tiles), so 2 tiles apart
// guarantees mutual visibility.
void SpawnScoutContact(SimWorld& World)
{
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
}

const Recon::PerceivedTrack* FindSingleTrack(const SimWorld& World, PlayerId P)
{
    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(P).GetTracksInRegion(0, 0, 63, 63, Found);
    return Found.size() == 1 ? Found[0] : nullptr;
}

} // namespace

RA4_TEST(Recon, TruthfulPipelineMirrorsVisibleEnemy)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Enabled = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(555), &Enabled);
    SpawnScoutContact(World);

    World.Tick(nullptr); // fog reveals, intel observes, report arrives same tick

    // Player 0 sees exactly one contact: the enemy rifleman, at its true position,
    // true class, count interval collapsed to [1,1], full confidence.
    const Recon::PerceivedTrack* T = FindSingleTrack(World, 0);
    RA4_REQUIRE(T != nullptr);
    RA4_EXPECT(T->BelievedClass == RA4Test::Ids::AllRifleman);
    RA4_EXPECT(T->BelievedPosition.X == Fixed::FromInt(3400));
    RA4_EXPECT(T->BelievedPosition.Y == Fixed::FromInt(3000));
    RA4_EXPECT(T->BelievedCountMin == 1 && T->BelievedCountMax == 1);
    RA4_EXPECT(T->Confidence == Fixed::FromInt(1));
    RA4_EXPECT(T->PositionErrorRadius == Fixed::Zero());
    RA4_EXPECT(!T->bStale);

    // And symmetrically: player 1 tracks player 0's conscript.
    const Recon::PerceivedTrack* T1 = FindSingleTrack(World, 1);
    RA4_REQUIRE(T1 != nullptr);
    RA4_EXPECT(T1->BelievedClass == RA4Test::Ids::SovConscript);
}

RA4_TEST(Recon, TrackFollowsMovingContactWithoutDuplicates)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Enabled = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(556), &Enabled);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    const EntityId Enemy =
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    World.Tick(nullptr);
    RA4_EXPECT(World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() == 1);

    // Order the tank to drive within vision; the same track must update in place
    // (association table), not spawn a second contact per new position.
    // Destination stays inside the conscript's 600-unit vision: this test pins
    // in-place track UPDATES; leaving vision (the freeze case) is the next test.
    Command Move = RA4Test::MakeCommand(CommandType::Move, 1);
    Move.Primary = Enemy;
    Move.Location = Vec2(Fixed::FromInt(3350), Fixed::FromInt(3300));
    RA4_REQUIRE(World.ApplyCommand(Move).IsAccepted());

    Vec2 LastBelieved(Fixed::Zero(), Fixed::Zero());
    for (int32_t T = 0; T < 100; ++T)
    {
        World.Tick(nullptr);
    }
    RA4_EXPECT(World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() == 1);

    const Recon::PerceivedTrack* Track = FindSingleTrack(World, 0);
    RA4_REQUIRE(Track != nullptr);
    // The believed position tracked the movement: it is no longer the spawn point.
    RA4_EXPECT(Track->BelievedPosition.X != Fixed::FromInt(3400) ||
               Track->BelievedPosition.Y != Fixed::FromInt(3000));
    // And with zero distortion it equals the true position exactly.
    const TransformComp* True = World.GetTransform(Enemy);
    RA4_REQUIRE(True != nullptr);
    RA4_EXPECT(Track->BelievedPosition.X == True->Position.X);
    RA4_EXPECT(Track->BelievedPosition.Y == True->Position.Y);
    (void)LastBelieved;
}

RA4_TEST(Recon, LostContactFreezesAsLastKnownPositionAndGoesStale)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Enabled = MakeMinimalSettings(true);
    Enabled.Tracks.StaleAfterTicks = 40; // 2 s, keeps the test fast

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(557), &Enabled);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    const EntityId Enemy =
        World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    // Keeps player 1 alive after the rifleman dies -- a finished match stops
    // ticking, and a stopped clock can never mark anything stale. Far corner:
    // outside player 0's vision, so player 0 still tracks exactly one contact.
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(11000), Fixed::FromInt(11000)));

    World.Tick(nullptr);
    const Recon::PerceivedTrack* Track = FindSingleTrack(World, 0);
    RA4_REQUIRE(Track != nullptr);
    const Vec2 LastKnown = Track->BelievedPosition;

    // The enemy dies out of nowhere (debug damage). The HQ map must NOT learn
    // this: the track freezes at last known position and only goes stale.
    World.DebugDamage(Enemy, 1000000);
    for (int32_t T = 0; T < 60; ++T)
    {
        World.Tick(nullptr);
    }

    RA4_EXPECT(!World.IsAlive(Enemy));
    const Recon::PerceivedTrack* Frozen = FindSingleTrack(World, 0);
    RA4_REQUIRE(Frozen != nullptr);
    RA4_EXPECT(Frozen->BelievedPosition.X == LastKnown.X);
    RA4_EXPECT(Frozen->BelievedPosition.Y == LastKnown.Y);
    RA4_EXPECT(Frozen->bStale);
}

RA4_TEST(Recon, BeliefIsReplayReconstructible)
{
    // INVARIANT 11 / I-B5: "what did player P believe at tick T" must be
    // answerable from (seed, command stream) alone. Two independent SimWorlds
    // fed the same recorded frames must agree on the full state checksum --
    // which includes every PerceivedWorld -- at every checkpoint, and on the
    // exact belief contents at the end.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Enabled = MakeMinimalSettings(true);

    MatchSetup Setup = MakeTestSetup(8181);

    SimWorld Live;
    Live.Initialize(&Content, Setup, &Enabled);
    Live.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    const EntityId Enemy =
        Live.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    // Record the command stream exactly as a replay file stores it: the spawn
    // above stands in for scenario setup, the move command is the player input.
    std::vector<CommandFrame> Frames;
    std::vector<uint64_t> Checkpoints;
    for (int32_t T = 0; T < 120; ++T)
    {
        CommandFrame Frame;
        Frame.Tick = Live.GetTick();
        if (T == 10)
        {
            Command Move = RA4Test::MakeCommand(CommandType::Move, 1);
            Move.Primary = Enemy;
            Move.Location = Vec2(Fixed::FromInt(4200), Fixed::FromInt(3800));
            Frame.Commands.push_back(Move);
        }
        Live.Tick(Frame.Commands.empty() ? nullptr : &Frame);
        Frames.push_back(Frame);
        Checkpoints.push_back(Live.ComputeStateChecksum());
    }

    // Reconstruction: fresh world, same seed, same scripted spawns, same frames.
    SimWorld Replayed;
    Replayed.Initialize(&Content, Setup, &Enabled);
    Replayed.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    Replayed.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    for (int32_t T = 0; T < 120; ++T)
    {
        const CommandFrame& Frame = Frames[size_t(T)];
        Replayed.Tick(Frame.Commands.empty() ? nullptr : &Frame);
        RA4_EXPECT(Replayed.ComputeStateChecksum() == Checkpoints[size_t(T)]);
    }

    // Belief-level comparison, not just hashes: player 0's HQ map is identical.
    std::vector<const Recon::PerceivedTrack*> LiveTracks, ReplayTracks;
    Live.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, LiveTracks);
    Replayed.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, ReplayTracks);
    RA4_REQUIRE(LiveTracks.size() == ReplayTracks.size());
    for (size_t I = 0; I < LiveTracks.size(); ++I)
    {
        RA4_EXPECT(LiveTracks[I]->BelievedPosition.X == ReplayTracks[I]->BelievedPosition.X);
        RA4_EXPECT(LiveTracks[I]->BelievedPosition.Y == ReplayTracks[I]->BelievedPosition.Y);
        RA4_EXPECT(LiveTracks[I]->BelievedClass == ReplayTracks[I]->BelievedClass);
        RA4_EXPECT(LiveTracks[I]->LastUpdateTick == ReplayTracks[I]->LastUpdateTick);
        RA4_EXPECT(LiveTracks[I]->Confidence == ReplayTracks[I]->Confidence);
    }
}
