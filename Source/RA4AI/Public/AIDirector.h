// Copyright (c) Red Alert 4 project.

#pragma once

#include <vector>
#include <cstdint>
#include "CoreMinimal.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Command.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4
{
namespace AI
{

enum class AIPersonality : uint8_t
{
    Standard = 0,
    Rusher,
    Turtler,
    BoomTech,
    Adaptive
};

class RA4AI_API FAIDirector
{
public:
    FAIDirector(PlayerId InPlayer, AIPersonality InPersonality);

    // Main AI decision tick. This should be called by an AI subsystem/controller
    // at a lower frequency than the simulation tick (e.g., 2-5 Hz) to generate Commands.
    void Tick(const SimWorld& World, std::vector<Command>& OutCommands);

private:
    void UpdateEconomyAndProduction(const SimWorld& World, std::vector<Command>& OutCommands);
    void UpdateTacticalGroups(const SimWorld& World, std::vector<Command>& OutCommands);
    void EvaluateThreats(const SimWorld& World);

    PlayerId ControlledPlayer;
    AIPersonality Personality;
    
    // Simple state tracking
    bool bHasBarracks = false;
    bool bHasWarFactory = false;
    bool bUnderAttack = false;

    // Tick throttling
    uint32_t TicksSinceLastDecision = 0;
    static constexpr uint32_t DecisionInterval = 10; // Evaluate every 10 sim ticks (0.5s at 20Hz)
};

} // namespace AI
} // namespace RA4
