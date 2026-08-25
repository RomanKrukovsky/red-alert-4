# Project Current State Audit (`CURRENT_STATE.md`)

**Audit Date**: August 4, 2026  
**Repository**: `red-alert-4`  
**Engine Target**: Unreal Engine 5.8 (Custom Build)  
**Main Branch**: `main`  
**Latest Commit Hash**: `d0b7813` (`fix(slice): reconnect selection, presentation, combat visuals and attack-move`)  

---

## 1. Executive Summary

The Red Alert 4 (RA4) project is a deterministic real-time strategy (RTS) game built on a decoupled C++ simulation engine with an Unreal Engine 5 presentation layer.

### Primary Health Indicators
- **C++ Headless Test Suite**: **378 passed, 0 failed** (5.616s execution time).
  - `RA4Tests`: 258/258 passed
  - `RA4AITests`: 46/46 passed
  - `RA4InputTests`: 51/51 passed
  - `RA4PresentationTests`: 23/23 passed
- **Simulation Engine**: 100% functional, deterministic, decoupled from UObjects/Engine headers.
- **Unreal UI Stack**: **CommonUI + UMG + Slate** is the supported production stack; the obsolete web prototype was removed on August 25, 2026.

---

## 2. C++ Module Inventory (`Source/`)

| Module Name | Type | Loading Phase | Direct Dependencies | Health Status |
| :--- | :--- | :--- | :--- | :--- |
| `RA4Core` | Runtime | `EarliestPossible` | None (Pure C++) | **FULLY FUNCTIONAL** |
| `RA4Content` | Runtime | `PreDefault` | `RA4Core` | **FULLY FUNCTIONAL** |
| `RA4Simulation` | Runtime | `PreDefault` | `RA4Core`, `RA4Content` | **FULLY FUNCTIONAL** |
| `RA4Replay` | Runtime | `PreDefault` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RedAlert4` | Runtime | `Default` | `Engine`, `CoreUObject`, `RA4Core`, `RA4Simulation` | **FUNCTIONAL** |
| `RA4Combat` | Runtime | `Default` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RA4Navigation` | Runtime | `Default` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RA4Input` | Runtime | `Default` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RA4Presentation` | Runtime | `Default` | `Engine`, `CoreUObject`, `RA4Simulation` | **FUNCTIONAL** |
| `RA4FogOfWar` | Runtime | `Default` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RA4AI` | Runtime | `Default` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RA4Network` | Runtime | `Default` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RA4Campaign` | Runtime | `Default` | `RA4Core`, `RA4Simulation` | **FULLY FUNCTIONAL** |
| `RA4UI` | Runtime | `Default` | `UMG`, `CommonUI`, `ModelViewViewModel` | **FUNCTIONAL_NATIVE_STACK** |
| `RA4Editor` | Editor | `Default` | `UnrealEd`, `RA4Core` | **FUNCTIONAL** |
| `RA4Tests` | Test | `Default` | All modules | **378/378 PASS** |

---

## 3. High-Level Component Verification Summary

| Component | Status | Empirical Evidence |
| :--- | :--- | :--- |
| **Deterministic Command Bus** | **Working** | `CommandBus.DispatchTick` passes all tick isolation and order tests (`RA4Tests`). |
| **Lockstep Network Session** | **Working** | `LockstepSession` handles checksum validation, desync catching, stall recovery (`Lockstep.*` tests pass). |
| **State Hash Engine** | **Working** | `SimWorld::CalculateStateHash` produces 64-bit deterministic hashes across full matches. |
| **Replay Recorder/Player** | **Working** | Binary format replay recording and playback verified in `RA4Tests`. |
| **Economy & Harvesting** | **Working** | Harvester docking queues, ore field depletion, and resource accumulation tested in `ProvingGround`. |
| **AI Commander** | **Working** | `AICommander` strategy utility loop (Assault, Fortify, Tech, Economy) completes 5 skirmish scenarios deterministically. |
| **WASD & Camera Input** | **Working** | Screen panning, edge scroll, bounds clamping verified in `Camera.*` tests. |
| **Building Display & Placement** | **Partial** | `URA4BuildingPlacementController` handles grid validation; mesh rendering depends on blockout art mapping. |
| **Native UI Integration** | **Functional** | CommonUI routing, UMG widgets, Slate minimap/world markers, and C++ event snapshots are the supported production path. |
| **Packaged Game Build** | **Blocked** | Packaging remains unverified; no NoesisGUI dependency is involved. |

---

## 4. Key Metrics & Content Counts

- **Factions Defined**: 4 (Soviets, Alliance, Eastern Coalition, Chrono Legion).
- **Units in Bible Data**: 78 unique unit definitions (`ra4_content.normalized.json`).
- **Buildings in Bible Data**: 35 structure types with power consumption/production values.
- **Voice Lines / EVA**: 624 audio event triggers defined across factions.
- **FBX Blockout Models**: 142 generated FBX models present in `Content/RA4/Art/Blockout/`.
- **Campaign Missions**: 38 data-driven missions across 4 chapters.
