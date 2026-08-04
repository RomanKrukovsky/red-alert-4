# Industrial Vertical Slice Known Issues & Risk Register (`VERTICAL_SLICE_KNOWN_ISSUES.md`)

**Document Version**: 5.0  
**Evaluation Date**: August 4, 2026  
**Status**: **ALL ISSUES CLASSIFIED — NON-BLOCKING FOR GATE 5**  

---

## 1. Issue Classification Matrix

Issues are categorized by severity:
* **P0 (Blocker)**: Crash, desync, memory leak, game-breaking bug -> **0 Open Issues**.
* **P1 (High)**: Gameplay imbalance, UI cosmetic defect, minor audio delay -> **2 Tracked Issues**.
* **P2 (Medium)**: Technical debt, optimization refactoring for Milestone 3 -> **3 Tracked Issues**.

---

## 2. Tracked Non-Blocking Known Issues

| Issue ID | Severity | Category | Description | Workaround / Deferred Target |
| :--- | :---: | :--- | :--- | :--- |
| **ISSUE-VS-001** | P1 | UI / Visual | Mini-map fog update exhibits 1-frame latency during rapid camera jump | Scheduled for NoesisGUI texture buffer batching optimization in Milestone 3. |
| **ISSUE-VS-002** | P1 | Audio | Voice line queue can drop lower-priority chatter when 10 units are selected simultaneously | Priority voice queue throttling logic planned for Milestone 3 audio polish. |
| **ISSUE-VS-003** | P2 | Tech Debt | `BibleContentLoader.cpp` string-to-enum parsing uses linear lookup tables | Refactor to compile-time `std::array` lookup maps in Milestone 3. |
| **ISSUE-VS-004** | P2 | Tech Debt | `NoesisGUI` plugin directory optional inclusion requires manual preprocessor guard check when plugin is not installed | Managed via `#if WITH_NOESIS` preprocessor directives. |
| **ISSUE-VS-005** | P2 | Animation | Harvester docking rotation animation uses linear lerp instead of cubic Hermite curve | Deferred to Milestone 4 Art/Animation pass. |

---

## 3. Post-Gate Action Items

Upon passing Milestone Gate 5 (Vertical Slice), production transitions to **Milestone 2 (Industrial Vertical Slice Integration & Content Production)** without expanding core architecture.
