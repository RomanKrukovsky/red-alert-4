// Copyright (c) Red Alert 4 project. Deterministic Damage & Armor Matrix calculation.
#pragma once

#include <array>
#include <cstdint>
#include "RA4Content/ContentTypes.h"
#include "RA4Core/Fixed.h"

namespace RA4
{

/**
 * Deterministic matrix specifying damage multipliers for each (WarheadClass, ArmorClass) pair.
 * Values are stored as integer percentage points (e.g. 100 = 1.0x, 25 = 0.25x, 150 = 1.5x).
 */
class ArmorMatrix
{
public:
    ArmorMatrix();

    /** Returns the damage multiplier percentage (100 = 100%). */
    int32_t GetMultiplierPercent(WarheadClass Warhead, ArmorClass Armor) const;

    /** Calculates final damage after applying warhead vs armor multiplier. */
    int32_t CalculateDamage(int32_t BaseDamage, WarheadClass Warhead, ArmorClass Armor) const;

    /** Overrides a multiplier entry in the matrix. */
    void SetMultiplierPercent(WarheadClass Warhead, ArmorClass Armor, int32_t Percent);

    /** Resets the matrix to canonical C&C / RA balance defaults. */
    void ResetToDefaults();

private:
    static constexpr size_t kWarheadCount = static_cast<size_t>(WarheadClass::Count);
    static constexpr size_t kArmorCount = static_cast<size_t>(ArmorClass::Count);

    // Matrix storage [Warhead][Armor]
    std::array<std::array<int32_t, kArmorCount>, kWarheadCount> Matrix{};
};

/** Global access to default armor matrix instance. */
const ArmorMatrix& GetDefaultArmorMatrix();

} // namespace RA4
