// Copyright (c) Red Alert 4 project. Tests for Stage 15 (Spatial 3D Acoustics & Doppler Shift).
#include "TestFramework.h"

#include "RA4Presentation/SpatialAudio3D.h"

using namespace RA4;
using namespace RA4Test;

// --- 1. Distance Attenuation Curve ---

RA4_TEST(SpatialAudio3D, DistanceAttenuationNearMidFar)
{
    Audio3DListener Listener;
    Listener.X = 0.0f;
    Listener.Y = 0.0f;
    Listener.Z = 0.0f;

    Audio3DEmitter NearEmitter;
    NearEmitter.X = 100.0f;
    NearEmitter.MinDistance = 200.0f;
    NearEmitter.MaxDistance = 3000.0f;

    const auto NearOut = SpatialAudio3D::ComputeSpatial(Listener, NearEmitter);
    RA4_EXPECT_NEAR(NearOut.TotalAttenuation, 1.0f, 0.01f);

    Audio3DEmitter MidEmitter;
    MidEmitter.X = 1600.0f; // Exact midpoint between 200 and 3000
    MidEmitter.MinDistance = 200.0f;
    MidEmitter.MaxDistance = 3000.0f;

    const auto MidOut = SpatialAudio3D::ComputeSpatial(Listener, MidEmitter);
    RA4_EXPECT_NEAR(MidOut.TotalAttenuation, 0.5f, 0.02f);

    Audio3DEmitter FarEmitter;
    FarEmitter.X = 3500.0f; // Past MaxDistance
    FarEmitter.MinDistance = 200.0f;
    FarEmitter.MaxDistance = 3000.0f;

    const auto FarOut = SpatialAudio3D::ComputeSpatial(Listener, FarEmitter);
    RA4_EXPECT_NEAR(FarOut.TotalAttenuation, 0.0f, 0.001f);
}

// --- 2. Stereo Panning Left / Right Channels ---

RA4_TEST(SpatialAudio3D, StereoPanningLeftRight)
{
    Audio3DListener Listener;
    Listener.X = 0.0f;
    Listener.Y = 0.0f;
    Listener.Z = 0.0f;
    Listener.RightX = 1.0f;
    Listener.RightY = 0.0f;

    Audio3DEmitter LeftEmitter;
    LeftEmitter.X = -500.0f;
    LeftEmitter.Y = 0.0f;
    LeftEmitter.Z = 0.0f;
    LeftEmitter.MinDistance = 1000.0f;

    const auto LeftOut = SpatialAudio3D::ComputeSpatial(Listener, LeftEmitter);
    RA4_EXPECT_NEAR(LeftOut.Pan, -1.0f, 0.01f);
    RA4_EXPECT(LeftOut.LeftVolume > LeftOut.RightVolume);
    RA4_EXPECT_NEAR(LeftOut.RightVolume, 0.0f, 0.01f);

    Audio3DEmitter RightEmitter;
    RightEmitter.X = 500.0f;
    RightEmitter.Y = 0.0f;
    RightEmitter.Z = 0.0f;
    RightEmitter.MinDistance = 1000.0f;

    const auto RightOut = SpatialAudio3D::ComputeSpatial(Listener, RightEmitter);
    RA4_EXPECT_NEAR(RightOut.Pan, 1.0f, 0.01f);
    RA4_EXPECT(RightOut.RightVolume > RightOut.LeftVolume);
    RA4_EXPECT_NEAR(RightOut.LeftVolume, 0.0f, 0.01f);
}

// --- 3. Doppler Pitch Shift for High-Speed Movers ---

RA4_TEST(SpatialAudio3D, DopplerPitchShift)
{
    Audio3DListener Listener;
    Listener.X = 0.0f;
    Listener.Y = 0.0f;
    Listener.Z = 0.0f;

    Audio3DEmitter ApproachingJet;
    ApproachingJet.X = 1000.0f;
    ApproachingJet.Y = 0.0f;
    ApproachingJet.Z = 0.0f;
    ApproachingJet.VelX = -1000.0f; // Moving towards (0,0,0) at half the speed of sound (2000 units/s)

    const auto ApproachOut = SpatialAudio3D::ComputeSpatial(Listener, ApproachingJet);
    // Pitch scale = 2000 / (2000 - 1000) = 2.0x
    RA4_EXPECT_NEAR(ApproachOut.PitchScale, 2.0f, 0.05f);

    Audio3DEmitter RecedingJet;
    RecedingJet.X = 1000.0f;
    RecedingJet.Y = 0.0f;
    RecedingJet.Z = 0.0f;
    RecedingJet.VelX = 1000.0f; // Moving away from (0,0,0)

    const auto RecedeOut = SpatialAudio3D::ComputeSpatial(Listener, RecedingJet);
    // Pitch scale = 2000 / (2000 - (-1000)) = 2000 / 3000 = 0.667x
    RA4_EXPECT_NEAR(RecedeOut.PitchScale, 0.667f, 0.05f);
}
