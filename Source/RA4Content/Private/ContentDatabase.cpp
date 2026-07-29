// Copyright (c) Red Alert 4 project.
#include "RA4Content/ContentDatabase.h"

#include "RA4Content/DamageMatrix.h"
#include "RA4Core/Checksum.h"

namespace RA4
{

void ContentDatabase::Clear()
{
    Entities.clear();
    Weapons.clear();
    ResourceNodes.clear();
    Factions.clear();
    EntityIndex.clear();
    WeaponIndex.clear();
    ResourceIndex.clear();
    ResetDamageTableToDefaults();
}

ContentId ContentDatabase::AddEntity(const EntityDef& Def)
{
    EntityDef Copy = Def;
    if (!Copy.Id.IsValid())
    {
        Copy.Id = MakeContentId(Copy.Name.c_str());
    }
    const auto Existing = EntityIndex.find(Copy.Id.Value);
    if (Existing != EntityIndex.end())
    {
        Entities[Existing->second] = Copy;   // redefinition: mods override base content
        return Copy.Id;
    }
    EntityIndex[Copy.Id.Value] = Entities.size();
    Entities.push_back(Copy);
    return Copy.Id;
}

ContentId ContentDatabase::AddWeapon(const WeaponDef& Def)
{
    WeaponDef Copy = Def;
    if (!Copy.Id.IsValid())
    {
        Copy.Id = MakeContentId(Copy.Name.c_str());
    }
    const auto Existing = WeaponIndex.find(Copy.Id.Value);
    if (Existing != WeaponIndex.end())
    {
        Weapons[Existing->second] = Copy;
        return Copy.Id;
    }
    WeaponIndex[Copy.Id.Value] = Weapons.size();
    Weapons.push_back(Copy);
    return Copy.Id;
}

ContentId ContentDatabase::AddResourceNode(const ResourceNodeDef& Def)
{
    ResourceNodeDef Copy = Def;
    if (!Copy.Id.IsValid())
    {
        Copy.Id = MakeContentId(Copy.Name.c_str());
    }
    const auto Existing = ResourceIndex.find(Copy.Id.Value);
    if (Existing != ResourceIndex.end())
    {
        ResourceNodes[Existing->second] = Copy;
        return Copy.Id;
    }
    ResourceIndex[Copy.Id.Value] = ResourceNodes.size();
    ResourceNodes.push_back(Copy);
    return Copy.Id;
}

void ContentDatabase::AddFaction(const FactionDef& Def)
{
    Factions.push_back(Def);
}

const EntityDef* ContentDatabase::FindEntity(ContentId Id) const
{
    const auto It = EntityIndex.find(Id.Value);
    return It != EntityIndex.end() ? &Entities[It->second] : nullptr;
}

const EntityDef* ContentDatabase::FindEntityByName(const std::string& Name) const
{
    return FindEntity(MakeContentId(Name.c_str()));
}

const WeaponDef* ContentDatabase::FindWeapon(ContentId Id) const
{
    const auto It = WeaponIndex.find(Id.Value);
    return It != WeaponIndex.end() ? &Weapons[It->second] : nullptr;
}

const ResourceNodeDef* ContentDatabase::FindResourceNode(ContentId Id) const
{
    const auto It = ResourceIndex.find(Id.Value);
    return It != ResourceIndex.end() ? &ResourceNodes[It->second] : nullptr;
}

const FactionDef* ContentDatabase::FindFaction(FactionId Id) const
{
    for (const auto& F : Factions)
    {
        if (F.Id == Id) return &F;
    }
    return nullptr;
}

int32_t ContentDatabase::GetDamageMultiplier(WarheadClass Warhead, ArmorClass Armor) const
{
    const size_t W = size_t(Warhead);
    const size_t A = size_t(Armor);
    if (W >= size_t(WarheadClass::Count) || A >= size_t(ArmorClass::Count))
    {
        return 100;
    }
    return DamageTable[W][A];
}

void ContentDatabase::SetDamageMultiplier(WarheadClass Warhead, ArmorClass Armor, int32_t Percent)
{
    const size_t W = size_t(Warhead);
    const size_t A = size_t(Armor);
    if (W < size_t(WarheadClass::Count) && A < size_t(ArmorClass::Count))
    {
        DamageTable[W][A] = Percent;
    }
}

void ContentDatabase::ResetDamageTableToDefaults()
{
    for (size_t W = 0; W < size_t(WarheadClass::Count); ++W)
    {
        for (size_t A = 0; A < size_t(ArmorClass::Count); ++A)
        {
            Fixed Mult = DamageMatrix::GetMultiplier(static_cast<WarheadClass>(W), static_cast<ArmorClass>(A));
            DamageTable[W][A] = static_cast<int32_t>(Mult.Raw * 100 / Fixed::FromInt(1).Raw);
        }
    }
}

uint64_t ContentDatabase::ComputeContentHash() const
{
    Hash64 H;

    for (const WeaponDef& W : Weapons)
    {
        H.FeedUInt32(W.Id.Value);
        H.FeedInt32(W.Damage);
        H.FeedUInt8(uint8_t(W.Warhead));
        H.FeedInt64(W.MinRange.Raw);
        H.FeedInt64(W.MaxRange.Raw);
        H.FeedInt32(W.CooldownTicks);
        H.FeedInt32(W.BurstCount);
        H.FeedInt32(W.BurstDelayTicks);
        H.FeedInt64(W.ProjectileSpeed.Raw);
        H.FeedInt64(W.SplashRadius.Raw);
        H.FeedInt32(W.SplashFalloffPercent);
        H.FeedBool(W.bCanTargetGround);
        H.FeedBool(W.bCanTargetAir);
        H.FeedBool(W.bRequiresTurretAligned);
        H.FeedInt64(W.ScatterAtMaxRange.Raw);
    }

    for (const EntityDef& E : Entities)
    {
        H.FeedUInt32(E.Id.Value);
        H.FeedUInt8(uint8_t(E.Kind));
        H.FeedUInt8(uint8_t(E.Faction));
        H.FeedInt32(E.MaxHealth);
        H.FeedUInt8(uint8_t(E.Armor));
        H.FeedInt64(E.VisionRange.Raw);
        H.FeedUInt32(E.Weapon.Value);
        H.FeedUInt32(E.SecondaryWeapon.Value);

        H.FeedInt32(E.Production.Cost);
        H.FeedInt32(E.Production.BuildTimeTicks);
        H.FeedUInt8(uint8_t(E.Production.Category));
        H.FeedInt32(E.Production.CancelRefundPercent);
        for (const ContentId& P : E.Production.ProducedBy) { H.FeedUInt32(P.Value); }
        for (const ContentId& P : E.Production.Prerequisites) { H.FeedUInt32(P.Value); }

        H.FeedInt32(E.Building.FootprintX);
        H.FeedInt32(E.Building.FootprintY);
        H.FeedInt32(E.Building.PowerProduced);
        H.FeedInt32(E.Building.PowerConsumed);
        H.FeedBool(E.Building.bIsConstructionYard);
        H.FeedBool(E.Building.bIsRefinery);
        H.FeedBool(E.Building.bIsPowerPlant);
        H.FeedBool(E.Building.bIsRadar);
        H.FeedBool(E.Building.bProvidesBuildRadius);
        H.FeedInt64(E.Building.BuildRadius.Raw);
        H.FeedUInt32(E.Building.BundledUnit.Value);
        H.FeedInt32(E.Building.SellRefundPercent);

        H.FeedUInt8(uint8_t(E.Unit.Layer));
        H.FeedInt64(E.Unit.MaxSpeed.Raw);
        H.FeedInt64(E.Unit.Acceleration.Raw);
        H.FeedInt32(E.Unit.TurnRatePerSecond);
        H.FeedInt32(E.Unit.TurretTurnRatePerSecond);
        H.FeedInt64(E.Unit.CollisionRadius.Raw);
        H.FeedBool(E.Unit.bIsHarvester);
        H.FeedInt32(E.Unit.CargoCapacity);
        H.FeedInt32(E.Unit.HarvestPerTick);
        H.FeedInt32(E.Unit.UnloadPerTick);
        H.FeedBool(E.Unit.bCanCrushInfantry);
        H.FeedBool(E.Unit.bIsBuilder);
        H.FeedUInt32(E.Unit.DeploysInto.Value);
    }

    for (const ResourceNodeDef& R : ResourceNodes)
    {
        H.FeedUInt32(R.Id.Value);
        H.FeedInt32(R.InitialAmount);
        H.FeedInt32(R.ValuePerUnit);
        H.FeedBool(R.bRegrows);
        H.FeedInt32(R.RegrowPerTick);
        H.FeedInt32(R.MaxAmount);
    }

    for (size_t W = 0; W < size_t(WarheadClass::Count); ++W)
    {
        for (size_t A = 0; A < size_t(ArmorClass::Count); ++A)
        {
            H.FeedInt32(DamageTable[W][A]);
        }
    }

    return H.Get();
}

bool ContentDatabase::Validate(std::vector<std::string>& OutErrors) const
{
    const size_t Before = OutErrors.size();

    for (const EntityDef& E : Entities)
    {
        if (E.Name.empty())
        {
            OutErrors.push_back("Entity with id " + std::to_string(E.Id.Value) + " has an empty name");
        }
        if (E.DisplayNameKey.empty())
        {
            OutErrors.push_back(E.Name + ": missing DisplayNameKey (all player-facing text must be localized)");
        }
        if (E.MaxHealth <= 0)
        {
            OutErrors.push_back(E.Name + ": MaxHealth must be positive");
        }
        if (E.Production.Cost < 0)
        {
            OutErrors.push_back(E.Name + ": negative cost");
        }
        if (E.Weapon.IsValid() && FindWeapon(E.Weapon) == nullptr)
        {
            OutErrors.push_back(E.Name + ": references unknown weapon " + std::to_string(E.Weapon.Value));
        }
        if (E.SecondaryWeapon.IsValid() && FindWeapon(E.SecondaryWeapon) == nullptr)
        {
            OutErrors.push_back(E.Name + ": references unknown secondary weapon");
        }
        for (const ContentId& Prereq : E.Production.Prerequisites)
        {
            if (FindEntity(Prereq) == nullptr)
            {
                OutErrors.push_back(E.Name + ": unknown prerequisite " + std::to_string(Prereq.Value));
            }
        }
        for (const ContentId& Producer : E.Production.ProducedBy)
        {
            const EntityDef* ProducerDef = FindEntity(Producer);
            if (ProducerDef == nullptr)
            {
                OutErrors.push_back(E.Name + ": unknown producer " + std::to_string(Producer.Value));
            }
            else if (ProducerDef->Kind != EntityKind::Building)
            {
                OutErrors.push_back(E.Name + ": producer " + ProducerDef->Name + " is not a building");
            }
        }
        if (E.Kind == EntityKind::Building)
        {
            if (E.Building.FootprintX <= 0 || E.Building.FootprintY <= 0)
            {
                OutErrors.push_back(E.Name + ": building footprint must be at least 1x1");
            }
            if (E.Building.bIsPowerPlant && E.Building.PowerProduced <= 0)
            {
                OutErrors.push_back(E.Name + ": flagged as power plant but produces no power");
            }
            if (E.Building.BundledUnit.IsValid() && FindEntity(E.Building.BundledUnit) == nullptr)
            {
                OutErrors.push_back(E.Name + ": bundled unit does not exist");
            }
        }
        if (E.Kind == EntityKind::Unit)
        {
            if (E.Unit.MaxSpeed <= Fixed::Zero())
            {
                OutErrors.push_back(E.Name + ": unit has non-positive MaxSpeed");
            }
            if (E.Unit.bIsHarvester && (E.Unit.CargoCapacity <= 0 || E.Unit.HarvestPerTick <= 0))
            {
                OutErrors.push_back(E.Name + ": harvester needs positive CargoCapacity and HarvestPerTick");
            }
            if (E.Unit.bIsBuilder && !E.Unit.DeploysInto.IsValid())
            {
                OutErrors.push_back(E.Name + ": builder unit has no DeploysInto target");
            }
        }
    }

    for (const WeaponDef& W : Weapons)
    {
        if (W.MaxRange <= W.MinRange)
        {
            OutErrors.push_back(W.Name + ": MaxRange must exceed MinRange");
        }
        if (W.CooldownTicks <= 0)
        {
            OutErrors.push_back(W.Name + ": CooldownTicks must be positive");
        }
        if (W.Damage < 0)
        {
            OutErrors.push_back(W.Name + ": negative damage");
        }
        if (!W.bCanTargetGround && !W.bCanTargetAir)
        {
            OutErrors.push_back(W.Name + ": weapon can target neither ground nor air");
        }
    }

    return OutErrors.size() == Before;
}

} // namespace RA4
