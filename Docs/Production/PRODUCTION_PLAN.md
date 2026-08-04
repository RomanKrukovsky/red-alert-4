# Production Plan & Stream Specifications (`PRODUCTION_PLAN.md`)

**Document Version**: 4.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Production Domain Streams (27 Streams)

The production pipeline is divided into 27 specialized domain streams. Each stream operates under explicit ownership, dependency contracts, performance budgets, and binary definition of done criteria.

```
+-----------------------------------------------------------------------------------+
|                            CORE SIMULATION ENGINE GROUP                           |
|  1. Core Sim | 2. Gameplay | 3. Economy | 4. Construction | 5. Combat | 6. Pathing |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                           AI, NETWORKING & CONTENT GROUP                          |
|  7. AI | 8. Net | 9. UE Integration | 10. UI/UX | 11. Tools | 12. Editor | 13. Maps |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|                        ART, AUDIO & PRODUCTION SERVICES                           |
|  14. Campaign | 15. Narrative | 16. Art | 17. Anim | 18. VFX | 19. Audio | 20. Loc  |
|  21. Access | 22. QA | 23. Perf | 24. Sec | 25. Build | 26. Legal | 27. LiveOps   |
+-----------------------------------------------------------------------------------+
```

---

## 2. Detailed Stream Contracts (Selected Core Sample)

### Stream 1: Core Simulation (`RA4Simulation`)
- **Owner**: Simulation Architect Agent.
- **Input Dependencies**: `RA4Core`, `RA4Content`.
- **Output Interfaces**: `SimWorld`, `CommandBus`, `LockstepSession`.
- **Definition of Done**: 100% deterministic fixed-point 60Hz tick execution; zero floating-point math; passes all unit tests.
- **Test Strategy**: `RA4Tests` unit test suite + desync state hash verification.
- **Performance Budget**: <= 4.0 ms per 60Hz tick for 2,000 active entities.
- **Risks**: Accidental float usage introducing cross-platform non-determinism.
- **External Dependencies**: None (Pure C++).
- **Integration Order**: Priority 1 (Foundation).

### Stream 3: Economy & Resource Harvesting
- **Owner**: Economy Systems Agent.
- **Input Dependencies**: Core Sim, `ContentDatabase`.
- **Output Interfaces**: `HarvesterComp`, `RefineryComp`, `ResourceNodeComp`.
- **Definition of Done**: Harvester state machine handles mining, docking queues, unloading, and base power grid penalties without entity stacking.
- **Test Strategy**: Headless stress match simulation (`ProvingGround`).
- **Performance Budget**: <= 0.8 ms per tick for 200 active harvesters.
- **Risks**: Harvester pathing deadlocks near refinery docking ports.
- **Integration Order**: Priority 2.

### Stream 7: AI Commander (`RA4AI`)
- **Owner**: AI Systems Agent.
- **Input Dependencies**: Core Sim, `RA4Navigation`, `RA4FogOfWar`.
- **Output Interfaces**: `AICommander`, `HTNPlan`.
- **Definition of Done**: Bot operates utility decision loop deterministically via `CommandBus` adhering to zero-cheat fog of war.
- **Test Strategy**: `RA4AITests` (46/46 PASS).
- **Performance Budget**: <= 1.5 ms per tick across 7 AI commanders.
- **Integration Order**: Priority 3.

### Stream 10: User Interface (`RA4UI` & UMG)
- **Owner**: UI/UX Agent.
- **Input Dependencies**: Presentation Subsystem, `RA4HUDViewModel`.
- **Output Interfaces**: `RA4UIInputRouter`, UMG HUD Widgets.
- **Definition of Done**: HUD snapshot updates resource counters, minimap, selection portrait, and build cards without input click leaks.
- **Test Strategy**: `RA4PresentationTests` + UMG viewport validation.
- **Performance Budget**: <= 2.5 ms per frame Game Thread.
- **Integration Order**: Priority 4.
