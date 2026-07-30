// Copyright (c) Red Alert 4 project. AIDoctrine registry implementation.
#include "RA4AI/AIDoctrine.h"

namespace RA4
{
namespace AI
{

FactionDoctrineDef AIDoctrineRegistry::GetDoctrineForFaction(FactionId Faction, AIProfile Profile)
{
    FactionDoctrineDef Doc;
    Doc.Faction = Faction;

    switch (Faction)
    {
    case FactionId::Soviet:
        Doc.Type = AIDoctrineType::SovietArmoredPush;
        Doc.Name = "Soviet Armored Push";
        Doc.Description = "Mass armor, V2 artillery prep, relentless front assault.";
        Doc.MinimumAssaultArmySize = 12;
        Doc.TargetHarvesterCount = 4;
        Doc.PowerPlantBuffer = 60;
        Doc.Personality = CreatePersonality(Doc.Type, Profile);
        break;

    case FactionId::Alliance:
        Doc.Type = AIDoctrineType::AllianceMobilePrecision;
        Doc.Name = "Alliance Mobile Precision";
        Doc.Description = "High mobility, recon, surgical strikes, air & light armor.";
        Doc.MinimumAssaultArmySize = 8;
        Doc.TargetHarvesterCount = 3;
        Doc.PowerPlantBuffer = 40;
        Doc.Personality = CreatePersonality(Doc.Type, Profile);
        break;

    case FactionId::EasternCoalition:
        Doc.Type = AIDoctrineType::CoalitionSynchronized;
        Doc.Name = "Coalition Synchronized";
        Doc.Description = "Synchronized formations, shield protection, area denial.";
        Doc.MinimumAssaultArmySize = 10;
        Doc.TargetHarvesterCount = 3;
        Doc.PowerPlantBuffer = 50;
        Doc.Personality = CreatePersonality(Doc.Type, Profile);
        break;

    case FactionId::ChronoLegion:
        Doc.Type = AIDoctrineType::ChronoTemporalHarass;
        Doc.Name = "Chrono Temporal Harass";
        Doc.Description = "Hit-and-run tactics, phase shifts, rapid redeployment.";
        Doc.MinimumAssaultArmySize = 6;
        Doc.TargetHarvesterCount = 3;
        Doc.PowerPlantBuffer = 40;
        Doc.Personality = CreatePersonality(Doc.Type, Profile);
        break;

    default:
        Doc.Type = AIDoctrineType::SovietArmoredPush;
        Doc.Personality = CreatePersonality(Doc.Type, Profile);
        break;
    }

    return Doc;
}

AIPersonality AIDoctrineRegistry::CreatePersonality(AIDoctrineType Doctrine, AIProfile Profile)
{
    AIPersonality P;
    P.Doctrine = Doctrine;

    switch (Doctrine)
    {
    case AIDoctrineType::SovietArmoredPush:
        P.Name = "General Sokolov";
        P.Aggressiveness = 75;
        P.Cautiousness = 30;
        P.EconomicRisk = 40;
        P.ScoutPriority = 50;
        P.AcceptableLossesPercent = 60;
        P.ReserveDepthPercent = 15;
        P.FlankingTendency = 20;
        P.RatioInfantry = 25;
        P.RatioAntiArmor = 45;
        P.RatioAntiAir = 15;
        P.RatioArtillery = 15;
        break;

    case AIDoctrineType::AllianceMobilePrecision:
        P.Name = "Commander Hart";
        P.Aggressiveness = 45;
        P.Cautiousness = 70;
        P.EconomicRisk = 30;
        P.ScoutPriority = 85;
        P.AcceptableLossesPercent = 25;
        P.ReserveDepthPercent = 30;
        P.FlankingTendency = 65;
        P.RatioInfantry = 30;
        P.RatioAntiArmor = 30;
        P.RatioAntiAir = 25;
        P.RatioArtillery = 15;
        break;

    case AIDoctrineType::CoalitionSynchronized:
        P.Name = "Marshal Mei";
        P.Aggressiveness = 55;
        P.Cautiousness = 60;
        P.EconomicRisk = 50;
        P.ScoutPriority = 60;
        P.AcceptableLossesPercent = 35;
        P.ReserveDepthPercent = 25;
        P.FlankingTendency = 40;
        P.RatioInfantry = 35;
        P.RatioAntiArmor = 35;
        P.RatioAntiAir = 15;
        P.RatioArtillery = 15;
        break;

    case AIDoctrineType::ChronoTemporalHarass:
        P.Name = "Archon Voss";
        P.Aggressiveness = 60;
        P.Cautiousness = 75;
        P.EconomicRisk = 60;
        P.ScoutPriority = 90;
        P.AcceptableLossesPercent = 20;
        P.ReserveDepthPercent = 35;
        P.FlankingTendency = 80;
        P.RatioInfantry = 20;
        P.RatioAntiArmor = 40;
        P.RatioAntiAir = 20;
        P.RatioArtillery = 20;
        break;
    }

    // Adjust for AI profile modifiers
    if (Profile == AIProfile::Aggressive)
    {
        P.Aggressiveness = std::min(100, P.Aggressiveness + 20);
        P.Cautiousness = std::max(10, P.Cautiousness - 20);
        P.AcceptableLossesPercent += 15;
    }
    else if (Profile == AIProfile::Defensive)
    {
        P.Aggressiveness = std::max(10, P.Aggressiveness - 20);
        P.Cautiousness = std::min(100, P.Cautiousness + 20);
        P.ReserveDepthPercent += 15;
    }
    else if (Profile == AIProfile::Economic)
    {
        P.EconomicRisk += 25;
        P.ReserveDepthPercent += 10;
    }

    return P;
}

} // namespace AI
} // namespace RA4
