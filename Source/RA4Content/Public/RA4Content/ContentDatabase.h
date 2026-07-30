// Copyright (c) Red Alert 4 project.
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#if __has_include("HAL/Platform.h")
#include "HAL/Platform.h"
#endif

#include "RA4Content/ContentTypes.h"

#ifndef RA4CONTENT_API
#define RA4CONTENT_API
#endif

namespace RA4
{

// Immutable-after-load registry of every definition in a match.
//
// Lookup is a hash map, but iteration is always over the insertion-ordered vectors:
// unordered_map iteration order differs between libstdc++ and libc++, and anything
// that feeds simulation state must not depend on standard library internals.
class RA4CONTENT_API ContentDatabase
{
public:
    void Clear();

    ContentId AddEntity(const EntityDef& Def);
    ContentId AddWeapon(const WeaponDef& Def);
    ContentId AddResourceNode(const ResourceNodeDef& Def);
    void AddFaction(const FactionDef& Def);
    void AddVoiceSet(const VoiceSetDef& Def);
    void AddEvaLine(const EvaLineDef& Def);
    void SetFactionResource(const FactionResourceDef& Def);
    void SetVeterancy(const VeterancyDef& Def);
    void SetDamageMatrix(const DamageMatrixDef& Def);

    const EntityDef* FindEntity(ContentId Id) const;
    const EntityDef* FindEntityByName(const std::string& Name) const;
    const WeaponDef* FindWeapon(ContentId Id) const;
    const ResourceNodeDef* FindResourceNode(ContentId Id) const;
    const FactionDef* FindFaction(FactionId Id) const;
    const VoiceSetDef* FindVoiceSet(ContentId UnitId) const;
    const EvaLineDef* FindEva(const std::string& EventTag, const std::string& Faction) const;
    const FactionResourceDef* FindFactionResource(FactionId Id) const;
    const VeterancyDef& GetVeterancy() const { return Veterancy; }
    const DamageMatrixDef& GetDamageMatrix() const { return DamageMatrix; }

    const std::vector<EntityDef>& GetEntities() const { return Entities; }
    const std::vector<WeaponDef>& GetWeapons() const { return Weapons; }
    const std::vector<VoiceSetDef>& GetVoiceSets() const { return VoiceSets; }
    const std::vector<EvaLineDef>& GetEvaLines() const { return EvaLines; }
    const std::vector<FactionResourceDef>& GetFactionResources() const { return FactionResources; }

    // Damage multiplier in percent for a warhead against an armor class.
    int32_t GetDamageMultiplier(WarheadClass Warhead, ArmorClass Armor) const;
    void SetDamageMultiplier(WarheadClass Warhead, ArmorClass Armor, int32_t Percent);
    void ResetDamageTableToDefaults();

    // Content hash covers every value that can change simulation outcomes. Clients
    // and server compare it during the lobby handshake; a mismatch means someone is
    // running different data and would desync within seconds.
    uint64_t ComputeContentHash() const;

    // Fails loudly on unresolvable prerequisites, unknown weapon references,
    // negative costs and other authoring mistakes that would otherwise surface as a
    // mid-match crash or an unbuildable tech tree.
    bool Validate(std::vector<std::string>& OutErrors) const;

private:
    std::vector<EntityDef> Entities;
    std::vector<WeaponDef> Weapons;
    std::vector<ResourceNodeDef> ResourceNodes;
    std::vector<FactionDef> Factions;
    std::vector<VoiceSetDef> VoiceSets;
    std::vector<EvaLineDef> EvaLines;
    std::vector<FactionResourceDef> FactionResources;
    VeterancyDef Veterancy;
    DamageMatrixDef DamageMatrix;

    std::unordered_map<uint32_t, size_t> EntityIndex;
    std::unordered_map<uint32_t, size_t> WeaponIndex;
    std::unordered_map<uint32_t, size_t> ResourceIndex;
    std::unordered_map<uint32_t, size_t> VoiceSetIndex;

    int32_t DamageTable[size_t(WarheadClass::Count)][size_t(ArmorClass::Count)] = {};
};

// Builds the shipping content set for the current milestone. This is the temporary
// home for definitions until the Data Asset pipeline lands; the same structs are
// produced either way, so nothing downstream changes when it does.
RA4CONTENT_API void BuildDefaultContent(ContentDatabase& Db);

} // namespace RA4
