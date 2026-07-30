// Copyright (c) Red Alert 4 project. HTN tasks and methods.
//
// A domain is a flat array of tasks. Compound tasks decompose via methods that
// match the current worldstate and yield an ordered list of subtasks. Primitive
// tasks have preconditions and effects; executing one mutates the worldstate
// clone used during planning and maps to an HTNAction the commander buys.
//
// Effects are modeled as key/value pairs so the planner can apply them without
// branching on action type. Most actions only push one or two effects; the few
// that need richer state changes (e.g. "build power plant" raising power and
// deducting credits) get two effect slots. Two slots is enough for every Red
// Alert domain action we currently author.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4AI/HTNTypes.h"
#include "RA4AI/HTNWorldState.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

// A single precondition: "worldstate[Key] Op Operand". Grouped into small
// arrays on the methods and primitive tasks that use them.
struct HTNPrecondition
{
    WSKey Key = WSKey::Credits;
    WSCompare Op = WSCompare::GreaterEqual;
    int32_t Operand = 0;
};

// A single effect: either set absolute ("Set(Key, Value)") or adjust ("Add(Key,
// Value)"). The "IsDelta" flag is read by the planner when applying the effect.
struct HTNEffect
{
    WSKey Key = WSKey::Credits;
    int32_t Value = 0;
    bool bIsDelta = false;
};

enum class HTNTaskKind : uint8_t
{
    Primitive = 0,
    Compound = 1
};

struct HTNMethod
{
    HTNMethodId Id = kInvalidHTNMethod;
    HTNTaskId DecomposingTask = kInvalidHTNTask;     // parent compound, for debugging
    std::vector<HTNPrecondition> Preconditions;       // ALL must hold
    std::vector<HTNTaskId> Subtasks;                  // ordered decomposition
    int32_t Priority = 0;                             // higher wins ties at the planner level

    bool PrecondsHold(const HTNWorldState& WS) const;
};

struct HTNTask
{
    HTNTaskId Id = kInvalidHTNTask;
    HTNTaskKind Kind = HTNTaskKind::Primitive;
    const char* Name = "";                            // debugging + decision trace
    HTNAction Action = HTNAction::None;              // primitive only
    int32_t Cost = 1;                                  // primitive only; lower = preferred

    std::vector<HTNPrecondition> Preconditions;        // primitive only
    std::vector<HTNEffect> Effects;                    // primitive only
    std::vector<HTNMethod> Methods;                    // compound only; tried in authored order

    bool PrecondsHold(const HTNWorldState& WS) const;
};

// A flat container the planner traverses. Adding a task is append-only, which
// keeps existing task indices stable across edits (important because plans and
// cached method indices reference task ids).
struct RA4AI_API HTNDomain
{
    std::vector<HTNTask> Tasks;
    HTNTaskId RootTask = kInvalidHTNTask;

    HTNTaskId AddCompound(const char* Name);
    HTNTaskId AddPrimitive(const char* Name, HTNAction Action, int32_t Cost);

    HTNMethodId AddMethod(HTNTaskId CompoundId,
                          std::vector<HTNPrecondition> Preconditions,
                          std::vector<HTNTaskId> Subtasks,
                          int32_t Priority);

    void AddPrecondition(HTNTaskId TaskId, WSKey Key, WSCompare Op, int32_t Operand);
    void AddEffect(HTNTaskId TaskId, WSKey Key, int32_t Value, bool bIsDelta);

    const HTNTask& Get(HTNTaskId Id) const { return Tasks[Id]; }
    HTNTask& Get(HTNTaskId Id) { return Tasks[Id]; }
};

} // namespace AI
} // namespace RA4