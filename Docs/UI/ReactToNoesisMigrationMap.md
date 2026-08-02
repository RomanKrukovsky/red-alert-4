# React to NoesisGUI (C++ / XAML) Migration Mapping Matrix

This document provides an explicit 1-to-1 conversion map for transitioning all React/CSS constructs to native NoesisGUI (WPF/XAML) elements in Unreal Engine 5.

---

## 1. Construct Conversion Rules

| React / HTML Concept | NoesisGUI / XAML Equivalent | Unreal Engine C++ Backend |
| :--- | :--- | :--- |
| `<div className="panel">` | `<Border>` or `<Grid>` | `UUserControl` / `NoesisXaml` |
| `style={{ color: 'var(--theme-primary)' }}` | `Foreground="{DynamicResource ThemePrimaryBrush}"` | `FSlateColor` / `FLinearColor` |
| `useState(0)` | `{Binding PropertyName, Mode=TwoWay}` | `UPROPERTY()` with `INotifyPropertyChanged` |
| `useEffect()` | VisualStateManager / `Loaded` event | ViewModel lifecycle / Delegate subscription |
| `onClick={() => navigate('/route')}` | `Command="{Binding NavigateCommand}"` | `ICommand` / `FUIAction` / `UFUNCTION()` |
| `list.map(item => <Tile />)` | `<ItemsControl ItemsSource="{Binding Items}">` | `TArray<UObject*>` / `ObservableCollection` |
| CSS `@keyframes` | `<Storyboard>` & `<DoubleAnimation>` | `Noesis::Storyboard` |
| CSS `clip-path: polygon(...)` | `<Path Data="M 10 0 L 100 0 ... Z">` | `FPathData` / Vector Geometry |

---

## 2. Component Migration Catalog

### 2.1 Core Controls

* **`Button.tsx` -> `ButtonStyles.xaml` & `RA4Button.xaml`**
  * *React*: Angled button with `clip-path: polygon(...)`, linear gradient, and hover glow shadow.
  * *Noesis*: `<Style TargetType="Button">` with `<ControlTemplate>` containing a `<Path>` background for angled corners, `Triggers` for `IsMouseOver` and `IsPressed`, and `DropShadowEffect` for glow.

* **`Panel.tsx` -> `PanelStyles.xaml`**
  * *React*: `clip-angled-all`, `var(--theme-bg-panel)`, `backdrop-filter: blur(10px)`.
  * *Noesis*: `<Border>` with `Background="{DynamicResource PanelBackgroundBrush}"`, `BorderBrush="{DynamicResource PanelBorderBrush}"`, and `<Border.Effect>`.

* **`ProgressBar.tsx` -> `ProgressBarStyles.xaml`**
  * *React*: Flexbox wrapper, inner fill with `width: ${progress}%`.
  * *Noesis*: `<ProgressBar Value="{Binding ProgressValue}" Minimum="0" Maximum="100" Style="{StaticResource RA4ProgressBarStyle}"/>`.

### 2.2 Screen Views

* **`StartScreen.tsx` -> `StartScreen.xaml` & `URA4StartScreenViewModel`**
* **`MainMenu.tsx` -> `MainMenuScreen.xaml` & `URA4MainMenuViewModel`**
* **`CampaignSelect.tsx` -> `CampaignSelectScreen.xaml` & `URA4CampaignSelectViewModel`**
* **`FactionBriefing.tsx` -> `MissionBriefingScreen.xaml` & `URA4BriefingViewModel`**
* **`SkirmishScreen.tsx` -> `SkirmishSetupScreen.xaml` & `URA4SkirmishLobbyViewModel`**
* **`InGameHUD.tsx` -> `InGameHUD.xaml` & `URA4HUDViewModel`**

---

## 3. Data Binding & Event Flow Architecture

```
+------------------+         Data Binding          +-----------------------+
|  NoesisGUI View  |  ===========================> |    C++ UViewModel     |
|   (XAML Layout)  | <===========================  | (INotifyPropertyChanged)
+------------------+     PropertyChange Event      +-----------------------+
                                                               |
                                                       Command Execution
                                                               v
                                                   +-----------------------+
                                                   | UE5 Game Subsystem    |
                                                   | (Simulation Source)   |
                                                   +-----------------------+
```
