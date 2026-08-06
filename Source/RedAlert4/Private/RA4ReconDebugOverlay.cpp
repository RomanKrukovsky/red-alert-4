// Copyright (c) Red Alert 4 project.
#include "RA4ReconDebugOverlay.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

#include "RA4SimCoords.h"
#include "RA4SimWorldSubsystem.h"

#include "RA4Core/SimConfig.h"
#include "RA4Recon/PerceivedWorld.h"
#include "RA4Recon/ReconSystem.h"
#include "RA4Simulation/SimWorld.h"

#include <vector>

namespace
{

// 0 off, 1 belief only, 2 belief + ground truth. A CVar rather than a UPROPERTY
// because this must be reachable mid-playtest from the console with no UI built
// yet -- and it lives entirely in the presentation layer, so it cannot desync
// anything (the simulation never reads it).
//
// FAutoConsoleVariableRef, not TAutoConsoleVariable<int32>. The templated form has
// a destructor that calls AsVariable()->ClearOnChangedCallback(), and for a static
// living in a game module that destructor runs from __cxa_finalize_ranges during
// exit(), after the console manager has already been torn down. The result is a
// dereference of freed memory and a "Caught signal" crash on quit with this exact
// stack:
//     TAutoConsoleVariable<int>::~TAutoConsoleVariable
//     libsystem_c __cxa_finalize_ranges -> exit
//     AppKit -[NSApplication terminate:]
// The Ref form has no such destructor, so shutdown order stops mattering. It reads
// through a plain int32, which is also cheaper than GetValueOnGameThread().
int32 GReconOverlayMode = 0;
static FAutoConsoleVariableRef CVarReconOverlay(
    TEXT("recon.Overlay"), GReconOverlayMode,
    TEXT("Recon two-maps debug overlay: 0=off, 1=belief, 2=belief+ground truth"),
    ECVF_Cheat);

// Palette. Colour is never the only channel (P-5 accessibility rule): every
// marker also differs in glyph -- box for belief, cross for truth, ring for
// anonymous -- and the text label carries the numbers.
const FLinearColor kBeliefFresh(0.25f, 0.75f, 1.0f, 0.9f);
const FLinearColor kBeliefStale(0.55f, 0.55f, 0.60f, 0.8f);
const FLinearColor kBeliefContested(1.0f, 0.65f, 0.15f, 0.9f);
const FLinearColor kTruth(1.0f, 0.25f, 0.25f, 0.9f);
const FLinearColor kErrorRadius(0.25f, 0.75f, 1.0f, 0.30f);

void DrawScreenBox(UCanvas* Canvas, const FVector2D& Centre, float HalfSize, const FLinearColor& Color)
{
    FCanvasBoxItem Box(FVector2D(Centre.X - HalfSize, Centre.Y - HalfSize),
                       FVector2D(HalfSize * 2.0f, HalfSize * 2.0f));
    Box.SetColor(Color);
    Box.LineThickness = 1.5f;
    Canvas->DrawItem(Box);
}

void DrawScreenCross(UCanvas* Canvas, const FVector2D& Centre, float HalfSize, const FLinearColor& Color)
{
    FCanvasLineItem A(FVector2D(Centre.X - HalfSize, Centre.Y - HalfSize),
                      FVector2D(Centre.X + HalfSize, Centre.Y + HalfSize));
    FCanvasLineItem B(FVector2D(Centre.X - HalfSize, Centre.Y + HalfSize),
                      FVector2D(Centre.X + HalfSize, Centre.Y - HalfSize));
    A.SetColor(Color);
    B.SetColor(Color);
    A.LineThickness = 1.5f;
    B.LineThickness = 1.5f;
    Canvas->DrawItem(A);
    Canvas->DrawItem(B);
}

// Ground-plane circle projected point by point, same approach as the move-order
// ring in RA4RtsHud: cheap, and it reads as terrain-attached rather than a
// sticker on the screen.
void DrawGroundCircle(UCanvas* Canvas, const APlayerController* Projector, const FVector& CentreWorld,
                      float RadiusUnits, const FLinearColor& Color)
{
    constexpr int32 kSegments = 24;
    FVector2D Prev = FVector2D::ZeroVector;
    bool bHavePrev = false;
    for (int32 I = 0; I <= kSegments; ++I)
    {
        const float Angle = (2.0f * PI * float(I)) / float(kSegments);
        const FVector WorldPoint = CentreWorld + FVector(FMath::Cos(Angle) * RadiusUnits,
                                                         FMath::Sin(Angle) * RadiusUnits, 0.0f);
        FVector2D Point;
        if (!Projector->ProjectWorldLocationToScreen(WorldPoint, Point))
        {
            bHavePrev = false;
            continue;
        }
        if (bHavePrev)
        {
            FCanvasLineItem Line(Prev, Point);
            Line.SetColor(Color);
            Canvas->DrawItem(Line);
        }
        Prev = Point;
        bHavePrev = true;
    }
}

const TCHAR* CategoryLabel(RA4::Recon::ObservedCategory C)
{
    switch (C)
    {
        case RA4::Recon::ObservedCategory::Infantry: return TEXT("INF");
        case RA4::Recon::ObservedCategory::LightVehicle: return TEXT("LT VEH");
        case RA4::Recon::ObservedCategory::HeavyVehicle: return TEXT("HV VEH");
        case RA4::Recon::ObservedCategory::Aircraft: return TEXT("AIR");
        case RA4::Recon::ObservedCategory::Ship: return TEXT("SHIP");
        case RA4::Recon::ObservedCategory::Structure: return TEXT("STRUCT");
        default: return TEXT("?");
    }
}

} // namespace

// --- Console commands (§7) ---------------------------------------------------------
//
// recon.DumpTracks            what the staff map currently believes
// recon.LogChain <TrackIndex> why it believes one specific contact
//
// Both are read-only over the simulation and ECVF_Cheat, because DumpTracks prints
// belief (harmless) while LogChain prints the ground-truth comparison behind it,
// which is exactly the information the layer exists to withhold mid-match.
namespace
{

const RA4::SimWorld* FindSimWorldForConsole(UWorld* World)
{
    if (World == nullptr)
    {
        return nullptr;
    }
    const URA4SimWorldSubsystem* Sim = World->GetSubsystem<URA4SimWorldSubsystem>();
    return Sim != nullptr ? Sim->GetSimWorld() : nullptr;
}

FAutoConsoleCommandWithWorld GCmdDumpTracks(
    TEXT("recon.DumpTracks"),
    TEXT("Print the local player's perceived tracks (belief only)."),
    FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World)
    {
        const RA4::SimWorld* Sim = FindSimWorldForConsole(World);
        if (Sim == nullptr || !Sim->GetRecon().IsEnabled())
        {
            UE_LOG(LogTemp, Display, TEXT("recon.DumpTracks: recon layer is not enabled"));
            return;
        }
        std::vector<const RA4::Recon::PerceivedTrack*> Tracks;
        Sim->GetRecon().GetPerceivedWorld(0).GetTracksInRegion(
            0, 0, Sim->GetMap().Width - 1, Sim->GetMap().Height - 1, Tracks);
        UE_LOG(LogTemp, Display, TEXT("recon.DumpTracks: %d track(s) believed by player 0"),
               int32(Tracks.size()));
        for (const RA4::Recon::PerceivedTrack* T : Tracks)
        {
            UE_LOG(LogTemp, Display,
                   TEXT("  [%u] %s x%d-%d  conf %d%%  age %ds%s%s"),
                   T->Id.Index,
                   T->bAnonymous ? TEXT("unidentified") : CategoryLabel(T->BelievedCategory),
                   T->BelievedCountMin, T->BelievedCountMax,
                   int32((T->Confidence * 100).ToIntFloor()),
                   (int32(Sim->GetTick()) - int32(T->LastUpdateTick)) / RA4::kTicksPerSecond,
                   T->bContested ? TEXT("  CONTESTED") : TEXT(""),
                   T->bStale ? TEXT("  stale") : TEXT(""));
        }
    }),
    ECVF_Cheat);

FAutoConsoleCommandWithWorldAndArgs GCmdLogChain(
    TEXT("recon.LogChain"),
    TEXT("recon.LogChain <TrackIndex> -- print the report chain that produced one contact."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
    {
        const RA4::SimWorld* Sim = FindSimWorldForConsole(World);
        if (Sim == nullptr || !Sim->GetRecon().IsEnabled())
        {
            UE_LOG(LogTemp, Display, TEXT("recon.LogChain: recon layer is not enabled"));
            return;
        }
        if (Args.Num() < 1)
        {
            UE_LOG(LogTemp, Display, TEXT("recon.LogChain: expected a track index (see recon.DumpTracks)"));
            return;
        }
        const uint32 WantIndex = uint32(FCString::Atoi(*Args[0]));
        std::vector<const RA4::Recon::PerceivedTrack*> Tracks;
        Sim->GetRecon().GetPerceivedWorld(0).GetTracksInRegion(
            0, 0, Sim->GetMap().Width - 1, Sim->GetMap().Height - 1, Tracks);
        for (const RA4::Recon::PerceivedTrack* T : Tracks)
        {
            if (T->Id.Index == WantIndex)
            {
                // One shared explanation routine for console and UI, so a post-match
                // screen and a debug dump can never tell the player different stories.
                const std::string Text = Sim->GetRecon().ExplainTrack(0, *T);
                UE_LOG(LogTemp, Display, TEXT("%s"), UTF8_TO_TCHAR(Text.c_str()));
                return;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("recon.LogChain: no live track with index %u"), WantIndex);
    }),
    ECVF_Cheat);

} // namespace

int32 URA4ReconDebugOverlay::GetOverlayMode()
{
    // Read the backing int32 directly. FAutoConsoleVariableRef writes straight into
    // it, so there is no GetValueOnGameThread() indirection to go through.
    return GReconOverlayMode;
}

void URA4ReconDebugOverlay::Draw(UCanvas* Canvas, const APlayerController* Projector,
                                 const URA4SimWorldSubsystem* Sim, uint8 ViewerPlayer)
{
    const int32 Mode = GetOverlayMode();
    if (Mode <= 0 || Canvas == nullptr || Projector == nullptr || Sim == nullptr)
    {
        return;
    }
    const RA4::SimWorld* World = Sim->GetSimWorld();
    if (World == nullptr || !World->GetRecon().IsEnabled())
    {
        return;
    }

    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;

    // --- Belief markers (the right map of the two) -----------------------------
    static std::vector<const RA4::Recon::PerceivedTrack*> Tracks; // reused scratch
    Tracks.clear();
    const RA4::Recon::PerceivedWorld& Belief = World->GetRecon().GetPerceivedWorld(ViewerPlayer);
    Belief.GetTracksInRegion(0, 0, World->GetMap().Width - 1, World->GetMap().Height - 1, Tracks);

    for (const RA4::Recon::PerceivedTrack* T : Tracks)
    {
        const FVector Centre = RA4Coords::ToUnreal(T->BelievedPosition);
        FVector2D Screen;
        if (!Projector->ProjectWorldLocationToScreen(Centre, Screen))
        {
            continue;
        }

        const FLinearColor Color = T->bContested ? kBeliefContested
                                 : T->bStale ? kBeliefStale
                                             : kBeliefFresh;

        if (T->bAnonymous)
        {
            // Ring: a contact without identity. Distinct glyph, not just colour.
            DrawGroundCircle(Canvas, Projector, Centre, 60.0f, Color);
        }
        else
        {
            DrawScreenBox(Canvas, Screen, 9.0f, Color);
        }

        // Position error circle: the honest "somewhere around here".
        const float ErrorUnits = float(T->PositionErrorRadius.ToIntFloor());
        if (ErrorUnits > 1.0f)
        {
            DrawGroundCircle(Canvas, Projector, Centre, ErrorUnits, kErrorRadius);
        }

        if (Font != nullptr)
        {
            // Count interval, confidence, age: exactly the §4.6 read-surface
            // fields, so the overlay doubles as a living proof that the UI
            // contract carries everything a real HUD will need.
            const int32 AgeSeconds =
                (int32(World->GetTick()) - int32(T->LastUpdateTick)) / RA4::kTicksPerSecond;
            const int32 ConfidencePercent = int32((T->Confidence * 100).ToIntFloor());
            FString Label;
            if (T->bAnonymous)
            {
                Label = FString::Printf(TEXT("? contact  %ds"), AgeSeconds);
            }
            else if (T->BelievedCountMin == T->BelievedCountMax)
            {
                Label = FString::Printf(TEXT("%s x%d  %d%%  %ds"), CategoryLabel(T->BelievedCategory),
                                        T->BelievedCountMin, ConfidencePercent, AgeSeconds);
            }
            else
            {
                Label = FString::Printf(TEXT("%s x%d-%d  %d%%  %ds"), CategoryLabel(T->BelievedCategory),
                                        T->BelievedCountMin, T->BelievedCountMax, ConfidencePercent,
                                        AgeSeconds);
            }
            FCanvasTextItem Text(Screen + FVector2D(12.0f, -6.0f), FText::FromString(Label), Font, Color);
            Canvas->DrawItem(Text);
        }
    }

    // --- Ground truth crosses (mode 2: the left map) ----------------------------
    // The one sanctioned GT read outside the simulation: this IS the comparison
    // view (ADR-0026 §7 calls it mandatory for tuning). Red crosses over enemy
    // entities; the offset to the blue boxes is the distortion, visible at a
    // glance. ECVF_Cheat keeps it out of shipping multiplayer.
    if (Mode >= 2)
    {
        const auto& Cores = World->GetAllCores();
        const auto& Transforms = World->GetAllTransforms();
        for (uint32 I = 0; I < World->GetEntityCapacity(); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Owner == ViewerPlayer)
            {
                continue;
            }
            FVector2D Screen;
            if (!Projector->ProjectWorldLocationToScreen(RA4Coords::ToUnreal(Transforms[I].Position), Screen))
            {
                continue;
            }
            DrawScreenCross(Canvas, Screen, 7.0f, kTruth);
        }
    }
}
