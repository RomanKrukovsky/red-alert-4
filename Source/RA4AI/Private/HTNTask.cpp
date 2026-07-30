// Copyright (c) Red Alert 4 project.
#include "RA4AI/HTNTask.h"

namespace RA4
{
namespace AI
{

bool HTNMethod::PrecondsHold(const HTNWorldState& WS) const
{
    for (const HTNPrecondition& P : Preconditions)
    {
        if (!WS.EvaluateCondition(P.Key, P.Op, P.Operand))
        {
            return false;
        }
    }
    return true;
}

bool HTNTask::PrecondsHold(const HTNWorldState& WS) const
{
    for (const HTNPrecondition& P : Preconditions)
    {
        if (!WS.EvaluateCondition(P.Key, P.Op, P.Operand))
        {
            return false;
        }
    }
    return true;
}

HTNTaskId HTNDomain::AddCompound(const char* Name)
{
    HTNTaskId Id = static_cast<HTNTaskId>(Tasks.size());
    HTNTask Task;
    Task.Id = Id;
    Task.Kind = HTNTaskKind::Compound;
    Task.Name = Name;
    Tasks.push_back(std::move(Task));
    return Id;
}

HTNTaskId HTNDomain::AddPrimitive(const char* Name, HTNAction Action, int32_t Cost)
{
    HTNTaskId Id = static_cast<HTNTaskId>(Tasks.size());
    HTNTask Task;
    Task.Id = Id;
    Task.Kind = HTNTaskKind::Primitive;
    Task.Name = Name;
    Task.Action = Action;
    Task.Cost = Cost;
    Tasks.push_back(std::move(Task));
    return Id;
}

HTNMethodId HTNDomain::AddMethod(HTNTaskId CompoundId,
                                 std::vector<HTNPrecondition> Preconditions,
                                 std::vector<HTNTaskId> Subtasks,
                                 int32_t Priority)
{
    HTNMethod Method;
    Method.Id = static_cast<HTNMethodId>(Tasks[CompoundId].Methods.size());
    Method.DecomposingTask = CompoundId;
    Method.Preconditions = std::move(Preconditions);
    Method.Subtasks = std::move(Subtasks);
    Method.Priority = Priority;
    Tasks[CompoundId].Methods.push_back(std::move(Method));
    return Method.Id;
}

void HTNDomain::AddPrecondition(HTNTaskId TaskId, WSKey Key, WSCompare Op, int32_t Operand)
{
    Tasks[TaskId].Preconditions.push_back({Key, Op, Operand});
}

void HTNDomain::AddEffect(HTNTaskId TaskId, WSKey Key, int32_t Value, bool bIsDelta)
{
    Tasks[TaskId].Effects.push_back({Key, Value, bIsDelta});
}

} // namespace AI
} // namespace RA4