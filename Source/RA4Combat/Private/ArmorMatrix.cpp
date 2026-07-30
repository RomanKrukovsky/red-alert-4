// Copyright (c) Red Alert 4 project. Deterministic Damage & Armor Matrix implementation.
#include "RA4Combat/ArmorMatrix.h"
#include <algorithm>

namespace RA4
{

ArmorMatrix::ArmorMatrix()
{
    ResetToDefaults();
}

void ArmorMatrix::ResetToDefaults()
{
    // Default 100% for everything
    for (size_t W = 0; W < kWarheadCount; ++W)
    {
        for (size_t A = 0; A < kArmorCount; ++A)
        {
            Matrix[W][A] = 100;
        }
    }

    auto Set = [this](WarheadClass W, ArmorClass A, int32_t P) {
        SetMultiplierPercent(W, A, P);
    };

    // --- Ballistic (Small Arms, Guns) ---
    Set(WarheadClass::Ballistic, ArmorClass::LightInfantry, 100);
    Set(WarheadClass::Ballistic, ArmorClass::HeavyInfantry, 75);
    Set(WarheadClass::Ballistic, ArmorClass::LightVehicle, 50);
    Set(WarheadClass::Ballistic, ArmorClass::HeavyVehicle, 25);
    Set(WarheadClass::Ballistic, ArmorClass::SiegeVehicle, 25);
    Set(WarheadClass::Ballistic, ArmorClass::Air, 60);
    Set(WarheadClass::Ballistic, ArmorClass::Building, 10);

    // --- Fragmentation (HE, Explosive, Grenades) ---
    Set(WarheadClass::Fragmentation, ArmorClass::LightInfantry, 150);
    Set(WarheadClass::Fragmentation, ArmorClass::HeavyInfantry, 125);
    Set(WarheadClass::Fragmentation, ArmorClass::LightVehicle, 75);
    Set(WarheadClass::Fragmentation, ArmorClass::HeavyVehicle, 50);
    Set(WarheadClass::Fragmentation, ArmorClass::SiegeVehicle, 60);
    Set(WarheadClass::Fragmentation, ArmorClass::Air, 50);
    Set(WarheadClass::Fragmentation, ArmorClass::Building, 40);

    // --- ArmorPiercing (Anti-Tank Rockets, Heavy Cannons) ---
    Set(WarheadClass::ArmorPiercing, ArmorClass::LightInfantry, 25);
    Set(WarheadClass::ArmorPiercing, ArmorClass::HeavyInfantry, 40);
    Set(WarheadClass::ArmorPiercing, ArmorClass::LightVehicle, 100);
    Set(WarheadClass::ArmorPiercing, ArmorClass::HeavyVehicle, 100);
    Set(WarheadClass::ArmorPiercing, ArmorClass::SiegeVehicle, 100);
    Set(WarheadClass::ArmorPiercing, ArmorClass::Air, 80);
    Set(WarheadClass::ArmorPiercing, ArmorClass::Building, 60);

    // --- Siege (Artillery, Demolition) ---
    Set(WarheadClass::Siege, ArmorClass::LightInfantry, 200);
    Set(WarheadClass::Siege, ArmorClass::HeavyInfantry, 150);
    Set(WarheadClass::Siege, ArmorClass::LightVehicle, 125);
    Set(WarheadClass::Siege, ArmorClass::HeavyVehicle, 125);
    Set(WarheadClass::Siege, ArmorClass::SiegeVehicle, 100);
    Set(WarheadClass::Siege, ArmorClass::Air, 10);
    Set(WarheadClass::Siege, ArmorClass::Building, 200);

    // --- Electric (Tesla, EMP) ---
    Set(WarheadClass::Electric, ArmorClass::LightInfantry, 120);
    Set(WarheadClass::Electric, ArmorClass::HeavyInfantry, 100);
    Set(WarheadClass::Electric, ArmorClass::LightVehicle, 100);
    Set(WarheadClass::Electric, ArmorClass::HeavyVehicle, 100);
    Set(WarheadClass::Electric, ArmorClass::SiegeVehicle, 100);
    Set(WarheadClass::Electric, ArmorClass::Shielded, 150);

    // --- AntiAir ---
    Set(WarheadClass::AntiAir, ArmorClass::LightInfantry, 10);
    Set(WarheadClass::AntiAir, ArmorClass::HeavyVehicle, 10);
    Set(WarheadClass::AntiAir, ArmorClass::Air, 150);
}

int32_t ArmorMatrix::GetMultiplierPercent(WarheadClass Warhead, ArmorClass Armor) const
{
    size_t W = static_cast<size_t>(Warhead);
    size_t A = static_cast<size_t>(Armor);
    if (W >= kWarheadCount || A >= kArmorCount)
    {
        return 100;
    }
    return Matrix[W][A];
}

int32_t ArmorMatrix::CalculateDamage(int32_t BaseDamage, WarheadClass Warhead, ArmorClass Armor) const
{
    int32_t Multiplier = GetMultiplierPercent(Warhead, Armor);
    int64_t Scaled = (static_cast<int64_t>(BaseDamage) * Multiplier) / 100;
    return std::max<int32_t>(0, static_cast<int32_t>(Scaled));
}

void ArmorMatrix::SetMultiplierPercent(WarheadClass Warhead, ArmorClass Armor, int32_t Percent)
{
    size_t W = static_cast<size_t>(Warhead);
    size_t A = static_cast<size_t>(Armor);
    if (W < kWarheadCount && A < kArmorCount)
    {
        Matrix[W][A] = Percent;
    }
}

const ArmorMatrix& GetDefaultArmorMatrix()
{
    static ArmorMatrix DefaultMatrix;
    return DefaultMatrix;
}

} // namespace RA4
