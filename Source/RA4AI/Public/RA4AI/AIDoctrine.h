// Copyright (c) Red Alert 4 project. Faction Doctrines and Commander Personalities.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "RA4AI/AIStrategy.h"
#include "RA4Content/ContentTypes.h"
#include "RA4Core/Ids.h"


#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

enum class AIDoctrineType : uint8_t
{
    SovietArmoredPush = 0,    // Soviet: Heavy front, armored push, V2 artillery prep
    AllianceMobilePrecision,  // Alliance: High mobility, recon, flanking, high-value unit preservation
    CoalitionSynchronized,   // Coalition: Formation shields, area denial, synchronized strikes
    ChronoTemporalHarass     // Chrono: Hit-and-run, temporal abilities, mobile reserves
};

struct AIPersonality
{
    std::string Name = "General";
    AIDoctrineType Doctrine = AIDoctrineType::SovietArmoredPush;

    int32_t Aggressiveness = 50;           // 0..100
    int32_t Cautiousness = 50;             // 0..100
    int32_t EconomicRisk = 50;             // 0..100
    int32_t ScoutPriority = 70;            // 0..100
    int32_t AcceptableLossesPercent = 40;  // 0..100
    int32_t ReserveDepthPercent = 20;     // 0..100
    int32_t FlankingTendency = 30;         // 0..100
    int32_t RegroupFrequencyTicks = 40;    // decision ticks between regroups
    int32_t ThreatSensitivity = 60;        // 0..100

    // Preferred army composition ratios by role
    int32_t RatioInfantry = 30;
    int32_t RatioAntiArmor = 30;
    int32_t RatioAntiAir = 20;
    int32_t RatioArtillery = 10;
    int32_t RatioSupport = 10;
};

struct FactionDoctrineDef
{
    FactionId Faction = FactionId::Soviet;
    AIDoctrineType Type = AIDoctrineType::SovietArmoredPush;
    std::string Name;
    std::string Description;

    // Minimum target army size before launching major assaults
    int32_t MinimumAssaultArmySize = 10;

    // Build order priorities
    int32_t TargetHarvesterCount = 3;
    int32_t PowerPlantBuffer = 50; // Keep 50 surplus power

    // Role composition
    AIPersonality Personality;
};

class RA4AI_API AIDoctrineRegistry
{
public:
    static FactionDoctrineDef GetDoctrineForFaction(FactionId Faction, AIProfile Profile);
    static AIPersonality CreatePersonality(AIDoctrineType Doctrine, AIProfile Profile);
};

} // namespace AI
} // namespace RA4
