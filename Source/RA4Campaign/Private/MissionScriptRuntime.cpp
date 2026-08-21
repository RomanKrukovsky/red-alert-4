#include "MissionScriptRuntime.h"
#include "RA4Core/SimConfig.h"

namespace RA4
{

void MissionScriptRuntime::Initialize(MissionRuntime* InObjectiveRuntime)
{
    ObjectiveRuntime = InObjectiveRuntime;
    Reset();
}

void MissionScriptRuntime::Reset()
{
    Triggers.clear();
    TransmissionQueue.clear();
    CurrentTransmission = CinematicTransmission{};
    TransmissionHistory.clear();
}

void MissionScriptRuntime::AddTrigger(const MissionTrigger& Trigger)
{
    Triggers.push_back(Trigger);
}

bool MissionScriptRuntime::EvaluateCondition(const MissionTrigger& Trigger, const SimWorld& World) const
{
    switch (Trigger.Condition)
    {
    case TriggerConditionType::None:
        return false;

    case TriggerConditionType::TickReached:
        return World.GetTick() >= Trigger.TriggerTick;

    case TriggerConditionType::AreaEntered:
    {
        const int32_t RadiusUnits = Trigger.AreaRadiusTiles * static_cast<int32_t>(kTileSizeUnits);
        const int64_t RadiusSq = static_cast<int64_t>(RadiusUnits) * static_cast<int64_t>(RadiusUnits);
        const Vec2 CenterPos = World.GetMap().TileCenterToWorld(Trigger.AreaCenter);


        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (Cores[I].bAlive && Cores[I].Owner == Trigger.ConditionPlayer && Cores[I].Kind == EntityKind::Unit)
            {
                const TransformComp* T = World.GetTransform(World.MakeId(I));
                if (T != nullptr)
                {
                    if (DistanceSquared(T->Position, CenterPos) <= Fixed::FromInt(RadiusSq))
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }


    case TriggerConditionType::EntityDestroyed:
        return Trigger.TargetEntity.IsValid() && !World.IsAlive(Trigger.TargetEntity);

    case TriggerConditionType::EntityHealthBelowPercent:
    {
        if (!Trigger.TargetEntity.IsValid() || !World.IsAlive(Trigger.TargetEntity))
        {
            return false;
        }
        const HealthComp* H = World.GetHealth(Trigger.TargetEntity);
        if (H == nullptr || H->Max <= 0)
        {
            return false;
        }
        return (H->Current * 100 / H->Max) <= Trigger.HealthPercent;
    }

    case TriggerConditionType::ObjectiveCompleted:
        if (ObjectiveRuntime != nullptr)
        {
            const MissionObjective* Obj = ObjectiveRuntime->FindObjective(Trigger.RequiredObjectiveId);
            return Obj != nullptr && Obj->State == ObjectiveState::Completed;
        }
        return false;

    case TriggerConditionType::CreditsThreshold:
        if (Trigger.ConditionPlayer < kMaxPlayers)
        {
            return World.GetPlayer(Trigger.ConditionPlayer).Credits >= Trigger.HealthPercent;
        }
        return false;
    }
    return false;
}

void MissionScriptRuntime::ExecuteAction(const ScriptTriggerAction& Action, SimWorld& World)
{
    switch (Action.Type)
    {
    case TriggerActionType::None:
        break;

    case TriggerActionType::SpawnReinforcements:
        for (const auto& R : Action.Reinforcements)
        {
            World.SpawnUnit(R.Def, R.Owner, R.Location);
        }
        break;

    case TriggerActionType::RevealObjective:
        if (ObjectiveRuntime != nullptr)
        {
            ObjectiveRuntime->RevealObjective(Action.TargetObjectiveId);
        }
        break;

    case TriggerActionType::PlayCinematicTransmission:
    {
        CinematicTransmission Trans;
        Trans.Id = Action.PortraitVideoId;
        Trans.Speaker = Action.SpeakerName;
        Trans.Text = Action.DialogueTextKey;
        Trans.PortraitVideo = Action.PortraitVideoId;
        Trans.StartTick = World.GetTick();
        Trans.EndTick = World.GetTick() + Action.DurationTicks;
        Trans.bActive = false;
        TransmissionQueue.push_back(Trans);
        break;
    }

    case TriggerActionType::ShiftMapBounds:
    case TriggerActionType::ChangePlayerDiplomacy:
    case TriggerActionType::SetAIProfile:
    case TriggerActionType::RevealFogArea:
        break;
    }
}

void MissionScriptRuntime::Tick(SimWorld& World)
{
    for (auto& Trig : Triggers)
    {
        if (Trig.bFired && Trig.bOneShot)
        {
            continue;
        }

        if (EvaluateCondition(Trig, World))
        {
            Trig.bFired = true;
            for (const auto& Act : Trig.Actions)
            {
                ExecuteAction(Act, World);
            }
        }
    }

    // Process transmission queue
    if (!CurrentTransmission.bActive && !TransmissionQueue.empty())
    {
        CurrentTransmission = TransmissionQueue.front();
        TransmissionQueue.pop_front();
        const uint32_t Duration = CurrentTransmission.EndTick > CurrentTransmission.StartTick
                                      ? (CurrentTransmission.EndTick - CurrentTransmission.StartTick)
                                      : 100;
        CurrentTransmission.StartTick = World.GetTick();
        CurrentTransmission.EndTick = World.GetTick() + Duration;
        CurrentTransmission.bActive = true;
    }
    else if (CurrentTransmission.bActive)
    {
        if (World.GetTick() >= CurrentTransmission.EndTick)
        {
            CurrentTransmission.bActive = false;
            TransmissionHistory.push_back(CurrentTransmission);
        }
    }
}

const CinematicTransmission* MissionScriptRuntime::GetActiveTransmission() const
{
    if (CurrentTransmission.bActive)
    {
        return &CurrentTransmission;
    }
    return nullptr;
}

} // namespace RA4
