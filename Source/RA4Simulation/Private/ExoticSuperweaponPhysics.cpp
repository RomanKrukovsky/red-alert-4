// Copyright (c) Red Alert 4 project. Exotic superweapon mechanics implementation.
#include "RA4Simulation/ExoticSuperweaponPhysics.h"

#include <algorithm>
#include <vector>

namespace RA4
{

void VacuumImploderState::Update(SimWorld& World)
{
    if (bCompleted) return;

    ++ElapsedTicks;

    const auto& Cores = World.GetAllCores();
    const auto& Transforms = World.GetAllTransforms();
    const Fixed RadiusSq = Radius * Radius;

    for (uint32_t I = 0; I < Cores.size(); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }

        const Vec2 Pos = Transforms[I].Position;
        const Fixed DistSq = DistanceSquared(Pos, Epicenter);
        if (DistSq <= RadiusSq)
        {
            const Fixed Dist = FxSqrt(DistSq);
            if (Dist > Fixed::FromInt(5))
            {
                const Vec2 Diff = Epicenter - Pos;
                const Fixed StepDist = PullSpeed * Fixed::FromRatio(1, 20); // 1 tick displacement @ 20Hz
                const Vec2 Step = Vec2((Diff.X * StepDist) / Dist, (Diff.Y * StepDist) / Dist);
                World.TeleportEntity(World.MakeId(I), Pos + Step);
            }
        }
    }

    if (ElapsedTicks >= TotalTicks)
    {
        World.ApplySplashDamage(Epicenter, Radius, FinalDamage, WarheadClass::Siege, 80, EntityId::Invalid(), kInvalidPlayer);
        bCompleted = true;
    }
}

void ExoticSuperweaponPhysics::TriggerVacuumImploder(const Vec2& Epicenter, Fixed Radius, int32_t Damage)
{
    VacuumImploderState State;
    State.Epicenter = Epicenter;
    State.Radius = Radius;
    State.FinalDamage = Damage;
    ActiveImploders.push_back(State);
}

void ExoticSuperweaponPhysics::TriggerIronCurtain(const Vec2& Center, Fixed Radius, uint32_t DurationTicks, SimWorld& World)
{
    (void)DurationTicks;
    const auto& Cores = World.GetAllCores();
    const auto& Transforms = World.GetAllTransforms();
    const Fixed RadiusSq = Radius * Radius;

    for (uint32_t I = 0; I < Cores.size(); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }

        const Vec2 Pos = Transforms[I].Position;
        if (DistanceSquared(Pos, Center) <= RadiusSq)
        {
            const EntityId Id = World.MakeId(I);
            const auto* Core = World.GetCore(Id);
            if (Core && World.GetContent())
            {
                const auto* Def = World.GetContent()->FindEntity(Core->Def);
                if (Def && (Def->Armor == ArmorClass::LightInfantry || Def->Armor == ArmorClass::HeavyInfantry))
                {
                    // Lore rule: Iron Curtain vaporizes biological infantry instantly
                    World.DestroyEntity(Id, EntityId::Invalid(), false);
                }
                else if (Def)
                {
                    World.Healths[Id.Index].bInvulnerable = true;
                }
            }
        }
    }
}

void ExoticSuperweaponPhysics::TriggerChronoSphere(const Vec2& SourceCenter, const Vec2& TargetCenter, Fixed Radius, SimWorld& World)
{
    const auto& Cores = World.GetAllCores();
    const auto& Transforms = World.GetAllTransforms();
    const Fixed RadiusSq = Radius * Radius;

    std::vector<EntityId> UnitsToTeleport;
    for (uint32_t I = 0; I < Cores.size(); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Kind != EntityKind::Unit)
        {
            continue;
        }

        const Vec2 Pos = Transforms[I].Position;
        if (DistanceSquared(Pos, SourceCenter) <= RadiusSq)
        {
            UnitsToTeleport.push_back(World.MakeId(I));
        }
    }

    for (const auto& Id : UnitsToTeleport)
    {
        if (!World.IsAlive(Id)) continue;

        const Vec2 OldPos = World.GetTransform(Id)->Position;
        const Vec2 Offset = OldPos - SourceCenter;
        const Vec2 NewPos = TargetCenter + Offset;

        World.TeleportEntity(Id, NewPos);

        const TileCoord Tile = World.GetMap().WorldToTile(NewPos);
        const uint8_t TileFlagsVal = World.GetMap().GetTile(Tile.X, Tile.Y);

        if ((TileFlagsVal & Tile_Water) != 0)
        {
            const auto* Core = World.GetCore(Id);
            if (Core && World.GetContent())
            {
                const auto* Def = World.GetContent()->FindEntity(Core->Def);
                if (Def && Def->Unit.Layer != MovementLayer::Naval && Def->Unit.Layer != MovementLayer::Amphibious)
                {
                    // Non-amphibious ground vehicles dropped into water sink and drown
                    World.DestroyEntity(Id, EntityId::Invalid(), false);
                }
            }
        }
    }
}



void ExoticSuperweaponPhysics::Tick(SimWorld& World)
{
    for (auto& Imploder : ActiveImploders)
    {
        Imploder.Update(World);
    }

    ActiveImploders.erase(
        std::remove_if(ActiveImploders.begin(), ActiveImploders.end(),
                       [](const VacuumImploderState& S) { return S.bCompleted; }),
        ActiveImploders.end());
}

} // namespace RA4
