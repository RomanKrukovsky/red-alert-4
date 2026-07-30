// Copyright (c) Red Alert 4 project. Operational Army Group implementation.
#include "RA4AI/ArmyGroup.h"

#include <algorithm>

namespace RA4
{
namespace AI
{

ArmyGroup* ArmyGroupManager::CreateGroup(uint32_t GroupId, GroupRole Role, const std::string& Name)
{
    for (auto& G : Groups)
    {
        if (G.GroupId == GroupId)
        {
            G.Role = Role;
            G.Name = Name;
            return &G;
        }
    }

    ArmyGroup NewGroup;
    NewGroup.GroupId = GroupId;
    NewGroup.Role = Role;
    NewGroup.Name = Name;
    Groups.push_back(NewGroup);
    return &Groups.back();
}

ArmyGroup* ArmyGroupManager::FindGroup(uint32_t GroupId)
{
    for (auto& G : Groups)
    {
        if (G.GroupId == GroupId) return &G;
    }
    return nullptr;
}

const ArmyGroup* ArmyGroupManager::FindGroup(uint32_t GroupId) const
{
    for (const auto& G : Groups)
    {
        if (G.GroupId == GroupId) return &G;
    }
    return nullptr;
}

void ArmyGroupManager::RemoveGroup(uint32_t GroupId)
{
    Groups.erase(
        std::remove_if(Groups.begin(), Groups.end(), [GroupId](const ArmyGroup& G) {
            return G.GroupId == GroupId;
        }),
        Groups.end()
    );
}

void ArmyGroupManager::Clear()
{
    Groups.clear();
    NextGroupId = 1;
}

} // namespace AI
} // namespace RA4
