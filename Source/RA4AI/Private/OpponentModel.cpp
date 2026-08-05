// Copyright (c) Red Alert 4 project.
#include "RA4AI/OpponentModel.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/SimTypes.h"

#include <algorithm>
#include <cmath>

namespace RA4
{
namespace AI
{

void OpponentModel::Reset()
{
    for (int32_t I = 0; I < kMaxTrackedPlayers; ++I)
    {
        Profiles[I] = OpponentProfile{};
    }
}

int32_t OpponentModel::EmaUpdate(int32_t Old, int32_t NewSample)
{
    // Exponential moving average with alpha = 1/10.
    // Integer-only: NewValue = (Old * 9 + NewSample) / 10
    // Clamped to 0..100 for ratio fields.
    const int32_t Result = (Old * 9 + NewSample) / 10;
    return std::max(0, std::min(100, Result));
}

int32_t OpponentModel::ClassifyDirection(int32_t AttackerX, int32_t AttackerY,
                                         int32_t TargetX, int32_t TargetY)
{
    // Quadrant index: 0=NW, 1=NE, 2=SW, 3=SE
    const int32_t Dy = AttackerY - TargetY; // negative = attacker is north
    const int32_t Dx = AttackerX - TargetX;
    const int32_t Vertical = Dy < 0 ? 0 : 2;
    const int32_t Horizontal = Dx < 0 ? 0 : 1;
    return Vertical + Horizontal;
}

void OpponentModel::UpdateFromEvents(const SimEvent* Events, int32_t EventCount,
                                     PlayerId Self, TickIndex Now)
{
    if (Events == nullptr || EventCount <= 0)
    {
        return;
    }

    for (int32_t I = 0; I < EventCount; ++I)
    {
        const SimEvent& E = Events[I];

        if (E.Player == Self || E.Player >= kMaxTrackedPlayers)
        {
            continue;
        }

        OpponentProfile& P = Profiles[E.Player];
        P.LastObservationTick = Now;

        switch (E.Type)
        {
            case SimEventType::DamageApplied:
                P.AttacksObserved = std::min(P.AttacksObserved + 1, 10000);
                P.LastAttackTick = Now;
                P.Aggressiveness = EmaUpdate(P.Aggressiveness, 100);
                break;

            case SimEventType::BuildingCompleted:
                P.BuildingsObserved = std::min(P.BuildingsObserved + 1, 10000);
                P.ExpansionRate = EmaUpdate(P.ExpansionRate, 80);
                break;

            case SimEventType::ProductionCompleted:
                P.UnitsObserved = std::min(P.UnitsObserved + 1, 10000);
                break;

            case SimEventType::EntityDestroyed:
                if (E.Player != Self)
                {
                    P.TotalUnitsLost = std::min(P.TotalUnitsLost + 1, 10000);
                }
                break;

            case SimEventType::WeaponFired:
                P.Aggressiveness = EmaUpdate(P.Aggressiveness, 70);
                break;

            default:
                break;
        }
    }

    // Decay aggression toward 50 if no attacks recently.
    for (int32_t I = 0; I < kMaxTrackedPlayers; ++I)
    {
        OpponentProfile& P = Profiles[I];
        if (P.LastAttackTick > 0 && Now > P.LastAttackTick + 600)
        {
            P.Aggressiveness = EmaUpdate(P.Aggressiveness, 50);
        }
    }
}

const OpponentProfile& OpponentModel::GetProfile(PlayerId Enemy) const
{
    if (Enemy >= kMaxTrackedPlayers)
    {
        static const OpponentProfile Empty;
        return Empty;
    }
    return Profiles[Enemy];
}

bool OpponentModel::EnemyPrefersAir(PlayerId Enemy) const
{
    if (Enemy >= kMaxTrackedPlayers) return false;
    return Profiles[Enemy].AirPreference > 40;
}

bool OpponentModel::EnemyPrefersArmor(PlayerId Enemy) const
{
    if (Enemy >= kMaxTrackedPlayers) return false;
    return Profiles[Enemy].ArmorRatio > 40;
}

bool OpponentModel::EnemyIsAggressive(PlayerId Enemy) const
{
    if (Enemy >= kMaxTrackedPlayers) return false;
    return Profiles[Enemy].Aggressiveness > 60;
}

int32_t OpponentModel::GetPreferredAttackDirection(PlayerId Enemy) const
{
    if (Enemy >= kMaxTrackedPlayers) return 0;
    return Profiles[Enemy].PreferredAttackDirection;
}

} // namespace AI
} // namespace RA4
