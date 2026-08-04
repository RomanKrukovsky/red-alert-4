# Domain Stream Ownership Matrix (`OWNERSHIP.md`)

**Document Version**: 4.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## Stream Ownership Mapping

| Domain Stream | Assigned Agent Role | Primary Input Contract | Primary Output Interface | Verification Suite |
| :--- | :--- | :--- | :--- | :--- |
| **1. Core Sim** | Simulation Architect | `RA4Core`, `RA4Content` | `SimWorld`, `CommandBus` | `RA4Tests` |
| **2. Gameplay** | Mechanics Lead | `SimWorld` | `EntityComp`, `SimTypes` | `RA4Tests` |
| **3. Economy** | Economy Systems Lead | Core Sim, JSON Bible | `HarvesterComp` | `ProvingGround` |
| **4. Construction** | Building Placement Lead | Core Sim, Land Grid | `URA4BuildingPlacement` | `RA4InputTests` |
| **5. Combat** | Combat Balance Lead | Core Sim, Armor Matrix | `RA4Combat` | `RA4Tests` |
| **6. Pathfinding** | Navigation Lead | Core Sim, Map Grid | `RA4Navigation` (Flowfield) | `RA4Tests` |
| **7. AI Systems** | AI Systems Lead | Core Sim, Fog of War | `RA4AI` (`AICommander`) | `RA4AITests` |
| **8. Networking** | Netcode Lead | Core Sim, UDP/TCP | `RA4Network` | `Lockstep.*` |
| **9. UE Integration**| Presentation Lead | Core Sim, UE Engine | `RA4Presentation` | `RA4PresentationTests` |
| **10. UI / UX** | UI/UX Lead | Presentation, ViewModels | `RA4UI`, UMG Widgets | `RA4PresentationTests` |
| **11. Tools** | Pipeline Automation Lead| Asset Registry | `Tools/` Scripts | Tool Smoke Tests |
| **12. Editor** | Map Editor Lead | UE Slate, Core Sim | `RA4Editor` | Editor Smoke Tests |
| **13. Maps** | Level Design Lead | Terrain Material, Props | `Content/Maps/` | Scene Validator |
| **14. Campaign** | Narrative Systems Lead | Data Bible, Mission Logic | `RA4Campaign` | `Campaign.*` |
| **15. Legal/IP** | Legal Counsel Agent | Trademarks, Licenses | `LEGAL_AND_LICENSES.md` | Legal Audit Check |
