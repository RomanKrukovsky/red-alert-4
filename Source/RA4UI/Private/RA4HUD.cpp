// Copyright (c) Red Alert 4 project.

#include "RA4HUD.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "RA4HUD"

ARA4HUD::ARA4HUD()
{
}

void ARA4HUD::UpdateDirectControlDisplay(bool bActive, int32 Health, int32 MaxHealth,
                                        const FText& InPrimaryName, const FText& InSecondaryName,
                                        float InPrimaryCd, float InSecondaryCd, float InSpeedKph,
                                        bool bInOpticsZoomed)
{
    bDirectControlActive = bActive;
    DirectControlHealth = Health;
    DirectControlMaxHealth = MaxHealth;
    DirectControlPrimaryWeapon = InPrimaryName;
    DirectControlSecondaryWeapon = InSecondaryName;
    DirectControlPrimaryCd = InPrimaryCd;
    DirectControlSecondaryCd = InSecondaryCd;
    DirectControlSpeedKph = InSpeedKph;
    bDirectControlOpticsZoomed = bInOpticsZoomed;
}

void ARA4HUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas)
    {
        return;
    }

    auto DrawColoredText = [this](const FText& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.0f)
    {
        if (GEngine && GEngine->GetSmallFont())
        {
            FCanvasTextItem TextItem(FVector2D(X, Y), Text, GEngine->GetSmallFont(), Color);
            TextItem.Scale = FVector2D(Scale, Scale);
            Canvas->DrawItem(TextItem);
        }
    };

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

    // 1. RTS Box Selection Rect
    if (bDrawSelectionRect)
    {
        float Width = SelectionEnd.X - SelectionStart.X;
        float Height = SelectionEnd.Y - SelectionStart.Y;

        // Fill rect
        FCanvasTileItem TileItem(SelectionStart, FVector2D(Width, Height), SelectionRectColor);
        TileItem.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(TileItem);

        // Border rect
        FCanvasBoxItem BoxItem(SelectionStart, FVector2D(Width, Height));
        BoxItem.SetColor(SelectionRectBorderColor);
        BoxItem.LineThickness = 1.5f;
        Canvas->DrawItem(BoxItem);
    }

    // 2. First-Person / Direct Control Tactical Combat HUD & Bottom Ability Bar
    if (bDirectControlActive)
    {
        const float ScreenW = Canvas->SizeX;
        const float ScreenH = Canvas->SizeY;
        const FVector2D Center(ScreenW * 0.5f, ScreenH * 0.5f);

        // Center Tactical Reticle
        const float ReticleSize = bDirectControlOpticsZoomed ? 28.0f : 20.0f;
        const FLinearColor ReticleColor = bDirectControlOpticsZoomed ? FLinearColor(0.2f, 1.0f, 0.4f, 0.95f) : FLinearColor(0.2f, 0.85f, 1.0f, 0.85f);

        // Crosshair Lines
        DrawHudLine(Center.X - ReticleSize, Center.Y, Center.X - 5.0f, Center.Y, ReticleColor, 2.0f);
        DrawHudLine(Center.X + 5.0f, Center.Y, Center.X + ReticleSize, Center.Y, ReticleColor, 2.0f);
        DrawHudLine(Center.X, Center.Y - ReticleSize, Center.X, Center.Y - 5.0f, ReticleColor, 2.0f);
        DrawHudLine(Center.X, Center.Y + 5.0f, Center.X, Center.Y + ReticleSize, ReticleColor, 2.0f);

        // Reticle Center Dot
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
                                              DirectControlSpeedKph,
                                              bDirectControlOpticsZoomed ? TEXT("2.5x ZOOM") : TEXT("1.0x СТАНДАРТ"));
        DrawColoredText(FText::FromString(ReticleInfo), Center.X - 100.0f, Center.Y + ReticleSize + 10.0f, FLinearColor::White, 1.0f);

        // --- Bottom Tactical Ability / Action Bar ---
        const float BarW = 860.0f;
        const float BarH = 80.0f;
        const FVector2D BarPos((ScreenW - BarW) * 0.5f, ScreenH - BarH - 16.0f);

        // Panel Background & Border
        FCanvasTileItem BarBg(BarPos, FVector2D(BarW, BarH), FLinearColor(0.015f, 0.035f, 0.055f, 0.88f));
        BarBg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(BarBg);

        FCanvasBoxItem BarBorder(BarPos, FVector2D(BarW, BarH));
        BarBorder.SetColor(FLinearColor(0.18f, 0.55f, 0.75f, 0.9f));
        BarBorder.LineThickness = 2.0f;
        Canvas->DrawItem(BarBorder);

        // 4 Action Buttons
        const float ButtonW = (BarW - 40.0f) / 4.0f;
        const float ButtonH = BarH - 16.0f;
        const float ButtonY = BarPos.Y + 8.0f;

        // Button 1: [ ЛКМ ] Основное орудие
        {
            const float BtnX = BarPos.X + 8.0f;
            FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
            BtnBox.SetColor(DirectControlPrimaryCd <= 0.01f ? FLinearColor(0.1f, 0.85f, 0.35f, 0.7f) : FLinearColor(0.9f, 0.55f, 0.1f, 0.7f));
            Canvas->DrawItem(BtnBox);

            DrawColoredText(LOCTEXT("LmbKey", "[ ЛКМ ] ОСНОВНОЕ ОРУДИЕ"), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(0.15f, 0.95f, 0.4f, 1.0f), 1.0f);
            DrawColoredText(DirectControlPrimaryWeapon.IsEmpty() ? LOCTEXT("DefCannon", "120-мм ПУШКА") : DirectControlPrimaryWeapon, BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
            
            FString StatusStr = DirectControlPrimaryCd <= 0.01f ? TEXT("ГОТОВО К СТРЕЛЬБЕ") : FString::Printf(TEXT("ЗАРЯДКА: %.0f%%"), (1.0f - DirectControlPrimaryCd) * 100.0f);
            DrawColoredText(FText::FromString(StatusStr), BtnX + 8.0f, ButtonY + 42.0f, DirectControlPrimaryCd <= 0.01f ? FLinearColor(0.2f, 1.0f, 0.4f, 1.0f) : FLinearColor(1.0f, 0.7f, 0.2f, 1.0f), 0.85f);
        }

        // Button 2: [ ПКМ ] Спецспособность / Второе орудие
        {
            const float BtnX = BarPos.X + 16.0f + ButtonW;
            FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
            BtnBox.SetColor(FLinearColor(0.15f, 0.75f, 0.95f, 0.7f));
            Canvas->DrawItem(BtnBox);

            DrawColoredText(LOCTEXT("RmbKey", "[ ПКМ ] СПЕЦСПОСОБНОСТЬ"), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(0.2f, 0.85f, 1.0f, 1.0f), 1.0f);
            DrawColoredText(DirectControlSecondaryWeapon.IsEmpty() ? LOCTEXT("DefAbility", "РАКЕТНЫЙ ЗАЛП / УСКОРЕНИЕ") : DirectControlSecondaryWeapon, BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
            DrawColoredText(LOCTEXT("AbilityReady", "АКТИВАЦИЯ [ГОТОВО]"), BtnX + 8.0f, ButtonY + 42.0f, FLinearColor(0.2f, 1.0f, 0.9f, 1.0f), 0.85f);
        }

        // Button 3: [ Z / СКМ ] Прицел и оптика
        {
            const float BtnX = BarPos.X + 24.0f + ButtonW * 2.0f;
            FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
            BtnBox.SetColor(bDirectControlOpticsZoomed ? FLinearColor(0.2f, 1.0f, 0.5f, 0.7f) : FLinearColor(0.4f, 0.5f, 0.6f, 0.7f));
            Canvas->DrawItem(BtnBox);

            DrawColoredText(LOCTEXT("ZoomKey", "[ Z ] ПРИЦЕЛ / ОПТИКА"), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(0.85f, 0.85f, 0.85f, 1.0f), 1.0f);
            DrawColoredText(bDirectControlOpticsZoomed ? LOCTEXT("ZoomOn", "ПРИБЛИЖЕНИЕ 2.5X") : LOCTEXT("ZoomOff", "ШИРОКИЙ ОБЗОР"), BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
            DrawColoredText(LOCTEXT("ZoomToggle", "ПЕРЕКЛЮЧЕНИЕ [Z / СКМ]"), BtnX + 8.0f, ButtonY + 42.0f, FLinearColor(0.7f, 0.7f, 0.7f, 1.0f), 0.85f);
        }

        // Button 4: [ J / ESC ] Выход в стратегический режим RTS
        {
            const float BtnX = BarPos.X + 32.0f + ButtonW * 3.0f;
            FCanvasBoxItem BtnBox(FVector2D(BtnX, ButtonY), FVector2D(ButtonW, ButtonH));
            BtnBox.SetColor(FLinearColor(0.85f, 0.25f, 0.25f, 0.7f));
            Canvas->DrawItem(BtnBox);

            DrawColoredText(LOCTEXT("ExitKey", "[ J / ESC ] ВЫХОД [RTS]"), BtnX + 8.0f, ButtonY + 6.0f, FLinearColor(1.0f, 0.4f, 0.4f, 1.0f), 1.0f);
            DrawColoredText(LOCTEXT("ExitSub", "СТРАТЕГИЧЕСКИЙ ВИД"), BtnX + 8.0f, ButtonY + 24.0f, FLinearColor::White, 0.9f);
            DrawColoredText(LOCTEXT("ExitHint", "ВОЗВРАТ К БАЗЕ"), BtnX + 8.0f, ButtonY + 42.0f, FLinearColor(0.8f, 0.8f, 0.8f, 1.0f), 0.85f);
        }

        // --- Vehicle Hull / Armor Status Card at Bottom Left ---
        const float CardW = 240.0f;
        const float CardH = 80.0f;
        const FVector2D CardPos(20.0f, ScreenH - CardH - 16.0f);

        FCanvasTileItem CardBg(CardPos, FVector2D(CardW, CardH), FLinearColor(0.015f, 0.035f, 0.055f, 0.88f));
        CardBg.BlendMode = SE_BLEND_Translucent;
        Canvas->DrawItem(CardBg);

        FCanvasBoxItem CardBorder(CardPos, FVector2D(CardW, CardH));
        CardBorder.SetColor(FLinearColor(0.2f, 0.6f, 0.8f, 0.9f));
        Canvas->DrawItem(CardBorder);

        const float HealthRatio = DirectControlMaxHealth > 0 ? FMath::Clamp(float(DirectControlHealth) / float(DirectControlMaxHealth), 0.0f, 1.0f) : 1.0f;
        DrawColoredText(LOCTEXT("VehStatus", "СОСТОЯНИЕ ТЕХНИКИ"), CardPos.X + 8.0f, CardPos.Y + 6.0f, FLinearColor(0.2f, 0.8f, 1.0f, 1.0f), 1.0f);
        
        // HP Bar Fill
        const float BarFillW = (CardW - 16.0f) * HealthRatio;
        FCanvasTileItem HpBar(FVector2D(CardPos.X + 8.0f, CardPos.Y + 28.0f), FVector2D(BarFillW, 14.0f), HealthRatio > 0.4f ? FLinearColor(0.1f, 0.9f, 0.3f, 1.0f) : FLinearColor(0.95f, 0.2f, 0.2f, 1.0f));
        Canvas->DrawItem(HpBar);

        FCanvasBoxItem HpBarBorder(FVector2D(CardPos.X + 8.0f, CardPos.Y + 28.0f), FVector2D(CardW - 16.0f, 14.0f));
        HpBarBorder.SetColor(FLinearColor::White);
        Canvas->DrawItem(HpBarBorder);

        FString HpText = FString::Printf(TEXT("ПРОЧНОСТЬ: %d / %d HP"), DirectControlHealth, DirectControlMaxHealth);
        DrawColoredText(FText::FromString(HpText), CardPos.X + 8.0f, CardPos.Y + 48.0f, FLinearColor::White, 0.85f);
    }
}

#undef LOCTEXT_NAMESPACE
