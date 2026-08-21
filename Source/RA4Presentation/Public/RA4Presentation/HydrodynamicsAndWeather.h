// Copyright (c) Red Alert 4 project. Hydrodynamic Ship Wakes & Dynamic Weather.
#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{

enum class WeatherCondition : uint8_t
{
    Clear,
    Rain,
    Thunderstorm,
    HeavyFog
};

struct WeatherState
{
    WeatherCondition Condition = WeatherCondition::Clear;
    float Intensity = 0.0f;     // 0.0 .. 1.0
    float SeaRoughness = 0.1f;  // 0.0 (calm) .. 1.0 (stormy seas)
    float AirVisionFactor = 1.0f; // Vision range multiplier for aircraft
};

struct ShipWakeOutput
{
    float KelvinCuspAngleDeg = 19.47f; // Universal Kelvin wake angle in deep water
    float FoamIntensity = 0.0f;        // 0.0 .. 1.0
    float SprayParticleRate = 0.0f;    // Particles / sec
    float SternWaveHeight = 0.0f;      // Peak wave crest at stern
};

class RA4PRESENTATION_API HydrodynamicsAndWeather
{
public:
    static WeatherState ComputeWeather(WeatherCondition Condition, float Intensity);

    /** Computes hydrodynamic wake, foam trail, and bow spray parameters for moving naval/amphibious vessels. */
    static ShipWakeOutput ComputeShipWake(float SpeedUnitsPerSec, float HullDisplacementTons, float SeaRoughness);
};

} // namespace RA4
