// Copyright (c) Red Alert 4 project. Spatial 3D Audio Acoustics & Doppler Shift.
#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>

#ifndef RA4PRESENTATION_API
#define RA4PRESENTATION_API
#endif

namespace RA4
{

struct Audio3DListener
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 800.0f; // Typical isometric camera height
    float RightX = 1.0f;
    float RightY = 0.0f;
};

struct Audio3DEmitter
{
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float VelX = 0.0f;
    float VelY = 0.0f;
    float VelZ = 0.0f;
    float MinDistance = 200.0f;  // Full volume within 200 units
    float MaxDistance = 3000.0f; // Silence beyond 3000 units
};

struct SpatialAudioOutput
{
    float LeftVolume = 1.0f;
    float RightVolume = 1.0f;
    float PitchScale = 1.0f;
    float TotalAttenuation = 1.0f;
    float Pan = 0.0f; // -1.0 (hard left) .. +1.0 (hard right)
};

class RA4PRESENTATION_API SpatialAudio3D
{
public:
    static constexpr float kSpeedOfSoundUnits = 2000.0f; // Scaled sound speed in RTS world units/s

    /** Calculates 3D attenuation, stereo panning, and Doppler pitch shifting. */
    static SpatialAudioOutput ComputeSpatial(const Audio3DListener& Listener, const Audio3DEmitter& Emitter);
};

} // namespace RA4
