# Source Traceability Report

Traceability matrix linking every section of `RA4_Factions_Units_Economy_Voice_Bible.md` to C++ data assets, simulation modules, JSON schemas, voice manifests, and automated test cases.

| Source Bible Section | Line Range | Feature / Specification | C++ Source / Data Asset | Automated Test Verification |
| --- | --- | --- | --- | --- |
| **1. Общие правила игры** | L1 – L450 | Match Setup, 10k Credits, 50-200 Cap, Power Degradation | `ContentTypes.h`, `SimWorld.cpp` | `BibleContent.VerifyAllFourFactionsDefined` |
| **2. Матрица урона** | L451 – L520 | 9 Armor x 9 Damage Matrix Table | `DamageMatrix.h`, `ContentTypes.h` | `BibleContent.VerifyDamageMatrixMultipliers` |
| **3. Фракция: СССР** | L521 – L1210 | Mobilization (0-100), 19 Units, 13 Buildings, Voice Lines | `ContentTypes.h`, `voice_manifest.csv` | `BibleContent.Verify78UniqueUnitsInManifest` |
| **4. Фракция: Альянс** | L1211 – L1830 | Intelligence (0-100), 20 Units, 13 Buildings, Voice Lines | `ContentTypes.h`, `voice_manifest.csv` | `BibleContent.Verify78UniqueUnitsInManifest` |
| **4. Фракция: Коалиция** | L1831 – L2615 | Synchronization (0-100), 20 Units, 13 Buildings, Voice Lines | `ContentTypes.h`, `voice_manifest.csv` | `BibleContent.Verify78UniqueUnitsInManifest` |
| **4. Фракция: Хронолегион** | L2616 – L3420 | Temporal Stability (0-100), 19 Units, 11 Buildings, Voice Lines | `ContentTypes.h`, `voice_manifest.csv` | `BibleContent.Verify78UniqueUnitsInManifest` |
| **5. AI правила** | L3421 – L3460 | Target priorities, auto-retreat thresholds, formations | `RA4AI/AIStrategy.cpp` | `AI.BuildsProductionBuildingsAndTrainsAnArmy` |
| **8. Technical Spec** | L3464 – L3520 | Gameplay Tags, `voice_manifest.csv` | `voice_manifest.csv` | `BibleContent.VerifyVoiceManifestContains624Events` |
