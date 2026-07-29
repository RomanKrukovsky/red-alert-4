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
    DrawSelectionRings(Controller);
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

void ARA4RtsHud::DrawSelectionRings(const ARA4PlayerController* Controller)
{
    UWorld* World = GetWorld();
    const URA4SimWorldSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    const RA4::SimWorld* Sim = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (Sim == nullptr || Controller == nullptr)
    {
        return;
    }

    constexpr int32 SegmentCount = 16;
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

        // Projected per segment so the ring sits flat on the ground and reads
        // correctly under the tilted camera.
        FVector2D Previous = FVector2D::ZeroVector;
        bool bHasPrevious = false;
        for (int32 Segment = 0; Segment <= SegmentCount; ++Segment)
        {
            const double Angle = (double(Segment) / double(SegmentCount)) * 2.0 * PI;
            const FVector WorldPoint = RA4Coords::ToUnreal(Transform->Position) +
                                       FVector(FMath::Cos(Angle) * RadiusUnits, FMath::Sin(Angle) * RadiusUnits, 0.0);
            FVector2D Screen;
            if (Controller->ProjectWorldLocationToScreen(WorldPoint, Screen))
            {
                if (bHasPrevious)
                {
                    DrawLine(float(Previous.X), float(Previous.Y), float(Screen.X), float(Screen.Y), SelectionColor, 1.2f);
                }
                Previous = Screen;
                bHasPrevious = true;
            }
            else
            {
                bHasPrevious = false;
            }
        }
    }
}
