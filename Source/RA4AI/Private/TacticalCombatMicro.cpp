// Copyright (c) Red Alert 4 project. Advanced AI Tactical Combat Micro & Kiting.
#include "RA4AI/TacticalCombatMicro.h"
#include "RA4Content/ContentDatabase.h"

namespace RA4
{

MicroDecision TacticalCombatMicro::EvaluateKiteStep(EntityId MyUnit, EntityId ThreatTarget, const SimWorld& World, Fixed PreferredRange)
{
    MicroDecision Decision;

    if (!World.IsAlive(MyUnit) || !World.IsAlive(ThreatTarget))
    {
        Decision.Action = MicroActionType::MaintainPosition;
        return Decision;
    }

    const auto* MyTransform = World.GetTransform(MyUnit);
    const auto* ThreatTransform = World.GetTransform(ThreatTarget);
    if (!MyTransform || !ThreatTransform)
    {
        return Decision;
    }

    const Vec2 MyPos = MyTransform->Position;
    const Vec2 ThreatPos = ThreatTransform->Position;
    const Fixed DistSq = DistanceSquared(MyPos, ThreatPos);
    const Fixed SafeDist = PreferredRange - Fixed::FromInt(50);

    const auto* Combat = World.GetCombat(MyUnit);
    const bool bWeaponCooldownActive = Combat && (Combat->CooldownTicks > 0);


    if (DistSq < SafeDist * SafeDist)
    {
        // If weapon is on cooldown or the threat is dangerously close, kite backwards
        const Fixed Dist = FxSqrt(DistSq);
        if (Dist > Fixed::Zero())
        {
            const Vec2 AwayDir = MyPos - ThreatPos;
            const Fixed StepDist = Fixed::FromInt(120);
            const Vec2 Step = Vec2((AwayDir.X * StepDist) / Dist, (AwayDir.Y * StepDist) / Dist);

            Decision.Action = MicroActionType::HoldFireAndRetreat;
            Decision.TargetLocation = MyPos + Step;
            return Decision;
        }
    }

    if (!bWeaponCooldownActive && DistSq <= PreferredRange * PreferredRange)
    {
        Decision.Action = MicroActionType::EngageTarget;
        Decision.TargetEntity = ThreatTarget;
        return Decision;
    }

    Decision.Action = MicroActionType::MaintainPosition;
    return Decision;
}

EntityId TacticalCombatMicro::SelectOptimalTarget(EntityId AttackerId, const std::vector<EntityId>& CandidateTargets, const SimWorld& World)
{
    if (!World.IsAlive(AttackerId) || CandidateTargets.empty())
    {
        return EntityId::Invalid();
    }

    const auto* AttackerTransform = World.GetTransform(AttackerId);
    if (!AttackerTransform)
    {
        return EntityId::Invalid();
    }

    EntityId BestTarget = EntityId::Invalid();
    int32_t HighestPriorityScore = -1;

    for (const auto& TargetId : CandidateTargets)
    {
        if (!World.IsAlive(TargetId))
        {
            continue;
        }

        const auto* Health = World.GetHealth(TargetId);
        const auto* Core = World.GetCore(TargetId);
        if (!Health || !Core)
        {
            continue;
        }

        const int32_t MaxHp = Health->Max > 0 ? Health->Max : 1;
        const int32_t HealthPercent = (Health->Current * 100) / MaxHp;

        int32_t UnitCost = 100;
        if (World.GetContent())
        {
            const auto* Def = World.GetContent()->FindEntity(Core->Def);
            if (Def)
            {
                UnitCost = Def->Production.Cost;
            }
        }

        // Priority formula: prioritize low HP enemies (to eliminate firepower) + high value targets
        const int32_t KillUrgencyScore = (100 - HealthPercent) * 15;
        const int32_t ValueScore = UnitCost / 10;
        const int32_t TotalScore = KillUrgencyScore + ValueScore;

        if (TotalScore > HighestPriorityScore)
        {
            HighestPriorityScore = TotalScore;
            BestTarget = TargetId;
        }
    }

    return BestTarget;
}

} // namespace RA4
