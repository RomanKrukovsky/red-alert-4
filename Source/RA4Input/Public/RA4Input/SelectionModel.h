// Copyright (c) Red Alert 4 project. Unit selection and control groups, engine-free.
//
// Selection is a client-side concept -- the server never sees it, only the commands
// it produces. It lives here rather than in a PlayerController so the priority
// rules ("a marquee over my tanks and an enemy scout selects my tanks") are covered
// by tests instead of by clicking around in PIE.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Ids.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4INPUT_API
#define RA4INPUT_API
#endif

namespace RA4
{
namespace Input
{

enum class SelectionMode : uint8_t
{
    Replace = 0,   // plain click
    Add,           // shift-click
    Toggle,        // ctrl-click, or shift-click on an already selected unit
};

constexpr int32_t kControlGroupCount = 10;
// Beyond this a single order frame would exceed the server's per-tick command
// budget, so the selection itself is capped rather than silently truncated later.
constexpr int32_t kMaxSelectedEntities = 200;

class RA4INPUT_API SelectionModel
{
public:
    void SetLocalPlayer(PlayerId InPlayer) { LocalPlayer = InPlayer; }
    PlayerId GetLocalPlayer() const { return LocalPlayer; }

    void Clear();

    // --- clicking ------------------------------------------------------------
    // Single click. Candidates are everything the cursor ray hit, in any order;
    // the most relevant one is chosen by ownership and kind.
    void SelectAtCursor(const SimWorld& World, const std::vector<EntityId>& Candidates, SelectionMode Mode);

    // Marquee. Candidates are the entities inside the rectangle, hit-tested by the
    // caller (which owns the projection).
    void SelectInMarquee(const SimWorld& World, const std::vector<EntityId>& Candidates, SelectionMode Mode);

    // Double-click: every visible entity of the same type as the clicked one.
    void SelectSameType(const SimWorld& World, EntityId Prototype, const std::vector<EntityId>& Visible,
                        SelectionMode Mode);

    // Modern selection filters
    void SelectIdleUnits(const SimWorld& World, SelectionMode Mode);
    void SelectWoundedUnits(const SimWorld& World, int32_t HealthPercentThreshold, SelectionMode Mode);

    // --- control groups ------------------------------------------------------

    bool AssignControlGroup(int32_t GroupIndex);
    bool AddToControlGroup(int32_t GroupIndex);
    // Returns false when the group is empty or everything in it has died.
    bool RecallControlGroup(int32_t GroupIndex, const SimWorld& World);
    int32_t GetControlGroupSize(int32_t GroupIndex, const SimWorld& World) const;

    // --- maintenance ---------------------------------------------------------
    // Must run every tick: a selected unit that dies has to leave the selection, or
    // the next order references a stale handle and is rejected by the server.
    void PruneDead(const SimWorld& World);

    // --- queries -------------------------------------------------------------
    const std::vector<EntityId>& Get() const { return Selected; }
    bool IsSelected(EntityId Id) const;
    int32_t Num() const { return int32_t(Selected.size()); }
    bool IsEmpty() const { return Selected.empty(); }
    // The entity whose portrait and command card the HUD shows.
    EntityId GetPrimary() const { return Selected.empty() ? EntityId::Invalid() : Selected.front(); }
    // True when the selection contains at least one entity the local player owns.
    bool HasOwnedEntities(const SimWorld& World) const;
    bool ContainsAnyUnit(const SimWorld& World) const;

private:
    void ApplyMode(const std::vector<EntityId>& Incoming, SelectionMode Mode);
    void SortAndCap(const SimWorld& World);

    PlayerId LocalPlayer = 0;
    std::vector<EntityId> Selected;
    std::vector<EntityId> ControlGroups[kControlGroupCount];
};

// Ranking used for single-click disambiguation and for sorting the selection so the
// primary entity is stable. Lower sorts first.
RA4INPUT_API int32_t GetSelectionPriority(const SimWorld& World, EntityId Id, PlayerId LocalPlayer);

} // namespace Input
} // namespace RA4
