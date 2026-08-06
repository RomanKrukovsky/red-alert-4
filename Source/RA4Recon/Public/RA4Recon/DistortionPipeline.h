// Copyright (c) Red Alert 4 project. The distortion pipeline: how truth becomes a report.
//
// Every observation passes through these stages before it reaches a report
// (ADR-0026 §4.3). Each stage is a PURE function -- (input, params, rng) to
// output, no hidden state -- so each is unit-testable in isolation and the
// whole pipeline is deterministic from the match seed. Distortion is
// MOTIVATED, not random: fear exaggerates numbers (asymmetrically -- a scared
// observer almost never undercounts), fatigue loses observations, confusion
// follows a designer-authored matrix. All math is Fixed/integer; a float here
// would be a cross-platform desync.
//
// Stage order is fixed: Clarity -> Count -> Classification -> Position ->
// Omission. (Fabrication and self-report bias are M4 and live at the report
// level, not per-observation.) Each stage has an enable flag in the profile so
// a playtest can kill exactly the stage that confuses players.
#pragma once

#include <cstdint>

#include "RA4Core/Fixed.h"
#include "RA4Core/Random.h"
#include "RA4Recon/ReconConfig.h"
#include "RA4Recon/ReconTypes.h"

#ifndef RA4RECON_API
#define RA4RECON_API
#endif

namespace RA4
{
namespace Recon
{

// Psychological/skill inputs of one observer, sampled once per report so every
// observation in the report is distorted by the same state of mind.
struct ObserverState
{
    Fixed Competence = Fixed::FromInt(1);   // 0..1
    Fixed Morale = Fixed::FromInt(1);       // 0..1
    Fixed Fatigue = Fixed::Zero();          // 0..1
    Fixed Suppression = Fixed::Zero();      // 0..1
    Fixed DistanceRatio = Fixed::Zero();    // 0..1: distance / max observation range
    // Whether this observer is currently in contact. Fabrication (stage 6) needs
    // it because a unit resting behind the lines should stop seeing ghosts even
    // while its fatigue is still draining. NOTE: deliberately a bool rather than
    // MoraleComp::TicksUnderFire, which counts ticks since the LAST stimulus and is
    // therefore smallest when a unit is being shelled hardest.
    bool bIsUnderFire = false;
};

// --- Stage 1: clarity ---------------------------------------------------------
// How well the observer sees at all. Below MinClarity the observation is never
// born (gate; the caller checks). Falls with distance, rises with competence.
RA4RECON_API Fixed StageClarity(const ObserverState& Observer, const DistortionProfile& P);

// --- Stage 2: count distortion --------------------------------------------------
// PerceivedCount = TrueCount * (1 + FearBias + CompetenceNoise).
// FearBias >= 0 always (asymmetry requirement §4.3.2): fear only inflates.
// CompetenceNoise is symmetric, its spread grows as competence falls.
RA4RECON_API int32_t StageCountDistortion(int32_t TrueCount, const ObserverState& Observer,
                                          const DistortionProfile& P, Random& Rng);

// --- Stage 3: classification error ------------------------------------------------
// Rolls the observed category from the confusion matrix row of the true one.
// Returns the observed category; the caller maps category -> representative
// ContentId. Clarity scales the error: at clarity 1 the identity wins.
RA4RECON_API ObservedCategory StageClassification(ObservedCategory TrueCategory, Fixed Clarity,
                                                  const ConfusionMatrix& M, const DistortionProfile& P,
                                                  Random& Rng);

// --- Stage 4: position error ---------------------------------------------------
// Displaces the observed position inside an error circle whose radius grows
// with distance and shrinks with clarity. Returns the offset to add.
RA4RECON_API Vec2 StagePositionError(Fixed Clarity, const ObserverState& Observer,
                                     const DistortionProfile& P, Random& Rng);

// --- Stage 5: omission -----------------------------------------------------------
// True = the observation is simply lost (tired eyes, bad light, nobody wrote
// it down). Probability grows with fatigue and falls with clarity.
RA4RECON_API bool StageOmission(Fixed Clarity, const ObserverState& Observer,
                                const DistortionProfile& P, Random& Rng);

// --- Stage 6: fabrication (M4) ------------------------------------------------------
// True = this observer invents a contact that is not there ("something is moving
// in the trees"). The most valuable and most dangerous stage in the layer, so it
// sits behind its own enable flag AND its own multiplier -- a playtest must be
// able to kill phantoms with one switch without losing the rest of the model.
//
// Probability rises as morale falls and as time under fire accumulates: this is
// exhaustion and dread, not dice. A calm unit never fabricates, which is what
// makes a phantom informative rather than noise.
RA4RECON_API bool StageFabrication(const ObserverState& Observer, const DistortionProfile& P,
                                   Random& Rng);

// Where a fabricated contact appears: near the observer's own attention, offset by
// up to the position-error radius. A phantom in the middle of nowhere reads as a
// bug; a phantom just beyond the treeline reads as fear.
RA4RECON_API Vec2 StageFabricationOffset(const ObserverState& Observer, const DistortionProfile& P,
                                         Random& Rng);

// --- Stage 7: self-report bias (M4) -------------------------------------------------
// A unit's report about ITSELF is also distorted: a broken company with poor
// discipline overstates its remaining strength and understates its losses, because
// admitting a rout is harder than fudging a number.
//
// Owner decision D3: this affects INFORMATION PANELS only. Selection, orders and
// direct control keep using true own-unit state, so the player never loses the
// ability to command what they own -- they lose the ability to trust the summary.
RA4RECON_API int32_t StageSelfReportStrength(int32_t TrueStrength, Fixed Discipline,
                                             const DistortionProfile& P, Random& Rng);
RA4RECON_API int32_t StageSelfReportLosses(int32_t TrueLosses, Fixed Discipline,
                                           const DistortionProfile& P, Random& Rng);

// Maps an entity definition to its confusion category. Lives here so the
// pipeline and the aggregation agree on one mapping. Content-driven inputs
// (EntityKind + role flags) keep this table-free.
RA4RECON_API ObservedCategory CategorizeForConfusion(bool bIsBuilding, bool bIsAircraft, bool bIsShip,
                                                     bool bIsInfantry, bool bIsHeavy);

} // namespace Recon
} // namespace RA4
