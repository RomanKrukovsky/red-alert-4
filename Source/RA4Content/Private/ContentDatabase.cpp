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
    VoiceSets.clear();
    EvaLines.clear();
    FactionResources.clear();
    VoiceSetIndex.clear();
    EntityIndex.clear();
    WeaponIndex.clear();
    ResourceIndex.clear();
    ResetDamageTableToDefaults();
}

EntityRole ContentDatabase::DeriveEntityRoles(const EntityDef& Def) const
{
    EntityRole Roles = EntityRole::None;

    if (Def.Kind == EntityKind::Building)
    {
        Roles |= EntityRole::BaseBuilding;
        if (Def.Building.bIsPowerPlant)
        {
            Roles |= EntityRole::Power;
        }
        if (Def.Building.bIsRefinery)
        {
            Roles |= EntityRole::Refinery;
        }
        if (Def.Building.bIsConstructionYard || Def.Production.Category == ProductionCategory::Structure)
        {
            Roles |= EntityRole::Production;
        }
        if (Def.Production.Category == ProductionCategory::Defense || Def.Weapon.IsValid())
        {
            Roles |= EntityRole::Defense;
        }
    }
    else if (Def.Kind == EntityKind::Unit)
    {
        if (Def.Unit.bIsHarvester)
        {
            Roles |= EntityRole::Harvester;
        }
        if (Def.Unit.bIsBuilder)
        {
            Roles |= EntityRole::Builder;
        }
        if (Def.Name.find("engineer") != std::string::npos)
        {
            Roles |= EntityRole::Engineer;
        }

        const bool bIsArmed = Def.Weapon.IsValid() || Def.SecondaryWeapon.IsValid();
        if (bIsArmed && !Def.Unit.bIsHarvester && !Def.Unit.bIsBuilder)
        {
            Roles |= EntityRole::Combat;
        }

        if (!Def.Unit.bIsHarvester && !Def.Unit.bIsBuilder && Def.Unit.MaxSpeed > Fixed::Zero())
        {
            if (Def.Production.Cost <= 300 || Def.Unit.MaxSpeed >= Fixed::FromInt(90) || Def.VisionRange >= Fixed::FromInt(500) ||
                Def.Name.find("scout") != std::string::npos || Def.Name.find("dog") != std::string::npos ||
                Def.Name.find("rifle") != std::string::npos || Def.Name.find("conscript") != std::string::npos)
            {
                Roles |= EntityRole::Scout;
            }
        }

        const WeaponDef* Primary = FindWeapon(Def.Weapon);
        const WeaponDef* Secondary = FindWeapon(Def.SecondaryWeapon);
        const WeaponDef* WeaponsToTest[2] = {Primary, Secondary};

        for (const WeaponDef* Wpn : WeaponsToTest)
        {
            if (Wpn == nullptr)
            {
                continue;
            }
            if (Wpn->bCanTargetAir)
            {
                Roles |= EntityRole::AntiAir;
            }
            if (Wpn->Warhead == WarheadClass::ArmorPiercing || Wpn->Warhead == WarheadClass::Siege ||
                Wpn->Warhead == WarheadClass::Plasma || Wpn->Warhead == WarheadClass::Rocket)
            {
                Roles |= EntityRole::AntiArmor;
            }
            if (Wpn->MinRange > Fixed::Zero() || Wpn->Warhead == WarheadClass::Siege)
            {
                Roles |= EntityRole::Artillery;
            }
        }
    }

    return Roles;
}

ContentId ContentDatabase::AddEntity(const EntityDef& Def)
{
    EntityDef Copy = Def;
    if (!Copy.Id.IsValid())
    {
        Copy.Id = MakeContentId(Copy.Name.c_str());
    }
    if (Copy.Roles == EntityRole::None)
    {
        Copy.Roles = DeriveEntityRoles(Copy);
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

void ContentDatabase::AddVoiceSet(const VoiceSetDef& Def)
{
    VoiceSets.push_back(Def);
    if (Def.UnitId.IsValid())
    {
        VoiceSetIndex[Def.UnitId.Value] = VoiceSets.size() - 1;
    }
}

void ContentDatabase::AddEvaLine(const EvaLineDef& Def)
{
    EvaLines.push_back(Def);
}

void ContentDatabase::SetFactionResource(const FactionResourceDef& Def)
{
    for (auto& Existing : FactionResources)
    {
        if (Existing.Faction == Def.Faction)
        {
            Existing = Def;
            return;
        }
    }
    FactionResources.push_back(Def);
}

void ContentDatabase::SetVeterancy(const VeterancyDef& Def)
{
    Veterancy = Def;
}

ContentId ContentDatabase::AddUpgrade(const UpgradeDef& Def)
{
    UpgradeDef Copy = Def;
    if (!Copy.Id.IsValid())
    {
        Copy.Id = MakeContentId(Copy.Name.c_str());
    }
    const auto Existing = UpgradeIndex.find(Copy.Id.Value);
    if (Existing != UpgradeIndex.end())
    {
        Upgrades[Existing->second] = Copy;
        return Copy.Id;
    }
    UpgradeIndex[Copy.Id.Value] = Upgrades.size();
    Upgrades.push_back(Copy);
    return Copy.Id;
}

const UpgradeDef* ContentDatabase::FindUpgrade(ContentId Id) const
{
    const auto It = UpgradeIndex.find(Id.Value);
    return It != UpgradeIndex.end() ? &Upgrades[It->second] : nullptr;
}

void ContentDatabase::SetDamageMatrix(const DamageMatrixDef& Def)
{
    for (size_t W = 0; W < size_t(WarheadClass::Count); ++W)
    {
        for (size_t A = 0; A < size_t(ArmorClass::Count); ++A)
        {
            if (Def.Multipliers[W][A] > 0)
            {
                DamageMatrix.Multipliers[W][A] = Def.Multipliers[W][A];
                DamageTable[W][A] = Def.Multipliers[W][A] / 10;
            }
        }
    }
}

const EntityDef* ContentDatabase::FindEntity(ContentId Id) const
{
    const auto It = EntityIndex.find(Id.Value);
    return It != EntityIndex.end() ? &Entities[It->second] : nullptr;
}

const EntityDef* ContentDatabase::FindEntityByName(const std::string& Name) const
{
    std::string Key = Name;
    if (Key.rfind("unit.", 0) == 0)
    {
        Key = Key.substr(5);
    }
    const EntityDef* E = FindEntity(MakeContentId(Key.c_str()));
    if (E != nullptr)
    {
        return E;
    }
    for (const auto& Entity : Entities)
    {
        if (Entity.Name == Name || Entity.Name == Key)
        {
            return &Entity;
        }
    }
    return nullptr;
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

const VoiceSetDef* ContentDatabase::FindVoiceSet(ContentId UnitId) const
{
    const auto It = VoiceSetIndex.find(UnitId.Value);
    return It != VoiceSetIndex.end() ? &VoiceSets[It->second] : nullptr;
}

const EvaLineDef* ContentDatabase::FindEva(const std::string& EventTag, const std::string& Faction) const
{
    const EvaLineDef* Best = nullptr;
    for (const auto& E : EvaLines)
    {
        if (E.EventTag != EventTag) continue;
        if (!Faction.empty() && E.Faction != Faction) continue;
        Best = &E;
        // Prefer faction-specific over generic.
        if (!Faction.empty() && E.Faction == Faction) break;
    }
    return Best;
}

const FactionResourceDef* ContentDatabase::FindFactionResource(FactionId Id) const
{
    for (const auto& R : FactionResources)
    {
        if (R.Faction == Id) return &R;
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
        DamageMatrix.Multipliers[W][A] = Percent * 10;
    }
}

void ContentDatabase::ResetDamageTableToDefaults()
{
    for (size_t W = 0; W < size_t(WarheadClass::Count); ++W)
    {
        for (size_t A = 0; A < size_t(ArmorClass::Count); ++A)
        {
            Fixed Mult = DamageMatrix::GetMultiplier(static_cast<WarheadClass>(W), static_cast<ArmorClass>(A));
            int32_t Percent = static_cast<int32_t>((Mult.Raw * 100 + Fixed::FromInt(1).Raw / 2) / Fixed::FromInt(1).Raw);
            DamageTable[W][A] = Percent;
            DamageMatrix.Multipliers[W][A] = Percent * 10;
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
        H.FeedUInt32(static_cast<uint32_t>(E.Roles));
        H.FeedInt32(E.MaxHealth);
        H.FeedUInt8(uint8_t(E.Armor));
        H.FeedInt64(E.VisionRange.Raw);
        H.FeedUInt32(E.Weapon.Value);
        H.FeedUInt32(E.SecondaryWeapon.Value);

        H.FeedInt32(E.Production.Cost);
        H.FeedInt32(E.Production.BuildTimeTicks);
        H.FeedUInt8(uint8_t(E.Production.Category));
        // ADR-0013: the tier decides whether a power deficit pauses this item, so it
        // changes match outcomes and must invalidate a replay recorded against the
        // old value.
        H.FeedUInt8(uint8_t(E.Production.Tier));
        H.FeedInt32(E.Production.CancelRefundPercent);
        for (const ContentId& P : E.Production.ProducedBy) { H.FeedUInt32(P.Value); }
        for (const ContentId& P : E.Production.Prerequisites) { H.FeedUInt32(P.Value); }
        for (const ContentId& P : E.Production.PrerequisitesGroup.AllOf) { H.FeedUInt32(P.Value); }
        for (const ContentId& P : E.Production.PrerequisitesGroup.AnyOf) { H.FeedUInt32(P.Value); }
        for (const ContentId& P : E.Production.PrerequisitesGroup.NoneOf) { H.FeedUInt32(P.Value); }

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
        // ADR-0013: Tier decides whether a power deficit pauses this item outright, and
        // TechTier::Count is a perfectly castable uint8_t. An out-of-range value
        // satisfies `>= TechTier::T2` and silently pauses the item at Severe with no
        // diagnostic anywhere, so it is caught here rather than in a match.
        if (E.Production.Tier >= TechTier::Count)
        {
            OutErrors.push_back(E.Name + ": tech tier out of range");
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
            const EntityDef* PrereqDef = FindEntity(Prereq);
            if (PrereqDef == nullptr)
            {
                OutErrors.push_back(E.Name + ": unknown prerequisite " + std::to_string(Prereq.Value));
                continue;
            }
            // ADR-0013: an item cannot sit at a lower tech tier than something it is
            // gated behind. Such an item would be exempt from the high-tech power pause
            // while only being reachable *through* high tech -- a contradiction that is
            // invisible in play and shows up as a balance oddity nobody can trace.
            if (E.Production.Tier < PrereqDef->Production.Tier)
            {
                OutErrors.push_back(E.Name + ": tech tier is below its prerequisite " +
                                    PrereqDef->Name);
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
