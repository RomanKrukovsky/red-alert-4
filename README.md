# 🎮 Red Alert 4 (Working Title)

A real-time strategy (RTS) game prototype built on a **100% deterministic, engine-independent C++ simulation core** with **Unreal Engine 5** as the presentation layer.

> ⚠️ **Disclaimer.** "Red Alert 4" is an internal working title for demonstration purposes. The project holds no Electronic Arts license and contains zero EA assets, code, or copyrighted data. All faction names, terminology, and content live in data assets and localization keys.

---

## 🌟 Highlights & Features

- **Engine-Free C++ Core**: 100% deterministic simulation built with fixed-point math (`Fixed48.16`). Compiles in seconds and runs 242 automated tests without launching Unreal Engine.
- **Playable Skirmish Match Loop**: HQ construction, energy & credit economy, harvester mining loops, unit production queues, armor/warhead combat matrix, and AI commander profiles (Aggressive, Defensive, Economic, Adaptive).
- **Pathfinding & Formations**: Hierarchical NavGrid, FlowField navigation, ReservationGrid local avoidance, and squad formations.
- **RTS HUD & UI**: MVVM-based sidebar HUD with production cards, minimap fog of war, control groups, and match statistics.
- **AI Voice Lines**: Over 300 Russian voice lines generated using neural TTS (`VoxCPM`), covering 8 events per unit (`Selected`, `Move`, `Attack`, `Ability`, `Damaged`, `Elite`, `Idle`, `Death`).
- **AI Music**: Thematic background gameplay tracks generated with Suno AI.

---

## 🛠 Quick Start

### 1. Building and Running the Headless C++ Core (Fastest)

The simulation has zero Unreal dependencies. You can compile and run all 242 tests in a few seconds:

```bash
cmake -S Tools/HeadlessBuild -B build/hb -DCMAKE_BUILD_TYPE=Release
cmake --build build/hb -j8
./build/hb/RA4Tests
```

### 2. Launching in Unreal Engine 5 (UE 5.6 / 5.8)

1. Open `RedAlert4.uproject` in **Unreal Engine 5**.
2. Open the main skirmish level: `/Game/Maps/RA4_Skirmish_Production.umap`.
3. Click **Play (PIE)**:
   - **WASD / Mouse edges**: Pan RTS camera (scroll wheel to zoom).
   - **LMB**: Select units / drag-box group selection.
   - **RMB**: Contextual move / attack commands.
   - **Sidebar**: Queue buildings and units.

---

## 📁 Repository Structure

```
Source/RA4Core/         Fixed-point math (48.16), RNG, IDs, serialization
Source/RA4Content/      Unit data definitions, damage matrix, content hash validation
Source/RA4Simulation/   Match state, System-of-Arrays (SoA) storage, gameplay systems
Source/RA4Navigation/   NavGrid, FlowField, ReservationGrid, squad formations
Source/RA4AI/           AI Commander profiles (Doctrines, Operations, WorldView)
Source/RA4Replay/       Replay recorder, playback, checksum verification
Source/RedAlert4/       Unreal presentation layer (Entity Actors, Landscape, Camera)
Source/RA4UI/           RTS HUD (MVVM sidebar, minimap, building placement)
Tools/HeadlessBuild/    CMake harness for the engine-free modules
Audio/Voice/            Generated neural voice lines, manifests, and speaker profiles
Docs/                   Architecture ADRs, threat models, and implementation guides
```

---

## 🎨 3D AI Integration Guide

The gameplay mechanics and simulation are fully functional, while the visual layer currently uses primitive blockouts. The project is designed as an ideal "before & after" canvas for 3D AI workflows:
- **Concept Art**: Generate unit/building concepts using ChatGPT ImageGen.
- **3D Generation**: Convert concepts into 3D meshes using **Tripo3D** or **Hunyuan3D**.
- **Engine Setup**: Replace grey blockouts in `/Content/RA4/` with generated FBX models, PBR materials, and animations.
