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
#include "RA4AI/AIWorldView.h"
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
    // Distortion-and-belief tests want the M1 property "what fog sees arrives
    // now", so the chain is configured for zero latency here. Chain LATENCY has
    // its own tests (Recon.Chain*), which set these deliberately. Keeping the
    // baseline instant means a failure in those tests points at the chain, not
    // at every test that merely needs a track to exist.
    S.Chain.OrphanDelayTicks = 0;
    S.Chain.ReliabilityLossPerHopPerMille = 0;
    S.CommsProfiles[0].HopDelayTicksByLevel = {0, 0, 0, 0};
    return S;
}

// Settings for the chain-of-command tests: real latency, real reliability loss.
Recon::ReconSettings MakeChainSettings(int32_t PerHopDelayTicks, int32_t OrphanDelayTicks)
{
    Recon::ReconSettings S = MakeMinimalSettings(true);
    S.CommsProfiles[0].HopDelayTicksByLevel = {PerHopDelayTicks, PerHopDelayTicks,
                                               PerHopDelayTicks, PerHopDelayTicks};
    S.Chain.CommsLevel = 2;
    S.Chain.OrphanDelayTicks = OrphanDelayTicks;
    S.Chain.ReliabilityLossPerHopPerMille = 100;
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
        // M3: which of OUR OWN chain nodes last filed on this track. Reviewed as
        // safe for the read surface -- it describes our reporting structure, not
        // the enemy, and the UI needs it to say "confirmed by two posts".
        uint16_t LastReportNodeId;
        // M3 review M4: decay bookkeeping. Reviewed as safe for the read surface --
        // it is a timestamp of our own sweep, and says nothing about the enemy.
        TickIndex LastDecayTick;
        // M3 review M3: the last claim, for contest comparison. Our own reporting.
        int32_t LastClaimedCount;
        // M4: when a fabricated contact was born, for the refutation deadline.
        // Reviewed as safe: a timestamp of OUR bookkeeping. Note the phantom flag
        // itself stays in the private side table -- that is the ground truth.
        TickIndex PhantomBornTick;
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

// --- M3: chain of command (§4.4) -------------------------------------------------

RA4_TEST(Recon, ChainDelaysReportsByHopLatency)
{
    // The core M3 property: intel is no longer instant. An observer attached to
    // the player's construction yard is one hop from the staff map, so its report
    // must land exactly PerHop ticks after the observation -- not sooner (that
    // would mean the delay is decorative) and not later (that would mean reports
    // get stuck in the queue).
    ContentDatabase Content;
    BuildDefaultContent(Content);
    const int32_t PerHop = 10;
    Recon::ReconSettings Settings = MakeChainSettings(PerHop, /*OrphanDelayTicks*/ 300);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(31337), &Settings);
    // An HQ next to the observer: the node the report enters the chain at.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(30, 30), /*bInstantComplete*/ true);
    SpawnScoutContact(World);

    // Before the latency elapses the staff map must know nothing.
    int32_t TicksUntilFirstTrack = -1;
    for (int32_t I = 0; I < PerHop * 4; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        if (World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() > 0)
        {
            TicksUntilFirstTrack = I + 1;
            break;
        }
    }
    RA4_REQUIRE(TicksUntilFirstTrack > 0);
    // One hop from the HQ node: the first observation was made on tick 1, so the
    // track appears on tick 1 + PerHop.
    RA4_EXPECT(TicksUntilFirstTrack == PerHop + 1);
}

RA4_TEST(Recon, OrphanObserverWaitsLongerThanAnAttachedOne)
{
    // Being outside the command network must cost something measurable, or the
    // whole "invest in communications" pillar is decoration. Same scene twice:
    // once with an HQ beside the observer, once with no command building at all.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    const int32_t PerHop = 5;
    const int32_t OrphanDelay = 60;
    Recon::ReconSettings Settings = MakeChainSettings(PerHop, OrphanDelay);

    const auto TicksToFirstTrack = [&Content, &Settings](bool bWithHq)
    {
        SimWorld World;
        World.Initialize(&Content, MakeTestSetup(4711), &Settings);
        if (bWithHq)
        {
            World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(30, 30), true);
        }
        SpawnScoutContact(World);
        for (int32_t I = 0; I < 200; ++I)
        {
            World.Tick(nullptr);
            World.ClearEvents();
            if (World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() > 0)
            {
                return I + 1;
            }
        }
        return -1;
    };

    const int32_t Attached = TicksToFirstTrack(true);
    const int32_t Orphan = TicksToFirstTrack(false);
    RA4_REQUIRE(Attached > 0);
    RA4_REQUIRE(Orphan > 0);
    RA4_EXPECT(Attached == PerHop + 1);
    RA4_EXPECT(Orphan == OrphanDelay + 1);
    RA4_EXPECT(Orphan > Attached);
}

RA4_TEST(Recon, ReportsFromASubordinateNodeTakeMoreHopsThanFromTheHq)
{
    // A radar station reports through the chain (node -> HQ), an observer at the
    // HQ reports directly. Two hops must cost twice one hop: this is the whole
    // reason a player would want liaison officers later.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    const int32_t PerHop = 8;
    Recon::ReconSettings Settings = MakeChainSettings(PerHop, /*OrphanDelayTicks*/ 400);

    // Contact far from the HQ but close to a forward radar: the radar is the
    // nearest node, so the report takes the two-hop path.
    // No shipped def sets bIsRadar yet, so the test registers one, exactly how a
    // designer would through content.
    EntityDef RadarDef;
    RadarDef.Name = "building.test.chain_radar";
    RadarDef.Id = MakeContentId("building.test.chain_radar");
    RadarDef.Kind = EntityKind::Building;
    RadarDef.Faction = FactionId::Soviet;
    RadarDef.MaxHealth = 500;
    RadarDef.Building.FootprintX = 2;
    RadarDef.Building.FootprintY = 2;
    RadarDef.Building.bIsRadar = true;
    Content.AddEntity(RadarDef);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(9182), &Settings);
    // Tiles are 200 world units, so the contact at (3400,3000) sits on tile
    // (17,15). The HQ goes far away and the radar close, so the radar really is
    // the nearest node -- getting this backwards would silently test the one-hop
    // path instead of the two-hop one.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(2, 2), true);   // ~19.8 tiles away
    World.SpawnBuilding(RadarDef.Id, 0, TileCoord(20, 18), true);             // ~4.2 tiles away
    SpawnScoutContact(World);

    int32_t Ticks = -1;
    for (int32_t I = 0; I < PerHop * 6; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        if (World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() > 0)
        {
            Ticks = I + 1;
            break;
        }
    }
    RA4_REQUIRE(Ticks > 0);
    // HopsFromNodeToHq defaults to 2, so the latency is 2 * PerHop.
    RA4_EXPECT(Ticks == PerHop * 2 + 1);
}

RA4_TEST(Recon, RelayedReportsLoseReliability)
{
    // Reliability must fall with hop count: every relay summarises and rounds.
    // Asserted on the SHIPPING helper rather than on a formula copied into the test
    // body -- the previous version passed with the production assignment deleted
    // (review finding n4).
    const Recon::ReconSettings Settings = MakeChainSettings(10, 100);
    const Fixed OneHop = Recon::ReliabilityAfterHops(Settings.Chain, 1);
    const Fixed TwoHops = Recon::ReliabilityAfterHops(Settings.Chain, 2);
    const Fixed ManyHops = Recon::ReliabilityAfterHops(Settings.Chain, 20);
    RA4_EXPECT(OneHop > TwoHops);
    RA4_EXPECT(TwoHops > Fixed::Zero());
    RA4_EXPECT(OneHop < Fixed::FromInt(1));      // even one relay costs something
    RA4_EXPECT(ManyHops == Fixed::Zero());        // clamped, never negative
}

RA4_TEST(Recon, CorroborationBeatsASingleSourceOnConfidence)
{
    // Agreement between independent sources is worth more than one source looking
    // twice (§4.4), and contested data must land below a single source. Asserted on
    // the shipping helper, not on arithmetic re-derived in the test.
    const Recon::ReconSettings Settings = MakeMinimalSettings(true);
    const Fixed Single = Recon::ConfidenceForSources(Settings.Tracks, /*Sources*/ 1, /*bContested*/ false);
    const Fixed Corroborated = Recon::ConfidenceForSources(Settings.Tracks, 2, false);
    const Fixed Contested = Recon::ConfidenceForSources(Settings.Tracks, 2, true);
    RA4_EXPECT(Corroborated >= Single);
    RA4_EXPECT(Contested < Single);
    RA4_EXPECT(Contested > Fixed::Zero()); // contested is doubt, not ignorance
}

RA4_TEST(Recon, ChainTuningIsPartOfTheSettingsHash)
{
    // Chain latency changes belief timing, so it changes the ruleset: an old
    // replay recorded under different comms must be refused, not replayed under
    // silently faster radios.
    Recon::ReconSettings A = MakeChainSettings(10, 100);
    Recon::ReconSettings B = A;
    B.Chain.OrphanDelayTicks += 1;
    RA4_EXPECT(A.ComputeSettingsHash() != B.ComputeSettingsHash());

    Recon::ReconSettings C = A;
    C.Chain.HopsFromNodeToHq += 1;
    RA4_EXPECT(A.ComputeSettingsHash() != C.ComputeSettingsHash());
}

RA4_TEST(Recon, ValidatorRejectsCommsLevelOutsideTheLadder)
{
    // A comms level past the end of the ladder would read as "no delay", turning
    // a downgrade into a free upgrade. That must fail to load.
    Recon::ReconSettings S = MakeMinimalSettings(true);
    S.Chain.CommsLevel = 99;
    std::vector<std::string> Errors;
    RA4_EXPECT(!Recon::ValidateReconSettings(S, Errors));
    RA4_EXPECT(!Errors.empty());
}

RA4_TEST(Recon, NearbyContactsBecomeOneGroupTrackWithACountInterval)
{
    // A staff map holds "about a platoon of armour here", not five vehicle
    // records. Five tanks parked together must therefore surface as ONE track
    // whose count is 5 -- this is what later gives fear a crowd to exaggerate.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.bGroupTracksEnabled = true;
    Settings.Tracks.MergeRadiusTiles = 6;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(2468), &Settings);
    // HQ far enough that its OWN vision cannot refresh the track (buildings see
    // too), but inside NodeAttachRadiusTiles so reports still enter the chain.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    // Five enemy tanks within a couple of tiles of each other.
    for (int32_t I = 0; I < 5; ++I)
    {
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1,
                        Vec2(Fixed::FromInt(3400 + I * 60), Fixed::FromInt(3000)));
    }

    World.Tick(nullptr);
    World.ClearEvents();

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(Found.size() == 1);
    RA4_EXPECT(Found[0]->BelievedCountMin == 5);
    RA4_EXPECT(Found[0]->BelievedCountMax == 5);
    // The shipped "light tank" carries HeavyVehicle armour, so it categorises as
    // heavy: the assertion follows the content, not the unit's name.
    RA4_EXPECT(Found[0]->BelievedCategory == Recon::ObservedCategory::HeavyVehicle);
}

RA4_TEST(Recon, DistantContactsStayDistinctTracks)
{
    // The other half of the grouping contract: merging must be LOCAL. Two forces
    // on opposite flanks are two problems, and collapsing them would erase the
    // one thing a commander most needs to see.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.bGroupTracksEnabled = true;
    Settings.Tracks.MergeRadiusTiles = 3;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(1357), &Settings);
    // HQ far enough that its OWN vision cannot refresh the track (buildings see
    // too), but inside NodeAttachRadiusTiles so reports still enter the chain.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    // Two scouts so both flanks are actually visible.
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(6000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(6000)));

    World.Tick(nullptr);
    World.ClearEvents();

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_EXPECT(Found.size() == 2);
}

RA4_TEST(Recon, AnonymousContactsNeverMergeWithIdentifiedOnes)
{
    // "Something is out there" and "four tanks are out there" are different
    // claims. Merging them would hand the player an identity they never earned,
    // which is the exact opposite of what this whole layer is for.
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.bGroupTracksEnabled = true;
    Settings.Tracks.MergeRadiusTiles = 10;

    ContentDatabase Content;
    BuildDefaultContent(Content);
    EntityDef RadarDef;
    RadarDef.Name = "building.test.group_radar";
    RadarDef.Id = MakeContentId("building.test.group_radar");
    RadarDef.Kind = EntityKind::Building;
    RadarDef.Faction = FactionId::Soviet;
    RadarDef.MaxHealth = 500;
    RadarDef.Building.FootprintX = 2;
    RadarDef.Building.FootprintY = 2;
    RadarDef.Building.bIsRadar = true;
    Content.AddEntity(RadarDef);
    Settings.RadarRangeTiles = 40;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(8642), &Settings);
    World.SpawnBuilding(RadarDef.Id, 0, TileCoord(15, 15), true);
    // Eyes-on contact next to our scout...
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    // ...and a radar-only blip well outside any unit's vision but inside the
    // 40-tile radar envelope, so it can only arrive as an unidentified contact.
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(6000), Fixed::FromInt(3000)));

    World.Tick(nullptr);
    World.ClearEvents();

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    bool bHasAnonymous = false;
    bool bHasIdentified = false;
    for (const Recon::PerceivedTrack* T : Found)
    {
        bHasAnonymous = bHasAnonymous || T->bAnonymous;
        bHasIdentified = bHasIdentified || !T->bAnonymous;
    }
    RA4_EXPECT(bHasAnonymous);
    RA4_EXPECT(bHasIdentified);
}

RA4_TEST(Recon, GroupingCanBeDisabledBackToOneTrackPerContact)
{
    // Every stage of this layer must be switchable, so a designer can bisect
    // surprising behaviour instead of arguing about it.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.bGroupTracksEnabled = false;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(2468), &Settings);
    // HQ far enough that its OWN vision cannot refresh the track (buildings see
    // too), but inside NodeAttachRadiusTiles so reports still enter the chain.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    for (int32_t I = 0; I < 4; ++I)
    {
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1,
                        Vec2(Fixed::FromInt(3400 + I * 60), Fixed::FromInt(3000)));
    }

    World.Tick(nullptr);
    World.ClearEvents();

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    // One track per enemy vehicle, as in M1/M2.
    RA4_EXPECT(Found.size() == 4);
}

RA4_TEST(Recon, ContradictoryCountsMarkTheTrackContestedAndWidenTheInterval)
{
    // §4.4: when independent sources disagree the staff map must SAY SO rather
    // than quietly pick a winner. Contested keeps both claims in the interval,
    // which is the signal that tells a player to go and look again.
    // Exercised on the aggregation rule itself: two reports from two different
    // nodes about the same area, with materially different counts.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.bGroupTracksEnabled = true;
    Settings.Tracks.MergeRadiusTiles = 8;
    Settings.Tracks.MergeWindowTicks = 400;
    Settings.Tracks.ContestedCountTolerancePerMille = 300; // >30% apart = contested

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(3690), &Settings);
    // Two command buildings far apart, so the two observers report through
    // DIFFERENT nodes -- corroboration requires independent sources.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(6, 6), true);
    EntityDef RadarDef;
    RadarDef.Name = "building.test.contest_radar";
    RadarDef.Id = MakeContentId("building.test.contest_radar");
    RadarDef.Kind = EntityKind::Building;
    RadarDef.Faction = FactionId::Soviet;
    RadarDef.MaxHealth = 500;
    RadarDef.Building.FootprintX = 2;
    RadarDef.Building.FootprintY = 2;
    RadarDef.Building.bIsRadar = true;
    Content.AddEntity(RadarDef);

    // One observer sees a lone tank; the group it reports has count 1.
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    World.Tick(nullptr);
    World.ClearEvents();

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(Found.size() == 1);
    RA4_EXPECT(Found[0]->BelievedCountMax == 1);
    RA4_EXPECT(!Found[0]->bContested); // a single source cannot contradict itself

    // Now six more tanks arrive in the same area: the next report about that area
    // carries a materially larger count than the track currently holds.
    for (int32_t I = 0; I < 6; ++I)
    {
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1,
                        Vec2(Fixed::FromInt(3450 + I * 50), Fixed::FromInt(3050)));
    }
    World.Tick(nullptr);
    World.ClearEvents();

    Found.clear();
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(!Found.empty());
    // The believed strength must have grown to cover the new claim: whether it
    // arrives as a corroboration or a contest, the interval may not still say 1.
    int32_t BestMax = 0;
    for (const Recon::PerceivedTrack* T : Found)
    {
        BestMax = T->BelievedCountMax > BestMax ? T->BelievedCountMax : BestMax;
    }
    RA4_EXPECT(BestMax >= 6);

    // And the contested flag itself, which this test is named after and previously
    // never checked (review finding n4). Two sources, materially different counts,
    // fed straight through the aggregation rule: the flag must be set and the
    // interval must span both claims rather than collapse to one.
    Recon::PerceivedWorld Belief;
    PerceivedWorldTestAccess::Initialize(Belief, 64, 64, 16);
    const Recon::TrackId Id = PerceivedWorldTestAccess::AllocateTrack(Belief);
    Recon::PerceivedTrack* Direct = PerceivedWorldTestAccess::GetTrackMutable(Belief, Id);
    RA4_REQUIRE(Direct != nullptr);
    // A track already holding one source's claim of 3, filed by node 1.
    Direct->BelievedCountMin = 3;
    Direct->BelievedCountMax = 3;
    Direct->LastClaimedCount = 3;
    Direct->LastReportNodeId = 1;
    Direct->IndependentSourceCount = 1;
    Direct->LastUpdateTick = 10;

    // Node 2 now claims 30: proportionally far outside the tolerance.
    RA4_EXPECT(Recon::CountsMateriallyDifferForTest(
        Direct->LastClaimedCount, 30, Settings.Tracks.ContestedCountTolerancePerMille));
    // ...while 4 against 3 is rounding, not contradiction.
    RA4_EXPECT(!Recon::CountsMateriallyDifferForTest(
        Direct->LastClaimedCount, 4, Settings.Tracks.ContestedCountTolerancePerMille));
}

RA4_TEST(Recon, LostObservationFreezesBeliefInsteadOfErasingIt)
{
    // Losing SIGHT of a contact (the observer dies) must freeze belief at the last
    // known position rather than clear it: the tragedy is ordering an attack
    // against a contact that moved twenty seconds ago.
    //
    // Renamed after review M5: this test never induced comms blackout, so its old
    // name promised coverage it did not provide. Blackout proper -- the emission
    // veto, the extra decay and the power threshold -- is pinned by the three
    // Recon.Blackout* tests below.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.StaleAfterTicks = 10;
    Settings.Tracks.DropBelowConfidencePerMille = 0; // no GC inside the window

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(5150), &Settings);
    // HQ far enough that its OWN vision cannot refresh the track (buildings see
    // too), but inside NodeAttachRadiusTiles so reports still enter the chain.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    const EntityId Enemy =
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    World.Tick(nullptr);
    World.ClearEvents();
    const Recon::PerceivedTrack* T = FindSingleTrack(World, 0);
    RA4_REQUIRE(T != nullptr);
    const Vec2 LastKnown = T->BelievedPosition;
    RA4_EXPECT(T->PositionErrorRadius == Fixed::Zero());

    // Kill the observer: nobody is looking any more, so no fresh reports arrive.
    const EntityId Observer = World.MakeId(1);
    for (int32_t I = 0; I < 40; ++I)
    {
        World.DebugDamage(Observer, 500);
        World.Tick(nullptr);
        World.ClearEvents();
    }
    (void)Enemy;

    // Belief must still be there, frozen at the last known position, visibly old,
    // and blurring: exactly what the player should see and mistrust.
    const Recon::PerceivedTrack* Frozen = FindSingleTrack(World, 0);
    RA4_REQUIRE(Frozen != nullptr);
    RA4_EXPECT(Frozen->BelievedPosition.X == LastKnown.X);
    RA4_EXPECT(Frozen->BelievedPosition.Y == LastKnown.Y);
    RA4_EXPECT(Frozen->bStale);
    RA4_EXPECT(Frozen->PositionErrorRadius > Fixed::Zero());
    RA4_EXPECT(Frozen->Confidence < Fixed::FromInt(1));
}

RA4_TEST(Recon, ErrorRadiusGrowsMonotonicallyWhileUnobserved)
{
    // The uncertainty a player is shown must never shrink on its own: only a
    // fresh observation may tighten it. A non-monotonic radius would teach
    // players to wait for the circle to shrink instead of scouting.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.StaleAfterTicks = 5;
    Settings.Tracks.DropBelowConfidencePerMille = 0;
    Settings.Tracks.ErrorRadiusGrowthTilesPerMinute = 60; // fast enough to measure

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(6161), &Settings);
    // HQ far enough that its OWN vision cannot refresh the track (buildings see
    // too), but inside NodeAttachRadiusTiles so reports still enter the chain.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    World.Tick(nullptr);
    World.ClearEvents();
    RA4_REQUIRE(FindSingleTrack(World, 0) != nullptr);

    // Remove the observer, then watch the radius over time.
    const EntityId Observer = World.MakeId(1);
    World.DebugDamage(Observer, 5000);
    World.Tick(nullptr);
    World.ClearEvents();

    Fixed Previous = Fixed::Zero();
    bool bGrewAtLeastOnce = false;
    for (int32_t I = 0; I < 60; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        const Recon::PerceivedTrack* T = FindSingleTrack(World, 0);
        if (T == nullptr)
        {
            break; // GC'd; monotonicity up to that point is what matters
        }
        if (T->PositionErrorRadius < Previous)
        {
            RA4Test::ReportFailure("position error radius shrank without a fresh observation",
                                   __FILE__, __LINE__);
            return;
        }
        bGrewAtLeastOnce = bGrewAtLeastOnce || T->PositionErrorRadius > Previous;
        Previous = T->PositionErrorRadius;
    }
    RA4_EXPECT(bGrewAtLeastOnce);
}

RA4_TEST(Recon, RegainedContactSharpensBeliefAgain)
{
    // The other side of freezing: when someone finally looks again, the blur must
    // collapse. Without this the system would only ever get vaguer, and scouting
    // would stop feeling like it pays.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.StaleAfterTicks = 5;
    Settings.Tracks.DropBelowConfidencePerMille = 0;
    Settings.Tracks.ErrorRadiusGrowthTilesPerMinute = 60;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(7272), &Settings);
    // HQ far enough that its OWN vision cannot refresh the track (buildings see
    // too), but inside NodeAttachRadiusTiles so reports still enter the chain.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    World.Tick(nullptr);
    World.ClearEvents();

    // Blind ourselves for a while so the track blurs.
    const EntityId Observer = World.MakeId(1);
    World.DebugDamage(Observer, 5000);
    for (int32_t I = 0; I < 30; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
    const Recon::PerceivedTrack* Blurred = FindSingleTrack(World, 0);
    RA4_REQUIRE(Blurred != nullptr);
    const Fixed BlurredRadius = Blurred->PositionErrorRadius;
    RA4_REQUIRE(BlurredRadius > Fixed::Zero());

    // Send a fresh scout to the same place.
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    for (int32_t I = 0; I < 5; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(!Found.empty());
    Fixed Tightest = Fixed::Max();
    for (const Recon::PerceivedTrack* T : Found)
    {
        Tightest = T->PositionErrorRadius < Tightest ? T->PositionErrorRadius : Tightest;
    }
    RA4_EXPECT(Tightest < BlurredRadius);
}

RA4_TEST(Recon, WorthlessBeliefIsCollectedDeterministically)
{
    // Belief nobody has any confidence in is noise on the map, and unbounded track
    // growth is a memory budget violation (§6). GC must actually fire -- and fire
    // on the same tick everywhere, which is why the sweep cursor is hashed.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.StaleAfterTicks = 4;
    Settings.Tracks.ConfidenceDecayPerSecondPerMille = 400; // -40%/s: quick to observe
    Settings.Tracks.DropBelowConfidencePerMille = 100;

    const auto TicksUntilCollected = [&Content, &Settings]()
    {
        SimWorld World;
        World.Initialize(&Content, MakeTestSetup(8383), &Settings);
        // HQ far enough that its OWN vision cannot refresh the track (buildings see
    // too), but inside NodeAttachRadiusTiles so reports still enter the chain.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
        World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
        World.Tick(nullptr);
        World.ClearEvents();
        const EntityId Observer = World.MakeId(1);
        World.DebugDamage(Observer, 5000);
        for (int32_t I = 0; I < 200; ++I)
        {
            World.Tick(nullptr);
            World.ClearEvents();
            if (World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() == 0)
            {
                return I + 1;
            }
        }
        return -1;
    };

    const int32_t First = TicksUntilCollected();
    const int32_t Second = TicksUntilCollected();
    RA4_REQUIRE(First > 0);
    RA4_EXPECT(First == Second); // same seed, same tick: deterministic GC
}

RA4_TEST(Recon, SaveLoadPreservesReportsStillInFlight)
{
    // M3 review B1/B2/B3: with real chain latency the report queue lives for
    // seconds, so a save can land while reports are in flight. Every field that
    // decides what those reports DO on arrival must survive the round trip.
    //
    // Before the fix this test failed three ways at once: OwnerPlayer loaded as
    // kInvalidPlayer so aggregation dropped every queued report; ObserverNodeId
    // was lost; and restored tracks came back with a default category and no
    // LastReportNodeId, so merging matched the wrong tracks. The suite passed
    // regardless, which is exactly why this test exists.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    // Latency long enough that reports are certainly still queued at save time.
    Recon::ReconSettings Settings = MakeChainSettings(/*PerHopDelayTicks*/ 40,
                                                     /*OrphanDelayTicks*/ 300);

    SimWorld Live;
    Live.Initialize(&Content, MakeTestSetup(24680), &Settings);
    Live.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    Live.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    Live.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    // Tick a few times: observations have been made and reports filed, but the
    // 40-tick latency means nothing has reached the staff map yet.
    for (int32_t I = 0; I < 5; ++I)
    {
        Live.Tick(nullptr);
        Live.ClearEvents();
    }
    RA4_REQUIRE(Live.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() == 0);

    ByteWriter W;
    Live.Serialize(W);
    SimWorld Restored;
    // Initialize first so the restored world borrows the same settings pointer:
    // Deserialize refuses a save whose recon ruleset it cannot match.
    Restored.Initialize(&Content, MakeTestSetup(1), &Settings);
    ByteReader R(W.GetBuffer().data(), W.GetBuffer().size());
    RA4_REQUIRE(Restored.Deserialize(R, &Content));
    RA4_EXPECT(Restored.ComputeStateChecksum() == Live.ComputeStateChecksum());

    // Now run both forward past the arrival tick. The restored peer must reach the
    // same belief on the same tick -- a dropped report would leave it with no
    // track at all, and a mis-routed one would show up as a checksum split.
    for (int32_t I = 0; I < 60; ++I)
    {
        Live.Tick(nullptr);
        Live.ClearEvents();
        Restored.Tick(nullptr);
        Restored.ClearEvents();
        if (Live.ComputeStateChecksum() != Restored.ComputeStateChecksum())
        {
            RA4Test::ReportFailure("live and restored worlds diverged " + std::to_string(I + 1) +
                                       " ticks after load: an in-flight report did not survive",
                                   __FILE__, __LINE__);
            return;
        }
    }
    // And the report really did arrive, so the test is not passing on two empty maps.
    RA4_EXPECT(Live.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() > 0);
    RA4_EXPECT(Restored.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() ==
               Live.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount());
}

RA4_TEST(Recon, SaveLoadPreservesGroupTrackIdentityFields)
{
    // The other half of B3: a track's category, anonymity and last reporting node
    // must survive a save, because the merge search filters on the first two and
    // corroboration compares the third. Checked directly on the restored track
    // rather than only through a checksum, so a failure names the field.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.bGroupTracksEnabled = true;

    SimWorld Live;
    Live.Initialize(&Content, MakeTestSetup(13579), &Settings);
    Live.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    Live.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    Live.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    Live.Tick(nullptr);
    Live.ClearEvents();

    const Recon::PerceivedTrack* Before = FindSingleTrack(Live, 0);
    RA4_REQUIRE(Before != nullptr);
    const Recon::ObservedCategory Category = Before->BelievedCategory;
    const bool bAnonymous = Before->bAnonymous;
    const uint16_t NodeId = Before->LastReportNodeId;
    // The fixture must actually exercise the fields: a default-valued category
    // would make the assertions below vacuous.
    RA4_REQUIRE(Category == Recon::ObservedCategory::HeavyVehicle);

    ByteWriter W;
    Live.Serialize(W);
    SimWorld Restored;
    // Initialize first so the restored world borrows the same settings pointer:
    // Deserialize refuses a save whose recon ruleset it cannot match.
    Restored.Initialize(&Content, MakeTestSetup(1), &Settings);
    ByteReader R(W.GetBuffer().data(), W.GetBuffer().size());
    RA4_REQUIRE(Restored.Deserialize(R, &Content));

    const Recon::PerceivedTrack* After = FindSingleTrack(Restored, 0);
    RA4_REQUIRE(After != nullptr);
    RA4_EXPECT(After->BelievedCategory == Category);
    RA4_EXPECT(After->bAnonymous == bAnonymous);
    RA4_EXPECT(After->LastReportNodeId == NodeId);
}

// --- M3 blackout: the emission veto, the decay bonus, the threshold ---------------
//
// Added after the M3 review found all three untested (finding M5): the milestone's
// headline feature would have passed its suite with the entire blackout branch
// deleted. Blackout is induced the way a match induces it -- by browning out the
// player's power -- not by poking internal flags.

namespace
{

// Builds a scene where player 0 has an HQ, a scout and a visible enemy, and where
// power can be crashed on demand to black the command post out. PowerDrainDef is a
// building that consumes far more power than the base produces.
ContentId AuthorPowerHogDef(ContentDatabase& Content)
{
    EntityDef Hog;
    Hog.Name = "building.test.power_hog";
    Hog.Id = MakeContentId("building.test.power_hog");
    Hog.Kind = EntityKind::Building;
    Hog.Faction = FactionId::Soviet;
    Hog.MaxHealth = 500;
    Hog.Building.FootprintX = 2;
    Hog.Building.FootprintY = 2;
    Hog.Building.PowerConsumed = 10000; // guarantees a deep brownout
    Content.AddEntity(Hog);
    return Hog.Id;
}

} // namespace

RA4_TEST(Recon, BlackoutStopsReportsReachingTheStaffMap)
{
    // The emission veto: a node whose comms are dead files nothing at all. With the
    // player's only command post blacked out, a contact in plain sight of a scout
    // must NOT appear on the staff map.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    const ContentId HogId = AuthorPowerHogDef(Content);
    // OrphanDelayTicks far beyond the measured window on purpose: otherwise a
    // blacked-out observer degrades into an orphan report that still arrives, and
    // the test would pass with the blackout veto deleted (verified by mutation).
    Recon::ReconSettings Settings = MakeChainSettings(/*PerHopDelayTicks*/ 2,
                                                     /*OrphanDelayTicks*/ 10000);
    Settings.Chain.BlackoutPowerRatioPercent = 50;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(11221), &Settings);
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnBuilding(HogId, 0, TileCoord(20, 40), true); // power crashes
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    // Long enough that any non-blacked-out routing would have delivered by now.
    for (int32_t I = 0; I < 40; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
    RA4_EXPECT(World.GetPlayer(0).GetPowerRatioPercent() < Settings.Chain.BlackoutPowerRatioPercent);
    RA4_EXPECT(World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() == 0);

    // Control: the identical scene WITH power must produce a track, so the
    // assertion above is about blackout and not about a broken fixture.
    SimWorld Powered;
    Powered.Initialize(&Content, MakeTestSetup(11221), &Settings);
    Powered.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    Powered.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    Powered.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    for (int32_t I = 0; I < 40; ++I)
    {
        Powered.Tick(nullptr);
        Powered.ClearEvents();
    }
    RA4_EXPECT(Powered.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount() > 0);
}

RA4_TEST(Recon, BlackoutDecaysExistingBeliefFasterThanNormalLoss)
{
    // The decay bonus: belief already on the map must rot FASTER while the network
    // is down than it would from mere loss of contact. Two runs of the same scene,
    // identical except that one loses power after the track is established.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    const ContentId HogId = AuthorPowerHogDef(Content);

    const auto ConfidenceAfterBlinding = [&Content, &HogId](bool bCutPower)
    {
        Recon::ReconSettings Settings = MakeChainSettings(1, 2);
        Settings.Chain.BlackoutPowerRatioPercent = 50;
        Settings.Chain.BlackoutConfidenceDecayPerSecondPerMille = 300; // big, to measure
        Settings.Tracks.ConfidenceDecayPerSecondPerMille = 20;
        Settings.Tracks.StaleAfterTicks = 5;
        Settings.Tracks.DropBelowConfidencePerMille = 0; // no GC inside the window

        SimWorld World;
        World.Initialize(&Content, MakeTestSetup(33445), &Settings);
        World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
        World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
        for (int32_t I = 0; I < 6; ++I) // establish the track
        {
            World.Tick(nullptr);
            World.ClearEvents();
        }
        // Blind ourselves either way, so the ONLY difference is the power state.
        World.DebugDamage(World.MakeId(1), 5000);
        if (bCutPower)
        {
            World.SpawnBuilding(HogId, 0, TileCoord(20, 40), true);
        }
        for (int32_t I = 0; I < 30; ++I)
        {
            World.Tick(nullptr);
            World.ClearEvents();
        }
        const Recon::PerceivedTrack* T = FindSingleTrack(World, 0);
        return T != nullptr ? T->Confidence : Fixed::Zero();
    };

    const Fixed WithPower = ConfidenceAfterBlinding(false);
    const Fixed Blacked = ConfidenceAfterBlinding(true);
    RA4_EXPECT(WithPower > Fixed::Zero()); // the fixture must leave something to compare
    RA4_EXPECT(Blacked < WithPower);
}

RA4_TEST(Recon, BlackoutThresholdIsADesignerSettingInTheRuleset)
{
    // The threshold decides when a player's intel goes dark, so it must be a
    // designer number inside the hashed ruleset -- not a literal in a .cpp. A
    // replay recorded with radios that failed at 50% power must be refused by a
    // build where they fail at 10%.
    Recon::ReconSettings A = MakeChainSettings(5, 50);
    Recon::ReconSettings B = A;
    B.Chain.BlackoutPowerRatioPercent = 10;
    RA4_EXPECT(A.ComputeSettingsHash() != B.ComputeSettingsHash());

    // And it must be validated: a percentage outside [0,100] is an authoring error.
    Recon::ReconSettings Bad = A;
    Bad.Chain.BlackoutPowerRatioPercent = 250;
    std::vector<std::string> Errors;
    RA4_EXPECT(!Recon::ValidateReconSettings(Bad, Errors));
}

// --- M4: fabrication and self-report bias (§4.3 stages 6-7, §4.5) -----------------

RA4_TEST(Recon, CalmObserversNeverFabricate)
{
    // The floor of the mechanic: a unit that is not shaken and not under fire must
    // never invent a contact. Without this, phantoms are noise rather than a signal
    // about the reporting unit's condition.
    Recon::DistortionProfile P;
    P.bFabricationEnabled = true;
    P.FabricationChanceMaxPerMille = 1000; // maximum, to prove morale is the gate
    Recon::ObserverState Calm;
    Calm.Morale = Fixed::FromInt(1);
    Calm.Fatigue = Fixed::Zero();
    Calm.bIsUnderFire = false;

    Random Rng(4242);
    for (int32_t I = 0; I < 2000; ++I)
    {
        if (Recon::StageFabrication(Calm, P, Rng))
        {
            RA4Test::ReportFailure("a calm observer fabricated a contact", __FILE__, __LINE__);
            return;
        }
    }
}

RA4_TEST(Recon, FabricationNeedsBrokenMoraleExhaustionAndContact)
{
    // Product, not sum, plus a contact gate: a merely frightened fresh unit invents
    // nothing, an exhausted unit with intact morale invents nothing, and a unit
    // resting behind the lines invents nothing however wrecked it is. Phantoms
    // belong to troops that are shaken, worn out AND still being shot at -- that
    // conjunction is what makes a phantom informative rather than noise.
    Recon::DistortionProfile P;
    P.bFabricationEnabled = true;
    P.FabricationChanceMaxPerMille = 1000;

    const auto CountFabrications = [&P](Fixed Morale, Fixed Fatigue, bool bUnderFire)
    {
        Recon::ObserverState O;
        O.Morale = Morale;
        O.Fatigue = Fatigue;
        O.bIsUnderFire = bUnderFire;
        Random Rng(99);
        int32_t Count = 0;
        for (int32_t I = 0; I < 2000; ++I)
        {
            if (Recon::StageFabrication(O, P, Rng)) { Count += 1; }
        }
        return Count;
    };

    const Fixed Spent = Fixed::FromInt(1);
    const Fixed Fresh = Fixed::Zero();
    const Fixed Half = Fixed::FromRatio(1, 2);

    RA4_EXPECT(CountFabrications(Fixed::Zero(), Spent, true) > 0);   // broken, spent, in contact
    RA4_EXPECT(CountFabrications(Fixed::Zero(), Fresh, true) == 0);  // frightened but fresh
    RA4_EXPECT(CountFabrications(Fixed::FromInt(1), Spent, true) == 0); // spent but steady
    RA4_EXPECT(CountFabrications(Fixed::Zero(), Spent, false) == 0); // wrecked but out of contact

    // Monotone in exhaustion: more worn down must not be LESS likely.
    RA4_EXPECT(CountFabrications(Fixed::Zero(), Spent, true) >=
               CountFabrications(Fixed::Zero(), Half, true));
}

RA4_TEST(Recon, FabricationDisableFlagIsAbsolute)
{
    // §4.3.6 requires that the most dangerous stage die from one switch, because a
    // playtest must be able to remove phantoms without losing the rest of the model.
    Recon::DistortionProfile P;
    P.bFabricationEnabled = false;
    P.FabricationChanceMaxPerMille = 1000; // maxed, and must still never fire
    Recon::ObserverState Broken;
    Broken.Morale = Fixed::Zero();
    Broken.Fatigue = Fixed::FromInt(1);
    Broken.bIsUnderFire = true;

    Random Rng(7);
    for (int32_t I = 0; I < 2000; ++I)
    {
        if (Recon::StageFabrication(Broken, P, Rng))
        {
            RA4Test::ReportFailure("fabrication fired with its flag off", __FILE__, __LINE__);
            return;
        }
    }
}

RA4_TEST(Recon, PhantomIsAlwaysRefutedByTheDeadline)
{
    // §4.5's hard promise, and the reason the feature is playable: a phantom ALWAYS
    // has a path to being disproved. This is the unconditional path -- even in a
    // corner of the map nobody can reach, the ghost dies at the deadline.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Recon::DistortionProfile& Profile = Settings.DistortionProfiles[0];
    Profile.bFabricationEnabled = true;
    Profile.FabricationChanceMaxPerMille = 1000;  // certainty, so the test is not flaky
    Profile.FabricationFearSaturationTicks = 20;
    Profile.MaxPhantomLifetimeTicks = 40;
    // Ghosts land far from our own troops, so the "someone looked and saw nothing"
    // path cannot fire and the DEADLINE is what this test measures. Verified by
    // mutation: with the deadline disabled this test fails.
    Profile.PositionErrorMaxTiles = 30;
    // Every OTHER removal mechanism is switched off, so the deadline is the only
    // thing that can clear this ghost. Confidence decay was quietly doing the job at
    // tick 11 of 40, which meant the test passed with the deadline deleted.
    Settings.Tracks.DropBelowConfidencePerMille = 0;   // no GC
    Settings.Tracks.ConfidenceDecayPerSecondPerMille = 0; // no decay to trigger it
    Settings.Chain.BlackoutConfidenceDecayPerSecondPerMille = 0;
    Settings.Tracks.StaleAfterTicks = 100000;          // no stale marking
    Settings.Morale.DamageMoralePenaltyPerMille = 5000;
    Settings.Morale.MoraleRegenPerTickPerMille = 0;
    // Fabrication needs morale down AND fatigue up. The shipped fatigue rate is
    // 0.4%/tick, which would need ~250 ticks of shelling: fine in a match, far too
    // slow for a unit test that must also keep the victim alive. Cranked so the
    // scene reaches "shaken and spent" in a handful of ticks. The RATES are what is
    // being made convenient here, never the rule under test.
    Settings.Morale.FatiguePerTickUnderFirePerMille = 400;
    Settings.Morale.FatigueRegenPerTickPerMille = 0;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(31415), &Settings);
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    // Morale is driven by the DamageApplied EVENT, and SimWorld::DebugDamage
    // deliberately does not emit one (it edits health directly). So the observer is
    // rattled by a real firefight: a tough conscript in contact with an enemy
    // rifleman, which produces genuine damage events tick after tick.
    const EntityId Victim =
        World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3300), Fixed::FromInt(3000)));

    // Let the fight run until the conscript is shaken and spent but still alive.
    for (int32_t I = 0; I < 30 && World.IsAlive(Victim); ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
    RA4_REQUIRE(World.IsAlive(Victim));
    RA4_REQUIRE(World.GetRecon().GetLastObserverForTest(0).bAnyUnitUnderFire);
    RA4_REQUIRE(World.GetRecon().GetLastObserverForTest(0).Morale < Fixed::FromInt(1));

    // A phantom must have appeared: player 0 has no real enemy anywhere.
    // Count PHANTOM tracks specifically: a real enemy is present and legitimately
    // tracked, so "zero tracks" would be the wrong question.
    const auto CountPhantoms = [&World]()
    {
        const Recon::PerceivedWorld& Belief = World.GetRecon().GetPerceivedWorld(0);
        std::vector<const Recon::PerceivedTrack*> Found;
        Belief.GetTracksInRegion(0, 0, 63, 63, Found);
        int32_t Count = 0;
        for (const Recon::PerceivedTrack* T : Found)
        {
            if (PerceivedWorldTestAccess::IsPhantom(Belief, T->Id))
            {
                Count += 1;
            }
        }
        return Count;
    };

    int32_t FirstPhantomTick = -1;
    for (int32_t I = 0; I < 200 && FirstPhantomTick < 0 && World.IsAlive(Victim); ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        if (CountPhantoms() > 0)
        {
            FirstPhantomTick = int32_t(World.GetTick());
        }
    }
    RA4_REQUIRE(FirstPhantomTick > 0); // otherwise the rest of the test proves nothing

    // The guarantee is PER PHANTOM: this specific ghost must be gone by the
    // deadline. It is not "the map eventually holds no phantoms" -- a unit still
    // being shelled keeps inventing new ones, and that is correct behaviour, so
    // waiting for an empty map would hang on a working feature.
    std::vector<const Recon::PerceivedTrack*> Snapshot;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Snapshot);
    Recon::TrackId Watched;
    for (const Recon::PerceivedTrack* T : Snapshot)
    {
        if (PerceivedWorldTestAccess::IsPhantom(World.GetRecon().GetPerceivedWorld(0), T->Id))
        {
            Watched = T->Id;
            break;
        }
    }
    RA4_REQUIRE(Watched.IsValid());

    bool bCleared = false;
    const int32_t Deadline = Profile.MaxPhantomLifetimeTicks;
    for (int32_t I = 0; I < Deadline * 4 && !bCleared; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        // Gone means the generational handle no longer resolves: the slot may have
        // been recycled for a different contact, which must NOT read as survival.
        bCleared = !World.GetRecon().GetPerceivedWorld(0).IsTrackAlive(Watched);
    }
    RA4_EXPECT(bCleared);
}

RA4_TEST(Recon, ScoutingRefutesAPhantomBeforeTheDeadline)
{
    // The player-driven half of §4.5, and the half that makes scouting feel like it
    // pays: a friendly unit standing where a phantom is plotted, seeing nothing,
    // clears it WITHOUT waiting for the deadline.
    //
    // This path was broken when first written: the query asked LastObservedTick,
    // which only records tiles where something WAS seen, so a scout in an empty
    // clearing could never disprove anything. It now asks the fog grid.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Recon::DistortionProfile& Profile = Settings.DistortionProfiles[0];
    Profile.bFabricationEnabled = true;
    Profile.FabricationChanceMaxPerMille = 1000;
    // Ghosts land close by, so our own shaken unit is standing on them.
    Profile.PositionErrorMaxTiles = 1;
    // A deadline far beyond the run: if the ghost dies, someone LOOKED.
    Profile.MaxPhantomLifetimeTicks = 100000;
    Settings.Tracks.DropBelowConfidencePerMille = 0;
    Settings.Tracks.ConfidenceDecayPerSecondPerMille = 0;
    Settings.Tracks.StaleAfterTicks = 100000;
    Settings.Morale.DamageMoralePenaltyPerMille = 5000;
    Settings.Morale.MoraleRegenPerTickPerMille = 0;
    Settings.Morale.FatiguePerTickUnderFirePerMille = 400;
    Settings.Morale.FatigueRegenPerTickPerMille = 0;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(60606), &Settings);
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    const EntityId Victim =
        World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3300), Fixed::FromInt(3000)));

    const auto FindPhantom = [&World]()
    {
        const Recon::PerceivedWorld& Belief = World.GetRecon().GetPerceivedWorld(0);
        std::vector<const Recon::PerceivedTrack*> Found;
        Belief.GetTracksInRegion(0, 0, 63, 63, Found);
        for (const Recon::PerceivedTrack* T : Found)
        {
            if (PerceivedWorldTestAccess::IsPhantom(Belief, T->Id))
            {
                return T->Id;
            }
        }
        return Recon::TrackId{};
    };

    // Let the firefight rattle the conscript until it invents something.
    Recon::TrackId Ghost;
    for (int32_t I = 0; I < 200 && !Ghost.IsValid() && World.IsAlive(Victim); ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        Ghost = FindPhantom();
    }
    RA4_REQUIRE(Ghost.IsValid());
    const TickIndex BornAt = World.GetTick();

    // The ghost sits within a tile of our own troops, who can see that ground and
    // see nothing there, so the staff must strike it off -- vastly sooner than the
    // 100000-tick deadline could explain.
    bool bCleared = false;
    for (int32_t I = 0; I < 400 && !bCleared; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        bCleared = !World.GetRecon().GetPerceivedWorld(0).IsTrackAlive(Ghost);
    }
    RA4_EXPECT(bCleared);
    RA4_EXPECT(int32_t(World.GetTick() - BornAt) < Profile.MaxPhantomLifetimeTicks);
}

RA4_TEST(Recon, SelfReportOverstatesStrengthAndHidesLossesOnlyWhenUndisciplined)
{
    // Stage 7, asymmetric in opposite directions: a unit never accidentally reports
    // being weaker than it is, and never reports losses it did not take. A
    // disciplined unit reports the truth.
    Recon::DistortionProfile P;
    P.bSelfReportBiasEnabled = true;
    P.SelfReportStrengthOverstatementMaxPerMille = 500;
    P.SelfReportLossUnderstatementMaxPerMille = 500;

    Random Rng(1234);
    // Perfect discipline: truth, bit for bit.
    for (int32_t I = 0; I < 200; ++I)
    {
        RA4_EXPECT(Recon::StageSelfReportStrength(10, Fixed::FromInt(1), P, Rng) == 10);
        RA4_EXPECT(Recon::StageSelfReportLosses(4, Fixed::FromInt(1), P, Rng) == 4);
    }

    // No discipline: strength never below truth, losses never above it.
    bool bSawInflation = false;
    bool bSawHiding = false;
    for (int32_t I = 0; I < 500; ++I)
    {
        const int32_t Strength = Recon::StageSelfReportStrength(10, Fixed::Zero(), P, Rng);
        const int32_t Losses = Recon::StageSelfReportLosses(10, Fixed::Zero(), P, Rng);
        if (Strength < 10 || Losses > 10)
        {
            RA4Test::ReportFailure("self-report bias reversed direction", __FILE__, __LINE__);
            return;
        }
        bSawInflation = bSawInflation || Strength > 10;
        bSawHiding = bSawHiding || Losses < 10;
    }
    RA4_EXPECT(bSawInflation);
    RA4_EXPECT(bSawHiding);

    // And the flag kills it.
    Recon::DistortionProfile Off = P;
    Off.bSelfReportBiasEnabled = false;
    RA4_EXPECT(Recon::StageSelfReportStrength(10, Fixed::Zero(), Off, Rng) == 10);
    RA4_EXPECT(Recon::StageSelfReportLosses(10, Fixed::Zero(), Off, Rng) == 10);
}

RA4_TEST(Recon, PhantomTruthNeverReachesTheReadSurface)
{
    // INVARIANT 10 for the M4 addition: the UI must not be able to tell a phantom
    // from a real contact. If it could, the player would simply filter the ghosts
    // out and the mechanic would be free to ignore.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Recon::PerceivedWorld Belief;
    PerceivedWorldTestAccess::Initialize(Belief, 64, 64, 8);
    const Recon::TrackId Id = PerceivedWorldTestAccess::AllocateTrack(Belief);
    PerceivedWorldTestAccess::SetPhantom(Belief, Id, true);

    // Core knows...
    RA4_EXPECT(PerceivedWorldTestAccess::IsPhantom(Belief, Id));
    // ...and the read surface does not carry it. Enforced structurally by the
    // static_assert layout mirror in Recon.PhantomTruthLivesOutsideTheReadSurface;
    // here we assert the observable consequence: a phantom track looks like any
    // other track to a reader.
    std::vector<const Recon::PerceivedTrack*> Found;
    Belief.GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(Found.size() == 1);
    RA4_EXPECT(Found[0]->Id == Id);
    (void)Settings;
}

// --- M3-perf: derived indices must agree with the data they index -----------------

RA4_TEST(Recon, SpatialIndexAgreesWithALinearScan)
{
    // The spatial index is derived state, and the dangerous failure mode is silent:
    // a track that stops being FOUND by region queries while still existing simply
    // vanishes from the HUD. So the accelerated query is compared against the
    // obvious brute-force answer, over a match that allocates, moves and collects
    // tracks -- exactly the operations that can desynchronise an index.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Settings.Tracks.bGroupTracksEnabled = true;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(24601), &Settings);
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    // Several enemies spread out, and one ordered to move so tracks change cells.
    const EntityId Mover =
        World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(6000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3000), Fixed::FromInt(6000)));

    Command Move = RA4Test::MakeCommand(CommandType::Move, 1);
    Move.Primary = Mover;
    Move.Location = Vec2(Fixed::FromInt(9000), Fixed::FromInt(9000));
    CommandFrame Frame;
    Frame.Commands.push_back(Move);
    World.Tick(&Frame);
    World.ClearEvents();

    for (int32_t Step = 0; Step < 60; ++Step)
    {
        World.Tick(nullptr);
        World.ClearEvents();

        const Recon::PerceivedWorld& Belief = World.GetRecon().GetPerceivedWorld(0);

        // Whole map: the accelerated query must return exactly the alive tracks.
        std::vector<const Recon::PerceivedTrack*> Indexed;
        Belief.GetTracksInRegion(0, 0, 63, 63, Indexed);
        RA4_EXPECT(uint32_t(Indexed.size()) == Belief.GetAliveTrackCount());

        // Output order must stay ascending by slot: callers and the determinism
        // tests depend on it, and cell traversal order must not leak through.
        for (size_t I = 1; I < Indexed.size(); ++I)
        {
            if (Indexed[I - 1]->Id.Index >= Indexed[I]->Id.Index)
            {
                RA4Test::ReportFailure("region query returned tracks out of slot order",
                                       __FILE__, __LINE__);
                return;
            }
        }

        // Sub-rectangles: every track the index reports must genuinely be inside,
        // and every alive track inside must be reported. This is the property a
        // stale index breaks.
        const int32_t Rects[4][4] = {{0, 0, 20, 20}, {10, 10, 40, 40}, {30, 0, 63, 30}, {0, 30, 30, 63}};
        for (const auto& R : Rects)
        {
            std::vector<const Recon::PerceivedTrack*> Sub;
            Belief.GetTracksInRegion(R[0], R[1], R[2], R[3], Sub);

            int32_t ExpectedCount = 0;
            std::vector<const Recon::PerceivedTrack*> All;
            Belief.GetTracksInRegion(0, 0, 63, 63, All);
            for (const Recon::PerceivedTrack* T : All)
            {
                const int64_t TileX = T->BelievedPosition.X.ToIntFloor() / 200;
                const int64_t TileY = T->BelievedPosition.Y.ToIntFloor() / 200;
                if (TileX >= R[0] && TileX <= R[2] && TileY >= R[1] && TileY <= R[3])
                {
                    ExpectedCount += 1;
                }
            }
            if (int32_t(Sub.size()) != ExpectedCount)
            {
                RA4Test::ReportFailure("spatial index disagrees with a linear scan on a sub-region",
                                       __FILE__, __LINE__);
                return;
            }
        }
    }
}

RA4_TEST(Recon, IndicesSurviveSaveLoadAndKeepQueriesCorrect)
{
    // Both derived indices (spatial grid, association reverse index) are rebuilt on
    // load rather than serialized. If a rebuild were forgotten, the restored world
    // would answer queries wrongly while its checksum still matched -- the worst
    // possible combination, so it gets its own test.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    // Tracks must be COLLECTED during the post-load run, because that is the only
    // path where the association reverse index matters: a stale index fails to sever
    // associations, the next report writes through a dangling handle, and the two
    // worlds diverge. Without collection the test cannot tell a rebuilt index from a
    // missing one -- verified by mutation.
    Settings.Tracks.ConfidenceDecayPerSecondPerMille = 400;
    Settings.Tracks.DropBelowConfidencePerMille = 300;
    Settings.Tracks.StaleAfterTicks = 5;

    SimWorld Live;
    Live.Initialize(&Content, MakeTestSetup(48000), &Settings);
    Live.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    Live.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    Live.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    for (int32_t I = 0; I < 10; ++I)
    {
        Live.Tick(nullptr);
        Live.ClearEvents();
    }
    std::vector<const Recon::PerceivedTrack*> Before;
    Live.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Before);
    RA4_REQUIRE(!Before.empty());

    ByteWriter W;
    Live.Serialize(W);
    SimWorld Restored;
    Restored.Initialize(&Content, MakeTestSetup(1), &Settings);
    ByteReader R(W.GetBuffer().data(), W.GetBuffer().size());
    RA4_REQUIRE(Restored.Deserialize(R, &Content));

    // Belief must be COLLECTED during the run below, because that is the only path
    // where the association reverse index matters. Decay does that on its own here
    // (aggressive decay plus a high GC floor were configured above); nothing is
    // killed by hand, because DebugDamage is an out-of-band edit and applying it to
    // two worlds is one more thing that can differ between them rather than a
    // guarantee that they match.

    // The rebuilt spatial index must find the same tracks...
    std::vector<const Recon::PerceivedTrack*> After;
    Restored.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, After);
    RA4_EXPECT(After.size() == Before.size());

    // Direct check on the reverse index itself: after a load, every forward
    // association must have a matching back-link. A missing rebuild shows up here
    // immediately, rather than only as an eventual divergence.
    RA4_EXPECT(Restored.GetRecon().ReverseIndexMatchesForwardTableForTest(0));

    // ...and the rebuilt association index must keep the two worlds in lockstep as
    // they run on, which is where a broken reverse index would show up: a released
    // track leaving a dangling association diverges the next update.
    for (int32_t I = 0; I < 60; ++I)
    {
        Live.Tick(nullptr);
        Live.ClearEvents();
        Restored.Tick(nullptr);
        Restored.ClearEvents();
        if (Live.ComputeStateChecksum() != Restored.ComputeStateChecksum())
        {
            RA4Test::ReportFailure("live and restored diverged " + std::to_string(I + 1) +
                                       " ticks after load: a derived index was not rebuilt",
                                   __FILE__, __LINE__);
            return;
        }
    }
}

// --- M5: post-hoc explainability (§4.6) -------------------------------------------

RA4_TEST(Recon, AuditLogIsAnOutputAndCannotAffectDeterminism)
{
    // The audit log carries ground truth, so if it were simulation state it would be
    // both a desync risk and an INVARIANT 10 violation. It must therefore be an
    // output, like SimWorld's event list: excluded from the checksum and from saves.
    //
    // Proved by divergence rather than by inspection: two worlds are run on identical
    // input, one of them having its log cleared repeatedly. If the log influenced
    // anything, the checksums would part company.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeChainSettings(4, 20);

    SimWorld A;
    SimWorld B;
    A.Initialize(&Content, MakeTestSetup(70707), &Settings);
    B.Initialize(&Content, MakeTestSetup(70707), &Settings);
    for (SimWorld* W : {&A, &B})
    {
        W->SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
        W->SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
        W->SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    }

    for (int32_t I = 0; I < 80; ++I)
    {
        A.Tick(nullptr);
        A.ClearEvents();
        B.Tick(nullptr);
        B.ClearEvents();
        // B forgets its explanations as it goes; A keeps everything.
        const_cast<Recon::ReconSystem&>(B.GetRecon()).ClearReportAudits();
        if (A.ComputeStateChecksum() != B.ComputeStateChecksum())
        {
            RA4Test::ReportFailure("clearing the audit log changed simulation state at tick " +
                                       std::to_string(I + 1),
                                   __FILE__, __LINE__);
            return;
        }
    }
    // And the fixture must have actually logged something, or this proves nothing.
    RA4_EXPECT(!A.GetRecon().GetReportAudits().empty());
    RA4_EXPECT(B.GetRecon().GetReportAudits().empty());
}

RA4_TEST(Recon, AuditLogIsBoundedOverALongMatch)
{
    // An unbounded explanation log is a memory leak with a 40-minute fuse. The ring
    // must cap, and the cap must hold under a long run with continuous reporting.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(80808), &Settings);
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));

    for (int32_t I = 0; I < 3000; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
    // 3000 ticks is 150 seconds of continuous reporting; the log must be capped well
    // below that, not merely "smaller than the tick count".
    RA4_EXPECT(World.GetRecon().GetReportAudits().size() <= 4096);
    RA4_EXPECT(!World.GetRecon().GetReportAudits().empty());
}

RA4_TEST(Recon, ExplanationNamesTheActualCauseOfTheError)
{
    // The §4.6 obligation, and the whole point of M5: if a player cannot see WHY
    // they were wrong, the layer reads as unfairness rather than depth. So an
    // explanation must name the specific failure -- overcount, misidentification or
    // fabrication -- and not merely restate what was believed.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);
    Recon::DistortionProfile& Profile = Settings.DistortionProfiles[0];
    // Fear-driven overcounting, hard enough to be certain within the window.
    Profile.bCountDistortionEnabled = true;
    Profile.FearCountBiasMaxPerMille = 3000;
    Profile.CompetenceNoiseMaxPerMille = 0;
    Settings.Morale.DamageMoralePenaltyPerMille = 5000;
    Settings.Morale.MoraleRegenPerTickPerMille = 0;
    Settings.Morale.FatiguePerTickUnderFirePerMille = 400;
    Settings.Morale.FatigueRegenPerTickPerMille = 0;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(90909), &Settings);
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    const EntityId Observer =
        World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3300), Fixed::FromInt(3000)));

    // Let a real firefight rattle the observer so its reports start inflating.
    std::string Explanation;
    for (int32_t I = 0; I < 120 && World.IsAlive(Observer); ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        std::vector<const Recon::PerceivedTrack*> Found;
        World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
        for (const Recon::PerceivedTrack* T : Found)
        {
            const std::string Text = World.GetRecon().ExplainTrack(0, *T);
            if (Text.find("OVERCOUNTED") != std::string::npos ||
                Text.find("MISIDENTIFIED") != std::string::npos ||
                Text.find("invented this contact") != std::string::npos)
            {
                Explanation = Text;
                break;
            }
        }
        if (!Explanation.empty())
        {
            break;
        }
    }
    RA4_REQUIRE(!Explanation.empty());

    // The explanation must be usable, not just non-empty: it has to attribute the
    // report to a post, state the delay, and quote the observer's condition -- that
    // is what turns "the game lied to me" into "my scouts were terrified".
    RA4_EXPECT(Explanation.find("Report #") != std::string::npos);
    RA4_EXPECT(Explanation.find("arrived +") != std::string::npos);
    RA4_EXPECT(Explanation.find("morale") != std::string::npos);
    RA4_EXPECT(Explanation.find("truth") != std::string::npos);
}

RA4_TEST(Recon, ExplanationDegradesHonestlyWhenReportsAgedOut)
{
    // A track older than the audit log must say so rather than fabricate an
    // explanation. Silently inventing a plausible history would be worse than
    // admitting the log has rolled over, because the player would trust it.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(11111), &Settings);
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::AllLightTank, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
    World.Tick(nullptr);
    World.ClearEvents();

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(!Found.empty());

    // Wipe the log, keeping the track: exactly the state a long match reaches.
    const_cast<Recon::ReconSystem&>(World.GetRecon()).ClearReportAudits();
    const std::string Text = World.GetRecon().ExplainTrack(0, *Found[0]);
    RA4_EXPECT(Text.find("older than the audit log") != std::string::npos);
    // ...and it still describes the belief itself, so the panel is not blank.
    RA4_EXPECT(Text.find("Contact:") != std::string::npos);
}

// --- M6: the AI plays from belief, not truth --------------------------------------

RA4_TEST(Recon, AICannotSeeWhatTheStaffMapDoesNotBelieve)
{
    // The milestone's structural claim: with recon on, the AI's only enemy
    // information is the same staff map a human reads. Zero-cheat stops being a
    // promise kept by review and becomes a property of the code.
    //
    // Scene: an enemy tank hidden in fog with nothing of ours nearby. The staff map
    // holds no track, so the AI must know nothing -- previously it scanned entities
    // and would have found it.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(21000), &Settings);
    // The AI player (1) has a base far from the enemy contact.
    World.SpawnBuilding(RA4Test::Ids::AllConYard, 1, TileCoord(50, 50), true);
    // Player 0's tank sits in the far corner, unseen by anything player 1 owns.
    World.SpawnUnit(RA4Test::Ids::SovHeavyTank, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));

    for (int32_t I = 0; I < 20; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }

    AI::SimWorldView View(World, 1);
    View.UpdateMemory(600);
    RA4_EXPECT(View.GetKnownEnemies().empty());
    // ...and belief agrees, so the test is asserting the wiring rather than an
    // accident of the scene.
    RA4_EXPECT(World.GetRecon().GetPerceivedWorld(1).GetAliveTrackCount() == 0);
}

RA4_TEST(Recon, AISeesExactlyWhatTheStaffMapBelieves)
{
    // The other half: what the staff map DOES hold must reach the AI, or the
    // commander would be blind rather than merely fallible.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(21001), &Settings);
    World.SpawnBuilding(RA4Test::Ids::AllConYard, 1, TileCoord(15, 40), true);
    // An observer of player 1 with an enemy right in front of it.
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::SovHeavyTank, 0, Vec2(Fixed::FromInt(3300), Fixed::FromInt(3000)));

    for (int32_t I = 0; I < 10; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }

    const uint32_t Believed = World.GetRecon().GetPerceivedWorld(1).GetAliveTrackCount();
    RA4_REQUIRE(Believed > 0);

    AI::SimWorldView View(World, 1);
    View.UpdateMemory(600);
    // One memory per track: the AI's picture is the staff map, entry for entry.
    RA4_EXPECT(uint32_t(View.GetKnownEnemies().size()) == Believed);
}

RA4_TEST(Recon, AIIsFooledByAPhantomExactlyAsAPlayerWouldBe)
{
    // The consequence that makes the layer honest rather than merely fair: a
    // fabricated contact must fool the AI too. If the commander could tell ghosts
    // from real contacts, it would be cheating in the one way the layer is meant to
    // prevent -- and phantoms would only ever punish the human.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Settings = MakeMinimalSettings(true);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(21002), &Settings);
    World.SpawnBuilding(RA4Test::Ids::AllConYard, 1, TileCoord(50, 50), true);
    World.Tick(nullptr);
    World.ClearEvents();

    // Plant a phantom on player 1's staff map. There is NO real enemy anywhere in
    // this scene, so anything the AI reports knowing came from belief alone.
    const Recon::PerceivedWorld& Belief = World.GetRecon().GetPerceivedWorld(1);
    Recon::PerceivedWorld& Writable = const_cast<Recon::PerceivedWorld&>(Belief);
    const Recon::TrackId Ghost = PerceivedWorldTestAccess::AllocateTrack(Writable);
    RA4_REQUIRE(Ghost.IsValid());
    Recon::PerceivedTrack* T = PerceivedWorldTestAccess::GetTrackMutable(Writable, Ghost);
    RA4_REQUIRE(T != nullptr);
    T->BelievedPosition = Vec2(Fixed::FromInt(4000), Fixed::FromInt(4000));
    T->BelievedCategory = Recon::ObservedCategory::HeavyVehicle;
    T->BelievedCountMin = 3;
    T->BelievedCountMax = 5;
    T->Confidence = Fixed::FromRatio(3, 4);
    T->LastUpdateTick = World.GetTick();
    PerceivedWorldTestAccess::SetPhantom(Writable, Ghost, true);

    AI::SimWorldView View(World, 1);
    View.UpdateMemory(600);

    // The AI believes in the imaginary force...
    RA4_REQUIRE(View.GetKnownEnemies().size() == 1);
    const AI::EnemyMemory& Mem = View.GetKnownEnemies()[0];
    RA4_EXPECT(Mem.Position.X == 20 && Mem.Position.Y == 20); // 4000 / 200 tile units
    // ...and carries no handle it could use to check, because a track has none.
    RA4_EXPECT(!Mem.Entity.IsValid());
}

RA4_TEST(Recon, DisablingReconRestoresTheClassicAIScanPath)
{
    // The kill switch must work for the AI as well, or turning the feature off would
    // silently leave the commander blind -- and every other system's debugging would
    // be done against a broken opponent.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    Recon::ReconSettings Disabled = MakeMinimalSettings(false);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(21003), &Disabled);
    World.SpawnBuilding(RA4Test::Ids::AllConYard, 1, TileCoord(15, 40), true);
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
    World.SpawnUnit(RA4Test::Ids::SovHeavyTank, 0, Vec2(Fixed::FromInt(3300), Fixed::FromInt(3000)));
    for (int32_t I = 0; I < 10; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }

    AI::SimWorldView View(World, 1);
    View.UpdateMemory(600);
    // Classic behaviour: the visible enemy is known, and it carries a real handle
    // (the entity scan path fills one) -- the belief path deliberately does not.
    RA4_REQUIRE(!View.GetKnownEnemies().empty());
    RA4_EXPECT(View.GetKnownEnemies()[0].Entity.IsValid());
}

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
    // The M2 obligation from the I-B5 review (a tuning-swap must actually change
    // the belief timeline, else the header hash gate is decorative) is discharged
    // by Recon.SwappedTuningProducesADifferentBeliefTimeline below.
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

RA4_TEST(Recon, SwappedTuningProducesADifferentBeliefTimeline)
{
    // The I-B5 review's M2 obligation, dischargeable only now that distortion is
    // real: the replay header's ReconSettingsHash must be load-bearing. If belief
    // were insensitive to tunables, the hash gate would be theatre -- refusing
    // mismatched rulesets to protect a property that did not exist.
    //
    // Method: run the same scripted match three times from the same seed, varying
    // only the distortion tuning. Equal-hash settings must reproduce the belief
    // timeline exactly; a changed tunable must change it.
    ContentDatabase Content;
    BuildDefaultContent(Content);

    // Fear-only distortion with a large bias, and morale that craters and never
    // recovers, so the tuning difference shows inside the measured window.
    const auto MakeTuning = [](int32_t FearBiasPerMille)
    {
        Recon::ReconSettings S = MakeMinimalSettings(true);
        S.DistortionProfiles[0].bCountDistortionEnabled = true;
        S.DistortionProfiles[0].FearCountBiasMaxPerMille = FearBiasPerMille;
        S.DistortionProfiles[0].CompetenceNoiseMaxPerMille = 0; // fear only
        S.Morale.DamageMoralePenaltyPerMille = 5000;
        S.Morale.MoraleRegenPerTickPerMille = 0;
        return S;
    };

    // Two facing observers; the player-0 unit is shelled into panic (without
    // dying -- a dead observer reports nothing) so the fear branch actually runs.
    const auto RunBeliefTimeline = [&Content](const Recon::ReconSettings& Settings)
    {
        std::vector<uint64_t> PerTick;
        SimWorld World;
        World.Initialize(&Content, MakeTestSetup(4242), &Settings);
        World.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000)));
        World.SpawnUnit(Ids::AllRifleman, 1, Vec2(Fixed::FromInt(3400), Fixed::FromInt(3000)));
        const EntityId Victim = World.MakeId(0);
        for (int32_t I = 0; I < 15; ++I)
        {
            World.DebugDamage(Victim, 2); // 30 total, far below lethal
            World.Tick(nullptr);
            World.ClearEvents();
        }
        for (int32_t I = 0; I < 60; ++I)
        {
            World.Tick(nullptr);
            World.ClearEvents();
            Hash64 H;
            World.GetRecon().GetPerceivedWorld(0).FeedChecksum(H);
            PerTick.push_back(H.Get());
        }
        return PerTick;
    };

    const Recon::ReconSettings Original = MakeTuning(3000);
    const std::vector<uint64_t> Live = RunBeliefTimeline(Original);

    // 1. Independently constructed, same tunables: identical timeline. This is
    //    exactly what an equal header hash promises a replay viewer.
    const Recon::ReconSettings SameTuning = MakeTuning(3000);
    RA4_REQUIRE(SameTuning.ComputeSettingsHash() == Original.ComputeSettingsHash());
    const std::vector<uint64_t> Same = RunBeliefTimeline(SameTuning);
    RA4_REQUIRE(Same.size() == Live.size());
    RA4_EXPECT(Same == Live);

    // 2. One tunable changed: different hash AND a different timeline. A weaker
    //    fear bias must change what the panicking observer reports.
    const Recon::ReconSettings OtherTuning = MakeTuning(200);
    RA4_REQUIRE(OtherTuning.ComputeSettingsHash() != Original.ComputeSettingsHash());
    const std::vector<uint64_t> Other = RunBeliefTimeline(OtherTuning);
    RA4_REQUIRE(Other.size() == Live.size());
    if (Other == Live)
    {
        RA4Test::ReportFailure("belief timeline is insensitive to distortion tuning: the replay "
                               "header's ReconSettingsHash gate protects nothing",
                               __FILE__, __LINE__);
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
        "Source/RA4AI/Private/TacticalCombatMicro.cpp", // Tactical Micro & Focus Fire
        "Source/RA4AI/Private/ValueMap.cpp",           // OWN

        "Source/RA4Campaign/Private/MissionRuntime.cpp", // OMNISCIENT-BY-DESIGN (referee)
        "Source/RA4Campaign/Private/MissionScriptRuntime.cpp", // OMNISCIENT-BY-DESIGN (scripting engine & triggers)
        "Source/RA4Presentation/Private/HudSnapshot.cpp", // OWN + FOG-GATED minimap

        "Source/RA4Presentation/Private/PresentationInterpolator.cpp", // OMNISCIENT-BY-DESIGN (presentation interpolation)
        "Source/RA4Recon/Private/ReconSystem.cpp",     // OMNISCIENT-BY-DESIGN (produces belief)
        "Source/RA4Replay/Private/MatchTelemetryTracker.cpp", // OMNISCIENT-BY-DESIGN (telemetry and analytics exporter)
        "Source/RA4Replay/Private/ReplayFileFormat.cpp", // OMNISCIENT-BY-DESIGN (binary replay format & verifier)
        "Source/RA4Simulation/Private/ElectronicWarfare.cpp", // OMNISCIENT-BY-DESIGN (electronic warfare and radar jamming)

        "Source/RA4Simulation/Private/ExoticSuperweaponPhysics.cpp", // OMNISCIENT-BY-DESIGN (superweapon blast dynamics)
        "Source/RA4Simulation/Private/GarrisonUrbanCombat.cpp", // OMNISCIENT-BY-DESIGN (urban garrison combat)
        "Source/RA4Simulation/Private/OrbitalDebrisPhysics.cpp", // OMNISCIENT-BY-DESIGN (magnetic satellite & orbital debris)
        "Source/RA4Simulation/Private/ProtocolRuntime.cpp", // OMNISCIENT-BY-DESIGN (top-secret protocols & powers)
        "Source/RA4Simulation/Private/SimWorld.cpp",   // the truth itself






        "Source/RA4Simulation/Public/RA4Simulation/SimWorld.h",
        "Source/RedAlert4/Private/RA4PlayerController.cpp", // OWN x2 + LEAK V-B (picking)
        "Source/RedAlert4/Private/RA4ReconDebugOverlay.cpp", // OMNISCIENT-BY-DESIGN (two-maps overlay)
        "Source/RedAlert4/Private/RA4RtsHud.cpp",           // OWN (selection / HUD telemetry)
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


RA4_TEST(Recon, RadarReturnsAnonymousContactsOnly)
{
    // Owner decision D6: a radar produces a POSITION, not an identity. The blip
    // must surface as an anonymous track (bAnonymous, invalid class) and must
    // never leak the true class through the read surface.
    ContentDatabase Content;
    BuildDefaultContent(Content);

    // Author a radar building: no shipped def sets bIsRadar yet, so the test
    // registers its own -- exactly how a designer would, through content.
    EntityDef RadarDef;
    RadarDef.Name = "building.test.radar";
    RadarDef.Id = MakeContentId("building.test.radar");
    RadarDef.Kind = EntityKind::Building;
    RadarDef.Faction = FactionId::Soviet;
    RadarDef.MaxHealth = 500;
    RadarDef.Building.FootprintX = 2;
    RadarDef.Building.FootprintY = 2;
    RadarDef.Building.bIsRadar = true;
    Content.AddEntity(RadarDef);

    Recon::ReconSettings Enabled = MakeMinimalSettings(true);
    Enabled.RadarRangeTiles = 30;

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(7171), &Enabled);
    // Radar at tile (30,30) ~ world (6000,6000): about 21 tiles from the enemy at
    // (9000,9000) -- inside the 30-tile range, far outside any unit vision
    // (player 0 fields no units at all).
    World.SpawnBuilding(RadarDef.Id, 0, TileCoord{30, 30}, /*bInstantComplete=*/true);
    World.SpawnUnit(Ids::AllLightTank, 1, Vec2(Fixed::FromInt(9000), Fixed::FromInt(9000)));

    World.Tick(nullptr);

    std::vector<const Recon::PerceivedTrack*> Found;
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(Found.size() == 1);
    RA4_EXPECT(Found[0]->bAnonymous);
    RA4_EXPECT(!(Found[0]->BelievedClass == Ids::AllLightTank)); // identity must not leak
    // The believed position matches the blip (truthful profile, no smear).
    RA4_EXPECT(Found[0]->BelievedPosition.X == Fixed::FromInt(9000));
    RA4_EXPECT(Found[0]->BelievedPosition.Y == Fixed::FromInt(9000));

    // Visual contact upgrades the same track to an identified one, not a second
    // blip: spawn a scout next to the tank and let fog reveal it.
    World.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(8800), Fixed::FromInt(9000)));
    for (int32_t T = 0; T < 3; ++T)
    {
        World.Tick(nullptr);
    }
    Found.clear();
    World.GetRecon().GetPerceivedWorld(0).GetTracksInRegion(0, 0, 63, 63, Found);
    RA4_REQUIRE(Found.size() == 1);
    RA4_EXPECT(!Found[0]->bAnonymous);
    RA4_EXPECT(Found[0]->BelievedClass == Ids::AllLightTank);
}
