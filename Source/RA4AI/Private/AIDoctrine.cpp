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
    else if (Profile == AIProfile::Rush)
    {
        // Maximum pressure, minimum patience: throws its force in and expects to
        // trade badly. Scouting matters less because the plan barely adapts.
        P.Aggressiveness = std::min(100, P.Aggressiveness + 35);
        P.Cautiousness = std::max(5, P.Cautiousness - 35);
        P.AcceptableLossesPercent = std::min(95, P.AcceptableLossesPercent + 35);
        P.ReserveDepthPercent = std::max(0, P.ReserveDepthPercent - 15);
        P.ScoutPriority = std::max(10, P.ScoutPriority - 20);
        P.ThreatSensitivity = std::max(10, P.ThreatSensitivity - 20);
        // Cheap bodies now beat a balanced composition later.
        P.RatioInfantry = std::min(100, P.RatioInfantry + 25);
        P.RatioArtillery = std::max(0, P.RatioArtillery - 10);
        P.RatioAntiAir = std::max(0, P.RatioAntiAir - 10);
    }
    else if (Profile == AIProfile::Turtle)
    {
        // Values its own units highly and reacts strongly to threats, at the cost of
        // almost never seizing the initiative.
        P.Aggressiveness = std::max(5, P.Aggressiveness - 30);
        P.Cautiousness = std::min(100, P.Cautiousness + 30);
        P.AcceptableLossesPercent = std::max(5, P.AcceptableLossesPercent - 20);
        P.ReserveDepthPercent = std::min(90, P.ReserveDepthPercent + 30);
        P.ThreatSensitivity = std::min(100, P.ThreatSensitivity + 25);
        // Static firepower and artillery over mobile pushes.
        P.RatioArtillery = std::min(100, P.RatioArtillery + 15);
        P.RatioAntiArmor = std::min(100, P.RatioAntiArmor + 10);
    }
    else if (Profile == AIProfile::AirSuperiority)
    {
        // Invests heavily and needs intelligence to time the air transition, so it
        // scouts hard and keeps a deep reserve while teching.
        P.EconomicRisk = std::min(100, P.EconomicRisk + 15);
        P.ScoutPriority = std::min(100, P.ScoutPriority + 25);
        P.Cautiousness = std::min(100, P.Cautiousness + 15);
        P.ReserveDepthPercent = std::min(90, P.ReserveDepthPercent + 20);
        // Skews toward anti-air and support so its own air arm survives contact.
        P.RatioAntiAir = std::min(100, P.RatioAntiAir + 30);
        P.RatioSupport = std::min(100, P.RatioSupport + 10);
        P.RatioInfantry = std::max(0, P.RatioInfantry - 20);
    }
    else if (Profile == AIProfile::Guerrilla)
    {
        // Avoids pitched battles: highly mobile, flanks constantly, regroups often
        // and withdraws early rather than trading evenly.
        P.Aggressiveness = std::min(100, P.Aggressiveness + 15);
        P.FlankingTendency = std::min(100, P.FlankingTendency + 40);
        P.ScoutPriority = std::min(100, P.ScoutPriority + 20);
        P.AcceptableLossesPercent = std::max(5, P.AcceptableLossesPercent - 15);
        P.RegroupFrequencyTicks = std::max(5, P.RegroupFrequencyTicks / 2);
        P.ThreatSensitivity = std::min(100, P.ThreatSensitivity + 15);
        // Fast, cheap harassers rather than a siege line.
        P.RatioInfantry = std::min(100, P.RatioInfantry + 15);
        P.RatioArtillery = std::max(0, P.RatioArtillery - 10);
    }

    return P;
}

} // namespace AI
} // namespace RA4
