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

// Maps an entity definition to its confusion category. Lives here so the
// pipeline and the aggregation agree on one mapping. Content-driven inputs
// (EntityKind + role flags) keep this table-free.
RA4RECON_API ObservedCategory CategorizeForConfusion(bool bIsBuilding, bool bIsAircraft, bool bIsShip,
                                                     bool bIsInfantry, bool bIsHeavy);

} // namespace Recon
} // namespace RA4
