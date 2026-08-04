# Task & Milestone Dependency Graph (`DEPENDENCY_GRAPH.md`)

**Document Version**: 4.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Visual Dependency Graph (Mermaid)

```mermaid
graph TD
    M1T1[M1-T1: Data Path Fallback] --> Gate1[Gate 1: Architecture Baseline]
    M1T2[M1-T2: Noesis Guard] --> Gate1
    M1T3[M1-T3: Test Suite Run] --> Gate1

    Gate1 --> M2T1[M2-T1: Presentation Delta Queue]
    Gate1 --> M2T2[M2-T2: UMG HUD Integration]
    
    M2T1 --> Gate2[Gate 2: Industrial Vertical Slice]
    M2T2 --> Gate2

    Gate2 --> M3T1[M3-T1: 4 Factions Sim Logic]
    Gate2 --> M3T2[M3-T2: Superweapons & Destructible Maps]
    
    M3T1 --> Gate3[Gate 3: Systems Complete]
    M3T2 --> Gate3

    Gate3 --> M4T1[M4-T1: 78 PBR Models Import]
    Gate3 --> M4T2[M4-T2: 38 Campaign Missions Authoring]
    
    M4T1 --> Gate4[Gate 4: Content Complete]
    M4T2 --> Gate4

    Gate4 --> Gate5[Gate 5: Feature Complete] --> Gate6[Gate 6: Alpha] --> Gate7[Gate 7: Beta] --> Gate8[Gate 8: RC] --> Gate9[Gate 9: Gold Master]
```

---

## 2. Execution Dependency Rules

- **Rule 1**: Sub-agents MUST NOT claim or execute a task whose parent prerequisites in `DEPENDENCY_GRAPH.md` are not marked as `PASSED`.
- **Rule 2**: Tasks within the same milestone gate that touch different file paths MAY be executed in parallel by concurrent sub-agents.
