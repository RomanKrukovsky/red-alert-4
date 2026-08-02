# Red Alert 4: NoesisGUI XAML Style Guide & Design System

## 1. Resource Dictionary Hierarchy

All visual styles, colors, fonts, and animations are organized into reusable ResourceDictionaries located in `Assets/Noesis/Themes/` and `Assets/Noesis/Controls/`:

```
Assets/Noesis/
├── App.xaml                            <-- Global Master Dictionary
├── Themes/
│   ├── Typography.xaml                 <-- Fonts: Orbitron, Jura, Inter, Oswald
│   ├── ColorsGlobal.xaml               <-- Base Brushes
│   ├── FactionSoviet.xaml              <-- Red / Gold / Brutalist
│   ├── FactionAllies.xaml              <-- Blue / Silver / Hi-Tech
│   ├── FactionCoalition.xaml           <-- Emerald / Bronze / Ornate
│   └── FactionChronolegion.xaml        <-- Purple / Violet / Energy
└── Controls/
    ├── ButtonStyles.xaml               <-- Angled RA4 Button Templates
    ├── PanelStyles.xaml                <-- Glow Borders & Glassmorphism
    ├── ProgressBarStyles.xaml          <-- Angled Progress & Health Bars
    └── UnitTileStyles.xaml             <-- Production Grid Tiles
```

---

## 2. Multi-Resolution & DPI Scaling Rules

* **Target Aspect Ratios**: 16:9 (1920x1080, 2560x1440, 3840x2160), 16:10, 21:9 Ultra-Wide.
* **Layout Scaling**: All root containers use `<Grid>` or `<Viewbox>` with `UseLayoutRounding="True"` and `SnapsToDevicePixels="True"`.
* **Safe Zones**: Margin offsets (32px minimum) around critical HUD elements to prevent clipping on television monitors or ultrawide displays.
