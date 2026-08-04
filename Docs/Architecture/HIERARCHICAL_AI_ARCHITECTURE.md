# Hierarchical AI Engine Architecture Blueprint (`HIERARCHICAL_AI_ARCHITECTURE.md`)

**Document Version**: 1.0  
**Project Title**: *Iron Resonance: Command of Tomorrow* (RA4)  
**Scope**: Industrial Multi-Layer AI Engine, Opponent Modeling, Forward Combat Simulation, Probabilistic Fog of War, and Auto-Tournament League.  

---

## 1. Executive Summary & Architectural Overview

The AI architecture for *Iron Resonance: Command of Tomorrow* avoids the "monolithic single-brain" antipattern. Instead, it implements a **5-Level Decision Hierarchy** paired with **6 Specialized Strategic Directors**, a **Probabilistic Belief Model**, a **Fast Forward Combat Simulator**, and an **Authentic APM-Capped Micro Subsystem**.

```
                           ┌──────────────────────────────────────────┐
                           │    1. SUPREME COMMANDER (Utility/HTN)    │
                           │  - Multi-plan storage (Plan A/B/C)       │
                           │  - Low frequency (2-10 sec tick loop)    │
                           └────────────────────┬─────────────────────┘
                                                │
       ┌────────────────────────────────────────┴────────────────────────────────────────┐
       ▼                                        ▼                                        ▼
┌──────────────┐                       ┌──────────────────┐                     ┌──────────────────┐
│  ECONOMY     │                       │   PRODUCTION     │                     │   INTELLIGENCE   │
│  DIRECTOR    │                       │    DIRECTOR      │                     │    DIRECTOR      │
│ - Cashflow 90s                       │ - Dynamic Build  │                     │ - Uncertainty    │
│ - Risk calc  │                       │   Order Planner  │                     │ - Probable FoW   │
└──────────────┘                       └──────────────────┘                     └──────────────────┘
       │                                        │                                        │
       ▼                                        ▼                                        ▼
┌──────────────┐                       ┌──────────────────┐                     ┌──────────────────┐
│   DEFENSE    │                       │    OFFENSE       │                     │    ABILITIES     │
│  DIRECTOR    │                       │    DIRECTOR      │                     │    DIRECTOR      │
│ - Threat maps│                       │ - Fast Forward   │                     │ - Superweapons   │
│ - Garrisons  │                       │   Combat Sim     │                     │ - Global Powers  │
└──────────────┘                       └──────────────────┘                     └──────────────────┘
                                                │
                                                ▼
                           ┌──────────────────────────────────────────┐
                           │    3. ARMY & SQUAD GROUP COMMANDERS      │
                           │  - Task bidding & role assignment        │
                           │  - Formations, routes, retreats          │
                           └────────────────────┬─────────────────────┘
                                                │
                                                ▼
                           ┌──────────────────────────────────────────┐
                           │   4. HUMAN-LIKE TACTICAL MICRO SYSTEM    │
                           │  - Focus fire, anti-AoE, range hold      │
                           │  - APM caps (120-350), reaction delays  │
                           └──────────────────────────────────────────┘
```

---

## 2. Detailed Component Specifications

### Level 1: Supreme Commander (Главнокомандующий)
* **Frequency**: Evaluates once every **2.0 to 10.0 seconds** or upon high-priority game events (HQ under attack, Superweapon launched, Tech tier completed).
* **Multi-Plan Storage**:
  - **Plan A (Primary)**: E.g., Fast armored pressure with RSU heavy tanks.
  - **Plan B (Contingency)**: Air superiority pivot if player builds heavy stationary defenses.
  - **Plan C (Emergency)**: Base evacuation & fallback if >40% base value destroyed.
* **Goal Tree Generator**: Issues high-level objectives with priority scoring to the Directors.

### Level 2: The 6 Strategic Directors (Стратегические директора)
1. **Economy Director**: Forecasts cashflow 90 seconds ahead, calculates harvester safety, expansion risks, and reserve funds.
2. **Production Director**: Dynamic Build Order planner with backward dependency resolution, parallel queue management, and rush emergency cancels.
3. **Intelligence Director**: Manages uncertainty grid, computes probability of enemy base locations, schedules scout budgets.
4. **Defense Director**: Computes threat maps, calculates arrival ETA for enemy forces, evaluates garrison sufficiency.
5. **Offense Director**: Selects strike targets using Value-Based Target Selection, runs the Forward Combat Simulator, chooses attack corridors.
6. **Abilities Director**: Controls Superweapons and tactical powers, ensuring high-value target thresholds (never wasting superweapons on single cheap units).

### Level 3: Probabilistic Fog-of-War Memory (Вероятностный туман войны)
The AI maintains a **Belief State Grid** instead of cheating:
* **Exact Knowledge**: Units/Structures observed `< 15` seconds ago.
* **Probable Knowledge**: Derived probabilities (e.g., `72% chance of enemy air tech` based on observed power usage).
* **Stale Knowledge**: Position of enemy army observed `45` seconds ago.
* **Hypothesis**: Suspected expansion in unexplored sector.

### Level 4: Forward Combat Simulator (Ускоренный симулятор боя)
Before committing an army to battle, the Offense Director passes force comps to a **Fast Headless Combat Predictor** running in `Source/RA4Simulation/`:
$$\text{Target Value} = \frac{\text{Value} \times P_{\text{win}} \times \text{Strategic Impact}}{\text{Expected Loss} + \text{Path Time} + \text{Risk}_{\text{reinforce}}}$$
* **Outputs**: Win Probability (%), Army Value Retention (%), Expected Enemy Losses (%), Fight Duration (s).

### Level 5: Task Bidding & Squad Commanders (Командиры отрядов и система заявок)
* Tasks (e.g. *Defend Eastern Harvesters*: Anti-Armor 40, Anti-Air 20, Speed 55, ETA 18s) are broadcast.
* Available Squads calculate a **Suitability Bid Score** (`0..100%`) and winning squad assumes the order.

### Level 6: Authentic APM-Capped Micro (Гуманное микро)
* **Enforced Human Limits**:
  - Easy: 60 APM cap, 800ms reaction delay.
  - Medium: 120 APM cap, 400ms reaction delay.
  - Hard: 220 APM cap, 200ms reaction delay.
  - Expert: 350 APM cap, 100ms reaction delay (zero FoW cheating).
* **Micro Behavior**: Focus-fire without overkill, kite damaged units back, spread against AoE, choke point holding.

---

## 3. Explaining & Debugging (AI Debug Overlay)

The in-engine **AI Debug Overlay** (`AIDebugOverlay.h`) visualizes the AI's internal state in real-time:
```
[ AI COMMANDER DEBUG OVERLAY ] - Player 2 (RSU)
Active Strategy: Air Pressure (Confidence: 78%)
Plan State: Plan A (Primary) | Emergency Threshold: 40% Base Loss
Current Goal: Destroy East Power Plant (Reason: Disables 42% Production)
Forward Combat Sim: Win Prob: 68% | Expected Duration: 18s | Loss Est: 22%
APM Budget: 180 / 220 Peak | Reaction Delay: 200ms
Threat Grid: Max Threat Sector (12, 44) | Value Grid: High Value Sector (8, 56)
```

---

## 4. Implementation Roadmap for Red Alert 4

```
[ Phase 1: Fog-of-War Memory & Belief Grid ] ──► Implement IAIWorldView Probabilistic Grid
        │
        ▼
[ Phase 2: Supreme Commander & Multi-Plan Storage ] ──► Expand AICommander HTN / Utility
        │
        ▼
[ Phase 3: The 6 Strategic Directors ] ──► Create Economy, Production, Intel, Defense, Offense, Ability Directors
        │
        ▼
[ Phase 4: Forward Combat Predictor ] ──► Integrate Headless Fast Combat Simulation Kernel
        │
        ▼
[ Phase 5: Task Bidding & Squad Commanders ] ──► Implement Squad Group Bidding System
        │
        ▼
[ Phase 6: AI vs AI Self-Play League ] ──► Headless Auto-Tournament Benchmarking Engine
```

---

## 5. Architectural Invariants

1. **Deterministic Execution**: All AI state mutations occur in fixed-point 60Hz tick steps inside `Source/RA4AI/`.
2. **Zero Cheat Integrity**: AI reads world state strictly through `IAIWorldView` fog-filtered views and emits valid `Command` objects.
3. **No External Runtime API Lock-in**: All runtime AI calculations execute locally in pure C++ with zero web API latency.
