// Copyright (c) Red Alert 4 project.
#include "RA4SidebarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

#include "RA4HUDViewModel.h"
#include "RA4UIDataProviderSubsystem.h"

#include "RA4Presentation/HudSnapshot.h"

namespace
{
constexpr float kRadarDesiredSize = 208.0f;
}

// The Blueprint-facing minimap enums are a hand-written copy of the presentation ones, so
// the byte the sampler wrote must mean the same thing to the painter. Checked here because
// this is the one translation unit that sees both definitions; if either list is reordered
// or extended, this fails to compile instead of quietly painting water as ore.
static_assert(uint8(ERA4MinimapTerrain::Unknown) == uint8(RA4::Presentation::MinimapTerrain::Unknown), "minimap terrain drift");
static_assert(uint8(ERA4MinimapTerrain::Ground) == uint8(RA4::Presentation::MinimapTerrain::Ground), "minimap terrain drift");
static_assert(uint8(ERA4MinimapTerrain::Water) == uint8(RA4::Presentation::MinimapTerrain::Water), "minimap terrain drift");
static_assert(uint8(ERA4MinimapTerrain::Cliff) == uint8(RA4::Presentation::MinimapTerrain::Cliff), "minimap terrain drift");
static_assert(uint8(ERA4MinimapTerrain::Ore) == uint8(RA4::Presentation::MinimapTerrain::Ore), "minimap terrain drift");
static_assert(uint8(ERA4MinimapTerrain::Structure) == uint8(RA4::Presentation::MinimapTerrain::Structure), "minimap terrain drift");
static_assert(uint8(ERA4MinimapShroud::NeverSeen) == uint8(RA4::Presentation::MinimapShroud::NeverSeen), "minimap shroud drift");
static_assert(uint8(ERA4MinimapShroud::Remembered) == uint8(RA4::Presentation::MinimapShroud::Remembered), "minimap shroud drift");
static_assert(uint8(ERA4MinimapShroud::Visible) == uint8(RA4::Presentation::MinimapShroud::Visible), "minimap shroud drift");
static_assert(uint8(ERA4RadarPingKind::Attack) == uint8(RA4::Presentation::RadarPingKind::Attack), "radar ping drift");
static_assert(uint8(ERA4RadarPingKind::Loss) == uint8(RA4::Presentation::RadarPingKind::Loss), "radar ping drift");
static_assert(uint8(ERA4RadarPingKind::Construction) == uint8(RA4::Presentation::RadarPingKind::Construction), "radar ping drift");

class SRA4RadarSlate final : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRA4RadarSlate) {}
    SLATE_END_ARGS()

    void Construct(const FArguments&, URA4RadarWidget* InOwner)
    {
        Owner = InOwner;
    }

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D(kRadarDesiredSize, kRadarDesiredSize);
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                          const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                          int32 LayerId, const FWidgetStyle& InWidgetStyle,
                          bool bParentEnabled) const override
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        if (Size.X <= 0.0f || Size.Y <= 0.0f)
        {
            return LayerId;
        }

        const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId,
            AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform()),
            WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.018f, 0.028f, 0.040f, 1.0f));

        const FLinearColor GridColour(0.12f, 0.19f, 0.22f, 0.65f);
        for (int32 Division = 1; Division < 4; ++Division)
        {
            const float X = Size.X * float(Division) / 4.0f;
            const float Y = Size.Y * float(Division) / 4.0f;
            TArray<FVector2D> Vertical{FVector2D(X, 0.0f), FVector2D(X, Size.Y)};
            TArray<FVector2D> Horizontal{FVector2D(0.0f, Y), FVector2D(Size.X, Y)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                         Vertical, ESlateDrawEffect::None, GridColour, true, 1.0f);
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                         Horizontal, ESlateDrawEffect::None, GridColour, true, 1.0f);
        }

        const URA4RadarWidget* RadarOwner = Owner.Get();
        if (RadarOwner == nullptr)
        {
            return LayerId + 1;
        }

        // ADR-0013: a deficit that took the radar takes the whole overview with it. Drawn
        // as a distinctly dead panel rather than an empty one, so "no contacts" and "no
        // radar" do not look the same.
        if (!RadarOwner->IsOnline())
        {
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 2,
                AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform()),
                WhiteBrush, ESlateDrawEffect::None, FLinearColor(0.05f, 0.02f, 0.02f, 0.85f));
            return LayerId + 2;
        }

        const FVector2D MapSize = RadarOwner->GetMapSize();
        if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
        {
            return LayerId + 1;
        }

        // Letterbox so a non-square map keeps its shape. Shared with the click handler,
        // so a click always lands where the marker under the cursor was drawn.
        FVector2D MapOffset, MapExtent;
        URA4RadarWidget::ComputeMapRect(Size, MapSize, MapOffset, MapExtent);

        // Terrain and shroud, drawn underneath the markers. Without this the panel was a
        // blank grid: a player could see where their units were but nothing about the
        // ground they were standing on, and no record of what they had explored.
        const FIntPoint Cells = RadarOwner->GetBackgroundCellCounts();
        const TArray<uint8>& Terrain = RadarOwner->GetBackgroundTerrain();
        const TArray<uint8>& Shroud = RadarOwner->GetBackgroundShroud();
        if (Cells.X > 0 && Cells.Y > 0 &&
            Terrain.Num() >= Cells.X * Cells.Y && Shroud.Num() >= Cells.X * Cells.Y)
        {
            // Cell size is rounded up so adjacent cells overlap by less than a pixel rather
            // than leaving a seam of background between every pair of them.
            const FVector2D CellSize(FMath::CeilToFloat(float(MapExtent.X) / float(Cells.X)),
                                     FMath::CeilToFloat(float(MapExtent.Y) / float(Cells.Y)));
            for (int32 CellY = 0; CellY < Cells.Y; ++CellY)
            {
                for (int32 CellX = 0; CellX < Cells.X; ++CellX)
                {
                    const int32 Index = CellY * Cells.X + CellX;
                    const ERA4MinimapShroud CellShroud = ERA4MinimapShroud(Shroud[Index]);
                    if (CellShroud == ERA4MinimapShroud::NeverSeen)
                    {
                        continue;   // unexplored: leave the panel's own dark background
                    }

                    FLinearColor Colour = RA4MinimapTerrainColour(ERA4MinimapTerrain(Terrain[Index]));
                    if (CellShroud == ERA4MinimapShroud::Remembered)
                    {
                        // Dimmed, not greyed: the player must still recognise a river or an
                        // ore patch they scouted earlier, just not mistake it for live.
                        Colour *= 0.45f;
                        Colour.A = 1.0f;
                    }

                    // The simulation's Y grows northward and the panel's grows downward.
                    const FVector2D CellPos(
                        MapOffset.X + float(CellX) * float(MapExtent.X) / float(Cells.X),
                        MapOffset.Y + float(Cells.Y - 1 - CellY) * float(MapExtent.Y) / float(Cells.Y));
                    FSlateDrawElement::MakeBox(
                        OutDrawElements, LayerId + 2,
                        AllottedGeometry.ToPaintGeometry(CellSize, FSlateLayoutTransform(CellPos)),
                        WhiteBrush, ESlateDrawEffect::None, Colour);
                }
            }
        }

        // The camera's view rectangle, so the player can see where they are looking as well
        // as where their forces are. Drawn under the markers: it is a reference frame, and a
        // unit sitting on its edge must stay readable.
        const FVector2D ViewCentre = RadarOwner->GetCameraViewCentre();
        const FVector2D ViewExtent = RadarOwner->GetCameraViewExtent();
        double FrameLeft = 0.0, FrameTop = 0.0, FrameRight = 0.0, FrameBottom = 0.0;
        if (RA4::Presentation::ComputeMinimapCameraFrame(
                MapOffset.X, MapOffset.Y, MapExtent.X, MapExtent.Y, MapSize.X, MapSize.Y,
                ViewCentre.X, ViewCentre.Y, ViewExtent.X, ViewExtent.Y,
                FrameLeft, FrameTop, FrameRight, FrameBottom))
        {
            TArray<FVector2D> Outline{
                FVector2D(FrameLeft, FrameTop),
                FVector2D(FrameRight, FrameTop),
                FVector2D(FrameRight, FrameBottom),
                FVector2D(FrameLeft, FrameBottom),
                FVector2D(FrameLeft, FrameTop)};
            FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3,
                                         AllottedGeometry.ToPaintGeometry(), Outline,
                                         ESlateDrawEffect::None,
                                         FLinearColor(0.90f, 0.90f, 0.95f, 0.75f), true, 1.0f);
        }

        const int32 LocalPlayer = RadarOwner->GetLocalPlayer();
        for (const FRA4RadarMarker& Marker : RadarOwner->GetMarkers())
        {
            const float NormalizedX = FMath::Clamp(float(Marker.WorldPosition.X / MapSize.X), 0.0f, 1.0f);
            const float NormalizedY = FMath::Clamp(float(Marker.WorldPosition.Y / MapSize.Y), 0.0f, 1.0f);
            const FVector2D Centre(MapOffset.X + NormalizedX * MapExtent.X,
                                   MapOffset.Y + (1.0f - NormalizedY) * MapExtent.Y);

            float MarkerExtent = 4.0f;
            FLinearColor Colour;
            if (Marker.Kind == ERA4RadarMarkerKind::Resource)
            {
                MarkerExtent = 3.0f;
                Colour = FLinearColor(0.96f, 0.78f, 0.20f, 1.0f);
            }
            else if (Marker.Owner == LocalPlayer)
            {
                MarkerExtent = Marker.Kind == ERA4RadarMarkerKind::Building ? 6.0f : 4.0f;
                Colour = FLinearColor(0.20f, 0.92f, 0.38f, 1.0f);
            }
            else
            {
                MarkerExtent = Marker.Kind == ERA4RadarMarkerKind::Building ? 6.0f : 4.0f;
                Colour = FLinearColor(0.96f, 0.22f, 0.16f, 1.0f);
            }

            if (Marker.bSelected)
            {
                const FVector2D OutlineSize(MarkerExtent + 4.0f, MarkerExtent + 4.0f);
                FSlateDrawElement::MakeBox(
                    OutDrawElements, LayerId + 4,
                    AllottedGeometry.ToPaintGeometry(
                        OutlineSize, FSlateLayoutTransform(Centre - OutlineSize * 0.5f)),
                    WhiteBrush, ESlateDrawEffect::None, FLinearColor::White);
            }

            const FVector2D MarkerSize(MarkerExtent, MarkerExtent);
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 5,
                AllottedGeometry.ToPaintGeometry(
                    MarkerSize, FSlateLayoutTransform(Centre - MarkerSize * 0.5f)),
                WhiteBrush, ESlateDrawEffect::None, Colour);
        }

        // Pings last, above every marker: they are the "look here now" layer, and an event
        // hidden behind a unit icon would defeat the point.
        for (const FRA4RadarPing& Ping : RadarOwner->GetPings())
        {
            const float Intensity = FMath::Clamp(Ping.Intensity, 0.0f, 1.0f);
            if (Intensity <= 0.0f)
            {
                continue;
            }
            const float NormalizedX = FMath::Clamp(float(Ping.WorldPosition.X / MapSize.X), 0.0f, 1.0f);
            const float NormalizedY = FMath::Clamp(float(Ping.WorldPosition.Y / MapSize.Y), 0.0f, 1.0f);
            const FVector2D Centre(MapOffset.X + NormalizedX * MapExtent.X,
                                   MapOffset.Y + (1.0f - NormalizedY) * MapExtent.Y);

            // Shrinks as it fades, so a new event is unmistakable and an old one recedes
            // instead of sitting there at full size competing with the live markers.
            const float Extent = 6.0f + 10.0f * Intensity;
            FLinearColor Colour = RA4RadarPingColour(Ping.Kind);
            Colour.A = Intensity;

            const FVector2D RingSize(Extent, Extent);
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 6,
                AllottedGeometry.ToPaintGeometry(
                    RingSize, FSlateLayoutTransform(Centre - RingSize * 0.5f)),
                WhiteBrush, ESlateDrawEffect::None, Colour);
        }

        return LayerId + 6;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry,
                                     const FPointerEvent& MouseEvent) override
    {
        const FKey Button = MouseEvent.GetEffectingButton();
        FVector2D Normalized;
        if (!ResolveNormalized(MyGeometry, MouseEvent, Normalized))
        {
            // Inside a letterbox bar, or no map: the player pointed at nothing. Swallowed
            // rather than clamped to the nearest edge, which would fling the camera into a
            // corner the player did not click.
            return FReply::Handled();
        }

        URA4RadarWidget* RadarOwner = Owner.Get();
        if (RadarOwner == nullptr)
        {
            return FReply::Handled();
        }

        if (Button == EKeys::LeftMouseButton)
        {
            RadarOwner->HandleSlateClick(Normalized);
            // Capture so the camera keeps following the pointer after it leaves the panel.
            // Without this a drag that overshoots the edge stops dead, which feels like the
            // widget lost the mouse -- and it had.
            bDraggingCamera = true;
            return FReply::Handled().CaptureMouse(SharedThis(this));
        }
        if (Button == EKeys::RightMouseButton)
        {
            RadarOwner->HandleSlateOrder(Normalized);
            return FReply::Handled();
        }
        return FReply::Handled();
    }

    virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (!bDraggingCamera)
        {
            return FReply::Unhandled();
        }
        FVector2D Normalized;
        URA4RadarWidget* RadarOwner = Owner.Get();
        if (RadarOwner != nullptr && ResolveNormalized(MyGeometry, MouseEvent, Normalized))
        {
            // Every move while held, so the camera tracks the pointer continuously rather
            // than only jumping on press and release.
            RadarOwner->HandleSlateClick(Normalized);
        }
        return FReply::Handled();
    }

    virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
    {
        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDraggingCamera)
        {
            bDraggingCamera = false;
            return FReply::Handled().ReleaseMouseCapture();
        }
        return FReply::Handled();
    }

    // Releasing capture without clearing the flag would leave the panel convinced a drag was
    // still in progress, so the next stray move would yank the camera.
    virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override
    {
        bDraggingCamera = false;
        SLeafWidget::OnMouseCaptureLost(CaptureLostEvent);
    }

private:
    // Panel-local pointer position as a 0..1 fraction of the letterboxed map rect. False if
    // the pointer is outside that rect, which is not the same as outside the widget.
    bool ResolveNormalized(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent,
                           FVector2D& OutNormalized) const
    {
        const URA4RadarWidget* RadarOwner = Owner.Get();
        if (RadarOwner == nullptr)
        {
            return false;
        }
        const FVector2D Size = MyGeometry.GetLocalSize();
        if (Size.X <= 0.0f || Size.Y <= 0.0f)
        {
            return false;
        }

        // Normalize against the letterboxed map rect, not the whole panel, or a click is
        // offset by the size of the bars -- the same mapping the painter uses, so a click
        // lands on the marker that was drawn under the cursor.
        FVector2D MapOffset, MapExtent;
        URA4RadarWidget::ComputeMapRect(Size, RadarOwner->GetMapSize(), MapOffset, MapExtent);
        if (MapExtent.X <= 0.0 || MapExtent.Y <= 0.0)
        {
            return false;
        }

        const FVector2D Inside =
            MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) - MapOffset;
        if (Inside.X < 0.0 || Inside.Y < 0.0 || Inside.X > MapExtent.X || Inside.Y > MapExtent.Y)
        {
            return false;
        }
        OutNormalized = FVector2D(Inside.X / MapExtent.X, Inside.Y / MapExtent.Y);
        return true;
    }

    TWeakObjectPtr<URA4RadarWidget> Owner;
    bool bDraggingCamera = false;
};

namespace
{
// The sidebar is a fixed-width column, as in the originals: it does not reflow with
// content, it stays the same slice of screen so the cards keep their positions. It is
// however scaled by viewport height -- see ComputeSidebarScale -- so the column is not
// a third of a small window and a sliver of a 4K one.
constexpr float kSidebarWidth = URA4SidebarWidget::SidebarWidth;
constexpr float kMinimapHeight = kRadarDesiredSize;
constexpr int32 kCardColumns = 2;

// The height the reference layout was designed against, and the band the scale may move
// within. Past that the column stops growing: a legible sidebar is the goal, not a
// proportionally enormous one.
constexpr float kReferenceViewportHeight = 1080.0f;
constexpr float kMinSidebarScale = 0.82f;
constexpr float kMaxSidebarScale = 1.45f;

// How fast a card's hover swell eases, in progress per second. Fast enough to feel
// attached to the pointer, slow enough to read as motion rather than a state flip.
constexpr float kCardHoverSpeed = 9.0f;
// How much a hovered card grows. Small on purpose: the grid must not shift under the
// pointer, or the click lands on a neighbour.
constexpr float kCardHoverScale = 0.045f;

const FLinearColor kPanel(0.055f, 0.065f, 0.080f, 0.94f);
const FLinearColor kPanelDeep(0.030f, 0.036f, 0.046f, 0.96f);
const FLinearColor kTabIdle(0.10f, 0.12f, 0.15f, 1.0f);
const FLinearColor kTabActive(0.20f, 0.34f, 0.24f, 1.0f);
const FLinearColor kCardOk(0.13f, 0.17f, 0.14f, 1.0f);
const FLinearColor kCardBlocked(0.13f, 0.10f, 0.10f, 1.0f);
const FLinearColor kTextNormal(0.86f, 0.89f, 0.93f);
const FLinearColor kTextDim(0.50f, 0.55f, 0.61f);
const FLinearColor kTextFaint(0.34f, 0.38f, 0.44f);
const FLinearColor kCredits(0.94f, 0.80f, 0.32f);
const FLinearColor kPowerOk(0.42f, 0.82f, 0.48f);
const FLinearColor kPowerLow(0.94f, 0.36f, 0.28f);
const FLinearColor kPowerTight(0.94f, 0.74f, 0.30f);
const FLinearColor kBarTrack(0.09f, 0.11f, 0.13f, 1.0f);
const FLinearColor kQueueWaiting(0.30f, 0.44f, 0.58f);

// Keys that commit build cards, in grid order. Deliberately not the digits: those are
// control groups, and the ordinary RTS reflex of pressing a number to recall a squad has
// to keep working. Every entry here is bound for real by ARA4PlayerController, which
// asserts the two tables are the same length, so a badge cannot promise a dead key.
//
// H is deliberately absent: it is HoldPosition in RA4::Input::KeyBindingTable, and
// while this table also claimed it a single press ran the hold order and committed a
// build card. L takes the tenth slot instead.
const TCHAR* const kCardHotkeys[] = {
    TEXT("Q"), TEXT("E"), TEXT("R"), TEXT("T"),
    TEXT("Y"), TEXT("U"), TEXT("I"), TEXT("O"),
    TEXT("P"), TEXT("L"), TEXT("J"), TEXT("K"),
};

// Power is plotted as consumption against production rather than over a fixed range:
// what the player needs to see is how close the draw is to the ceiling.
float PowerFillRatio(int32 Produced, int32 Consumed)
{
    if (Produced <= 0)
    {
        // No plants at all: full bar if anything is drawing, empty if nothing is.
        return Consumed > 0 ? 1.0f : 0.0f;
    }
    return FMath::Clamp(float(Consumed) / float(Produced), 0.0f, 1.0f);
}

// Mirrors ProductionCategory. Naval and Ability are omitted until the content has
// entries for them -- an empty tab is worse than no tab.
struct TabDef
{
    int32 Category;
    const TCHAR* Caption;
};
const TabDef kTabs[] = {
    {0, TEXT("STR")},   // Structure
    {1, TEXT("DEF")},   // Defense
    {2, TEXT("INF")},   // Infantry
    {3, TEXT("VEH")},   // Vehicle
    {4, TEXT("AIR")},   // Aircraft
};

UTextBlock* MakeLabel(UWidgetTree* Tree, FName Name, const FLinearColor& Colour, int32 Size, bool bBold)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = Size;
    Font.TypefaceFontName = bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular"));
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(Colour));
    return Text;
}

// Thin bars for power and production. Centralised so every one of them gets the same
// dark track: a progress bar on the engine default washes out against the panel.
UProgressBar* MakeThinBar(UWidgetTree* Tree, FName Name, const FLinearColor& Fill, float Height)
{
    UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
    FProgressBarStyle Style = Bar->GetWidgetStyle();
    Style.BackgroundImage.TintColor = FSlateColor(kBarTrack);
    Style.BackgroundImage.ImageSize = FVector2D(1.0f, Height);
    Style.FillImage.ImageSize = FVector2D(1.0f, Height);
    Bar->SetWidgetStyle(Style);
    Bar->SetFillColorAndOpacity(Fill);
    Bar->SetPercent(0.0f);
    return Bar;
}

// A spacer used to keep vertical rhythm where a bar is deliberately absent.
USpacer* MakeGap(UWidgetTree* Tree, FName Name, float Height)
{
    USpacer* Gap = Tree->ConstructWidget<USpacer>(USpacer::StaticClass(), Name);
    Gap->SetSize(FVector2D(1.0f, Height));
    return Gap;
}

FText SelectionKindCaption(ERA4SelectionKind Kind)
{
    switch (Kind)
    {
    case ERA4SelectionKind::SingleUnit:
        return NSLOCTEXT("RA4", "SelKind_Unit", "UNIT");
    case ERA4SelectionKind::SingleBuilding:
        return NSLOCTEXT("RA4", "SelKind_Building", "STRUCTURE");
    case ERA4SelectionKind::MultipleUnits:
        return NSLOCTEXT("RA4", "SelKind_Group", "GROUP");
    case ERA4SelectionKind::Mixed:
        return NSLOCTEXT("RA4", "SelKind_Mixed", "MIXED");
    default:
        return NSLOCTEXT("RA4", "SelKind_Empty", "OBJECT INFO");
    }
}

// mm:ss for anything a minute or longer, bare seconds below that -- which is where
// nearly every build time lands.
FText FormatBuildRemaining(float Seconds)
{
    const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
    if (Total < 60)
    {
        return FText::Format(NSLOCTEXT("RA4", "Queue_SecondsFormat", "{0}s"), FText::AsNumber(Total));
    }

    FNumberFormattingOptions TwoDigits;
    TwoDigits.MinimumIntegralDigits = 2;
    return FText::Format(NSLOCTEXT("RA4", "Queue_ClockFormat", "{0}:{1}"),
                         FText::AsNumber(Total / 60),
                         FText::AsNumber(Total % 60, &TwoDigits));
}

void StyleButton(UButton* Button, const FLinearColor& Base)
{
    FButtonStyle Style = Button->GetStyle();
    Style.Normal.TintColor = FSlateColor(Base);
    Style.Hovered.TintColor = FSlateColor(Base * 1.35f);
    Style.Pressed.TintColor = FSlateColor(Base * 0.75f);
    Style.Disabled.TintColor = FSlateColor(Base * 0.55f);
    Button->SetStyle(Style);
}

FText BlockReasonText(ERA4BuildBlockReason Reason)
{
    switch (Reason)
    {
    case ERA4BuildBlockReason::InsufficientCredits:
        return NSLOCTEXT("RA4", "Block_Credits", "Low Funds");
    case ERA4BuildBlockReason::MissingPrerequisite:
        return NSLOCTEXT("RA4", "Block_Prereq", "Prerequisite Missing");
    case ERA4BuildBlockReason::NoProducer:
        return NSLOCTEXT("RA4", "Block_Producer", "Factory Missing");
    case ERA4BuildBlockReason::QueueFull:
        return NSLOCTEXT("RA4", "Block_Queue", "Queue Full");
    case ERA4BuildBlockReason::MatchOver:
        return NSLOCTEXT("RA4", "Block_Over", "Match Ended");
    default:
        return FText::GetEmpty();
    }
}
} // namespace

// ---------------------------------------------------------------------------
// URA4RadarWidget
// ---------------------------------------------------------------------------

TSharedRef<SWidget> URA4RadarWidget::RebuildWidget()
{
    RadarSlate = SNew(SRA4RadarSlate, this);
    return RadarSlate.ToSharedRef();
}

void URA4RadarWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    RadarSlate.Reset();
}

URA4UIDataProviderSubsystem* URA4RadarWidget::GetProvider() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4UIDataProviderSubsystem>() : nullptr;
}

const TArray<FRA4RadarMarker>& URA4RadarWidget::GetMarkers() const
{
    static const TArray<FRA4RadarMarker> Empty;
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetRadarMarkers() : Empty;
}

FVector2D URA4RadarWidget::GetMapSize() const
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetRadarMapSize() : FVector2D::ZeroVector;
}

void URA4RadarWidget::ComputeMapRect(const FVector2D& PanelSize, const FVector2D& MapSize,
                                     FVector2D& OutOffset, FVector2D& OutSize)
{
    // Delegates to the presentation layer so there is one implementation of the mapping
    // rather than one here and one in a test. The geometry is pure and headless-testable;
    // this wrapper exists only to speak FVector2D.
    double OffX = 0.0, OffY = 0.0, W = 0.0, H = 0.0;
    RA4::Presentation::ComputeMinimapRect(PanelSize.X, PanelSize.Y, MapSize.X, MapSize.Y,
                                         OffX, OffY, W, H);
    OutOffset = FVector2D(OffX, OffY);
    OutSize = FVector2D(W, H);
}

int32 URA4RadarWidget::GetLocalPlayer() const
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetRadarLocalPlayer() : 0;
}

bool URA4RadarWidget::IsOnline() const
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    // No provider means no match is running, so there is nothing to report as offline --
    // an editor preview must not render itself as a blacked-out radar.
    return Provider == nullptr || Provider->IsRadarOnline();
}

const TArray<uint8>& URA4RadarWidget::GetBackgroundTerrain() const
{
    static const TArray<uint8> Empty;
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetMinimapTerrain() : Empty;
}

const TArray<uint8>& URA4RadarWidget::GetBackgroundShroud() const
{
    static const TArray<uint8> Empty;
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetMinimapShroud() : Empty;
}

const TArray<FRA4RadarPing>& URA4RadarWidget::GetPings() const
{
    static const TArray<FRA4RadarPing> Empty;
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetRadarPings() : Empty;
}

FIntPoint URA4RadarWidget::GetBackgroundCellCounts() const
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    return Provider != nullptr ? Provider->GetMinimapCellCounts() : FIntPoint::ZeroValue;
}

void URA4RadarWidget::HandleSlateClick(const FVector2D& NormalizedPosition)
{
    const FVector2D MapSize = GetMapSize();
    if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
    {
        return;
    }

    OnRadarClicked.Broadcast(FVector2D(
        NormalizedPosition.X * MapSize.X,
        (1.0f - NormalizedPosition.Y) * MapSize.Y));
}

void URA4RadarWidget::HandleSlateOrder(const FVector2D& NormalizedPosition)
{
    const FVector2D MapSize = GetMapSize();
    if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f)
    {
        return;
    }

    // Same mapping as the camera click; only the delegate differs, so an order cannot land
    // somewhere other than where the camera would have gone for the same pixel.
    OnRadarOrdered.Broadcast(FVector2D(
        NormalizedPosition.X * MapSize.X,
        (1.0f - NormalizedPosition.Y) * MapSize.Y));
}

void URA4RadarWidget::SetCameraView(const FVector2D& CentreWorld, const FVector2D& ExtentWorld)
{
    if (CameraViewCentre.Equals(CentreWorld) && CameraViewExtent.Equals(ExtentWorld))
    {
        return;   // no change: do not invalidate Slate for an identical frame
    }
    CameraViewCentre = CentreWorld;
    CameraViewExtent = ExtentWorld;
    if (RadarSlate.IsValid())
    {
        RadarSlate->Invalidate(EInvalidateWidgetReason::Paint);
    }
}

// ---------------------------------------------------------------------------
// URA4IndexedButton
// ---------------------------------------------------------------------------

void URA4IndexedButton::BindForwarding()
{
    OnClicked.AddDynamic(this, &URA4IndexedButton::HandleClicked);
}

void URA4IndexedButton::HandleClicked()
{
    OnIndexedClicked.Broadcast(Index);
}

// ---------------------------------------------------------------------------
// URA4SidebarWidget -- layout metrics
// ---------------------------------------------------------------------------

float URA4SidebarWidget::ComputeSidebarScale(const UObject* WorldContextObject)
{
    // A viewport reports 0x0 for the first frames of a standalone launch. Returning the
    // reference scale keeps the column at its designed width until a real size arrives,
    // rather than collapsing it to nothing on frame one.
    if (WorldContextObject == nullptr)
    {
        return 1.0f;
    }

    const UWorld* World = WorldContextObject->GetWorld();
    if (World == nullptr || World->GetGameViewport() == nullptr)
    {
        return 1.0f;
    }

    FVector2D ViewportSize = FVector2D::ZeroVector;
    World->GetGameViewport()->GetViewportSize(ViewportSize);
    if (ViewportSize.Y <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }

    // Divide out the engine's DPI curve (Config/DefaultUserInterface.ini) first. Slate
    // has already multiplied every widget by it, so scaling by raw pixel height on top
    // would compound the two and overshoot badly on a high-DPI display.
    const float DPIScale = World->GetGameViewport()->GetDPIScale();
    const float LogicalHeight = DPIScale > KINDA_SMALL_NUMBER
                                    ? float(ViewportSize.Y) / DPIScale
                                    : float(ViewportSize.Y);

    return FMath::Clamp(LogicalHeight / kReferenceViewportHeight, kMinSidebarScale, kMaxSidebarScale);
}

float URA4SidebarWidget::ComputeSidebarWidth(const UObject* WorldContextObject)
{
    return kSidebarWidth * ComputeSidebarScale(WorldContextObject);
}

int32 URA4SidebarWidget::GetCardHotkeyCount()
{
    return int32(UE_ARRAY_COUNT(kCardHotkeys));
}

const TCHAR* URA4SidebarWidget::GetCardHotkeyLabel(int32 CardIndex)
{
    if (CardIndex < 0 || CardIndex >= GetCardHotkeyCount())
    {
        return nullptr;
    }
    return kCardHotkeys[CardIndex];
}

// ---------------------------------------------------------------------------
// URA4SidebarWidget
// ---------------------------------------------------------------------------

TSharedRef<SWidget> URA4SidebarWidget::RebuildWidget()
{
    if (WidgetTree == nullptr || WidgetTree->RootWidget != nullptr)
    {
        return Super::RebuildWidget();
    }

    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));

    auto AddRow = [&](UWidget* Child, float Padding) -> UVerticalBoxSlot*
    {
        UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Child);
        if (Slot != nullptr)
        {
            Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Padding));
            Slot->SetHorizontalAlignment(HAlign_Fill);
        }
        return Slot;
    };

    // --- minimap ------------------------------------------------------------
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(6.0f));

        RadarWidget = WidgetTree->ConstructWidget<URA4RadarWidget>(
            URA4RadarWidget::StaticClass(), TEXT("Radar"));
        RadarWidget->OnRadarClicked.AddUObject(this, &URA4SidebarWidget::HandleRadarClicked);
        RadarWidget->OnRadarOrdered.AddUObject(this, &URA4SidebarWidget::HandleRadarOrdered);
        Frame->AddChild(RadarWidget);

        USizeBox* Sizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinimapSizer"));
        Sizer->SetHeightOverride(kMinimapHeight);
        Sizer->AddChild(Frame);
        AddRow(Sizer, 6.0f);
    }

    // --- credits and power --------------------------------------------------
    // Denser than a stack of plain lines: the credit figure gets a label so it is not
    // read as a unit count, power gets both the raw pair and the headroom that actually
    // decides whether the next structure runs, and the bar makes that margin visible
    // without having to subtract two numbers under pressure.
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResourceFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(8.0f, 6.0f));

        UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResStack"));

        // Label left, figure pushed to the right edge so the digits stay in one column
        // as the number changes width.
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
                                                                              TEXT("CreditsRow"));

            UTextBlock* Mark = MakeLabel(WidgetTree, TEXT("CreditsMark"), kTextFaint, 9, true);
            Mark->SetText(NSLOCTEXT("RA4", "Sidebar_CreditsLabel", "CREDITS"));
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Mark))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(
                    MakeGap(WidgetTree, TEXT("CreditsGap"), 1.0f)))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }

            CreditsText = MakeLabel(WidgetTree, TEXT("SidebarCredits"), kCredits, 18, true);
            CreditsText->SetText(FText::AsNumber(0));
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(CreditsText))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            Stack->AddChildToVerticalBox(Row);
        }

        // Power: produced / consumed on the left, surplus or deficit on the right.
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
                                                                              TEXT("PowerRow"));

            PowerText = MakeLabel(WidgetTree, TEXT("SidebarPower"), kPowerOk, 11, false);
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(PowerText))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(
                    MakeGap(WidgetTree, TEXT("PowerGap"), 1.0f)))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }

            PowerSurplusText = MakeLabel(WidgetTree, TEXT("SidebarPowerSurplus"), kTextDim, 9, true);
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(PowerSurplusText))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Row))
            {
                Slot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
            }
        }

        PowerRatioBar = MakeThinBar(WidgetTree, TEXT("PowerRatioBar"), kPowerOk, 4.0f);
        if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(PowerRatioBar))
        {
            Slot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
        }

        // Constructed but hidden while the simulation has no population cap. HudSnapshot
        // is explicit that a fabricated limit must not be shown, so RefreshResources
        // keeps this collapsed rather than inventing a denominator.
        SupplyText = MakeLabel(WidgetTree, TEXT("SidebarSupply"), kTextDim, 9, false);
        SupplyText->SetText(FText::GetEmpty());
        SupplyText->SetVisibility(ESlateVisibility::Collapsed);
        if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(SupplyText))
        {
            Slot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
        }

        Frame->AddChild(Stack);
        AddRow(Frame, 6.0f);
    }

    // --- selected object info card ("OBJECT INFO") -------------------
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SelectionFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(8.0f, 6.0f));

        UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SelStack"));

        // Header: what kind of thing is selected, and how many. The count badge appears
        // only for a real group, so a single unit is never labelled "x1".
        {
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
                                                                              TEXT("SelHeaderRow"));

            SelectionKindText = MakeLabel(WidgetTree, TEXT("SelHeader"), kTextDim, 9, true);
            SelectionKindText->SetText(SelectionKindCaption(ERA4SelectionKind::Empty));
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(SelectionKindText))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(
                    MakeGap(WidgetTree, TEXT("SelHeaderGap"), 1.0f)))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }

            SelectionCountText = MakeLabel(WidgetTree, TEXT("SelCount"), kCredits, 10, true);
            SelectionCountText->SetText(FText::GetEmpty());
            SelectionCountText->SetVisibility(ESlateVisibility::Collapsed);
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(SelectionCountText))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            Stack->AddChildToVerticalBox(Row);
        }

        SelectionNameText = MakeLabel(WidgetTree, TEXT("SelName"), kTextNormal, 12, true);
        SelectionNameText->SetText(NSLOCTEXT("RA4", "Sidebar_NoSelection", "NO SELECTION"));
        SelectionNameText->SetAutoWrapText(true);
        Stack->AddChildToVerticalBox(SelectionNameText);

        SelectionHealthBar = MakeThinBar(WidgetTree, TEXT("SelHealthBar"), kPowerOk, 5.0f);
        if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(SelectionHealthBar))
        {
            Slot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 2.0f));
        }

        SelectionHealthText = MakeLabel(WidgetTree, TEXT("SelHealthText"), kTextDim, 10, false);
        SelectionHealthText->SetText(FText::GetEmpty());
        Stack->AddChildToVerticalBox(SelectionHealthText);

        SelectionDetailsText = MakeLabel(WidgetTree, TEXT("SelDetails"), kTextDim, 9, false);
        SelectionDetailsText->SetText(NSLOCTEXT("RA4", "Sidebar_SelectionHint", "Select a unit or structure"));
        SelectionDetailsText->SetAutoWrapText(true);
        Stack->AddChildToVerticalBox(SelectionDetailsText);

        // One row per unit type when several are selected, so a mixed group is readable
        // without clicking through it.
        SelectionGroupBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
                                                                      TEXT("SelGroupBox"));
        SelectionGroupBox->SetVisibility(ESlateVisibility::Collapsed);
        if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(SelectionGroupBox))
        {
            Slot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }

        Frame->AddChild(Stack);
        AddRow(Frame, 6.0f);
    }

    // --- category tabs ------------------------------------------------------
    {
        UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),
                                                                             TEXT("TabRow"));
        TabButtons.Reset();
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(kTabs); ++Index)
        {
            URA4IndexedButton* Button = WidgetTree->ConstructWidget<URA4IndexedButton>(
                URA4IndexedButton::StaticClass(), *FString::Printf(TEXT("Tab%d"), Index));
            Button->SetIndex(Index);
            Button->BindForwarding();
            Button->OnIndexedClicked.AddUObject(this, &URA4SidebarWidget::HandleTabClicked);
            StyleButton(Button, kTabs[Index].Category == ActiveCategory ? kTabActive : kTabIdle);

            UTextBlock* Caption = MakeLabel(WidgetTree, *FString::Printf(TEXT("TabText%d"), Index), kTextNormal, 10,
                                            true);
            Caption->SetText(FText::FromString(kTabs[Index].Caption));
            Caption->SetJustification(ETextJustify::Center);
            Button->AddChild(Caption);

            if (UHorizontalBoxSlot* Slot = TabRow->AddChildToHorizontalBox(Button))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                Slot->SetPadding(FMargin(0.0f, 0.0f, 2.0f, 0.0f));
            }
            TabButtons.Add(Button);
        }
        AddRow(TabRow, 6.0f);
    }

    // --- build cards --------------------------------------------------------
    {
        CardGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("CardGrid"));
        CardGrid->SetSlotPadding(FMargin(2.0f));
        UVerticalBoxSlot* Slot = AddRow(CardGrid, 6.0f);
        if (Slot != nullptr)
        {
            // The card grid takes the leftover height so the queue stays pinned to the
            // bottom of the column instead of floating under a short list.
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    // --- production queue ---------------------------------------------------
    {
        UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QueueFrame"));
        Frame->SetBrushColor(kPanelDeep);
        Frame->SetPadding(FMargin(6.0f));

        UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),
                                                                        TEXT("QueueStack"));

        QueueHeader = MakeLabel(WidgetTree, TEXT("QueueHeader"), kTextFaint, 9, true);
        QueueHeader->SetText(NSLOCTEXT("RA4", "Sidebar_QueueHeader", "PRODUCTION"));
        if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(QueueHeader))
        {
            Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
        }

        QueueBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QueueBox"));
        Stack->AddChildToVerticalBox(QueueBox);

        Frame->AddChild(Stack);
        AddRow(Frame, 0.0f);
    }

    UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SidebarBackground"));
    Background->SetBrushColor(kPanel);
    Background->SetPadding(FMargin(8.0f));
    Background->AddChild(Column);

    WidthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SidebarWidth"));
    AppliedSidebarWidth = ComputeSidebarWidth(this);
    WidthBox->SetWidthOverride(AppliedSidebarWidth);
    WidthBox->AddChild(Background);

    WidgetTree->RootWidget = WidthBox;
    return Super::RebuildWidget();
}

void URA4SidebarWidget::HandleRadarClicked(FVector2D WorldPosition)
{
    OnRadarClicked.Broadcast(WorldPosition);
}

void URA4SidebarWidget::HandleRadarOrdered(FVector2D WorldPosition)
{
    OnRadarOrdered.Broadcast(WorldPosition);
}

void URA4SidebarWidget::SetRadarCameraView(const FVector2D& CentreWorld, const FVector2D& ExtentWorld)
{
    if (RadarWidget != nullptr)
    {
        RadarWidget->SetCameraView(CentreWorld, ExtentWorld);
    }
}

void URA4SidebarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        ResourceChangeHandle = Provider->OnResourcesChanged.AddUObject(this, &URA4SidebarWidget::RefreshResources);
        ProductionChangeHandle = Provider->OnProductionChanged.AddUObject(this, &URA4SidebarWidget::RefreshCards);
        SelectionChangeHandle = Provider->OnSelectionChanged.AddUObject(this, &URA4SidebarWidget::RefreshSelection);
        RefreshResources();
        RefreshCards();
        RefreshSelection();
    }
}

void URA4SidebarWidget::NativeDestruct()
{
    if (URA4UIDataProviderSubsystem* Provider = GetProvider())
    {
        Provider->OnResourcesChanged.Remove(ResourceChangeHandle);
        Provider->OnProductionChanged.Remove(ProductionChangeHandle);
        Provider->OnSelectionChanged.Remove(SelectionChangeHandle);
    }
    ResourceChangeHandle.Reset();
    ProductionChangeHandle.Reset();
    SelectionChangeHandle.Reset();
    Super::NativeDestruct();
}

void URA4SidebarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Follow a resized window. Cheap enough to check every frame -- one float compare --
    // and the alternative is a viewport-resize delegate whose lifetime has to be managed
    // for a value that is read here anyway.
    if (WidthBox != nullptr)
    {
        const float DesiredWidth = ComputeSidebarWidth(this);
        if (!FMath::IsNearlyEqual(DesiredWidth, AppliedSidebarWidth, 0.5f))
        {
            AppliedSidebarWidth = DesiredWidth;
            WidthBox->SetWidthOverride(DesiredWidth);
        }
    }

    // Ease each card's hover swell towards its target. Driven from the tick rather than
    // from hover events so an interrupted transition continues from where it was instead
    // of jumping, and so a card whose pointer left during a rebuild settles back to rest
    // on its own.
    const int32 CardCount = FMath::Min(CardButtons.Num(), CardHoverProgress.Num());
    const float Step = FMath::Clamp(InDeltaTime * kCardHoverSpeed, 0.0f, 1.0f);
    for (int32 Index = 0; Index < CardCount; ++Index)
    {
        const URA4IndexedButton* Button = CardButtons[Index];
        if (Button == nullptr)
        {
            continue;
        }

        // A blocked card must not swell: it would advertise an interaction that is going
        // to be refused.
        const bool bHovered = Button->IsHovered() && Button->GetIsEnabled();
        const float Target = bHovered ? 1.0f : 0.0f;
        const float Current = CardHoverProgress[Index];
        if (FMath::IsNearlyEqual(Current, Target, 0.001f))
        {
            continue;
        }

        const float Next = FMath::Lerp(Current, Target, Step);
        CardHoverProgress[Index] = Next;

        if (CardHoverTargets.IsValidIndex(Index) && CardHoverTargets[Index] != nullptr)
        {
            FWidgetTransform Transform;
            Transform.Scale = FVector2D(1.0f + Next * kCardHoverScale);
            CardHoverTargets[Index]->SetRenderTransform(Transform);
        }
    }
}

void URA4SidebarWidget::RefreshSelection()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr || SelectionNameText == nullptr)
    {
        return;
    }

    const URA4HUDViewModel* VM = Provider->GetHUDViewModel();
    const int32 Count = VM != nullptr ? VM->GetSelectionCount() : 0;

    if (SelectionKindText != nullptr)
    {
        SelectionKindText->SetText(SelectionKindCaption(Provider->GetSelectionKind()));
    }

    if (Count == 0)
    {
        SelectionNameText->SetText(NSLOCTEXT("RA4", "Sidebar_NoSelection", "NO SELECTION"));
        SelectionNameText->SetColorAndOpacity(FSlateColor(kTextDim));
        if (SelectionCountText != nullptr)
        {
            SelectionCountText->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (SelectionHealthText != nullptr)
        {
            SelectionHealthText->SetText(FText::GetEmpty());
        }
        if (SelectionHealthBar != nullptr)
        {
            SelectionHealthBar->SetPercent(0.0f);
            // Hidden rather than collapsed: the card keeps its height, so the tabs and
            // the card grid below do not jump every time the selection is cleared.
            SelectionHealthBar->SetVisibility(ESlateVisibility::Hidden);
        }
        if (SelectionDetailsText != nullptr)
        {
            SelectionDetailsText->SetText(
                NSLOCTEXT("RA4", "Sidebar_SelectionHint", "Select a unit or structure"));
            SelectionDetailsText->SetColorAndOpacity(FSlateColor(kTextDim));
        }
        if (SelectionGroupBox != nullptr)
        {
            SelectionGroupBox->ClearChildren();
            SelectionGroupBox->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    const bool bOwned = VM->IsPrimaryOwned();

    SelectionNameText->SetText(FText::FromString(VM->GetPrimaryEntityName()));
    // An enemy selection is tinted so it cannot be mistaken for something the player is
    // about to give orders to.
    SelectionNameText->SetColorAndOpacity(FSlateColor(bOwned ? kTextNormal : kPowerLow));

    if (SelectionCountText != nullptr)
    {
        if (Count > 1)
        {
            SelectionCountText->SetText(
                FText::Format(NSLOCTEXT("RA4", "Sidebar_CountBadge", "x{0}"), FText::AsNumber(Count)));
            SelectionCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            SelectionCountText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    const float HP = VM->GetSelectionHealthRatio();
    if (SelectionHealthBar != nullptr)
    {
        SelectionHealthBar->SetVisibility(ESlateVisibility::HitTestInvisible);
        SelectionHealthBar->SetPercent(HP);
        SelectionHealthBar->SetFillColorAndOpacity(HP > 0.5f ? kPowerOk : (HP > 0.2f ? kCredits : kPowerLow));
    }
    if (SelectionHealthText != nullptr)
    {
        // Rounded down, so a unit one point from death never reads as a healthy 100%.
        SelectionHealthText->SetText(
            FText::Format(NSLOCTEXT("RA4", "Sidebar_HPFormat", "HEALTH: {0}%"),
                          FText::AsNumber(FMath::FloorToInt(FMath::Clamp(HP, 0.0f, 1.0f) * 100.0f))));
    }

    // --- group breakdown ----------------------------------------------------
    // The snapshot groups a multi-selection by type, which is what the reference HUD
    // shows. Group rows carry a content id rather than a name, so the name is resolved
    // through the build options that describe the same content.
    const TArray<FRA4SelectionGroup>& Groups = Provider->GetSelectionGroups();
    int32 RowsShown = 0;
    if (SelectionGroupBox != nullptr && WidgetTree != nullptr)
    {
        SelectionGroupBox->ClearChildren();

        if (Count > 1 && Groups.Num() > 0)
        {
            const TArray<FRA4BuildOption>& AllOptions = Provider->GetBuildOptions();

            // At most four rows: past that the card grid starts getting pushed around,
            // and the remainder is summarised on the line underneath instead.
            constexpr int32 kMaxGroupRows = 4;
            for (const FRA4SelectionGroup& Group : Groups)
            {
                if (RowsShown >= kMaxGroupRows)
                {
                    break;
                }

                // The group's own name when the provider filled one in, else the build
                // option for the same content, else a neutral fallback. Never blank: an
                // empty row would read as a rendering fault.
                FText GroupName = Group.DisplayName;
                if (GroupName.IsEmpty())
                {
                    for (const FRA4BuildOption& Option : AllOptions)
                    {
                        if (Option.ContentId == Group.ContentId)
                        {
                            GroupName = Option.DisplayName;
                            break;
                        }
                    }
                }
                if (GroupName.IsEmpty())
                {
                    GroupName = NSLOCTEXT("RA4", "Sidebar_GroupUnknown", "Unit");
                }

                UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
                    UHorizontalBox::StaticClass(), *FString::Printf(TEXT("SelGroupRow%d"), RowsShown));

                UTextBlock* CountLabel = MakeLabel(
                    WidgetTree, *FString::Printf(TEXT("SelGroupCount%d"), RowsShown), kCredits, 9, true);
                CountLabel->SetText(
                    FText::Format(NSLOCTEXT("RA4", "Sidebar_GroupCount", "{0}x"), FText::AsNumber(Group.Count)));
                if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(CountLabel))
                {
                    Slot->SetPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
                    Slot->SetVerticalAlignment(VAlign_Center);
                }

                UTextBlock* NameLabel = MakeLabel(
                    WidgetTree, *FString::Printf(TEXT("SelGroupName%d"), RowsShown), kTextDim, 9, false);
                NameLabel->SetText(GroupName);
                if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(NameLabel))
                {
                    Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                    Slot->SetVerticalAlignment(VAlign_Center);
                }

                // Per-type condition, so a mauled squad inside an otherwise healthy group
                // is visible without clicking through the selection.
                UTextBlock* HealthLabel = MakeLabel(
                    WidgetTree, *FString::Printf(TEXT("SelGroupHP%d"), RowsShown),
                    Group.HealthRatio > 0.5f ? kPowerOk : (Group.HealthRatio > 0.2f ? kPowerTight : kPowerLow),
                    9, false);
                HealthLabel->SetText(FText::Format(
                    NSLOCTEXT("RA4", "Sidebar_GroupHP", "{0}%"),
                    FText::AsNumber(FMath::FloorToInt(FMath::Clamp(Group.HealthRatio, 0.0f, 1.0f) * 100.0f))));
                if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(HealthLabel))
                {
                    Slot->SetVerticalAlignment(VAlign_Center);
                }

                if (UVerticalBoxSlot* Slot = SelectionGroupBox->AddChildToVerticalBox(Row))
                {
                    Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 1.0f));
                }
                ++RowsShown;
            }
        }

        SelectionGroupBox->SetVisibility(RowsShown > 0 ? ESlateVisibility::HitTestInvisible
                                                       : ESlateVisibility::Collapsed);
    }

    if (SelectionDetailsText != nullptr)
    {
        if (!bOwned)
        {
            SelectionDetailsText->SetText(NSLOCTEXT("RA4", "Sidebar_Enemy", "Enemy — cannot be ordered"));
            SelectionDetailsText->SetColorAndOpacity(FSlateColor(kPowerLow));
        }
        else if (RowsShown > 0 && Groups.Num() > RowsShown)
        {
            // Say what was left out rather than silently truncating the list.
            SelectionDetailsText->SetText(
                FText::Format(NSLOCTEXT("RA4", "Sidebar_GroupOverflow", "+{0} more types"),
                              FText::AsNumber(Groups.Num() - RowsShown)));
            SelectionDetailsText->SetColorAndOpacity(FSlateColor(kTextFaint));
        }
        else if (Count > 1)
        {
            SelectionDetailsText->SetText(
                FText::Format(NSLOCTEXT("RA4", "Sidebar_MultiSelFormat", "{0} selected"), FText::AsNumber(Count)));
            SelectionDetailsText->SetColorAndOpacity(FSlateColor(kTextDim));
        }
        else
        {
            SelectionDetailsText->SetText(NSLOCTEXT("RA4", "Sidebar_Owned", "Under your command"));
            SelectionDetailsText->SetColorAndOpacity(FSlateColor(kTextDim));
        }
    }
}

URA4UIDataProviderSubsystem* URA4SidebarWidget::GetProvider() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4UIDataProviderSubsystem>() : nullptr;
}

void URA4SidebarWidget::SetActiveCategory(int32 Category)
{
    if (ActiveCategory == Category)
    {
        return;
    }
    ActiveCategory = Category;

    for (int32 Index = 0; Index < TabButtons.Num() && Index < UE_ARRAY_COUNT(kTabs); ++Index)
    {
        if (TabButtons[Index] != nullptr)
        {
            StyleButton(TabButtons[Index], kTabs[Index].Category == ActiveCategory ? kTabActive : kTabIdle);
        }
    }
    RefreshCards();
}

void URA4SidebarWidget::HandleTabClicked(int32 TabIndex)
{
    if (TabButtons.IsValidIndex(TabIndex) && TabIndex < UE_ARRAY_COUNT(kTabs))
    {
        SetActiveCategory(kTabs[TabIndex].Category);
    }
}

void URA4SidebarWidget::HandleCardClicked(int32 CardIndex)
{
    if (CardContentIds.IsValidIndex(CardIndex))
    {
        OnBuildCardClicked.Broadcast(CardContentIds[CardIndex]);
    }
}

bool URA4SidebarWidget::ActivateCardByIndex(int32 CardIndex)
{
    if (!CardContentIds.IsValidIndex(CardIndex))
    {
        return false;
    }

    // A hotkey obeys the same rule as a click: a blocked card is inert, and pressing its
    // key queues nothing rather than sending a command the simulation will reject.
    if (CardButtons.IsValidIndex(CardIndex))
    {
        const URA4IndexedButton* Button = CardButtons[CardIndex];
        if (Button != nullptr && !Button->GetIsEnabled())
        {
            return false;
        }
    }

    OnBuildCardClicked.Broadcast(CardContentIds[CardIndex]);
    return true;
}

void URA4SidebarWidget::RefreshResources()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr)
    {
        return;
    }

    if (CreditsText != nullptr)
    {
        CreditsText->SetText(FText::AsNumber(Provider->GetCredits()));
    }

    const int32 Produced = Provider->GetPowerProduced();
    const int32 Consumed = Provider->GetPowerConsumed();
    const int32 Surplus = Produced - Consumed;
    const bool bShortage = Provider->IsPowerShortage();
    // Amber before red: a base running inside a tenth of its ceiling is one structure
    // away from a brownout, and that is worth seeing before it happens rather than after.
    const bool bTight = !bShortage && Produced > 0 && Surplus < FMath::Max(1, Produced / 10);
    const FLinearColor PowerColour = bShortage ? kPowerLow : (bTight ? kPowerTight : kPowerOk);

    if (PowerText != nullptr)
    {
        PowerText->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_PowerFormat", "POWER  {0} / {1}"),
                                         FText::AsNumber(Produced),
                                         FText::AsNumber(Consumed)));
        PowerText->SetColorAndOpacity(FSlateColor(PowerColour));
    }

    if (PowerSurplusText != nullptr)
    {
        if (Surplus < 0)
        {
            PowerSurplusText->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_PowerDeficit", "{0} DEFICIT"),
                                                    FText::AsNumber(Surplus)));
        }
        else
        {
            PowerSurplusText->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_PowerSurplus", "+{0} SPARE"),
                                                    FText::AsNumber(Surplus)));
        }
        PowerSurplusText->SetColorAndOpacity(FSlateColor(PowerColour));
    }

    if (PowerRatioBar != nullptr)
    {
        PowerRatioBar->SetPercent(PowerFillRatio(Produced, Consumed));
        PowerRatioBar->SetFillColorAndOpacity(PowerColour);
    }

    if (SupplyText != nullptr)
    {
        // HudSnapshot is explicit that the counter must be hidden rather than show an
        // invented cap while the simulation has no population limit.
        if (Provider->IsSupplyModelled())
        {
            const int32 Used = Provider->GetSupplyUsed();
            const int32 Cap = Provider->GetSupplyCap();
            SupplyText->SetVisibility(ESlateVisibility::HitTestInvisible);
            SupplyText->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_SupplyFormat", "UNITS  {0} / {1}"),
                                              FText::AsNumber(Used), FText::AsNumber(Cap)));
            SupplyText->SetColorAndOpacity(FSlateColor(Cap > 0 && Used >= Cap ? kPowerLow : kTextDim));
        }
        else
        {
            SupplyText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

#if !UE_BUILD_SHIPPING
    // The sidebar is 232 slate units wide, which is unreadable in a screenshot of a
    // 3456x2234 desktop -- upscaling blurred glyphs does not make them legible. So the
    // panel states what it actually put on screen, once per real change (this function
    // is driven by OnResourcesChanged, not by tick). Without this the only available
    // evidence was "the widget attached", which says nothing about its contents.
    UE_LOG(LogTemp, Display,
           TEXT("RA4 HUD res: credits=%d power=%d/%d surplus=%d shortage=%d tight=%d bar=%.2f supply=%s"),
           Provider->GetCredits(), Produced, Consumed, Surplus,
           bShortage ? 1 : 0, bTight ? 1 : 0, PowerFillRatio(Produced, Consumed),
           Provider->IsSupplyModelled() ? TEXT("shown") : TEXT("hidden"));
#endif
}

void URA4SidebarWidget::RefreshCards()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr || CardGrid == nullptr || WidgetTree == nullptr)
    {
        return;
    }

    const TArray<FRA4BuildOption> Options = Provider->GetBuildOptionsForCategory(ActiveCategory);

    // Queue progress fires the same change event as availability, and rebuilding two
    // dozen buttons every time a bar advances one percent is real work for no visible
    // difference. Rebuild the grid only when what it shows actually changed.
    uint32 Signature = ::GetTypeHash(ActiveCategory);
    for (const FRA4BuildOption& Option : Options)
    {
        Signature = HashCombine(Signature, ::GetTypeHash(Option.ContentId));
        Signature = HashCombine(Signature, ::GetTypeHash(Option.bAvailable));
        Signature = HashCombine(Signature, ::GetTypeHash(uint8(Option.BlockReason)));
    }
    if (Signature == CardsSignature && CardButtons.Num() == Options.Num())
    {
        RefreshQueue();
        return;
    }
    CardsSignature = Signature;

    CardGrid->ClearChildren();
    CardButtons.Reset();
    CardHoverTargets.Reset();
    CardContentIds.Reset();
    // Rebuilt cards start at rest: carrying a stale swell over would leave a card
    // enlarged with the pointer nowhere near it.
    CardHoverProgress.Reset();
    CardHoverProgress.SetNumZeroed(Options.Num());

    for (int32 Index = 0; Index < Options.Num(); ++Index)
    {
        const FRA4BuildOption& Option = Options[Index];

        URA4IndexedButton* Button = WidgetTree->ConstructWidget<URA4IndexedButton>(
            URA4IndexedButton::StaticClass(), *FString::Printf(TEXT("Card%d"), Index));
        Button->SetIndex(Index);
        Button->BindForwarding();
        Button->OnIndexedClicked.AddUObject(this, &URA4SidebarWidget::HandleCardClicked);
        StyleButton(Button, Option.bAvailable ? kCardOk : kCardBlocked);
        // Blocked cards stay clickable-looking but inert, and say why underneath --
        // a greyed-out card that gives no reason is the classic sidebar's one real
        // usability failure and there is no reason to reproduce it.
        Button->SetIsEnabled(Option.bAvailable);

        UVerticalBox* CardStack = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), *FString::Printf(TEXT("CardStack%d"), Index));

        // Name row, with the hotkey badge pinned to its left. The badge is drawn only for
        // indices the controller actually binds, so it can never promise a dead key.
        if (const TCHAR* Hotkey = GetCardHotkeyLabel(Index))
        {
            UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(
                UHorizontalBox::StaticClass(), *FString::Printf(TEXT("CardNameRow%d"), Index));

            UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(
                UBorder::StaticClass(), *FString::Printf(TEXT("CardKeyBadge%d"), Index));
            Badge->SetBrushColor(Option.bAvailable ? FLinearColor(0.06f, 0.09f, 0.07f, 0.95f)
                                                   : FLinearColor(0.09f, 0.06f, 0.06f, 0.95f));
            Badge->SetPadding(FMargin(3.0f, 0.0f));

            UTextBlock* KeyLabel = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardKey%d"), Index),
                                             Option.bAvailable ? kCredits : kTextFaint, 8, true);
            KeyLabel->SetText(FText::FromString(Hotkey));
            KeyLabel->SetJustification(ETextJustify::Center);
            Badge->AddChild(KeyLabel);

            if (UHorizontalBoxSlot* Slot = NameRow->AddChildToHorizontalBox(Badge))
            {
                Slot->SetPadding(FMargin(0.0f, 0.0f, 3.0f, 0.0f));
                Slot->SetVerticalAlignment(VAlign_Top);
            }

            UTextBlock* Name = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardName%d"), Index),
                                         kTextNormal, 10, true);
            Name->SetText(Option.DisplayName);
            Name->SetAutoWrapText(true);
            if (UHorizontalBoxSlot* Slot = NameRow->AddChildToHorizontalBox(Name))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            CardStack->AddChildToVerticalBox(NameRow);
        }
        else
        {
            // Past the end of the hotkey table: centred name, no badge.
            UTextBlock* Name = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardName%d"), Index),
                                         kTextNormal, 10, true);
            Name->SetText(Option.DisplayName);
            Name->SetJustification(ETextJustify::Center);
            Name->SetAutoWrapText(true);
            CardStack->AddChildToVerticalBox(Name);
        }

        FText InfoText = FText::Format(NSLOCTEXT("RA4", "Card_CostTimeFormat", "{0} Cr. | {1}s"),
                                       FText::AsNumber(Option.Cost),
                                       FText::AsNumber(FMath::RoundToInt(Option.BuildSeconds)));
        UTextBlock* Cost = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardCost%d"), Index), kCredits, 9, false);
        Cost->SetText(InfoText);
        Cost->SetJustification(ETextJustify::Center);
        CardStack->AddChildToVerticalBox(Cost);

        if (Option.PowerDelta != 0)
        {
            FLinearColor PowerColor = Option.PowerDelta > 0 ? kPowerOk : kPowerLow;
            FText CardPowerText = FText::Format(NSLOCTEXT("RA4", "Card_PowerFormat", "{0}{1} Power"),
                                                FText::FromString(Option.PowerDelta > 0 ? TEXT("+") : TEXT("")),
                                                FText::AsNumber(Option.PowerDelta));
            UTextBlock* PowerLabel = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardPower%d"), Index), PowerColor, 8, false);
            PowerLabel->SetText(CardPowerText);
            PowerLabel->SetJustification(ETextJustify::Center);
            CardStack->AddChildToVerticalBox(PowerLabel);
        }

        if (!Option.bAvailable)
        {
            FText WhyText = BlockReasonText(Option.BlockReason);
            if (Option.BlockReason == ERA4BuildBlockReason::MissingPrerequisite && !Option.PrerequisiteText.IsEmpty())
            {
                WhyText = FText::Format(NSLOCTEXT("RA4", "Card_PrereqFormat", "Req: {0}"), Option.PrerequisiteText);
            }
            UTextBlock* Reason = MakeLabel(WidgetTree, *FString::Printf(TEXT("CardWhy%d"), Index), kTextDim, 8, false);
            Reason->SetText(WhyText);
            Reason->SetJustification(ETextJustify::Center);
            Reason->SetAutoWrapText(true);
            CardStack->AddChildToVerticalBox(Reason);
        }

        Button->AddChild(CardStack);

        if (UUniformGridSlot* Slot = CardGrid->AddChildToUniformGrid(Button, Index / kCardColumns,
                                                                    Index % kCardColumns))
        {
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }

        CardButtons.Add(Button);
        // The stack, not the button, carries the swell: a UButton's render transform is
        // overwritten by its own style states, and scaling the contents reads the same.
        CardHoverTargets.Add(CardStack);
        CardContentIds.Add(Option.ContentId);
    }

#if !UE_BUILD_SHIPPING
    // What the grid actually built, so a hotkey badge can be verified as present rather
    // than inferred from the code that was supposed to draw it. Emitted only on a real
    // rebuild -- the signature check above returns early while only progress moves.
    {
        FString Built;
        for (int32 Index = 0; Index < CardContentIds.Num(); ++Index)
        {
            const TCHAR* Key = GetCardHotkeyLabel(Index);
            const bool bEnabled = CardButtons.IsValidIndex(Index) && CardButtons[Index] != nullptr
                                      ? CardButtons[Index]->GetIsEnabled()
                                      : false;
            Built += FString::Printf(TEXT(" [%s%s]"), Key != nullptr ? Key : TEXT("-"),
                                     bEnabled ? TEXT("") : TEXT(" blocked"));
        }
        UE_LOG(LogTemp, Display, TEXT("RA4 HUD cards: category=%d count=%d hotkeys=%d%s"),
               ActiveCategory, CardContentIds.Num(), GetCardHotkeyCount(), *Built);
    }
#endif

    RefreshQueue();
}

void URA4SidebarWidget::RefreshQueue()
{
    const URA4UIDataProviderSubsystem* Provider = GetProvider();
    if (Provider == nullptr || QueueBox == nullptr || WidgetTree == nullptr)
    {
        return;
    }

    const TArray<FRA4ProductionEntry>& Queue = Provider->GetProductionQueue();

    // The queue changes every tick while a factory runs, but only the head row's bar and
    // countdown actually move. Rebuilding the whole box for that throws away Slate's
    // layout for no visible gain, so rows are rebuilt only when their identity or state
    // changes; progress alone is pushed into the existing widgets.
    uint32 Signature = ::GetTypeHash(Queue.Num());
    for (const FRA4ProductionEntry& Entry : Queue)
    {
        Signature = HashCombine(Signature, ::GetTypeHash(Entry.ContentId));
        Signature = HashCombine(Signature, ::GetTypeHash(Entry.bPaused));
        Signature = HashCombine(Signature, ::GetTypeHash(Entry.bAwaitingPlacement));
    }

    if (QueueHeader != nullptr)
    {
        if (Queue.Num() > 1)
        {
            QueueHeader->SetText(FText::Format(NSLOCTEXT("RA4", "Sidebar_QueueHeaderCount", "PRODUCTION  ({0})"),
                                               FText::AsNumber(Queue.Num())));
        }
        else
        {
            QueueHeader->SetText(NSLOCTEXT("RA4", "Sidebar_QueueHeader", "PRODUCTION"));
        }
    }

    const bool bRebuild = Signature != QueueSignature;
    if (bRebuild)
    {
        QueueSignature = Signature;
        QueueBox->ClearChildren();
    }

    if (Queue.Num() == 0)
    {
        if (bRebuild)
        {
            UTextBlock* Idle = MakeLabel(WidgetTree, TEXT("QueueIdle"), kTextFaint, 9, false);
            Idle->SetText(NSLOCTEXT("RA4", "Sidebar_QueueIdle", "NOTHING IN PRODUCTION"));
            QueueBox->AddChildToVerticalBox(Idle);
        }
        return;
    }

    // Only the head of the queue is being worked on; everything behind it is waiting its
    // turn, and saying so is the difference between "why is nothing happening" and a
    // pipeline the player can read.
    for (int32 Index = 0; Index < Queue.Num(); ++Index)
    {
        const FRA4ProductionEntry& Entry = Queue[Index];
        const bool bActive = Index == 0 && !Entry.bPaused && !Entry.bAwaitingPlacement;
        const float Fraction = FMath::Clamp(float(Entry.ProgressPercent) / 100.0f, 0.0f, 1.0f);
        const bool bHasBar = bActive || Entry.bPaused || Entry.bAwaitingPlacement;

        FLinearColor RowColour = kQueueWaiting;
        if (Entry.bAwaitingPlacement)
        {
            RowColour = kPowerOk;
        }
        else if (Entry.bPaused)
        {
            RowColour = kPowerTight;
        }
        else if (bActive)
        {
            RowColour = kTextNormal;
        }

        if (bRebuild)
        {
            // Name left, state or countdown right, so the eye tracks one column for
            // "what" and one for "when" instead of parsing a run-on line.
            UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
                UHorizontalBox::StaticClass(), *FString::Printf(TEXT("QueueRow%d"), Index));

            // A caret for the item being built, an ordinal for the ones behind it.
            UTextBlock* Position = MakeLabel(WidgetTree, *FString::Printf(TEXT("QueuePos%d"), Index),
                                             bActive ? kPowerOk : kQueueWaiting, 8, true);
            Position->SetText(bActive
                                  ? NSLOCTEXT("RA4", "Queue_ActiveMark", ">")
                                  : FText::Format(NSLOCTEXT("RA4", "Queue_PositionMark", "{0}."),
                                                  FText::AsNumber(Index + 1)));
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Position))
            {
                Slot->SetPadding(FMargin(0.0f, 0.0f, 3.0f, 0.0f));
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            UTextBlock* Name = MakeLabel(WidgetTree, *FString::Printf(TEXT("QueueLine%d"), Index), RowColour, 9,
                                         bActive || Entry.bAwaitingPlacement);
            Name->SetText(Entry.DisplayName);
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Name))
            {
                Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            UTextBlock* Status = MakeLabel(WidgetTree, *FString::Printf(TEXT("QueueStatus%d"), Index), RowColour, 8,
                                           Entry.bAwaitingPlacement);
            if (Entry.bAwaitingPlacement)
            {
                // The one queue state that needs the player to act, so it says so instead
                // of showing a full bar and waiting to be understood.
                Status->SetText(NSLOCTEXT("RA4", "Sidebar_QueuePlace", "PLACE IT"));
            }
            else if (Entry.bPaused)
            {
                Status->SetText(NSLOCTEXT("RA4", "Sidebar_QueuePaused", "HELD"));
            }
            else if (bActive)
            {
                Status->SetText(FormatBuildRemaining(Entry.RemainingSeconds));
            }
            else
            {
                Status->SetText(NSLOCTEXT("RA4", "Sidebar_QueueWaiting", "QUEUED"));
            }
            if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Status))
            {
                Slot->SetVerticalAlignment(VAlign_Center);
            }

            QueueBox->AddChildToVerticalBox(Row);

            // Only what is under construction gets a bar. A stack of identical empty bars
            // behind it says nothing and makes the panel look busier than it is.
            if (bHasBar)
            {
                UProgressBar* Bar = MakeThinBar(WidgetTree, *FString::Printf(TEXT("QueueBar%d"), Index),
                                                RowColour, 3.0f);
                Bar->SetPercent(Entry.bAwaitingPlacement ? 1.0f : Fraction);
                if (UVerticalBoxSlot* Slot = QueueBox->AddChildToVerticalBox(Bar))
                {
                    Slot->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 4.0f));
                }
            }
            else if (UVerticalBoxSlot* Slot = QueueBox->AddChildToVerticalBox(
                         MakeGap(WidgetTree, *FString::Printf(TEXT("QueueGap%d"), Index), 2.0f)))
            {
                Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
            }
        }
        else if (bActive)
        {
            // Same rows, moved progress: update the two widgets that changed. Found by
            // name rather than cached in an array parallel to the queue, because a rebuild
            // is exactly what would invalidate such an array.
            if (UProgressBar* Bar = Cast<UProgressBar>(
                    WidgetTree->FindWidget(*FString::Printf(TEXT("QueueBar%d"), Index))))
            {
                Bar->SetPercent(Fraction);
            }
            if (UTextBlock* Status = Cast<UTextBlock>(
                    WidgetTree->FindWidget(*FString::Printf(TEXT("QueueStatus%d"), Index))))
            {
                Status->SetText(FormatBuildRemaining(Entry.RemainingSeconds));
            }
        }
    }
}
