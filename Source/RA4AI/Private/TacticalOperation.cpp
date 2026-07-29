// Copyright (c) Red Alert 4 project.
#include "RA4AI/TacticalOperation.h"

namespace RA4
{
namespace AI
{

const char* ToString(OperationState State)
{
    switch (State)
    {
    case OperationState::Proposed:
        return "Proposed";
    case OperationState::Gathering:
        return "Gathering";
    case OperationState::Staging:
        return "Staging";
    case OperationState::Advancing:
        return "Advancing";
    case OperationState::Engaging:
        return "Engaging";
    case OperationState::Retreating:
        return "Retreating";
    case OperationState::Completed:
        return "Completed";
    case OperationState::Aborted:
        return "Aborted";
    default:
        return "Unknown";
    }
}

} // namespace AI
} // namespace RA4
