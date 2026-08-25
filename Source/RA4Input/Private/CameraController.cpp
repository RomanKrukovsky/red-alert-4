// Copyright (c) Red Alert 4 project.
#include "RA4Input/CameraController.h"

#include <cmath>

namespace RA4
{
namespace Input
{

void CameraController::SetMapBounds(const Vec2f& InMin, const Vec2f& InMax)
{
    BoundsMin = InMin;
    BoundsMax = InMax;
    bHasBounds = true;
    Focus = ClampToBounds(Focus);
    TargetFocus = ClampToBounds(TargetFocus);
}

void CameraController::SetViewportSize(float InWidth, float InHeight)
{
    ViewportWidth = InWidth > 1.0f ? InWidth : 1.0f;
    ViewportHeight = InHeight > 1.0f ? InHeight : 1.0f;
}

void CameraController::SetCursorPosition(float PixelX, float PixelY, bool bInWindowFocused)
{
    CursorPixel = Vec2f(PixelX, PixelY);
    bWindowFocused = bInWindowFocused;
    bCursorInsideViewport = PixelX >= 0.0f && PixelY >= 0.0f && PixelX <= ViewportWidth && PixelY <= ViewportHeight;
}

void CameraController::AddZoomNotches(float Notches)
{
    TargetHeight = Clamp(TargetHeight - Notches * Config.ZoomStepPerNotch, Config.MinHeight, Config.MaxHeight);
}

void CameraController::RotateSteps(int32_t Steps)
{
    if (!Config.bRotationEnabled || Steps == 0)
    {
        return;
    }
    YawDegrees += float(Steps) * Config.RotationStepDegrees;
    YawDegrees = std::fmod(YawDegrees, 360.0f);
    if (YawDegrees < 0.0f)
    {
        YawDegrees += 360.0f;
    }
}

void CameraController::AddYawDegrees(float Delta)
{
    if (Delta == 0.0f)
    {
        return;
    }
    YawDegrees = std::fmod(YawDegrees + Delta, 360.0f);
    if (YawDegrees < 0.0f)
    {
        YawDegrees += 360.0f;
    }
}

void CameraController::AddPitchDegrees(float Delta)
{
    if (Delta == 0.0f)
    {
        return;
    }
    PitchDegrees = Clamp(PitchDegrees + Delta, -85.0f, -20.0f);
}

void CameraController::ResetRotation()
{
    // Face north (-Y): the tactical minimap draws north up, so the opening
    // camera orientation matches the map instead of being rotated 90 degrees
    // from it (which read as "the radar shows the map wrong").
    YawDegrees = 270.0f;
    PitchDegrees = -55.0f;
}

void CameraController::BeginMiddleDrag(float PixelX, float PixelY)
{
    bMiddleDragging = true;
    MiddleDragAnchorPixel = Vec2f(PixelX, PixelY);
}

void CameraController::UpdateMiddleDrag(float PixelX, float PixelY)
{
    if (!bMiddleDragging)
    {
        return;
    }
    const Vec2f Delta(PixelX - MiddleDragAnchorPixel.X, PixelY - MiddleDragAnchorPixel.Y);
    MiddleDragAnchorPixel = Vec2f(PixelX, PixelY);

    // Drag moves the world with the cursor, so the camera goes the other way.
    // Scaled by zoom so the grabbed point tracks the pointer at any altitude.
    const float Scale = Config.MiddleDragUnitsPerPixel * (Height / Config.MaxHeight);
    const float Radians = YawDegrees * 3.14159265358979323846f / 180.0f;
    const float SinYaw = std::sin(Radians);
    const float CosYaw = std::cos(Radians);

    const float PanX = Delta.X * Scale;
    const float PanY = -Delta.Y * Scale;

    const Vec2f WorldDrag(PanX * CosYaw + PanY * SinYaw,
                         -PanY * CosYaw + PanX * SinYaw);

    TargetFocus = ClampToBounds(Vec2f(TargetFocus.X - WorldDrag.X, TargetFocus.Y - WorldDrag.Y));
}

void CameraController::FocusOn(const Vec2f& WorldPosition, bool bInstant)
{
    TargetFocus = ClampToBounds(WorldPosition);
    if (bInstant)
    {
        Focus = TargetFocus;
    }
}

float CameraController::GetZoomAlpha() const
{
    const float Range = Config.MaxHeight - Config.MinHeight;
    if (Range <= 0.0f)
    {
        return 0.0f;
    }
    return Clamp((Height - Config.MinHeight) / Range, 0.0f, 1.0f);
}

Vec2f CameraController::ClampToBounds(const Vec2f& P) const
{
    if (!bHasBounds)
    {
        return P;
    }
    const float M = Config.BorderMarginUnits;
    const float MinX = BoundsMin.X - M;
    const float MaxX = BoundsMax.X + M;
    const float MinY = BoundsMin.Y - M;
    const float MaxY = BoundsMax.Y + M;
    // Guard against inverted bounds so a malformed map cannot invert the clamp.
    return Vec2f(Clamp(P.X, MinX < MaxX ? MinX : MaxX, MaxX > MinX ? MaxX : MinX),
                 Clamp(P.Y, MinY < MaxY ? MinY : MaxY, MaxY > MinY ? MaxY : MinY));
}

float CameraController::CurrentPanSpeed() const
{
    const float Range = Config.MaxHeight - Config.MinHeight;
    const float Alpha = Range > 0.0f ? Clamp((Height - Config.MinHeight) / Range, 0.0f, 1.0f) : 0.0f;
    const float ZoomScale = 1.0f + Alpha * Config.PanSpeedZoomFactor;
    return Config.PanSpeedAtMinHeight * ZoomScale * (bFastPan ? Config.FastPanMultiplier : 1.0f);
}

Vec2f CameraController::ComputeEdgeScroll() const
{
    // Suppressed while middle-dragging: the cursor is necessarily near an edge
    // during a long drag, and having both fight each other feels broken.
    if (!Config.bEdgeScrollEnabled || !bWindowFocused || !bCursorInsideViewport || bMiddleDragging)
    {
        return Vec2f();
    }

    const float Border = Config.EdgeScrollBorderPixels;
    if (Border <= 0.0f)
    {
        return Vec2f();
    }

    auto Ramp = [this, Border](float DistanceFromEdge) -> float
    {
        const float T = Clamp(1.0f - DistanceFromEdge / Border, 0.0f, 1.0f);
        if (T <= 0.0f)
        {
            return 0.0f;
        }
        const float MinF = Clamp(Config.EdgeScrollMinSpeedFraction, 0.0f, 1.0f);
        return MinF + (1.0f - MinF) * T;
    };

    Vec2f Result;
    if (CursorPixel.X <= Border)
    {
        Result.X = -Ramp(CursorPixel.X);
    }
    else if (CursorPixel.X >= ViewportWidth - Border)
    {
        Result.X = Ramp(ViewportWidth - CursorPixel.X);
    }

    // Screen Y grows downward; world Y grows "up" on the map.
    if (CursorPixel.Y <= Border)
    {
        Result.Y = Ramp(CursorPixel.Y);
    }
    else if (CursorPixel.Y >= ViewportHeight - Border)
    {
        Result.Y = -Ramp(ViewportHeight - CursorPixel.Y);
    }

    return Result;
}

void CameraController::Update(float DeltaSeconds)
{
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }
    // A hitch must not teleport the camera across the map.
    const float Dt = DeltaSeconds > 0.1f ? 0.1f : DeltaSeconds;

    const Vec2f Edge = ComputeEdgeScroll();
    Vec2f Pan(KeyboardPan.X + Edge.X, KeyboardPan.Y + Edge.Y);

    // Normalise so diagonal panning is not 41% faster than cardinal panning.
    const float LengthSq = Pan.X * Pan.X + Pan.Y * Pan.Y;
    if (LengthSq > 1.0f)
    {
        const float InvLength = 1.0f / std::sqrt(LengthSq);
        Pan = Vec2f(Pan.X * InvLength, Pan.Y * InvLength);
    }

    if (Pan.X != 0.0f || Pan.Y != 0.0f)
    {
        // Pan input is screen-relative: "W" means up-screen whatever way the camera
        // is facing. The camera's SpringArm has base rotation (Pitch, 90 + Yaw, 0).
        // At Yaw 0, camera faces +Y (Forward = +Y, Right = -X).
        // For arbitrary Yaw:
        // Forward in XY = (-sin(Yaw), cos(Yaw))
        // Right in XY   = (-cos(Yaw), -sin(Yaw))
        // WorldPan = Forward * Pan.Y + Right * Pan.X
        const float Radians = YawDegrees * 3.14159265358979323846f / 180.0f;
        const float SinYaw = std::sin(Radians);
        const float CosYaw = std::cos(Radians);

        const Vec2f WorldPan(-Pan.X * CosYaw - Pan.Y * SinYaw,
                              Pan.Y * CosYaw - Pan.X * SinYaw);

        const float Speed = CurrentPanSpeed();
        TargetFocus = ClampToBounds(Vec2f(TargetFocus.X + WorldPan.X * Speed * Dt,
                                          TargetFocus.Y + WorldPan.Y * Speed * Dt));
    }

    // Frame-rate independent exponential approach: the fraction of the remaining
    // distance covered depends on elapsed time, not on how many frames elapsed.
    const float PanAlpha = 1.0f - std::exp(-Config.PanSmoothing * Dt);
    Focus = Vec2f(Focus.X + (TargetFocus.X - Focus.X) * PanAlpha,
                  Focus.Y + (TargetFocus.Y - Focus.Y) * PanAlpha);

    const float ZoomAlpha = 1.0f - std::exp(-Config.ZoomSmoothing * Dt);
    Height += (TargetHeight - Height) * ZoomAlpha;
}

} // namespace Input
} // namespace RA4
