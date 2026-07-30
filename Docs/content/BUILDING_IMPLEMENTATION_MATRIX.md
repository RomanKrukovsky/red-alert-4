# Building Implementation Matrix

Complete implementation status for all faction structures defined in `RA4_Factions_Units_Economy_Voice_Bible.md`.

## Soviet Buildings (13)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `SU_HQ` | Мобильный командный модуль / Штаб | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_SU_HQ` |
| `SU_TeslaReactor` | Тепловая электростанция | T1 | 800 | 18s | +120 | Power generation | `DA_SU_TeslaReactor` |
| `SU_Refinery` | Рудный комбинат | T1 | 2400 | 45s | -20 | Ore processing + Harvester | `DA_SU_Refinery` |
| `SU_Barracks` | Казарма мобилизации | T1 | 700 | 18s | -15 | Infantry T1-T2, +5 Cap | `DA_SU_Barracks` |
| `SU_WarFactory` | Тяжёлый завод | T1 | 2300 | 42s | -40 | Vehicles, +10 Cap | `DA_SU_WarFactory` |
| `SU_Airfield` | Аэродром дальней авиации | T2 | 1900 | 36s | -45 | Aircraft, 3 pads | `DA_SU_Airfield` |
| `SU_NavalYard` | Военно-морской док | T2 | 2100 | 42s | -45 | Naval vessels | `DA_SU_NavalYard` |
| `SU_Radar` | Командный радар | T2 | 1500 | 30s | -60 | Minimap & T2 tech | `DA_SU_Radar` |
| `SU_TechLab` | Научный комплекс «Гром» | T3 | 3600 | 62s | -100 | T3 tech | `DA_SU_TechLab` |
| `SU_Bunker` | Бункер передовой | T1 | 1100 | 25s | -10 | Holds 5 infantry | `DA_SU_Bunker` |
| `SU_GunTurret` | Пулемётный дот | T1 | 700 | 15s | -15 | Anti-infantry defense | `DA_SU_GunTurret` |
| `SU_TeslaCoil` | Катушка «Перун» | T2 | 1900 | 35s | -75 | Heavy electric defense | `DA_SU_TeslaCoil` |
| `SU_NuclearSilo` | Ракетная шахта «Каратель» | T3 | 7000 | 110s | -220 | Superweapon strike | `DA_SU_NuclearSilo` |

## Alliance Buildings (13)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `AL_HQ` | Мобильный штаб | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_AL_HQ` |
| `AL_PowerPlant` | Электростанция | T1 | 800 | 18s | +120 | Power generation | `DA_AL_PowerPlant` |
| `AL_Refinery` | Переработчик руды | T1 | 2400 | 45s | -20 | Ore processing + Prospector | `DA_AL_Refinery` |
| `AL_Barracks` | Казарма Альянса | T1 | 700 | 18s | -15 | Infantry, +5 Cap | `DA_AL_Barracks` |
| `AL_Factory` | Завод техники | T1 | 2300 | 42s | -40 | Vehicles, +10 Cap | `DA_AL_Factory` |
| `AL_AirBase` | Авиабаза | T2 | 1900 | 36s | -45 | Aircraft | `DA_AL_AirBase` |
| `AL_NavalDock` | Морской док | T2 | 2100 | 42s | -45 | Naval vessels | `DA_AL_NavalDock` |
| `AL_Uplink` | Спутниковый узел | T2 | 1500 | 30s | -60 | Intelligence & T2 tech | `DA_AL_Uplink` |
| `AL_TechCenter` | Технологический центр | T3 | 3600 | 62s | -100 | T3 tech | `DA_AL_TechCenter` |
| `AL_Pillbox` | Дот Альянса | T1 | 700 | 15s | -15 | Anti-infantry defense | `DA_AL_Pillbox` |
| `AL_SpectrumTower` | Спектральная башня | T2 | 1900 | 35s | -75 | Spectrum beam defense | `DA_AL_SpectrumTower` |
| `AL_MultigunTurret` | Многоствольная турель | T2 | 950 | 20s | -25 | Multigun defense | `DA_AL_MultigunTurret` |
| `AL_ChronoSphere` | Хроносфера | T3 | 7000 | 110s | -220 | Superweapon teleport | `DA_AL_ChronoSphere` |

## Eastern Coalition Buildings (13)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `CO_HQ` | Небесный командный узел | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_CO_HQ` |
| `CO_Generator` | Генератор Коалиции | T1 | 800 | 18s | +120 | Power generation | `DA_CO_Generator` |
| `CO_Refinery` | Синхро-рудник | T1 | 2400 | 45s | -20 | Ore processing + Collector | `DA_CO_Refinery` |
| `CO_Barracks` | Дворец боевых искусств | T1 | 700 | 18s | -15 | Infantry, +5 Cap | `DA_CO_Barracks` |
| `CO_Factory` | Завод шагоходов | T1 | 2300 | 42s | -40 | Walkers & Tanks, +10 Cap | `DA_CO_Factory` |
| `CO_Airfield` | Воздушный павильон | T2 | 1900 | 36s | -45 | Aircraft | `DA_CO_Airfield` |
| `CO_NavalBase` | Причальный комплекс | T2 | 2100 | 42s | -45 | Naval vessels | `DA_CO_NavalBase` |
| `CO_SensorPost` | Сенсорный пост | T2 | 1500 | 30s | -60 | Sync sensor & T2 tech | `DA_CO_SensorPost` |
| `CO_TechNode` | Нефрилитовый узел | T3 | 3600 | 62s | -100 | T3 tech | `DA_CO_TechNode` |
| `CO_SentryNode` | Сенсорная турель | T1 | 700 | 15s | -15 | Defense node | `DA_CO_SentryNode` |
| `CO_WaveTower` | Резонансная башня | T2 | 1900 | 35s | -75 | Wave defense | `DA_CO_WaveTower` |
| `CO_MissileBattery` | Ракетный комплекс | T2 | 950 | 20s | -25 | AA defense | `DA_CO_MissileBattery` |
| `CO_OrbitalLaser` | Орбитальный излучатель | T3 | 7000 | 110s | -220 | Superweapon laser | `DA_CO_OrbitalLaser` |

## ChronoLegion Buildings (11)

| Building ID | Name (RU) | Tier | Cost | Build Time | Power | Purpose | C++ Data Asset |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `CH_HQ` | Хроно-командный узел | T1 | 5000 | 60s | +100 | HQ, +20 Cap | `DA_CH_HQ` |
| `CH_MatrixGenerator` | Темпоральный генератор | T1 | 800 | 18s | +120 | Power generation | `DA_CH_MatrixGenerator` |
| `CH_Refinery` | Квантовый приёмник | T1 | 2400 | 45s | -20 | Ore processing + Quantum Harvester | `DA_CH_Refinery` |
| `CH_Barracks` | Фазовый закрепитель | T1 | 700 | 18s | -15 | Infantry, +5 Cap | `DA_CH_Barracks` |
| `CH_Factory` | Фабрика временных единиц | T1 | 2300 | 42s | -40 | Vehicles, +10 Cap | `DA_CH_Factory` |
| `CH_RiftPad` | Разломный пилоп | T2 | 1900 | 36s | -45 | Aircraft | `DA_CH_RiftPad` |
| `CH_TemporalDock` | Хроно-док | T2 | 2100 | 42s | -45 | Naval vessels | `DA_CH_TemporalDock` |
| `CH_Beacon` | Хроно-маяк | T2 | 1500 | 30s | -60 | Temporal Beacon & T2 tech | `DA_CH_Beacon` |
| `CH_ParadoxCore` | Парадокс-ядро | T3 | 3600 | 62s | -100 | T3 tech | `DA_CH_ParadoxCore` |
| `CH_StasisTurret` | Стазисный излучатель | T1 | 700 | 15s | -15 | Stasis defense | `DA_CH_StasisTurret` |
| `CH_TimeEraser` | Стиратель времени | T3 | 7000 | 110s | -220 | Superweapon time erase | `DA_CH_TimeEraser` |
