// Copyright (c) Red Alert 4 project.
#include "RA4RtsHud.h"

#include "RA4PlayerController.h"
#include "RA4SimCoords.h"
#include "RA4SimWorldSubsystem.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

void ARA4RtsHud::DrawHUD()
{
    Super::DrawHUD();

    const ARA4PlayerController* Controller = Cast<ARA4PlayerController>(PlayerOwner);
    if (Controller == nullptr)
    {
        return;
    }
    DrawSelectionBrackets(Controller);
    DrawMarquee(Controller);
}

void ARA4RtsHud::DrawMarquee(const ARA4PlayerController* Controller)
{
    if (!Controller->IsMarqueeActive())
    {
        return;
    }
    const FVector2D Start = Controller->GetMarqueeStartScreen();
    const FVector2D End = Controller->GetMarqueeCurrentScreen();

    // An outline rather than a filled quad: a translucent fill over a dense battle
    // hides exactly the units the player is trying to box.
    const float MinX = float(FMath::Min(Start.X, End.X));
    const float MaxX = float(FMath::Max(Start.X, End.X));
    const float MinY = float(FMath::Min(Start.Y, End.Y));
    const float MaxY = float(FMath::Max(Start.Y, End.Y));

    DrawLine(MinX, MinY, MaxX, MinY, MarqueeColor, 1.5f);
    DrawLine(MaxX, MinY, MaxX, MaxY, MarqueeColor, 1.5f);
    DrawLine(MaxX, MaxY, MinX, MaxY, MarqueeColor, 1.5f);
    DrawLine(MinX, MaxY, MinX, MinY, MarqueeColor, 1.5f);
}

void ARA4RtsHud::DrawSelectionBrackets(const ARA4PlayerController* Controller)
{
    UWorld* World = GetWorld();
    const URA4SimWorldSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    const RA4::SimWorld* Sim = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (Sim == nullptr || Controller == nullptr)
    {
        return;
    }

    for (const RA4::EntityId& Id : Controller->GetSelection().Get())
    {
        const RA4::TransformComp* Transform = Sim->GetTransform(Id);
        const RA4::EntityCore* Core = Sim->GetCore(Id);
        if (Transform == nullptr || Core == nullptr)
        {
            continue;
        }

        double RadiusUnits = 70.0;
        if (Sim->GetContent() != nullptr)
        {
            if (const RA4::EntityDef* Def = Sim->GetContent()->FindEntity(Core->Def))
            {
                RadiusUnits = Def->Kind == RA4::EntityKind::Building
                                  ? double(FMath::Max(Def->Building.FootprintX, Def->Building.FootprintY)) *
                                        double(RA4::kTileSizeUnits) * 0.5
                                  : FMath::Max(Def->Unit.CollisionRadius.ToDoubleUnsafe(), 45.0);
            }
        }

        // The bracket is a screen-space box around the unit's ground footprint, which
        // is what the originals drew: four corner ticks, no full outline, so a packed
        // formation stays readable instead of turning into a mesh of circles.
        const FVector Centre = RA4Coords::ToUnreal(Transform->Position);
        const FVector Offsets[4] = {FVector(-RadiusUnits, -RadiusUnits, 0.0), FVector(RadiusUnits, -RadiusUnits, 0.0),
                                    FVector(RadiusUnits, RadiusUnits, 0.0), FVector(-RadiusUnits, RadiusUnits, 0.0)};

        double MinX = 0.0;
        double MaxX = 0.0;
        double MinY = 0.0;
        double MaxY = 0.0;
        bool bHaveBox = false;
        for (const FVector& Offset : Offsets)
        {
            FVector2D Screen;
            if (!Controller->ProjectWorldLocationToScreen(Centre + Offset, Screen))
            {
                continue;
            }
            if (!bHaveBox)
            {
                MinX = MaxX = Screen.X;
                MinY = MaxY = Screen.Y;
                bHaveBox = true;
                continue;
            }
            MinX = FMath::Min(MinX, Screen.X);
            MaxX = FMath::Max(MaxX, Screen.X);
            MinY = FMath::Min(MinY, Screen.Y);
            MaxY = FMath::Max(MaxY, Screen.Y);
        }
        if (!bHaveBox)
        {
            continue;
        }

        // A quarter of the shorter side, so the ticks stay ticks at every zoom instead
        // of meeting in the middle on a distant infantryman.
        const float Tick =
            float(FMath::Clamp(FMath::Min(MaxX - MinX, MaxY - MinY) * 0.25, 3.0, 14.0));
        constexpr float Thickness = 1.6f;

        const float L = float(MinX);
        const float R = float(MaxX);
        const float T = float(MinY);
        const float B = float(MaxY);

        DrawLine(L, T, L + Tick, T, SelectionColor, Thickness);
        DrawLine(L, T, L, T + Tick, SelectionColor, Thickness);
        DrawLine(R - Tick, T, R, T, SelectionColor, Thickness);
        DrawLine(R, T, R, T + Tick, SelectionColor, Thickness);
        DrawLine(L, B - Tick, L, B, SelectionColor, Thickness);
        DrawLine(L, B, L + Tick, B, SelectionColor, Thickness);
        DrawLine(R, B - Tick, R, B, SelectionColor, Thickness);
        DrawLine(R - Tick, B, R, B, SelectionColor, Thickness);

        // Health bar above the bracket, only once the thing has taken a hit -- a wall
        // of full green bars over an untouched army is noise.
        const RA4::HealthComp* Health = Sim->GetHealth(Id);
        if (Health == nullptr || Health->Max <= 0 || Health->Current >= Health->Max)
        {
            continue;
        }

        const float Fraction = FMath::Clamp(float(Health->Current) / float(Health->Max), 0.0f, 1.0f);
        const float BarWidth = R - L;
        const float BarY = T - 6.0f;
        const FLinearColor BarColour = Fraction > 0.66f   ? HealthHighColor
                                       : Fraction > 0.33f ? HealthMediumColor
                                                          : HealthLowColor;

        DrawRect(FLinearColor(0.02f, 0.02f, 0.03f, 0.75f), L, BarY, BarWidth, 3.0f);
        DrawRect(BarColour, L, BarY, BarWidth * Fraction, 3.0f);
    }
}
