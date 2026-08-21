// Copyright (c) Red Alert 4 project.
#include "RA4RtsHud.h"

#include "RA4PlayerController.h"
#include "RA4ReconDebugOverlay.h"
#include "RA4SimCoords.h"
#include "RA4SimWorldSubsystem.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

#include "RA4DirectControlSubsystem.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"

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
    DrawMoveTargetRing(Controller);

    // Two-maps recon overlay (ADR-0026 §7): console `recon.Overlay 1|2`, or the
    // ?ShowTruth=1 skirmish option. Local player is slot 0, matching the
    // SnapshotBuilder->Initialize(0) the HUD already assumes.
    if (const UWorld* W = GetWorld())
    {
        if (const URA4SimWorldSubsystem* Sim = W->GetSubsystem<URA4SimWorldSubsystem>())
        {
            URA4ReconDebugOverlay::Draw(Canvas, Controller, Sim, /*ViewerPlayer*/ 0);
        }
    }

    if (Controller->IsPlacementArmed())
    {
        DrawPlacementFootprint(Controller);
    }

    DrawDirectControlHUD(Controller);
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

void ARA4RtsHud::DrawMoveTargetRing(const ARA4PlayerController* Controller)
{
    // A one-shot confirmation at the ordered spot, not a marker that follows the
    // pointer: the player already knows where the cursor is.
    RA4::Vec2 Ground;
    double IssuedSeconds = 0.0;
    if (!Controller->GetMoveOrderPing(Ground, IssuedSeconds))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr || MoveTargetRingDurationSeconds <= 0.0f)
    {
        return;
    }

    const double Elapsed = World->GetTimeSeconds() - IssuedSeconds;
    if (Elapsed < 0.0 || Elapsed > double(MoveTargetRingDurationSeconds))
    {
        return;
    }

    // A single outward pulse that fades as it expands, so the eye is drawn to the
    // spot once and then left alone.
    const float Alpha = float(Elapsed / double(MoveTargetRingDurationSeconds));
    const float Radius = MoveTargetRingRadiusUnits * (0.45f + 0.55f * Alpha);
    FLinearColor Colour = MoveTargetRingColor;
    Colour.A *= (1.0f - Alpha);

    // Projected per segment so the ring lies flat on the ground and follows the
    // terrain's perspective instead of being a flat screen-space circle.
    constexpr int32 SegmentCount = 28;
    const FVector Centre = RA4Coords::ToUnreal(Ground);
    FVector2D Previous = FVector2D::ZeroVector;
    bool bHasPrevious = false;

    for (int32 Segment = 0; Segment <= SegmentCount; ++Segment)
    {
        const double Angle = (double(Segment) / double(SegmentCount)) * 2.0 * PI;
        const FVector WorldPoint =
            Centre + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
        FVector2D Screen;
        if (Controller->ProjectWorldLocationToScreen(WorldPoint, Screen))
        {
            if (bHasPrevious)
            {
                DrawLine(float(Previous.X), float(Previous.Y), float(Screen.X), float(Screen.Y), Colour, 2.5f);
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

void ARA4RtsHud::DrawPlacementFootprint(const ARA4PlayerController* Controller)
{
    UWorld* World = GetWorld();
    const URA4SimWorldSubsystem* Subsystem = World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    const RA4::SimWorld* Sim = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (Sim == nullptr || Controller == nullptr || World == nullptr)
    {
        return;
    }

    RA4::ContentId ContentId = Controller->GetPlacementContent();
    if (!ContentId.IsValid() || Sim->GetContent() == nullptr)
    {
        return;
    }

    const RA4::EntityDef* Def = Sim->GetContent()->FindEntity(ContentId);
    if (Def == nullptr || Def->Kind != RA4::EntityKind::Building)
    {
        return;
    }

    RA4::Vec2 CursorGround;
    if (!Controller->GetCursorGroundPosition(CursorGround))
    {
        return;
    }

    // Footprint is centered on CursorGround
    const RA4::TileCoord OriginTile = Sim->GetMap().WorldToTile(CursorGround);
    const RA4::PlayerId LocalPlayer = Controller->GetSelection().GetLocalPlayer();
    const bool bOverallValid = Sim->IsPlacementValid(ContentId, LocalPlayer, OriginTile);

    const double HalfSize = double(RA4::kTileSizeUnits) * 0.5;

    // Draw base build area / power radius preview circle
    const RA4::Vec2 OriginCenter = Sim->GetMap().TileCenterToWorld(OriginTile);
    const FVector OriginUnreal = RA4Coords::ToUnreal(OriginCenter, RA4Coords::GroundZ + 5.0);
    DrawDebugCircle(World, OriginUnreal, 900.0f, 36, FColor(40, 180, 255, 120), false, 0.04f, 0, 2.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);

    // The grid occupies OriginTile to OriginTile + (FootprintX, FootprintY)
    for (int32_t Y = 0; Y < Def->Building.FootprintY; ++Y)
    {
        for (int32_t X = 0; X < Def->Building.FootprintX; ++X)
        {
            RA4::TileCoord Tile = OriginTile;
            Tile.X += X;
            Tile.Y += Y;
            
            const bool bCellInBounds = Sim->GetMap().IsInBounds(Tile.X, Tile.Y);
            const bool bCellClear = bCellInBounds && ((Sim->GetMap().GetTile(Tile.X, Tile.Y) & RA4::Tile_GroundPassable) != 0);
            const bool bCellValid = bOverallValid && bCellClear;

            const FLinearColor CellColor = bCellValid
                ? FLinearColor(0.1f, 1.0f, 0.35f, 0.55f)
                : FLinearColor(1.0f, 0.15f, 0.15f, 0.55f);

            const RA4::Vec2 TileWorldCenter = Sim->GetMap().TileCenterToWorld(Tile);
            const FVector CenterUnreal = RA4Coords::ToUnreal(TileWorldCenter, RA4Coords::GroundZ + 4.0);

            // 3D terrain grid lines
            const FVector P0 = CenterUnreal + FVector(-HalfSize, -HalfSize, 0.0);
            const FVector P1 = CenterUnreal + FVector(HalfSize, -HalfSize, 0.0);
            const FVector P2 = CenterUnreal + FVector(HalfSize, HalfSize, 0.0);
            const FVector P3 = CenterUnreal + FVector(-HalfSize, HalfSize, 0.0);

            const FColor DebugColor = CellColor.ToFColor(true);
            DrawDebugLine(World, P0, P1, DebugColor, false, 0.04f, 0, 2.5f);
            DrawDebugLine(World, P1, P2, DebugColor, false, 0.04f, 0, 2.5f);
            DrawDebugLine(World, P2, P3, DebugColor, false, 0.04f, 0, 2.5f);
            DrawDebugLine(World, P3, P0, DebugColor, false, 0.04f, 0, 2.5f);

            // Screen space filled quad
            FVector2D S0, S1, S2, S3;
            if (Controller->ProjectWorldLocationToScreen(P0, S0) &&
                Controller->ProjectWorldLocationToScreen(P1, S1) &&
                Controller->ProjectWorldLocationToScreen(P2, S2) &&
                Controller->ProjectWorldLocationToScreen(P3, S3))
            {
                const float MinX = FMath::Min(FMath::Min(S0.X, S1.X), FMath::Min(S2.X, S3.X));
                const float MaxX = FMath::Max(FMath::Max(S0.X, S1.X), FMath::Max(S2.X, S3.X));
                const float MinY = FMath::Min(FMath::Min(S0.Y, S1.Y), FMath::Min(S2.Y, S3.Y));
                const float MaxY = FMath::Max(FMath::Max(S0.Y, S1.Y), FMath::Max(S2.Y, S3.Y));
                DrawRect(CellColor, MinX, MinY, MaxX - MinX, MaxY - MinY);
            }
        }
    }
}

void ARA4RtsHud::DrawDirectControlHUD(const ARA4PlayerController* Controller)
{
    if (Controller == nullptr || Canvas == nullptr)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const URA4DirectControlSubsystem* Dc = World ? World->GetSubsystem<URA4DirectControlSubsystem>() : nullptr;
    const URA4SimWorldSubsystem* Sim = World ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    const RA4::SimWorld* SimWorld = Sim ? Sim->GetSimWorld() : nullptr;

    const bool bInDc = (Dc != nullptr && (Dc->IsInDirectControl() ||
                                          Dc->GetClientPhase() == ERA4DirectControlClientPhase::Entering ||
                                          Dc->GetClientPhase() == ERA4DirectControlClientPhase::DirectControl));

    if (!bInDc)
    {
        return;
    }

    auto DrawHudLine = [this](float X1, float Y1, float X2, float Y2, const FLinearColor& Color, float Thickness = 1.0f)
    {
        if (Canvas)
        {
            FCanvasLineItem LineItem(FVector2D(X1, Y1), FVector2D(X2, Y2));
            LineItem.SetColor(Color);
            LineItem.LineThickness = Thickness;
            Canvas->DrawItem(LineItem);
        }
    };

    auto DrawColoredText = [this](const FText& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.0f)
    {
        if (Canvas)
        {
            UFont* RenderFont = GEngine ? GEngine->GetSmallFont() : nullptr;
            if (RenderFont != nullptr)
            {
                FCanvasTextItem TextItem(FVector2D(X, Y), Text, RenderFont, Color);
                TextItem.Scale = FVector2D(Scale, Scale);
                TextItem.BlendMode = SE_BLEND_Translucent;
                Canvas->DrawItem(TextItem);
            }
        }
    };

    const float ScreenW = Canvas->SizeX;
    const float ScreenH = Canvas->SizeY;
    const FVector2D Center(ScreenW * 0.5f, ScreenH * 0.5f);

    // Query vehicle health and info from SimWorld
    int32 VehHealth = 500;
    int32 VehMaxHealth = 500;
    FString VehName = TEXT("ТАНК");
    float SpeedKph = 0.0f;
    bool bOpticsZoomed = false;

    if (SimWorld != nullptr && Dc != nullptr)
    {
        const int32 VehIdx = Dc->GetControlledVehicleIndex();
        if (VehIdx >= 0)
        {
            const RA4::EntityId TargetId = SimWorld->MakeId(uint32_t(VehIdx));
            if (SimWorld->IsAlive(TargetId))
            {
                const RA4::EntityCore* Core = SimWorld->GetCore(TargetId);
                if (Core != nullptr && Core->bAlive)
                {
                    const RA4::EntityDef* Def = SimWorld->GetContent() ? SimWorld->GetContent()->FindEntity(Core->Def) : nullptr;
                    if (Def != nullptr)
                    {
                        VehName = UTF8_TO_TCHAR(Def->Name.c_str());
                        VehMaxHealth = Def->MaxHealth;
                    }
                    const RA4::HealthComp* HealthComp = SimWorld->GetHealth(TargetId);
                    if (HealthComp != nullptr)
                    {
                        VehHealth = HealthComp->Current;
                    }
                    const RA4::DirectControlComp* DcComp = SimWorld->GetDirectControl(TargetId);
                    if (DcComp != nullptr)
                    {
                        bOpticsZoomed = DcComp->bOpticsZoomed;
                    }
                    const RA4::MovementComp* MovComp = SimWorld->GetMovement(TargetId);
                    if (MovComp != nullptr)
                    {
                        SpeedKph = float(MovComp->CurrentSpeed.ToDoubleUnsafe() * 0.036);
                    }
                }
            }
        }
    }

    // 1. Center Tactical Reticle
    const float ReticleSize = bOpticsZoomed ? 28.0f : 20.0f;
    const FLinearColor ReticleColor = bOpticsZoomed ? FLinearColor(0.2f, 1.0f, 0.4f, 0.95f) : FLinearColor(0.2f, 0.85f, 1.0f, 0.85f);

    // Crosshairs
    DrawHudLine(Center.X - ReticleSize, Center.Y, Center.X - 5.0f, Center.Y, ReticleColor, 2.0f);
    DrawHudLine(Center.X + 5.0f, Center.Y, Center.X + ReticleSize, Center.Y, ReticleColor, 2.0f);
    DrawHudLine(Center.X, Center.Y - ReticleSize, Center.X, Center.Y - 5.0f, ReticleColor, 2.0f);
    DrawHudLine(Center.X, Center.Y + 5.0f, Center.X, Center.Y + ReticleSize, ReticleColor, 2.0f);

    // Center Dot
    FCanvasTileItem DotItem(Center - FVector2D(2.0f, 2.0f), FVector2D(4.0f, 4.0f), ReticleColor);
    Canvas->DrawItem(DotItem);

    // Target Brackets [ ]
    const float BracketW = 40.0f;
    const float BracketH = 26.0f;
    DrawHudLine(Center.X - BracketW, Center.Y - BracketH, Center.X - BracketW + 8.0f, Center.Y - BracketH, ReticleColor, 1.5f);
    DrawHudLine(Center.X - BracketW, Center.Y - BracketH, Center.X - BracketW, Center.Y - BracketH + 8.0f, ReticleColor, 1.5f);
    DrawHudLine(Center.X + BracketW, Center.Y - BracketH, Center.X + BracketW - 8.0f, Center.Y - BracketH, ReticleColor, 1.5f);
    DrawHudLine(Center.X + BracketW, Center.Y - BracketH, Center.X + BracketW, Center.Y - BracketH + 8.0f, ReticleColor, 1.5f);

    DrawHudLine(Center.X - BracketW, Center.Y + BracketH, Center.X - BracketW + 8.0f, Center.Y + BracketH, ReticleColor, 1.5f);
    DrawHudLine(Center.X - BracketW, Center.Y + BracketH, Center.X - BracketW, Center.Y + BracketH - 8.0f, ReticleColor, 1.5f);
    DrawHudLine(Center.X + BracketW, Center.Y + BracketH, Center.X + BracketW - 8.0f, Center.Y + BracketH, ReticleColor, 1.5f);
    DrawHudLine(Center.X + BracketW, Center.Y + BracketH, Center.X + BracketW, Center.Y + BracketH - 8.0f, ReticleColor, 1.5f);

    // Reticle Info Readout
    FString ReticleInfo = FString::Printf(TEXT("СКОРОСТЬ: %.0f КМ/Ч  •  ПРИЦЕЛ: %s"),
                                          SpeedKph,
                                          bOpticsZoomed ? TEXT("2.5x ZOOM") : TEXT("1.0x СТАНДАРТ"));
    DrawColoredText(FText::FromString(ReticleInfo), Center.X - 100.0f, Center.Y + ReticleSize + 10.0f, FLinearColor::White, 1.0f);

    // 2. Bottom Tactical Action / Ability Bar
    const float BarW = 880.0f;
    const float BarH = 82.0f;
    const FVector2D BarPos((ScreenW - BarW) * 0.5f, ScreenH - BarH - 16.0f);

    // Panel Background
    FCanvasTileItem BarBg(BarPos, FVector2D(BarW, BarH), FLinearColor(0.015f, 0.035f, 0.06f, 0.92f));
    BarBg.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(BarBg);

    // Panel Outer Border
    FCanvasBoxItem BarBorder(BarPos, FVector2D(BarW, BarH));
    BarBorder.SetColor(FLinearColor(0.2f, 0.65f, 0.9f, 0.95f));
    BarBorder.LineThickness = 2.0f;
    Canvas->DrawItem(BarBorder);

    // 4 Action Buttons
    const float ButtonW = (BarW - 40.0f) / 4.0f;
    const float ButtonH = BarH - 16.0f;
    const float ButtonY = BarPos.Y + 8.0f;

    // Button 1: [ ЛКМ ] Основное орудие
    {
        const float BtnX = BarPos.X + 8.0f;
        FCanvasTileItem BtnBg(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH), FLinearColor(0.04f, 0.12f, 0.08f, 0.85f));
        BtnBg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(BtnBg);

        FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
        BtnBox.SetColor(FLinearColor(0.15f, 0.95f, 0.4f, 0.9f));
        Canvas->DrawItem(BtnBox);

        DrawColoredText(FText::FromString(TEXT("[ ЛКМ ] ОСНОВНОЕ ОРУДИЕ")), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(0.2f, 1.0f, 0.45f, 1.0f), 1.0f);
        DrawColoredText(FText::FromString(TEXT("120-мм ТАНКОВОЕ ОРУДИЕ")), BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
        DrawColoredText(FText::FromString(TEXT("ГОТОВО К СТРЕЛЬБЕ")), BtnX + 8.0f, ButtonY + 44.0f, FLinearColor(0.2f, 1.0f, 0.4f, 1.0f), 0.85f);
    }

    // Button 2: [ ПКМ ] Спецспособность
    {
        const float BtnX = BarPos.X + 16.0f + ButtonW;
        FCanvasTileItem BtnBg(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH), FLinearColor(0.03f, 0.08f, 0.14f, 0.85f));
        BtnBg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(BtnBg);

        FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
        BtnBox.SetColor(FLinearColor(0.2f, 0.85f, 1.0f, 0.9f));
        Canvas->DrawItem(BtnBox);

        DrawColoredText(FText::FromString(TEXT("[ ПКМ ] СПЕЦСПОСОБНОСТЬ")), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(0.25f, 0.9f, 1.0f, 1.0f), 1.0f);
        DrawColoredText(FText::FromString(TEXT("РАКЕТНЫЙ ЗАЛП / ФОРСАЖ")), BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
        DrawColoredText(FText::FromString(TEXT("АКТИВАЦИЯ [ГОТОВО]")), BtnX + 8.0f, ButtonY + 44.0f, FLinearColor(0.3f, 1.0f, 0.95f, 1.0f), 0.85f);
    }

    // Button 3: [ Z / СКМ ] Прицел и оптика
    {
        const float BtnX = BarPos.X + 24.0f + ButtonW * 2.0f;
        FCanvasTileItem BtnBg(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH), FLinearColor(0.05f, 0.07f, 0.1f, 0.85f));
        BtnBg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(BtnBg);

        FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
        BtnBox.SetColor(bOpticsZoomed ? FLinearColor(0.2f, 1.0f, 0.5f, 0.9f) : FLinearColor(0.5f, 0.6f, 0.7f, 0.8f));
        Canvas->DrawItem(BtnBox);

        DrawColoredText(FText::FromString(TEXT("[ Z ] ПРИЦЕЛ / ОПТИКА")), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(0.9f, 0.9f, 0.9f, 1.0f), 1.0f);
        DrawColoredText(bOpticsZoomed ? FText::FromString(TEXT("ПРИБЛИЖЕНИЕ 2.5X")) : FText::FromString(TEXT("ШИРОКИЙ ОБЗОР")), BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
        DrawColoredText(FText::FromString(TEXT("ПЕРЕКЛЮЧЕНИЕ [Z / СКМ]")), BtnX + 8.0f, ButtonY + 44.0f, FLinearColor(0.75f, 0.75f, 0.75f, 1.0f), 0.85f);
    }

    // Button 4: [ J / ESC ] Выход в стратегический режим RTS
    {
        const float BtnX = BarPos.X + 32.0f + ButtonW * 3.0f;
        FCanvasTileItem BtnBg(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH), FLinearColor(0.12f, 0.04f, 0.04f, 0.85f));
        BtnBg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(BtnBg);

        FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
        BtnBox.SetColor(FLinearColor(1.0f, 0.35f, 0.35f, 0.9f));
        Canvas->DrawItem(BtnBox);

        DrawColoredText(FText::FromString(TEXT("[ J / ESC ] ВЫХОД [RTS]")), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(1.0f, 0.45f, 0.45f, 1.0f), 1.0f);
        DrawColoredText(FText::FromString(TEXT("СТРАТЕГИЧЕСКИЙ ВИД")), BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
        DrawColoredText(FText::FromString(TEXT("ВОЗВРАТ К БАЗЕ")), BtnX + 8.0f, ButtonY + 44.0f, FLinearColor(0.85f, 0.85f, 0.85f, 1.0f), 0.85f);
    }

    // 3. Vehicle Health & Armor Status Card at Bottom Left
    const float CardW = 250.0f;
    const float CardH = 82.0f;
    const FVector2D CardPos(20.0f, ScreenH - CardH - 16.0f);

    FCanvasTileItem CardBg(CardPos, FVector2D(CardW, CardH), FLinearColor(0.015f, 0.035f, 0.06f, 0.92f));
    CardBg.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(CardBg);

    FCanvasBoxItem CardBorder(CardPos, FVector2D(CardW, CardH));
    CardBorder.SetColor(FLinearColor(0.2f, 0.65f, 0.9f, 0.95f));
    CardBorder.LineThickness = 2.0f;
    Canvas->DrawItem(CardBorder);

    const float HealthRatio = VehMaxHealth > 0 ? FMath::Clamp(float(VehHealth) / float(VehMaxHealth), 0.0f, 1.0f) : 1.0f;
    DrawColoredText(FText::FromString(FString::Printf(TEXT("СОСТОЯНИЕ: %s"), *VehName)), CardPos.X + 8.0f, CardPos.Y + 6.0f, FLinearColor(0.25f, 0.9f, 1.0f, 1.0f), 1.0f);

    // HP Bar Fill
    const float BarFillW = (CardW - 16.0f) * HealthRatio;
    FCanvasTileItem HpBar(FVector2D(CardPos.X + 8.0f, CardPos.Y + 28.0f), FVector2D(BarFillW, 16.0f), HealthRatio > 0.4f ? FLinearColor(0.15f, 0.95f, 0.35f, 1.0f) : FLinearColor(0.95f, 0.2f, 0.2f, 1.0f));
    Canvas->DrawItem(HpBar);

    FCanvasBoxItem HpBarBorder(FVector2D(CardPos.X + 8.0f, CardPos.Y + 28.0f), FVector2D(CardW - 16.0f, 16.0f));
    HpBarBorder.SetColor(FLinearColor::White);
    Canvas->DrawItem(HpBarBorder);

    FString HpText = FString::Printf(TEXT("ПРОЧНОСТЬ: %d / %d HP"), VehHealth, VehMaxHealth);
    DrawColoredText(FText::FromString(HpText), CardPos.X + 8.0f, CardPos.Y + 50.0f, FLinearColor::White, 0.9f);
}
