// Copyright (c) Red Alert 4 project. Advanced AI Tactical Combat Micro & Kiting.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{

enum class MicroActionType : uint8_t
{
    HoldFireAndRetreat,
    EngageTarget,
    MaintainPosition
};

struct MicroDecision
{
    MicroActionType Action = MicroActionType::MaintainPosition;
    Vec2 TargetLocation;
    EntityId TargetEntity = EntityId::Invalid();
};

class RA4AI_API TacticalCombatMicro
{
public:
    TacticalCombatMicro() = default;

    /** Evaluates whether a ranged unit should stutter-step / kite backwards away from a dangerous approaching enemy. */
    static MicroDecision EvaluateKiteStep(EntityId MyUnit, EntityId ThreatTarget, const SimWorld& World, Fixed PreferredRange);

    /** Evaluates and selects the highest-value / most killable enemy among candidates to concentrate firepower (focus fire). */
    static EntityId SelectOptimalTarget(EntityId AttackerId, const std::vector<EntityId>& CandidateTargets, const SimWorld& World);
};

} // namespace RA4
