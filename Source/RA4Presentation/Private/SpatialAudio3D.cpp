// Copyright (c) Red Alert 4 project. Spatial 3D Audio Acoustics & Doppler Shift.
#include "RA4Presentation/SpatialAudio3D.h"

namespace RA4
{

SpatialAudioOutput SpatialAudio3D::ComputeSpatial(const Audio3DListener& Listener, const Audio3DEmitter& Emitter)
{
    SpatialAudioOutput Out;

    const float Dx = Emitter.X - Listener.X;
    const float Dy = Emitter.Y - Listener.Y;
    const float Dz = Emitter.Z - Listener.Z;
    const float Distance = std::sqrt(Dx * Dx + Dy * Dy + Dz * Dz);

    // 1. Distance Attenuation
    if (Distance <= Emitter.MinDistance)
    {
        Out.TotalAttenuation = 1.0f;
    }
    else if (Distance >= Emitter.MaxDistance)
    {
        Out.TotalAttenuation = 0.0f;
    }
    else
    {
        Out.TotalAttenuation = (Emitter.MaxDistance - Distance) / (Emitter.MaxDistance - Emitter.MinDistance);
    }

    // 2. Stereo Panning
    const float PlanarDist = std::sqrt(Dx * Dx + Dy * Dy);
    if (PlanarDist > 0.001f)
    {
        Out.Pan = (Dx * Listener.RightX + Dy * Listener.RightY) / PlanarDist;
        Out.Pan = std::clamp(Out.Pan, -1.0f, 1.0f);
    }
    else
    {
        Out.Pan = 0.0f;
    }

    // Constant power panning law
    Out.LeftVolume = std::sqrt(std::clamp((1.0f - Out.Pan) * 0.5f, 0.0f, 1.0f)) * Out.TotalAttenuation;
    Out.RightVolume = std::sqrt(std::clamp((1.0f + Out.Pan) * 0.5f, 0.0f, 1.0f)) * Out.TotalAttenuation;

    // 3. Doppler Pitch Shift
    if (Distance > 0.001f)
    {
        // Line-of-sight vector from Emitter to Listener
        const float Lx = -Dx / Distance;
        const float Ly = -Dy / Distance;
        const float Lz = -Dz / Distance;

        const float VApproach = Emitter.VelX * Lx + Emitter.VelY * Ly + Emitter.VelZ * Lz;
        const float ClampedApproach = std::clamp(VApproach, -kSpeedOfSoundUnits * 0.8f, kSpeedOfSoundUnits * 0.8f);

        Out.PitchScale = kSpeedOfSoundUnits / (kSpeedOfSoundUnits - ClampedApproach);
        Out.PitchScale = std::clamp(Out.PitchScale, 0.5f, 2.0f);
    }
    else
    {
        Out.PitchScale = 1.0f;
    }

    return Out;
}

} // namespace RA4
