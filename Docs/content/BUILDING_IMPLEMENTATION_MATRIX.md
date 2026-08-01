# Building Implementation Matrix

Complete implementation status for all faction structures defined in `RA4_Factions_Units_Economy_Voice_Bible.md`.

## Soviet Buildings (13)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `SU_HQ` | Mobile Command Module / HQ | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_SU_HQ` |
| `SU_TeslaReactor` | Thermal Power Plant | T1 | 800 | 18s | +120 | Power generation | `DA_SU_TeslaReactor` |
| `SU_Refinery` | Ore plant | T1 | 2400 | 45s | -20 | Ore processing + Harvester | `DA_SU_Refinery` |
| `SU_Barracks` | Barracks mobilization | T1 | 700 | 18s | -15 | Infantry T1-T2, +5 Cap | `DA_SU_Barracks` |
| `SU_WarFactory` | Heavy Factory | T1 | 2300 | 42s | -40 | Vehicles, +10 Cap | `DA_SU_WarFactory` |
| `SU_Airfield` | Airfield long-range aviation | T2 | 1900 | 36s | -45 | Aircraft, 3 pads | `DA_SU_Airfield` |
| `SU_NavalYard` | Naval Dock | T2 | 2100 | 42s | -45 | Naval vessels | `DA_SU_NavalYard` |
| `SU_Radar` | Command Radar | T2 | 1500 | 30s | -60 | Minimap & T2 tech | `DA_SU_Radar` |
| `SU_TechLab` | Scientific complex "Grom" | T3 | 3600 | 62s | -100 | T3 tech | `DA_SU_TechLab` |
| `SU_Bunker` | Front line bunker | T1 | 1100 | 25s | -10 | Holds 5 infantry | `DA_SU_Bunker` || `SU_GunTurret` | Machine gun pillbox | T1 | 700 | 15s | -15 | Anti-infantry defense | `DA_SU_GunTurret` |
| `SU_TeslaCoil` | Reel "Perun" | T2 | 1900 | 35s | -75 | Heavy electric defense | `DA_SU_TeslaCoil` |
| `SU_NuclearSilo` | Punisher Missile Silo | T3 | 7000 | 110s | -220 | Superweapon strike | `DA_SU_NuclearSilo` |
## Alliance Buildings (13)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `AL_HQ` | Mobile HQ | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_AL_HQ` || `AL_PowerPlant` | Power Plant | T1 | 800 | 18s | +120 | Power generation | `DA_AL_PowerPlant` |
| `AL_Refinery` | Ore Processor | T1 | 2400 | 45s | -20 | Ore processing + Prospector | `DA_AL_Refinery` |
| `AL_Barracks` | Alliance Barracks | T1 | 700 | 18s | -15 | Infantry, +5 Cap | `DA_AL_Barracks` |
| `AL_Factory` | Factory technology | T1 | 2300 | 42s | -40 | Vehicles, +10 Cap | `DA_AL_Factory` |
| `AL_AirBase` | Airbase | T2 | 1900 | 36s | -45 | Aircraft | `DA_AL_AirBase` |
| `AL_NavalDock` | Marine dock | T2 | 2100 | 42s | -45 | Naval vessels | `DA_AL_NavalDock` |
| `AL_Uplink` | Satellite Node | T2 | 1500 | 30s | -60 | Intelligence & T2 tech | `DA_AL_Uplink` |
| `AL_TechCenter` | Technology Center | T3 | 3600 | 62s | -100 | T3 tech | `DA_AL_TechCenter` |
| `AL_Pillbox` | Alliance Dot | T1 | 700 | 15s | -15 | Anti-infantry defense | `DA_AL_Pillbox` |
| `AL_SpectrumTower` | Spectral Tower | T2 | 1900 | 35s | -75 | Spectrum beam defense | `DA_AL_SpectrumTower` |
| `AL_MultigunTurret` | Multi-barrel turret | T2 | 950 | 20s | -25 | Multigun defense | `DA_AL_MultigunTurret` || `AL_ChronoSphere` | Chronosphere | T3 | 7000 | 110s | -220 | Superweapon teleport | `DA_AL_ChronoSphere` |
## Eastern Coalition Buildings (13)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `CO_HQ` | Sky Command Node | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_CO_HQ` |
| `CO_Generator` | Coalition Generator | T1 | 800 | 18s | +120 | Power generation | `DA_CO_Generator` |
| `CO_Refinery` | Synchro Mine | T1 | 2400 | 45s | -20 | Ore processing + Collector | `DA_CO_Refinery` |
| `CO_Barracks` | Martial Arts Palace | T1 | 700 | 18s | -15 | Infantry, +5 Cap | `DA_CO_Barracks` |
| `CO_Factory` | Factory walkers | T1 | 2300 | 42s | -40 | Walkers & Tanks, +10 Cap | `DA_CO_Factory` |
| `CO_Airfield` | Air pavilion | T2 | 1900 | 36s | -45 | Aircraft | `DA_CO_Airfield` |
| `CO_NavalBase` | Mooring complex | T2 | 2100 | 42s | -45 | Naval vessels | `DA_CO_NavalBase` |
| `CO_SensorPost` | Touch post | T2 | 1500 | 30s | -60 | Sync sensor & T2 tech | `DA_CO_SensorPost` |
| `CO_TechNode` | Jade Knot | T3 | 3600 | 62s | -100 | T3 tech | `DA_CO_TechNode` |
| `CO_SentryNode` | Sensor Turret | T1 | 700 | 15s | -15 | Defense node | `DA_CO_SentryNode` |
| `CO_WaveTower` | Resonance Tower | T2 | 1900 | 35s | -75 | Wave defense | `DA_CO_WaveTower` || `CO_MissileBattery` | Missile complex | T2 | 950 | 20s | -25 | AA defense | `DA_CO_MissileBattery` |
| `CO_OrbitalLaser` | Orbital Emitter | T3 | 7000 | 110s | -220 | Superweapon laser | `DA_CO_OrbitalLaser` |
## ChronoLegion Buildings (11)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `CH_HQ` | Chrono-command node | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_CH_HQ` |
| `CH_MatrixGenerator` | Temporal Generator | T1 | 800 | 18s | +120 | Power generation | `DA_CH_MatrixGenerator` |
| `CH_Refinery` | Quantum receiver | T1 | 2400 | 45s | -20 | Ore processing + Quantum Harvester | `DA_CH_Refinery` |
| `CH_Barracks` | Phase fixer | T1 | 700 | 18s | -15 | Infantry, +5 Cap | `DA_CH_Barracks` |
| `CH_Factory` | Factory of temporary units | T1 | 2300 | 42s | -40 | Vehicles, +10 Cap | `DA_CH_Factory` |
| `CH_RiftPad` | Rift pilop | T2 | 1900 | 36s | -45 | Aircraft | `DA_CH_RiftPad` |
| `CH_TemporalDock` | Chrono-doc | T2 | 2100 | 42s | -45 | Naval vessels | `DA_CH_TemporalDock` |
| `CH_Beacon` | Chrono Beacon | T2 | 1500 | 30s | -60 | Temporal Beacon & T2 tech | `DA_CH_Beacon` |
| `CH_ParadoxCore` | Paradox Core | T3 | 3600 | 62s | -100 | T3 tech | `DA_CH_ParadoxCore` |
| `CH_StasisTurret` | Stasis Emitter | T1 | 700 | 15s | -15 | Stasis defense | `DA_CH_StasisTurret` || `CH_TimeEraser` | Time Eraser | T3 | 7000 | 110s | -220 | Superweapon time erase | `DA_CH_TimeEraser` |