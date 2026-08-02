# Red Alert 4: NoesisGUI QA, Automation & Performance Testing Report

> **Status Notice**: Plugin Status: `BLOCKED_PLUGIN_MISSING`. `Plugins/NoesisGUI` is not installed in the repository. All test specifications and performance targets below are DECLARED specifications pending NoesisGUI plugin installation and editor verification.

---

## 1. Planned Automated Functional Test Suite (Status: DECLARED / BLOCKED)

* **XAML Import & Parse Test**: Test specification for verifying XAML assets in `Assets/Noesis/` import into Unreal Engine without missing resources or XML parsing errors (`UNTESTED - BLOCKED_PLUGIN_MISSING`).
* **Navigation Stack Test**: Planned automated test traversing Start Screen -> Main Menu -> Skirmish Setup -> Loading -> In-Game HUD -> Pause Menu -> Exit (`UNTESTED - BLOCKED_PLUGIN_MISSING`).
* **Localization Switching Test**: Planned test for runtime switching between Russian (`ru-RU`) and English (`en-US`) StringTables without UI reload artifacts (`UNTESTED - BLOCKED_PLUGIN_MISSING`).

---

## 2. Performance Budgets & Target Metrics (Status: DECLARED / UNVERIFIED)

| Metric | Target Budget | Measured Performance | Status |
| :--- | :--- | :--- | :--- |
| **UI Frame Time (CPU)** | < 1.5 ms per frame | N/A (Pending Plugin) | `DECLARED` |
| **Memory Overhead** | < 45 MB total | N/A (Pending Plugin) | `DECLARED` |
| **Binding Update Frequency** | Event-driven (0 tick allocations) | N/A (Pending Plugin) | `DECLARED` |
| **Unit Selection Scale Test (500 units)** | Stable 60+ FPS | N/A (Pending Plugin) | `DECLARED` |
