// Copyright (c) Red Alert 4 project. Data-driven 9x9 Damage & Armor Matrix.
#pragma once

#include <cstdint>
#include "RA4Content/ContentTypes.h"
#include "RA4Core/Fixed.h"

namespace RA4
{

class DamageMatrix
{
public:
    // Returns damage multiplier for given Warhead (damage type) against target Armor class.
    // Base table from Section 2 of RA4_Factions_Units_Economy_Voice_Bible.md.
    static Fixed GetMultiplier(WarheadClass Warhead, ArmorClass Armor)
    {
        static const int32_t Matrix[9][9] = {
            // Armor: 0:LightInfantry, 1:HeavyInfantry, 2:LightVehicle, 3:HeavyVehicle, 4:SiegeVehicle, 5:Air, 6:Naval, 7:Building, 8:Shielded
            // Warhead 0: Ballistic
            { 100,  75,  75,  50,  50,  75,  50,  30,  50 },
            // Warhead 1: Fragmentation
            { 150, 100,  75,  50,  50,  50,  50,  50,  50 },
            // Warhead 2: ArmorPiercing
            {  50,  75, 125, 150, 150,  75, 125,  75,  50 },
            // Warhead 3: Siege
            {  75,  75,  75, 100, 100,  10, 125, 200,  50 },
            // Warhead 4: Electric
            { 125, 125, 100,  85,  85, 100, 100,  50, 200 },
            // Warhead 5: Plasma
            { 110, 110, 115, 125, 125, 110, 120, 130, 150 },
            // Warhead 6: Cryogenic
            { 120, 100, 100,  90,  90, 100, 100,  50,  75 },
            // Warhead 7: Temporal
            { 100, 100, 100, 100, 100, 100, 100, 100, 150 },
            // Warhead 8: AntiAir
            {  10,  10,  10,  10,  10, 200,  10,   0,  50 }
        };

        uint8_t WIdx = static_cast<uint8_t>(Warhead);
        uint8_t AIdx = static_cast<uint8_t>(Armor);

        if (WIdx >= 9 || AIdx >= 9)
        {
            return Fixed::FromInt(1);
        }

        return Fixed::FromRatio(Matrix[WIdx][AIdx], 100);
    }
};

} // namespace RA4
