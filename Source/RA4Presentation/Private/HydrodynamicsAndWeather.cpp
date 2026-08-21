// Copyright (c) Red Alert 4 project. Hydrodynamic Ship Wakes & Dynamic Weather.
#include "RA4Presentation/HydrodynamicsAndWeather.h"

namespace RA4
{

WeatherState HydrodynamicsAndWeather::ComputeWeather(WeatherCondition Condition, float Intensity)
{
    WeatherState Out;
    Out.Condition = Condition;
    Out.Intensity = std::clamp(Intensity, 0.0f, 1.0f);

    switch (Condition)
    {
    case WeatherCondition::Clear:
        Out.SeaRoughness = 0.05f;
        Out.AirVisionFactor = 1.0f;
        break;
    case WeatherCondition::Rain:
        Out.SeaRoughness = 0.1f + Out.Intensity * 0.3f;
        Out.AirVisionFactor = 1.0f - Out.Intensity * 0.2f;
        break;
    case WeatherCondition::Thunderstorm:
        Out.SeaRoughness = 0.4f + Out.Intensity * 0.6f;
        Out.AirVisionFactor = 1.0f - Out.Intensity * 0.45f;
        break;
    case WeatherCondition::HeavyFog:
        Out.SeaRoughness = 0.05f;
        Out.AirVisionFactor = 1.0f - Out.Intensity * 0.6f;
        break;
    }

    return Out;
}

ShipWakeOutput HydrodynamicsAndWeather::ComputeShipWake(float SpeedUnitsPerSec, float HullDisplacementTons, float SeaRoughness)
{
    ShipWakeOutput Out;
    Out.KelvinCuspAngleDeg = 19.47f; // Universal theoretical Kelvin cusp angle

    const float SpeedNorm = std::max(0.0f, SpeedUnitsPerSec) / 150.0f;
    const float DisplacementNorm = std::max(1.0f, HullDisplacementTons) / 1000.0f;

    Out.FoamIntensity = std::clamp(SpeedNorm * DisplacementNorm, 0.0f, 1.0f);
    Out.SprayParticleRate = SpeedNorm * 50.0f * (1.0f + SeaRoughness);
    Out.SternWaveHeight = std::clamp(SpeedNorm * DisplacementNorm * 12.0f, 0.0f, 30.0f);

    return Out;
}

} // namespace RA4
