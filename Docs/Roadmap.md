# Red Alert 4 - state of the project

Last updated: 2026-07-28. This file records what is **built and verified**, what is
**scaffolded but unbuilt**, and what has **not been started**. It is the honest
counterpart to the design documents, which describe the target rather than reality.

Nothing is listed as done unless a command was actually run and its output checked.

## Verified working

Built with `cmake --build build/headless` and run as `./build/headless/RA4Tests`:
**51 tests, 51 passing, 0 failing** (also clean under `-fsanitize=address,undefined`).

| Area | State |
| --- | --- |
| Deterministic fixed-point math (48.16, CORDIC trig, integer sqrt) | working, unit tested against libm |
| Deterministic RNG (PCG-XSH-RR, rejection-sampled bounds) | working, unit tested |
| Canonical little-endian serialization with bounds-checked reads | working, fuzz-adjacent truncation tests |
| Stable entity handles (slot + generation) | working |
| Structure-of-arrays entity storage, fixed system order, 20 Hz tick | working |
| Command pipeline with full server-side validation and rate limiting | working |
| Build placement rules (footprint, terrain, build radius) | working |
| Production queues: pay, build, place, cancel with refund, rally points | working |
| Power grid, damaged plants derate, shortage slows production | working |
| Harvester economy: seek, gather, deliver, finite fields | working |
| Combat: armour/warhead table, hitscan and simulated projectiles, splash, turret tracking, min range | working |
| Movement: turn rates, acceleration, terrain blocking, order queues | working (straight-line steering only) |
| Victory / defeat / surrender | working |
| Replay record, serialize, load, verify against per-checkpoint checksums | working |
| Cross-build determinism | verified: identical checksum `edd01225b0d869ee` from `-O3` and from `-O0 + ASan + UBSan` |

The milestone acceptance scenario runs end to end: two players, headquarters placed,
power plant and refinery built and placed, bundled harvester gathers 6000 credits,
war factory built, four heavy tanks produced, assault ordered, enemy headquarters
destroyed, match ends with the correct winner, replay captured -- 2890 ticks
(144.5 s simulated) in 3 ms of wall clock.

## Scaffolded, not yet compiled

`RedAlert4.uproject`, the three `.Target.cs` files and the five `.Build.cs` files
exist and are written against UE 5.6. **They have not been run through UnrealBuildTool
yet**, so they are unverified. The C++ they reference is the same C++ the headless
harness already compiles.

## Not started

Massed navigation (hierarchical grid, sectors, portals, flow fields, reservation
grid, local avoidance, formations); fog of war; AI director and its profiles;
networking, dedicated server, lobby, spectator, reconnect; save/load; all UI;
campaigns and the mission graph editor; the map editor; modding; audio; every
visual asset; Eastern Coalition and Chrono Legion content; naval and air layers
(the data model has the layer enum, no systems use it yet); Gameplay Ability System
integration; MassEntity representation.

## Sequencing

Each stage ends in a buildable, tested state before the next begins.

1. **Done** - deterministic core plus the vertical slice above.
2. Navigation: hierarchical grid, flow fields for groups, local avoidance, layer
   masks, async budget, debug overlay. Target: 300 units repathing without a
   per-unit A*.
3. Presentation bridge: Actor/ISM representation driven by simulation events,
   interpolation between ticks, selection and order input, RTS camera.
4. Fog of war with dirty-region updates and server-side visibility filtering.
5. Networking: authoritative dedicated server, command batching, delta snapshots,
   reconnect, desync detection using the existing checksum.
6. AI: economy manager, base planner, production manager, scouting, tactical groups.
7. UI: CommonUI + MVVM, HUD, production sidebar, minimap, menus, localization.
8. Campaign and mission graph, editor tooling, modding, audio, content production.
