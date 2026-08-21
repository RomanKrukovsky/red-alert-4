// Copyright (c) Red Alert 4 project. Match Telemetry and APM Tracker implementation.
#include "RA4Replay/MatchTelemetryTracker.h"
#include "RA4Content/ContentDatabase.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace RA4
{

void MatchTelemetryTracker::Initialize(uint8_t NumPlayers, uint32_t SampleIntervalTicks)
{
    NumActivePlayers = NumPlayers > 0 ? NumPlayers : 2;
    SampleInterval = SampleIntervalTicks > 0 ? SampleIntervalTicks : 20;

    Reset();
}

void MatchTelemetryTracker::Reset()
{
    CommandWindows.clear();
    Stats.clear();

    for (PlayerId P = 0; P < NumActivePlayers; ++P)
    {
        PlayerMatchStats S;
        S.Player = P;
        Stats[P] = S;
        CommandWindows[P] = std::deque<TickIndex>{};
    }
}

void MatchTelemetryTracker::IngestTickCommands(TickIndex Tick, const CommandFrame* Frame)
{
    if (Frame != nullptr)
    {
        for (const auto& Cmd : Frame->Commands)
        {
            if (Cmd.Issuer < NumActivePlayers)
            {
                Stats[Cmd.Issuer].TotalCommandsIssued++;
                CommandWindows[Cmd.Issuer].push_back(Tick);
            }
        }
    }

    constexpr TickIndex kWindowTicks = 1200; // 60 seconds at 20 Hz

    for (PlayerId P = 0; P < NumActivePlayers; ++P)
    {
        auto& Window = CommandWindows[P];
        while (!Window.empty() && Window.front() + kWindowTicks < Tick)
        {
            Window.pop_front();
        }

        const float ElapsedTicks = std::max(static_cast<float>(std::min(Tick, kWindowTicks)), 20.0f);
        const float CurrentAPM = (static_cast<float>(Window.size()) * 1200.0f) / ElapsedTicks;

        Stats[P].PeakAPM = std::max(Stats[P].PeakAPM, CurrentAPM);
    }
}

void MatchTelemetryTracker::IngestSimEvents(const std::vector<SimEvent>& Events, const SimWorld& World)
{
    for (const auto& Ev : Events)
    {
        if (Ev.Type == SimEventType::EntityDestroyed)
        {
            if (Ev.Player < NumActivePlayers)
            {
                Stats[Ev.Player].TotalUnitsLost++;
            }
            if (World.IsAlive(Ev.Other))
            {
                const auto* Core = World.GetCore(Ev.Other);
                if (Core != nullptr && Core->Owner < NumActivePlayers && Core->Owner != Ev.Player)
                {
                    Stats[Core->Owner].TotalUnitsKilled++;
                }
            }
        }
    }
}


void MatchTelemetryTracker::SampleWorldState(const SimWorld& World)
{
    const TickIndex CurrentTick = World.GetTick();
    if (CurrentTick % SampleInterval != 0)
    {
        return;
    }

    const ContentDatabase* Content = World.GetContent();
    const std::vector<EntityCore>& Cores = World.GetAllCores();

    for (PlayerId P = 0; P < NumActivePlayers; ++P)
    {
        int32_t ArmyValue = 0;
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (Cores[I].bAlive && Cores[I].Owner == P && Cores[I].Kind == EntityKind::Unit)
            {
                if (Content != nullptr)
                {
                    const EntityDef* Def = Content->FindEntity(Cores[I].Def);
                    if (Def != nullptr)
                    {
                        ArmyValue += Def->Production.Cost;
                    }
                }
            }
        }

        Stats[P].PeakArmyValue = std::max(Stats[P].PeakArmyValue, ArmyValue);
        Stats[P].TotalHarvested = World.GetPlayer(P).TotalHarvested;

        const auto& Window = CommandWindows[P];
        constexpr TickIndex kWindowTicks = 1200;
        const float ElapsedTicks = std::max(static_cast<float>(std::min(CurrentTick, kWindowTicks)), 20.0f);
        const float CurrentAPM = (static_cast<float>(Window.size()) * 1200.0f) / ElapsedTicks;

        PlayerTimelineSample Sample;
        Sample.Tick = CurrentTick;
        Sample.ActionsPerMinute = CurrentAPM;
        Sample.TotalCreditsHarvested = World.GetPlayer(P).TotalHarvested;
        Sample.CurrentCredits = World.GetPlayer(P).Credits;
        Sample.ActiveArmyValue = ArmyValue;
        Sample.UnitsLostCount = Stats[P].TotalUnitsLost;
        Sample.UnitsKilledCount = Stats[P].TotalUnitsKilled;

        Stats[P].Timeline.push_back(Sample);

        // Update running average APM
        if (CurrentTick > 0)
        {
            const float TotalMinutes = static_cast<float>(CurrentTick) / 1200.0f;
            Stats[P].AverageAPM = static_cast<float>(Stats[P].TotalCommandsIssued) / std::max(TotalMinutes, 0.1f);
        }
    }
}

const PlayerMatchStats* MatchTelemetryTracker::GetPlayerStats(PlayerId Player) const
{
    auto It = Stats.find(Player);
    if (It != Stats.end())
    {
        return &It->second;
    }
    return nullptr;
}

std::string MatchTelemetryTracker::ExportToJson(const SimWorld& World) const
{
    std::stringstream SS;
    SS << "{\n";
    SS << "  \"totalTicks\": " << World.GetTick() << ",\n";
    SS << "  \"winner\": " << static_cast<int32_t>(World.GetWinner()) << ",\n";
    SS << "  \"players\": [\n";

    for (PlayerId P = 0; P < NumActivePlayers; ++P)
    {
        const auto* S = GetPlayerStats(P);
        SS << "    {\n";
        SS << "      \"player\": " << static_cast<int32_t>(P) << ",\n";
        if (S != nullptr)
        {
            SS << "      \"peakAPM\": " << S->PeakAPM << ",\n";
            SS << "      \"avgAPM\": " << S->AverageAPM << ",\n";
            SS << "      \"totalCommands\": " << S->TotalCommandsIssued << ",\n";
            SS << "      \"totalHarvested\": " << S->TotalHarvested << ",\n";
            SS << "      \"peakArmyValue\": " << S->PeakArmyValue << ",\n";
            SS << "      \"unitsLost\": " << S->TotalUnitsLost << ",\n";
            SS << "      \"unitsKilled\": " << S->TotalUnitsKilled << ",\n";
            SS << "      \"timeline\": [\n";
            for (size_t I = 0; I < S->Timeline.size(); ++I)
            {
                const auto& T = S->Timeline[I];
                SS << "        {\"t\":" << T.Tick << ",\"apm\":" << T.ActionsPerMinute
                   << ",\"credits\":" << T.CurrentCredits << ",\"harvested\":" << T.TotalCreditsHarvested
                   << ",\"army\":" << T.ActiveArmyValue << ",\"lost\":" << T.UnitsLostCount
                   << ",\"kills\":" << T.UnitsKilledCount << "}" << (I + 1 < S->Timeline.size() ? "," : "") << "\n";
            }
            SS << "      ]\n";
        }
        SS << "    }" << (P + 1 < NumActivePlayers ? "," : "") << "\n";
    }

    SS << "  ]\n";
    SS << "}\n";
    return SS.str();
}

} // namespace RA4
