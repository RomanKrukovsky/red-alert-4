// Copyright (c) Red Alert 4 project. Data-oriented presentation interpolator.
#include "RA4Presentation/PresentationInterpolator.h"

#include <algorithm>
#include <cmath>

namespace RA4
{
namespace Presentation
{

namespace
{
inline float AngleUnitsToDegrees(int32_t AngleUnits)
{
    const int32_t Wrapped = WrapAngle(AngleUnits);
    return (static_cast<float>(Wrapped) / static_cast<float>(kAngleTurn)) * 360.0f;
}

inline float InterpolateAngleDegrees(int32_t FromUnits, int32_t ToUnits, float Alpha)
{
    const int32_t Delta = AngleDelta(FromUnits, ToUnits);
    const float InterpUnits = static_cast<float>(FromUnits) + static_cast<float>(Delta) * Alpha;
    return AngleUnitsToDegrees(static_cast<int32_t>(std::round(InterpUnits)));
}
} // namespace

void PresentationInterpolator::Reset()
{
    CurrentSimTick = 0;
    ActiveCount = 0;
    PrevSamples.clear();
    CurrSamples.clear();
}

void PresentationInterpolator::IngestSimTick(const SimWorld& World)
{
    const uint32_t Capacity = World.GetEntityCapacity();
    if (CurrSamples.size() < Capacity)
    {
        CurrSamples.resize(Capacity);
    }
    if (PrevSamples.size() < Capacity)
    {
        PrevSamples.resize(Capacity);
    }

    // Move current samples to previous
    PrevSamples = CurrSamples;

    const auto& Cores = World.GetAllCores();
    const auto& Transforms = World.GetAllTransforms();
    const ContentDatabase* Content = World.GetContent();

    ActiveCount = 0;
    CurrentSimTick = World.GetTick();

    for (uint32_t I = 0; I < Capacity; ++I)
    {
        EntitySample& Sample = CurrSamples[I];
        const bool bAlive = (I < Cores.size()) && Cores[I].bAlive;
        Sample.bAlive = bAlive;

        if (!bAlive)
        {
            continue;
        }

        ActiveCount++;
        const EntityCore& Core = Cores[I];
        const TransformComp& Trans = Transforms[I];
        const HealthComp* Health = World.GetHealth(EntityId(I, Core.Generation));

        Sample.Position = Trans.Position;
        Sample.HullAngle = Trans.Facing;
        Sample.TurretAngle = Trans.TurretFacing;
        Sample.Def = Core.Def;
        Sample.Kind = Core.Kind;
        Sample.Owner = Core.Owner;
        Sample.Generation = Core.Generation;

        if (Health)
        {
            Sample.Health = Health->Current;
            Sample.HealthMax = Health->Max;
        }
        else if (Content)
        {
            const EntityDef* Def = Content->FindEntity(Core.Def);
            Sample.Health = Def ? Def->MaxHealth : 100;
            Sample.HealthMax = Sample.Health;
        }
        else
        {
            Sample.Health = 100;
            Sample.HealthMax = 100;
        }

        // Calculate discrete velocity from previous tick if valid
        if (I < PrevSamples.size() && PrevSamples[I].bAlive && PrevSamples[I].Generation == Core.Generation)
        {
            Sample.Velocity = Sample.Position - PrevSamples[I].Position;
        }
        else
        {
            Sample.Velocity = Vec2::Zero();
        }
    }
}

void PresentationInterpolator::InterpolateAll(float Alpha, std::vector<InterpolatedEntityState>& OutStates) const
{
    const float ClampedAlpha = std::max(0.0f, std::min(1.0f, Alpha));
    const Fixed FxAlpha = Fixed::FromRatio(static_cast<int64_t>(ClampedAlpha * 1000.0f), 1000);

    OutStates.clear();
    OutStates.reserve(ActiveCount);

    const size_t Capacity = CurrSamples.size();
    for (uint32_t I = 0; I < Capacity; ++I)
    {
        const EntitySample& Curr = CurrSamples[I];
        if (!Curr.bAlive)
        {
            continue;
        }

        InterpolatedEntityState State;
        State.Id = EntityId(I, Curr.Generation);
        State.Index = I;
        State.Def = Curr.Def;
        State.Kind = Curr.Kind;
        State.Owner = Curr.Owner;
        State.bAlive = true;
        State.HealthCurrent = Curr.Health;
        State.HealthMax = Curr.HealthMax;
        State.HealthPercent = (Curr.HealthMax > 0) ? (static_cast<float>(Curr.Health) / static_cast<float>(Curr.HealthMax)) : 1.0f;

        const bool bCanInterpolate = (I < PrevSamples.size()) && PrevSamples[I].bAlive && (PrevSamples[I].Generation == Curr.Generation);

        if (bCanInterpolate)
        {
            const EntitySample& Prev = PrevSamples[I];

            // Hermite cubic position spline with velocity tangents
            const Vec2 InterpPos = Vec2Hermite(Prev.Position, Curr.Position, Prev.Velocity, Curr.Velocity, FxAlpha);
            State.WorldX = static_cast<float>(InterpPos.X.ToDoubleUnsafe());
            State.WorldY = static_cast<float>(InterpPos.Y.ToDoubleUnsafe());
            State.WorldZ = 0.0f;

            // Shortest arc angle interpolation
            State.HullYawDegrees = InterpolateAngleDegrees(Prev.HullAngle, Curr.HullAngle, ClampedAlpha);
            State.TurretYawDegrees = InterpolateAngleDegrees(Prev.TurretAngle, Curr.TurretAngle, ClampedAlpha);
            State.bMoving = (Curr.Velocity.LengthSquared().Raw > 0);
        }
        else
        {
            // Freshly spawned: snap to current
            State.WorldX = static_cast<float>(Curr.Position.X.ToDoubleUnsafe());
            State.WorldY = static_cast<float>(Curr.Position.Y.ToDoubleUnsafe());
            State.WorldZ = 0.0f;
            State.HullYawDegrees = AngleUnitsToDegrees(Curr.HullAngle);
            State.TurretYawDegrees = AngleUnitsToDegrees(Curr.TurretAngle);
            State.bMoving = false;
        }

        OutStates.push_back(State);
    }
}

bool PresentationInterpolator::GetInterpolatedEntity(EntityId Id, float Alpha, InterpolatedEntityState& OutState) const
{
    if (Id.Index >= CurrSamples.size())
    {
        return false;
    }

    const EntitySample& Curr = CurrSamples[Id.Index];
    if (!Curr.bAlive || Curr.Generation != Id.Generation)
    {
        return false;
    }

    const float ClampedAlpha = std::max(0.0f, std::min(1.0f, Alpha));
    const Fixed FxAlpha = Fixed::FromRatio(static_cast<int64_t>(ClampedAlpha * 1000.0f), 1000);

    OutState.Id = Id;
    OutState.Index = Id.Index;
    OutState.Def = Curr.Def;
    OutState.Kind = Curr.Kind;
    OutState.Owner = Curr.Owner;
    OutState.bAlive = true;
    OutState.HealthCurrent = Curr.Health;
    OutState.HealthMax = Curr.HealthMax;
    OutState.HealthPercent = (Curr.HealthMax > 0) ? (static_cast<float>(Curr.Health) / static_cast<float>(Curr.HealthMax)) : 1.0f;

    const bool bCanInterpolate = (Id.Index < PrevSamples.size()) && PrevSamples[Id.Index].bAlive && (PrevSamples[Id.Index].Generation == Curr.Generation);

    if (bCanInterpolate)
    {
        const EntitySample& Prev = PrevSamples[Id.Index];
        const Vec2 InterpPos = Vec2Hermite(Prev.Position, Curr.Position, Prev.Velocity, Curr.Velocity, FxAlpha);
        OutState.WorldX = static_cast<float>(InterpPos.X.ToDoubleUnsafe());
        OutState.WorldY = static_cast<float>(InterpPos.Y.ToDoubleUnsafe());
        OutState.WorldZ = 0.0f;
        OutState.HullYawDegrees = InterpolateAngleDegrees(Prev.HullAngle, Curr.HullAngle, ClampedAlpha);
        OutState.TurretYawDegrees = InterpolateAngleDegrees(Prev.TurretAngle, Curr.TurretAngle, ClampedAlpha);
        OutState.bMoving = (Curr.Velocity.LengthSquared().Raw > 0);
    }
    else
    {
        OutState.WorldX = static_cast<float>(Curr.Position.X.ToDoubleUnsafe());
        OutState.WorldY = static_cast<float>(Curr.Position.Y.ToDoubleUnsafe());
        OutState.WorldZ = 0.0f;
        OutState.HullYawDegrees = AngleUnitsToDegrees(Curr.HullAngle);
        OutState.TurretYawDegrees = AngleUnitsToDegrees(Curr.TurretAngle);
        OutState.bMoving = false;
    }

    return true;
}

} // namespace Presentation
} // namespace RA4
