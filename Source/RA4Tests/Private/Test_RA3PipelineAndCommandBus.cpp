// Copyright (c) Red Alert 4 project. Unit tests for ArmorMatrix and CommandBus.
#include "TestFramework.h"

#include "RA4Combat/ArmorMatrix.h"
#include "RA4Simulation/CommandBus.h"
#include "RA4Simulation/SimWorld.h"

RA4_TEST(ArmorMatrix, DamageCalculation)
{
    const RA4::ArmorMatrix& Matrix = RA4::GetDefaultArmorMatrix();

    // Ballistic vs LightInfantry (100% -> 100 dmg)
    int32_t Dmg1 = Matrix.CalculateDamage(100, RA4::WarheadClass::Ballistic, RA4::ArmorClass::LightInfantry);
    RA4_EXPECT_EQ(Dmg1, 100);

    // Ballistic vs HeavyVehicle (25% -> 25 dmg)
    int32_t Dmg2 = Matrix.CalculateDamage(100, RA4::WarheadClass::Ballistic, RA4::ArmorClass::HeavyVehicle);
    RA4_EXPECT_EQ(Dmg2, 25);

    // ArmorPiercing vs HeavyVehicle (100% -> 100 dmg)
    int32_t Dmg3 = Matrix.CalculateDamage(100, RA4::WarheadClass::ArmorPiercing, RA4::ArmorClass::HeavyVehicle);
    RA4_EXPECT_EQ(Dmg3, 100);

    // Siege vs LightInfantry (200% -> 200 dmg)
    int32_t Dmg4 = Matrix.CalculateDamage(100, RA4::WarheadClass::Siege, RA4::ArmorClass::LightInfantry);
    RA4_EXPECT_EQ(Dmg4, 200);
}

RA4_TEST(CommandBus, QueueAndDispatch)
{
    RA4::CommandBus Bus;

    RA4::Command Cmd1;
    Cmd1.Type = RA4::CommandType::Move;
    Cmd1.Issuer = 0;
    Cmd1.Location = RA4::Vec2(RA4::Fixed::FromInt(500), RA4::Fixed::FromInt(500));

    Bus.EnqueueCommand(1, Cmd1);
    RA4_EXPECT_EQ(Bus.GetPendingCommandCount(), 1);

    RA4::CommandFrame Frame1 = Bus.FetchFrameForTick(1);
    RA4_EXPECT_EQ(Frame1.Commands.size(), 1);
    RA4_EXPECT_EQ(Frame1.Tick, 1);

    Bus.ClearUpToTick(1);
    RA4_EXPECT_EQ(Bus.GetPendingCommandCount(), 0);
}
