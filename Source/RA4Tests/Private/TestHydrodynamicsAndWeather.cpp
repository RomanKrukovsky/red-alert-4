// Copyright (c) Red Alert 4 project. Tests for Stage 19 (Hydrodynamic Ship Wakes & Dynamic Weather).
#include "TestFramework.h"

#include "RA4Presentation/HydrodynamicsAndWeather.h"

using namespace RA4;
using namespace RA4Test;

// --- 1. Dynamic Weather Modifiers ---

RA4_TEST(HydrodynamicsAndWeather, WeatherEffectsOnAircraftVisionAndRoughness)
{
    // Clear Weather
    const auto Clear = HydrodynamicsAndWeather::ComputeWeather(WeatherCondition::Clear, 0.0f);
    RA4_EXPECT_NEAR(Clear.AirVisionFactor, 1.0f, 0.01f);
    RA4_EXPECT_NEAR(Clear.SeaRoughness, 0.05f, 0.01f);

    // Full Thunderstorm
    const auto Storm = HydrodynamicsAndWeather::ComputeWeather(WeatherCondition::Thunderstorm, 1.0f);
    RA4_EXPECT_NEAR(Storm.AirVisionFactor, 0.55f, 0.02f);
    RA4_EXPECT_NEAR(Storm.SeaRoughness, 1.0f, 0.01f);

    // Heavy Fog
    const auto Fog = HydrodynamicsAndWeather::ComputeWeather(WeatherCondition::HeavyFog, 0.8f);
    RA4_EXPECT_NEAR(Fog.AirVisionFactor, 0.52f, 0.02f);
}

// --- 2. Hydrodynamic Ship Wakes & Bow Spray ---

RA4_TEST(HydrodynamicsAndWeather, ShipWakeKelvinAngleAndFoamGeneration)
{
    // High-speed naval battleship (150 units/s, 2000 tons displacement, calm seas)
    const auto FastBattleship = HydrodynamicsAndWeather::ComputeShipWake(150.0f, 2000.0f, 0.1f);

    RA4_EXPECT_NEAR(FastBattleship.KelvinCuspAngleDeg, 19.47f, 0.01f);
    RA4_EXPECT_NEAR(FastBattleship.FoamIntensity, 1.0f, 0.01f);
    RA4_EXPECT(FastBattleship.SprayParticleRate > 50.0f);
    RA4_EXPECT(FastBattleship.SternWaveHeight > 10.0f);

    // Stationary ship (0 units/s)
    const auto StationaryShip = HydrodynamicsAndWeather::ComputeShipWake(0.0f, 2000.0f, 0.0f);
    RA4_EXPECT_NEAR(StationaryShip.FoamIntensity, 0.0f, 0.001f);
    RA4_EXPECT_NEAR(StationaryShip.SprayParticleRate, 0.0f, 0.001f);
    RA4_EXPECT_NEAR(StationaryShip.SternWaveHeight, 0.0f, 0.001f);
}
