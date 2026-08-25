// Copyright (c) Red Alert 4 project. RTS camera, engine-free.
//
// The camera is presentation, not simulation: it uses float, it is frame-rate
// dependent by design, and it never feeds a value back into game state. Keeping it
// here rather than in a PlayerController means the feel of panning, zooming and
// edge scrolling can be regression-tested without launching the editor.
#pragma once

#include <cstdint>

#ifndef RA4INPUT_API
#define RA4INPUT_API
#endif

namespace RA4
{
namespace Input
{

struct Vec2f
{
    float X = 0.0f;
    float Y = 0.0f;

    constexpr Vec2f() = default;
    constexpr Vec2f(float InX, float InY) : X(InX), Y(InY) {}

    constexpr Vec2f operator+(const Vec2f& O) const { return Vec2f(X + O.X, Y + O.Y); }
    constexpr Vec2f operator-(const Vec2f& O) const { return Vec2f(X - O.X, Y - O.Y); }
    constexpr Vec2f operator*(float S) const { return Vec2f(X * S, Y * S); }
};

struct CameraConfig
{
    // Pan speed is expressed at the closest zoom and scaled up as the camera pulls
    // back, so a screen-width drag takes about the same time at every altitude.
    // Without that scaling a zoomed-out camera feels glued in place.
    float PanSpeedAtMinHeight = 1800.0f;   // world units per second
    float PanSpeedZoomFactor = 1.0f;       // 0 disables zoom-scaled panning
    float FastPanMultiplier = 2.5f;        // held shift

    bool bEdgeScrollEnabled = true;
    float EdgeScrollBorderPixels = 14.0f;
    // Edge scroll ramps up across the border band instead of snapping to full
    // speed, which is the difference between "responsive" and "twitchy".
    float EdgeScrollMinSpeedFraction = 0.35f;

    float MinHeight = 900.0f;
    float MaxHeight = 6500.0f;
    float ZoomStepPerNotch = 450.0f;

    // Exponential approach rates, per second. Higher is snappier.
    float PanSmoothing = 18.0f;
    float ZoomSmoothing = 12.0f;

    bool bRotationEnabled = false;
    float RotationStepDegrees = 45.0f;

    // Middle-mouse drag moves the world under the cursor rather than moving the
    // camera by the mouse delta, so the grabbed point stays under the pointer.
    float MiddleDragUnitsPerPixel = 6.0f;

    float BorderMarginUnits = 1200.0f;     // how far past the map edge the focus may go
};

class RA4INPUT_API CameraController
{
public:
    void Configure(const CameraConfig& InConfig) { Config = InConfig; }
    const CameraConfig& GetConfig() const { return Config; }

    // Map extent in world units. The focus point is clamped to this rectangle
    // expanded by BorderMarginUnits.
    void SetMapBounds(const Vec2f& InMin, const Vec2f& InMax);
    void SetViewportSize(float Width, float Height);

    // --- input ---------------------------------------------------------------
    // Keyboard pan, each axis in [-1, 1].
    void SetKeyboardPan(float AxisX, float AxisY) { KeyboardPan = Vec2f(Clamp(AxisX, -1.0f, 1.0f), Clamp(AxisY, -1.0f, 1.0f)); }
    void SetFastPan(bool bEnabled) { bFastPan = bEnabled; }
    // Edge scrolling only applies while the window has focus and the cursor is
    // actually inside the viewport; alt-tabbing must not slide the camera away.
    void SetCursorPosition(float PixelX, float PixelY, bool bWindowFocused);
    void AddZoomNotches(float Notches);
    void RotateSteps(int32_t Steps);
    // Continuous rotation, for dragging the mouse to spin the view. Unlike
    // RotateSteps this is not quantised, and it is not gated by bRotationEnabled --
    // the caller decides when a rotate gesture is active.
    void AddYawDegrees(float Delta);
    void AddPitchDegrees(float Delta);
    void ResetRotation();
    float GetPitchDegrees() const { return PitchDegrees; }

    void BeginMiddleDrag(float PixelX, float PixelY);
    void UpdateMiddleDrag(float PixelX, float PixelY);
    void EndMiddleDrag() { bMiddleDragging = false; }
    bool IsMiddleDragging() const { return bMiddleDragging; }

    // Jump to a position (minimap click, control-group recall). bInstant skips the
    // smoothing so a double-tap on a control group snaps rather than glides.
    void FocusOn(const Vec2f& WorldPosition, bool bInstant = false);

    void Update(float DeltaSeconds);

    // --- output --------------------------------------------------------------
    Vec2f GetFocus() const { return Focus; }
    Vec2f GetTargetFocus() const { return TargetFocus; }
    float GetHeight() const { return Height; }
    float GetTargetHeight() const { return TargetHeight; }
    float GetYawDegrees() const { return YawDegrees; }
    // 0 at MinHeight, 1 at MaxHeight. Drives LOD bias and HUD scaling.
    float GetZoomAlpha() const;

private:
    static float Clamp(float V, float Lo, float Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }
    Vec2f ClampToBounds(const Vec2f& P) const;
    Vec2f ComputeEdgeScroll() const;
    float CurrentPanSpeed() const;

    CameraConfig Config;

    Vec2f BoundsMin;
    Vec2f BoundsMax;
    bool bHasBounds = false;

    float ViewportWidth = 1920.0f;
    float ViewportHeight = 1080.0f;

    Vec2f KeyboardPan;
    bool bFastPan = false;

    Vec2f CursorPixel;
    bool bCursorInsideViewport = false;
    bool bWindowFocused = true;

    bool bMiddleDragging = false;
    Vec2f MiddleDragAnchorPixel;

    Vec2f Focus;
    Vec2f TargetFocus;
    float Height = 2500.0f;
    float TargetHeight = 2500.0f;
    float YawDegrees = 270.0f;
    float PitchDegrees = -55.0f;
};

} // namespace Input
} // namespace RA4
