// Copyright (c) Red Alert 4 project.
#include "RA4AI/HTNPlan.h"

namespace RA4
{
namespace AI
{

const char* ToString(HTNPlanStatus Status)
{
    switch (Status)
    {
        case HTNPlanStatus::Success:                 return "Success";
        case HTNPlanStatus::FailedNoPlan:           return "FailedNoPlan";
        case HTNPlanStatus::FailedDepthExceeded:    return "FailedDepthExceeded";
        case HTNPlanStatus::FailedNodeBudgetExceeded:return "FailedNodeBudgetExceeded";
        case HTNPlanStatus::FailedEmptyDomain:      return "FailedEmptyDomain";
    }
    return "Invalid";
}

} // namespace AI
} // namespace RA4