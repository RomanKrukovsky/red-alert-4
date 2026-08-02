# Red Alert 4: NoesisGUI Input & Navigation Architecture

## 1. Navigation Flow & Stack Management

```
[Start Screen] ---> [Main Menu] ---> [Campaign Select] ---> [Mission Briefing] ---> [Game HUD]
                                ---> [Skirmish Lobby] ----------------------------> [Game HUD]
                                ---> [Settings Modal] (Overlay Stack)
```

* **Stack Management**: `URA4UINavigationService` maintains an explicit navigation stack. Pressing `Escape` pops the top non-permanent screen or opens the Pause Menu when in-game.
* **Modal Dialogs**: Modals are pushed to a high-priority overlay layer (`ZIndex = 5000`) and capture input focus until dismissed.

---

## 2. Enhanced Input & Hotkey Mapping

* **RTS Hotkeys**:
  * `WASD` / `Arrow Keys`: Camera Pan (handled by Game Input Router when input mode is `GameAndUI`).
  * `A` + LeftClick: Attack Move.
  * `S`: Stop / Hold Position.
  * `1 - 0` / `Ctrl + 1 - 0`: Control Groups selection and assignment.
  * `Tab`: Cycle active production tab in HUD.
  * `Space`: Jump to latest EVA alert location.

* **Focus Isolation**: When an `<TextBox>` or chat input field is focused, all RTS hotkeys are suppressed until focus is cleared or `Enter`/`Escape` is pressed.
