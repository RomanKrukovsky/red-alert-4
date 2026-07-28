// Copyright (c) Red Alert 4 project.

#include "RA4HUD.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"

ARA4HUD::ARA4HUD()
{
}

void ARA4HUD::DrawHUD()
{
    Super::DrawHUD();

    if (bDrawSelectionRect && Canvas)
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
}

void ARA4HUD::SetSelectionRect(const FVector2D& InStart, const FVector2D& InEnd, bool bDraw)
{
    SelectionStart = InStart;
    SelectionEnd = InEnd;
    bDrawSelectionRect = bDraw;
}
