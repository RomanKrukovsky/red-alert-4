# Content Architecture & Modding Boundaries (`CONTENT_ARCHITECTURE.md`)

**Document Version**: 3.0  
**Project Title**: *Iron Resonance: Command of Tomorrow*  

---

## 1. Content Pipeline Overview

Game definitions are decoupled from compiled C++ binaries. Unit stats, building power consumption, warhead multipliers, and voice lines are loaded at runtime from normalized JSON files.

```
[ Master Markdown Bible ] ---> [ Python Exporter ] ---> [ ra4_content.normalized.json ]
                                                                   |
                                                                   v
                                                     [ BibleContentLoader C++ ]
                                                                   |
                                                                   v
                                                        [ ContentDatabase ]
```

---

## 2. Modding Boundaries & Overrides

Modders can override default gameplay parameters by placing custom JSON override files in `%USERPROFILE%/Documents/IronResonance/Mods/Data/`.
- **Allowed Overrides**: Unit costs, build times, health, weapon range, armor multipliers, speed.
- **Banned Overrides**: Native C++ simulation tick rate (fixed at 60Hz), network packet framing, state hash algorithm.
