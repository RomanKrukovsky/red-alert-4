// Copyright (c) Red Alert 4 project. Designer-facing configuration for the intel layer.
//
// Every tunable of the unreliable-intelligence feature lives here and is loaded
// from JSON (ADR-0009): no magic numbers in simulation code, and a designer can
// change behaviour without touching C++. The whole feature collapses to classic
// perfect information with a single flag (bEnabled = false) -- that switch is a
// hard requirement for debugging every other system and for A/B playtests.
//
// Values that feed simulation math are integers or per-mille (1000 = 1.0x)
// fixed-point, matching DamageMatrixDef: JSON numbers arrive as doubles, and a
// double that survives into sim state is a desync between compilers.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"

#ifndef RA4RECON_API
#define RA4RECON_API
#endif

namespace RA4
{
namespace Recon
{

// Per-mille helper: designers author "0.35" in JSON, the loader converts to 350
// exactly once, and everything downstream is integer math.
inline Fixed PerMilleToFixed(int32_t PerMille) { return Fixed::FromRatio(PerMille, 1000); }

// --- Distortion profile (stages of §4.3, applied M2/M4) -----------------------

// Each stage has its own enable flag so a playtest can switch off exactly the
// stage that is causing confusion without losing the rest of the feature.
struct DistortionProfile
{
    std::string Name;                        // stable key, e.g. "profile.default"

    // Stage 1: clarity gate.
    bool bClarityEnabled = true;
    int32_t MinClarityPerMille = 100;        // below this, no observation at all
    int32_t ClarityDistanceFalloffPerMille = 500; // clarity lost at max observation range

    // Stage 2: count distortion. Asymmetric by design: fear exaggerates,
    // it almost never understates.
    bool bCountDistortionEnabled = true;
    int32_t FearCountBiasMaxPerMille = 1500; // +150% at zero morale, full suppression
    int32_t CompetenceNoiseMaxPerMille = 400; // symmetric +-40% at zero competence

    // Stage 3: classification error (uses the confusion matrix).
    bool bClassificationErrorEnabled = true;

    // Stage 4: position error ellipse.
    bool bPositionErrorEnabled = true;
    int32_t PositionErrorMaxTiles = 4;       // at zero clarity, max observation range

    // Stage 5: omission.
    bool bOmissionEnabled = true;
    int32_t OmissionChanceMaxPerMille = 300; // at full fatigue and zero clarity

    // Stage 6: fabrication (phantoms). Separate master flag AND separate
    // multiplier: the most dangerous stage must die from one switch (§4.3.6).
    bool bFabricationEnabled = true;
    int32_t FabricationChanceMaxPerMille = 20; // per report at zero morale
    int32_t MaxPhantomLifetimeTicks = 1200;    // 60 s: guaranteed refutation window

    // Stage 7: self-report bias.
    bool bSelfReportBiasEnabled = true;
    int32_t SelfReportLossUnderstatementMaxPerMille = 500; // hides up to 50% of losses
};

// --- Confusion matrix (§4.3 stage 3) ------------------------------------------

// Designer-authored probabilities of misidentifying one unit category as another.
// Categories are coarse on purpose: per-unit-type matrices would be an authoring
// burden nobody sustains across four factions.
enum class ObservedCategory : uint8_t
{
    Infantry = 0,
    LightVehicle,
    HeavyVehicle,
    Aircraft,
    Ship,
    Structure,
    Count,
};

constexpr int32_t kObservedCategoryCount = int32_t(ObservedCategory::Count);

struct ConfusionMatrix
{
    // Rows[true][observed], per-mille; each row must sum to exactly 1000 so the
    // sampler needs no normalisation and the validator can catch authoring slips.
    int32_t PerMille[kObservedCategoryCount][kObservedCategoryCount] = {};

    ConfusionMatrix()
    {
        for (int32_t I = 0; I < kObservedCategoryCount; ++I)
        {
            PerMille[I][I] = 1000; // identity: no misclassification by default
        }
    }
};

// --- Comms profile (§4.4, applied M3) ------------------------------------------

// Delay a report accumulates at each hop of the command chain, per comms tech
// level. Courier-era orders take tens of seconds; an encrypted network is near
// instant. Indexed by the player's current comms level.
struct CommsProfile
{
    std::string Name;
    std::vector<int32_t> HopDelayTicksByLevel; // index = comms tech level
    int32_t OfficerBiasMaxPerMille = 200;      // distortion a low-quality node may add
};

// --- Track lifecycle tuning -----------------------------------------------------

struct TrackTuning
{
    int32_t ConfidenceDecayPerSecondPerMille = 20;  // -2%/s without fresh reports
    int32_t ErrorRadiusGrowthTilesPerMinute = 6;    // frozen tracks blur over time
    int32_t StaleAfterTicks = 600;                  // 30 s without reports -> bStale
    int32_t DropBelowConfidencePerMille = 50;       // GC threshold
    int32_t MergeRadiusTiles = 3;                   // spatial merge window (§4.4)
    int32_t MergeWindowTicks = 100;                 // temporal merge window
    int32_t AgreementConfidenceBonusPerMille = 300; // superlinear boost on agreement
    int32_t MaxTracksPerPlayer = 4096;              // hard cap; memory budget guard

    // Amortization budget for the decay sweep (ADR-0021 "amortized round-robin",
    // I-B4). PhaseTrackUpdate visits at most this many track slots per tick,
    // resuming from a persistent cursor, so a full sweep over MaxTracksPerPlayer
    // completes in ceil(Max/Budget) ticks regardless of load spikes. 512 @ 4096
    // cap = full sweep every 8 ticks (0.4 s at 20 Hz) -- far inside the decay
    // timescale (-2%/s), so amortization is invisible to gameplay. Decay math
    // itself lands in M2; the budget and cursor exist from M0 so the contract
    // (and its serialization) cannot fall out of the design again.
    int32_t TracksPerTickBudget = 512;
};

// --- Root settings ---------------------------------------------------------------

struct ReconSettings
{
    // Master switch. False = the perceived world mirrors ground truth exactly and
    // the distortion/propagation phases do not run. Default OFF until M2 ships so
    // every other system keeps its classic behaviour.
    bool bEnabled = false;

    std::string ActiveDistortionProfile = "profile.default";
    std::string ActiveCommsProfile = "comms.default";

    TrackTuning Tracks;
    std::vector<DistortionProfile> DistortionProfiles;
    std::vector<CommsProfile> CommsProfiles;
    ConfusionMatrix Confusion;

    const DistortionProfile* FindDistortionProfile(const std::string& InName) const;
    const CommsProfile* FindCommsProfile(const std::string& InName) const;
};

// Loads settings from a JSON document (see Content/RA4/Data/Recon/recon_settings.json).
// Returns false and fills OutErrors on any authoring mistake; a bad config must
// fail loudly at load, never surface as weird mid-match behaviour (CLAUDE.md).
RA4RECON_API bool LoadReconSettingsFromJson(const std::string& JsonText, ReconSettings& OutSettings,
                                            std::vector<std::string>& OutErrors);

// Validates ranges and cross-references (row sums, profile name resolution,
// non-negative delays). Called by the loader and directly by tests.
RA4RECON_API bool ValidateReconSettings(const ReconSettings& Settings, std::vector<std::string>& OutErrors);

} // namespace Recon
} // namespace RA4
