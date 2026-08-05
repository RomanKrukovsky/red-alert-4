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
#include "RA4Recon/DistortionPipeline.h"
#include "RA4Recon/MoraleModel.h"
#include "RA4Recon/ReconSystem.h"
#include "RA4Recon/PerceivedWorld.h"
#include "RA4Replay/Replay.h"

#include <cstring>
#include <filesystem>
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
// The profile is TRUTHFUL (every stage disabled): M1-contract tests assert
// PS == GT exactly, and each M2 test switches on precisely the stage it pins.
Recon::ReconSettings MakeMinimalSettings(bool bEnabled)
{
    Recon::ReconSettings S;
    S.bEnabled = bEnabled;
    Recon::DistortionProfile P;
    P.Name = "profile.default";
    P.bClarityEnabled = false;
    P.bCountDistortionEnabled = false;
    P.bClassificationErrorEnabled = false;
    P.bPositionErrorEnabled = false;
    P.bOmissionEnabled = false;
    P.bFabricationEnabled = false;
    P.bSelfReportBiasEnabled = false;
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
// --- I-B5: belief is replay-reconstructible (INVARIANT 11) -----------------------

namespace
{
// Records a short recon-enabled match and returns its bytes plus the belief
// checksum captured at every tick. The scenario seeds nothing outside the
// header, so playback rebuilds it exactly (same rule as
// Replay.PlaybackReproducesEveryCheckpointChecksum).
struct BeliefTimeline
{
    std::vector<uint8_t> ReplayBytes;
    std::vector<uint64_t> BeliefChecksumPerTick; // index = tick, player 0's world
};

BeliefTimeline RecordReconMatch(const ContentDatabase& Content, const Recon::ReconSettings& Settings,
                                uint64_t Seed, int32_t Ticks)
{
    BeliefTimeline Out;
    MatchSetup Setup = MakeTestSetup(Seed);
    SimWorld World;
    World.Initialize(&Content, Setup, &Settings);

    ReplayRecorder Recorder;
    Recorder.Begin(MakeHeaderFromSetup(Setup, Content, "test", &Settings));

    for (int32_t I = 0; I < Ticks; ++I)
    {
        CommandFrame Frame;
        Frame.Tick = World.GetTick();
        // The stream must contain real commands so the replayed sim (and the
        // fog input the recon phases read) evolves: an empty-world timeline
        // would pass reconstruction trivially (review finding on I-B5). Two
        // construction yards appear from the command stream itself, so the
        // rebuilt run owes ALL of its state to the replay bytes.
        if (I == 3)
        {
            Command C;
            C.Type = CommandType::PlaceBuilding;
            C.Issuer = 0;
            C.Content = Ids::SovConYard;
            C.Tile = TileCoord(10, 10);
            Frame.Commands.push_back(C);
        }
        if (I == 5)
        {
            Command C;
            C.Type = CommandType::PlaceBuilding;
            C.Issuer = 1;
            C.Content = Ids::AllConYard;
            C.Tile = TileCoord(50, 50);
            Frame.Commands.push_back(C);
        }
        World.Tick(Frame.Commands.empty() ? nullptr : &Frame);
        World.ClearEvents();
        Recorder.RecordFrame(Frame);
        if ((World.GetTick() % kChecksumIntervalTicks) == 0)
        {
            Recorder.RecordCheckpoint(World.GetTick(), World.ComputeStateChecksum());
        }
        Hash64 H;
        World.GetRecon().GetPerceivedWorld(0).FeedChecksum(H);
        Out.BeliefChecksumPerTick.push_back(H.Get());
    }
    Recorder.End(World.GetTick(), World.GetWinner());
    Out.ReplayBytes = Recorder.Serialize();
    return Out;
}
} // namespace

RA4_TEST(Recon, BeliefIsReconstructibleFromReplayAlone)
{
    // INVARIANT 11: "what did player P believe at tick T" must be answerable
    // from (replay, playerId). We record a live match, then rebuild the belief
    // timeline from nothing but the replay bytes and the settings the header
    // identifies -- every per-tick belief checksum must match the live run.
    //
    // M2 OBLIGATION: while the recon phases are M0-empty, belief only changes
    // through plumbing (map dims, enabled-ness, structures arriving from the
    // command stream). Once observation/distortion land, this test MUST be
    // extended with a tuning-swap case: replaying under settings with a
    // DIFFERENT hash must produce a DIFFERENT belief timeline, or the hash
    // gate is decorative. Tracked in NEXT_ACTIONS I-M2 acceptance criteria.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);

    const BeliefTimeline Live = RecordReconMatch(Content, Settings, 90210, 120);

    ReplayData Data;
    std::string Error;
    RA4_REQUIRE(DeserializeReplay(Live.ReplayBytes, Data, Error));
    RA4_REQUIRE(Data.Header.bReconEnabled);
    RA4_EXPECT(Data.Header.ReconSettingsHash == Settings.ComputeSettingsHash());

    // Reconstruction: header -> setup -> tick the command stream, reading
    // player 0's perceived world at every tick.
    MatchSetup Setup;
    Setup.Seed = Data.Header.Seed;
    Setup.Map.Name = Data.Header.MapName;
    Setup.Map.Width = Data.Header.MapWidth;
    Setup.Map.Height = Data.Header.MapHeight;
    Setup.Map.Tiles = Data.Header.MapTiles;
    for (int32_t I = 0; I < kMaxPlayers; ++I)
    {
        Setup.Players[I].bActive = Data.Header.Players[I].bActive;
        Setup.Players[I].Faction = FactionId(Data.Header.Players[I].Faction);
        Setup.Players[I].StartingCredits = Data.Header.Players[I].StartingCredits;
    }
    // An INDEPENDENTLY CONSTRUCTED settings object with an equal hash: this is
    // the contract the header pins ("equal hashes replay identically") -- reusing
    // the recording's own object would prove nothing about it.
    Recon::ReconSettings Reconstructed = MakeMinimalSettings(true);
    RA4_REQUIRE(Reconstructed.ComputeSettingsHash() == Data.Header.ReconSettingsHash);

    SimWorld Rebuilt;
    Rebuilt.Initialize(&Content, Setup, &Reconstructed);

    size_t NextFrame = 0;
    for (TickIndex Tick = 0; Tick < Data.FinalTick; ++Tick)
    {
        const CommandFrame* Frame = nullptr;
        if (NextFrame < Data.Frames.size() && Data.Frames[NextFrame].Tick == Tick)
        {
            Frame = &Data.Frames[NextFrame];
            ++NextFrame;
        }
        Rebuilt.Tick(Frame);
        Rebuilt.ClearEvents();

        Hash64 H;
        Rebuilt.GetRecon().GetPerceivedWorld(0).FeedChecksum(H);
        RA4_REQUIRE(size_t(Tick) < Live.BeliefChecksumPerTick.size());
        if (H.Get() != Live.BeliefChecksumPerTick[Tick])
        {
            RA4Test::ReportFailure("belief diverged from live run at tick " + std::to_string(Tick),
                                   __FILE__, __LINE__);
            return;
        }
    }
}

RA4_TEST(Recon, ReplayRefusesReconRulesetMismatch)
{
    // The three refusal modes, each as loud as a content-hash mismatch:
    // enabled-recording vs no settings, disabled-recording vs settings,
    // enabled-recording vs different tunables.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);

    const BeliefTimeline Live = RecordReconMatch(Content, Settings, 555, 60);
    ReplayData Data;
    std::string Error;
    RA4_REQUIRE(DeserializeReplay(Live.ReplayBytes, Data, Error));

    // 1. Recon recording, verifier given nothing.
    {
        const ReplayVerifyResult V = VerifyReplay(Data, Content, nullptr);
        RA4_EXPECT(!V.bSucceeded);
        RA4_EXPECT(V.Error.find("recon") != std::string::npos);
    }
    // 1b. Recon recording, settings object present but disabled: same refusal
    //     path as nullptr (disabled == absent), message must still say "intel".
    {
        Recon::ReconSettings Disabled = MakeMinimalSettings(false);
        const ReplayVerifyResult V = VerifyReplay(Data, Content, &Disabled);
        RA4_EXPECT(!V.bSucceeded);
        RA4_EXPECT(V.Error.find("recon") != std::string::npos);
    }
    // 2. Recon recording, different tunables.
    {
        Recon::ReconSettings Other = MakeMinimalSettings(true);
        Other.Tracks.StaleAfterTicks += 1;
        const ReplayVerifyResult V = VerifyReplay(Data, Content, &Other);
        RA4_EXPECT(!V.bSucceeded);
        RA4_EXPECT(V.Error.find("hash mismatch") != std::string::npos);
    }
    // 3. Matching settings verify clean.
    {
        const ReplayVerifyResult V = VerifyReplay(Data, Content, &Settings);
        if (!V.bSucceeded)
        {
            RA4Test::ReportFailure("intel replay failed to verify: " + V.Error + " at tick " +
                                       std::to_string(V.DivergedAtTick),
                                   __FILE__, __LINE__);
        }
        RA4_EXPECT(V.bSucceeded);
    }
    // 4. Classic recording, verifier wrongly given settings.
    {
        MatchSetup Setup = MakeTestSetup(556);
        SimWorld World;
        World.Initialize(&Content, Setup);
        ReplayRecorder Rec;
        Rec.Begin(MakeHeaderFromSetup(Setup, Content, "test"));
        for (int32_t I = 0; I < 40; ++I)
        {
            CommandFrame Frame;
            Frame.Tick = World.GetTick();
            World.Tick(nullptr);
            World.ClearEvents();
            Rec.RecordFrame(Frame);
            if ((World.GetTick() % kChecksumIntervalTicks) == 0)
            {
                Rec.RecordCheckpoint(World.GetTick(), World.ComputeStateChecksum());
            }
        }
        Rec.End(World.GetTick(), World.GetWinner());
        ReplayData Classic;
        RA4_REQUIRE(DeserializeReplay(Rec.Serialize(), Classic, Error));
        const ReplayVerifyResult V = VerifyReplay(Classic, Content, &Settings);
        RA4_EXPECT(!V.bSucceeded);
        RA4_EXPECT(V.Error.find("without the recon layer") != std::string::npos);
    }
}

RA4_TEST(Recon, PerceivedWorldRefusesForeignVersion)
{
    // Review MINOR from I-B4: the version-mismatch refusal existed but nothing
    // pinned it. A v2 (or corrupt-version) stream must be refused, not
    // misparsed as current.
    Recon::PerceivedWorld World;
    PerceivedWorldTestAccess::Initialize(World, 16, 16, 4);
    (void)PerceivedWorldTestAccess::AllocateTrack(World);

    ByteWriter W;
    World.Serialize(W);
    std::vector<uint8_t> Bytes = W.GetBuffer();

    // The version is the first uint32 of the stream; regress it by one.
    RA4_REQUIRE(Bytes.size() >= 4);
    uint32_t Version = 0;
    std::memcpy(&Version, Bytes.data(), sizeof(Version));
    Version -= 1;
    std::memcpy(Bytes.data(), &Version, sizeof(Version));

    Recon::PerceivedWorld Restored;
    ByteReader R(Bytes);
    RA4_EXPECT(!PerceivedWorldTestAccess::Deserialize(Restored, R));
}

// --- P-6: objective-state funnel inventory (leak detector, static half) ----------

RA4_TEST(Recon, ObjectiveStateFunnelInventory)
{
    // Docs/Architecture/VISIBILITY_CALLSITE_INVENTORY.md classifies every file
    // that reads objective entity state (GetAllCores / GetAllTransforms). This
    // test pins that list: a NEW file reaching for objective state fails here
    // until a human classifies it in the inventory and extends the whitelist.
    // It deliberately cannot judge HOW the data is used -- its job is to turn
    // "forgot to classify" from a silent leak into a build failure (RISK-17).
    // The runtime detector (per-read whitelisted-funnel enforcement) lands with
    // I-M6 and needs SimWorld instrumentation hooks.
    namespace fs = std::filesystem;

    static const char* Whitelist[] = {
        // Classified in VISIBILITY_CALLSITE_INVENTORY.md -- keep sorted.
        "Source/RA4AI/Private/AICommander.cpp",        // OWN x13
        "Source/RA4AI/Private/AIDebugOverlay.cpp",     // OMNISCIENT-BY-DESIGN (debug)
        "Source/RA4AI/Private/AIWorldView.cpp",        // FOG-GATED funnel (I-M6 site)
        "Source/RA4AI/Private/ValueMap.cpp",           // OWN
        "Source/RA4Campaign/Private/MissionRuntime.cpp", // OMNISCIENT-BY-DESIGN (referee)
        "Source/RA4Presentation/Private/HudSnapshot.cpp", // OWN + FOG-GATED minimap
        "Source/RA4Recon/Private/ReconSystem.cpp",     // OMNISCIENT-BY-DESIGN (produces belief)
        "Source/RA4Simulation/Private/SimWorld.cpp",   // the truth itself
        "Source/RA4Simulation/Public/RA4Simulation/SimWorld.h",
        "Source/RedAlert4/Private/RA4PlayerController.cpp", // OWN x2 + LEAK V-B (picking)
        "Source/RedAlert4/Private/RA4SimWorldSubsystem.cpp", // LEAK V-A (actor sync)
    };

    const fs::path Root = fs::current_path() / "Source";
    if (!fs::exists(Root))
    {
        RA4Test::ReportFailure("Source/ not found from test cwd", __FILE__, __LINE__);
        return;
    }

    std::vector<std::string> Offenders;
    for (auto It = fs::recursive_directory_iterator(Root); It != fs::recursive_directory_iterator(); ++It)
    {
        if (!It->is_regular_file())
        {
            continue;
        }
        const fs::path& Path = It->path();
        const std::string Ext = Path.extension().string();
        if (Ext != ".cpp" && Ext != ".h")
        {
            continue;
        }
        // Normalize to repo-relative with forward slashes.
        std::string Rel = fs::relative(Path, fs::current_path()).generic_string();
        if (Rel.rfind("Source/RA4Tests/", 0) == 0)
        {
            continue; // tests may read anything; they are not a player
        }

        std::ifstream F(Path);
        std::string Text((std::istreambuf_iterator<char>(F)), std::istreambuf_iterator<char>());
        if (Text.find("GetAllCores") == std::string::npos &&
            Text.find("GetAllTransforms") == std::string::npos)
        {
            continue;
        }

        bool bListed = false;
        for (const char* W : Whitelist)
        {
            if (Rel == W)
            {
                bListed = true;
                break;
            }
        }
        if (!bListed)
        {
            Offenders.push_back(Rel);
        }
    }

    for (const std::string& O : Offenders)
    {
        RA4Test::ReportFailure(
            "unclassified objective-state reader: " + O +
                " -- classify it in Docs/Architecture/VISIBILITY_CALLSITE_INVENTORY.md and extend the whitelist",
            __FILE__, __LINE__);
    }
    RA4_EXPECT(Offenders.empty());
}

// --- M2: distortion pipeline unit tests (each stage in isolation, §8) ---------------

namespace
{

Recon::ObserverState MakeObserver(int32_t MoralePM, int32_t CompetencePM, int32_t FatiguePM = 0,
                                  int32_t SuppressionPM = 0, int32_t DistancePM = 500)
{
    Recon::ObserverState O;
    O.Morale = Recon::PerMilleToFixed(MoralePM);
    O.Competence = Recon::PerMilleToFixed(CompetencePM);
    O.Fatigue = Recon::PerMilleToFixed(FatiguePM);
    O.Suppression = Recon::PerMilleToFixed(SuppressionPM);
    O.DistanceRatio = Recon::PerMilleToFixed(DistancePM);
    return O;
}

Recon::DistortionProfile MakeProfile()
{
    Recon::DistortionProfile P;
    P.Name = "profile.test";
    return P;
}

} // namespace

RA4_TEST(Recon, PerfectObserverReportsTruthBitwise)
{
    // Morale 1, competence 1, clarity 1 => the report equals the truth exactly
    // (§8 acceptance). Every stage must be an identity for the perfect observer.
    Recon::DistortionProfile P = MakeProfile();
    Recon::ObserverState O = MakeObserver(1000, 1000, 0, 0, 0);
    Random Rng(42);

    const Fixed Clarity = Recon::StageClarity(O, P);
    RA4_EXPECT(Clarity == Fixed::FromInt(1));

    for (int32_t Count : {1, 7, 250})
    {
        RA4_EXPECT(Recon::StageCountDistortion(Count, O, P, Rng) == Count);
    }

    Recon::ConfusionMatrix Identity; // defaults to identity rows
    for (int32_t C = 0; C < Recon::kObservedCategoryCount; ++C)
    {
        RA4_EXPECT(Recon::StageClassification(Recon::ObservedCategory(C), Clarity, Identity, P, Rng) ==
                   Recon::ObservedCategory(C));
    }

    const Vec2 Offset = Recon::StagePositionError(Clarity, O, P, Rng);
    RA4_EXPECT(Offset.X == Fixed::Zero() && Offset.Y == Fixed::Zero());

    // Omission chance is (fatigue/2 + (1-clarity)/2) * max: zero for the rested
    // perfect observer regardless of the roll.
    for (int32_t I = 0; I < 100; ++I)
    {
        RA4_EXPECT(!Recon::StageOmission(Clarity, O, P, Rng));
    }
}

RA4_TEST(Recon, FearInflatesCountsMonotonicallyAndNeverBelowTruthAlone)
{
    // §8: as morale falls, E[PerceivedCount] grows strictly; and with fear as
    // the only distortion (perfect competence => zero symmetric noise) the
    // perceived count NEVER drops below truth -- the asymmetry contract §4.3.2.
    Recon::DistortionProfile P = MakeProfile();
    const int32_t TrueCount = 100;
    const int32_t Samples = 2000;

    int64_t PrevMean = -1;
    for (int32_t MoralePM : {1000, 750, 500, 250, 0})
    {
        Recon::ObserverState O = MakeObserver(MoralePM, 1000);
        Random Rng(777); // same stream per morale level: paired comparison
        int64_t Sum = 0;
        int32_t Min = INT32_MAX;
        for (int32_t I = 0; I < Samples; ++I)
        {
            const int32_t V = Recon::StageCountDistortion(TrueCount, O, P, Rng);
            Sum += V;
            Min = V < Min ? V : Min;
        }
        RA4_EXPECT(Min >= TrueCount); // fear only ever inflates
        const int64_t Mean = Sum / Samples;
        if (PrevMean >= 0)
        {
            RA4_EXPECT(Mean >= PrevMean); // monotone in falling morale
        }
        if (MoralePM == 0)
        {
            RA4_EXPECT(Mean > PrevMean); // strictly at the extreme
        }
        PrevMean = Mean;
    }
}

RA4_TEST(Recon, IncompetenceNoiseIsSymmetricAroundTruth)
{
    // A calm but green observer errs both ways: mean stays near truth, spread
    // grows as competence falls.
    Recon::DistortionProfile P = MakeProfile();
    Recon::ObserverState O = MakeObserver(1000, 0); // zero competence, full morale
    Random Rng(1234);

    const int32_t TrueCount = 100;
    const int32_t Samples = 4000;
    int64_t Sum = 0;
    int32_t SawBelow = 0, SawAbove = 0;
    for (int32_t I = 0; I < Samples; ++I)
    {
        const int32_t V = Recon::StageCountDistortion(TrueCount, O, P, Rng);
        Sum += V;
        SawBelow += V < TrueCount ? 1 : 0;
        SawAbove += V > TrueCount ? 1 : 0;
    }
    const int64_t Mean = Sum / Samples;
    RA4_EXPECT(Mean > TrueCount - 8 && Mean < TrueCount + 8); // centred (±8%)
    RA4_EXPECT(SawBelow > Samples / 8 && SawAbove > Samples / 8); // genuinely two-sided
}

RA4_TEST(Recon, ConfusionMatrixHoldsAuthoredProbabilities)
{
    // §8: chi-squared check on 10^5 rolls against an authored row. At clarity 0
    // every roll consults the matrix, so observed frequencies must match the
    // authored per-milles within statistical noise.
    Recon::DistortionProfile P = MakeProfile();
    Recon::ConfusionMatrix M;
    // Authored row for HeavyVehicle: 60% right, 25% light vehicle, 10% structure,
    // 5% infantry -- the designer's "tanks get mistaken for trucks" row.
    const int32_t Row = int32_t(Recon::ObservedCategory::HeavyVehicle);
    for (int32_t C = 0; C < Recon::kObservedCategoryCount; ++C) { M.PerMille[Row][C] = 0; }
    M.PerMille[Row][int32_t(Recon::ObservedCategory::HeavyVehicle)] = 600;
    M.PerMille[Row][int32_t(Recon::ObservedCategory::LightVehicle)] = 250;
    M.PerMille[Row][int32_t(Recon::ObservedCategory::Structure)] = 100;
    M.PerMille[Row][int32_t(Recon::ObservedCategory::Infantry)] = 50;

    Random Rng(31337);
    const int32_t N = 100000;
    int32_t Counts[Recon::kObservedCategoryCount] = {};
    for (int32_t I = 0; I < N; ++I)
    {
        const Recon::ObservedCategory C = Recon::StageClassification(
            Recon::ObservedCategory::HeavyVehicle, Fixed::Zero(), M, P, Rng);
        Counts[int32_t(C)] += 1;
    }

    // Chi-squared statistic over the four non-zero cells. Critical value for
    // 3 degrees of freedom at p=0.001 is 16.27; integer math scaled by 1000.
    int64_t ChiSquaredMilli = 0;
    for (int32_t C = 0; C < Recon::kObservedCategoryCount; ++C)
    {
        const int64_t Expected = int64_t(M.PerMille[Row][C]) * N / 1000;
        if (Expected == 0)
        {
            RA4_EXPECT(Counts[C] == 0); // authored-zero cells must never fire
            continue;
        }
        const int64_t Delta = Counts[C] - Expected;
        ChiSquaredMilli += Delta * Delta * 1000 / Expected;
    }
    RA4_EXPECT(ChiSquaredMilli < 16270);
}

RA4_TEST(Recon, DistortionIsDeterministicPerSeed)
{
    // Same seed => bitwise identical outputs, across two independent rng streams
    // (stands in for the two-process determinism requirement, which the lockstep
    // suite covers at the SimWorld level).
    Recon::DistortionProfile P = MakeProfile();
    Recon::ObserverState O = MakeObserver(300, 400, 600, 500);
    Random A(2026), B(2026);
    for (int32_t I = 0; I < 500; ++I)
    {
        RA4_EXPECT(Recon::StageCountDistortion(50, O, P, A) == Recon::StageCountDistortion(50, O, P, B));
        const Vec2 Pa = Recon::StagePositionError(Fixed::FromRatio(1, 2), O, P, A);
        const Vec2 Pb = Recon::StagePositionError(Fixed::FromRatio(1, 2), O, P, B);
        RA4_EXPECT(Pa.X == Pb.X && Pa.Y == Pb.Y);
        RA4_EXPECT(Recon::StageOmission(Fixed::FromRatio(1, 4), O, P, A) ==
                   Recon::StageOmission(Fixed::FromRatio(1, 4), O, P, B));
    }
}

RA4_TEST(Recon, EveryStageHonoursItsDisableFlag)
{
    // §4.7: playtests must be able to kill exactly one stage. Disabled stages are
    // identities even for the worst observer.
    Recon::DistortionProfile P = MakeProfile();
    P.bClarityEnabled = false;
    P.bCountDistortionEnabled = false;
    P.bClassificationErrorEnabled = false;
    P.bPositionErrorEnabled = false;
    P.bOmissionEnabled = false;

    Recon::ObserverState Worst = MakeObserver(0, 0, 1000, 1000, 1000);
    Random Rng(1);

    RA4_EXPECT(Recon::StageClarity(Worst, P) == Fixed::FromInt(1));
    RA4_EXPECT(Recon::StageCountDistortion(10, Worst, P, Rng) == 10);
    Recon::ConfusionMatrix Chaos;
    for (int32_t R = 0; R < Recon::kObservedCategoryCount; ++R)
        for (int32_t C = 0; C < Recon::kObservedCategoryCount; ++C)
            Chaos.PerMille[R][C] = (C == (R + 1) % Recon::kObservedCategoryCount) ? 1000 : 0;
    RA4_EXPECT(Recon::StageClassification(Recon::ObservedCategory::Infantry, Fixed::Zero(), Chaos, P, Rng) ==
               Recon::ObservedCategory::Infantry);
    const Vec2 Off = Recon::StagePositionError(Fixed::Zero(), Worst, P, Rng);
    RA4_EXPECT(Off.X == Fixed::Zero() && Off.Y == Fixed::Zero());
    RA4_EXPECT(!Recon::StageOmission(Fixed::Zero(), Worst, P, Rng));
}

// --- M2: morale model unit tests ---------------------------------------------------

RA4_TEST(Recon, MoraleFallsUnderFireAndRecoversInQuiet)
{
    Recon::MoraleTuning T; // defaults from config header
    Recon::MoraleComp M;

    Recon::MoraleApplyDamage(M, 100, T);
    const Fixed AfterHit = M.Morale;
    RA4_EXPECT(AfterHit < Fixed::FromInt(1));
    RA4_EXPECT(M.Suppression > Fixed::Zero());
    RA4_EXPECT(M.TicksUnderFire == 0);

    // Under continuous stimuli fatigue climbs and no recovery happens.
    for (int32_t I = 0; I < 30; ++I)
    {
        Recon::MoraleMarkUnderFire(M);
        Recon::MoraleTickRecovery(M, T);
    }
    RA4_EXPECT(M.Fatigue > Fixed::Zero());
    RA4_EXPECT(M.Morale == AfterHit); // no regen while in contact

    // Quiet: after the out-of-fire delay, morale climbs and fatigue falls.
    for (int32_t I = 0; I < T.OutOfFireDelayTicks + 200; ++I)
    {
        Recon::MoraleTickRecovery(M, T);
    }
    RA4_EXPECT(M.Morale > AfterHit);
    RA4_EXPECT(M.Suppression == Fixed::Zero());
}

RA4_TEST(Recon, AllyDeathHurtsMoreThanAScratch)
{
    Recon::MoraleTuning T;
    Recon::MoraleComp Scratched, Bereaved;
    Recon::MoraleApplyDamage(Scratched, 10, T);   // a 10-point scratch
    Recon::MoraleApplyAllyDeath(Bereaved, T);     // watching a neighbour die
    RA4_EXPECT(Bereaved.Morale < Scratched.Morale);
}

RA4_TEST(Recon, SuperiorityDreadNeedsThresholdAndIsBounded)
{
    Recon::MoraleTuning T;
    Recon::MoraleComp M;

    // Below threshold (equal numbers): no effect.
    Recon::MoraleApplySuperiority(M, 10, 10, T);
    RA4_EXPECT(M.Morale == Fixed::FromInt(1));

    // 3x outnumbered: erodes per tick, clamped at zero, and does NOT reset the
    // quiet counter (dread is not a shellburst).
    M.TicksUnderFire = -1;
    for (int32_t I = 0; I < 5000; ++I)
    {
        Recon::MoraleApplySuperiority(M, 30, 10, T);
    }
    RA4_EXPECT(M.Morale == Fixed::Zero());
    RA4_EXPECT(M.TicksUnderFire == -1);
}

RA4_TEST(Recon, ScaredPlayerSeesInflatedContacts)
{
    // Integration: a frightened force overstates what it sees. Two identical
    // worlds, but in one the observing player's units have been shelled; its
    // belief about enemy COUNT must (in expectation) exceed the calm one's.
    // Uses the count interval on tracks after the full in-sim pipeline.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Enabled = MakeMinimalSettings(true);
    Recon::DistortionProfile& Profile = Enabled.DistortionProfiles[0];
    Profile.bCountDistortionEnabled = true; // the one stage under test
    Profile.FearCountBiasMaxPerMille = 3000; // up to 4x at full fear
    Profile.CompetenceNoiseMaxPerMille = 0;   // fear only
    // Crater morale fast without killing the victim: many weak hits, huge
    // per-hit penalty, no regen. A true count of 1 only inflates once the
    // multiplier crosses 2, which needs morale near zero (FearLevel 0.5).
    Enabled.Morale.DamageMoralePenaltyPerMille = 5000;
    Enabled.Morale.MoraleRegenPerTickPerMille = 0;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(9090), &Enabled);
    World.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    const EntityId Enemy =
        World.SpawnUnit(Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    (void)Enemy;

    // Shell the conscript into the dirt (without killing it) to crater morale.
    const EntityId Victim = World.MakeId(0);
    for (int32_t I = 0; I < 15; ++I)
    {
        World.DebugDamage(Victim, 2); // 30 total: far below lethal
        World.Tick(nullptr);
    }
    RA4_REQUIRE(World.IsAlive(Victim)); // a dead observer reports nothing

    // Across a window of ticks, the scared observer's believed count must
    // exceed truth at least once (fear is stochastic per report, bias is up).
    bool SawInflation = false;
    for (int32_t I = 0; I < 40 && !SawInflation; ++I)
    {
        World.Tick(nullptr);
        std::vector<const Recon::PerceivedTrack*> Found;
        World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
        for (const Recon::PerceivedTrack* Tr : Found)
        {
            if (Tr->BelievedCountMax > 1)
            {
                SawInflation = true;
            }
        }
    }
    RA4_EXPECT(SawInflation);
}
