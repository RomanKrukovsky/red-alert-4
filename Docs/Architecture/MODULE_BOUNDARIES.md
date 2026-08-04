# Module Boundary & Dependency Specification (`MODULE_BOUNDARIES.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Module Dependency Matrix

```
       [RedAlert4] ----> [RA4Presentation] ----> [RA4Simulation]
            |                    |                     |
            v                    v                     v
        [RA4UI] -------------> [RA4Input] ---------> [RA4Core]
                                                       ^
                                                       |
                                                 [RA4Content]
```

---

## 2. Strict Boundary Rules

1. **Rule 1 (Simulation Isolation)**: `RA4Simulation`, `RA4Core`, `RA4Content`, `RA4Combat`, `RA4Navigation`, `RA4AI`, and `RA4FogOfWar` must NEVER import Unreal Engine headers (`Engine.h`, `CoreUObject.h`, `AActor.h`).
2. **Rule 2 (Presentation One-Way Import)**: `RA4Presentation` may import `RA4Simulation` headers, but `RA4Simulation` must NEVER import `RA4Presentation` headers.
3. **Rule 3 (Fixed-Point Math Enforcement)**: All simulation math must use `FixedPoint.h`. Usage of `float` or `double` in `RA4Simulation` is prohibited.
