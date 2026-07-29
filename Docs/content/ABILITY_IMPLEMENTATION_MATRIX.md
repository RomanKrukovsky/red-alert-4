# Ability Implementation Matrix

Complete GAS and simulation ability registry across all 4 factions defined in `RA4_Factions_Units_Economy_Voice_Bible.md`.

## Active & Passive Abilities

| Faction | Entity ID | Ability Name | Target Policy | Cooldown | Effect Summary | GAS Gameplay Ability Tag | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Soviet** | `SU_Conscript` | Молотов | Area Ground | 15s | Fire splash damage over time | `Ability.Soviet.Conscript.Molotov` | VERIFIED |
| **Soviet** | `SU_ShockTrooper` | Тесла-заряд | Direct Unit | 20s | Chain electrical shock | `Ability.Soviet.ShockTrooper.TeslaCharge` | VERIFIED |
| **Soviet** | `SU_Buratino` | Огненный квадрат | Area Ground | 38s | Thermobaric area burn | `Ability.Soviet.Buratino.FireSquare` | VERIFIED |
| **Soviet** | `SU_TeslaRam` | Разряд по земле | Cone Ground | 32s | Stuns enemy vehicles in cone | `Ability.Soviet.TeslaRam.GroundDischarge` | VERIFIED |
| **Soviet** | `SU_Apocalypse` | Магнитный гарпун | Single Vehicle | 25s | Pulls enemy unit into saws | `Ability.Soviet.Apocalypse.MagneticHarpoon` | VERIFIED |
| **Soviet** | `SU_Kirov` | Индустриальный форсаж | Self | 45s | +40% speed, -10% HP/sec damage | `Ability.Soviet.Kirov.IndustrialAfterburner` | VERIFIED |
| **Soviet** | `SU_Hero_Morozova` | Поле «Красная звезда» | Area Self | 60s | Temporary invulnerability | `Ability.Soviet.Hero.RedStarField` | VERIFIED |
| **Alliance** | `AL_Peacekeeper` | Щит ОМОН | Self Toggle | 0s | Blocks 75% small arms damage, -30% speed | `Ability.Alliance.Peacekeeper.RiotShield` | VERIFIED |
| **Alliance** | `AL_Javelin` | Лазерное наведение | Single Target | 0s | Locks onto aircraft/armour, +50% accuracy | `Ability.Alliance.Javelin.LaserLock` | VERIFIED |
| **Alliance** | `AL_MirageTank` | Маскировка «Фантом» | Passive Self | 0s | Disguises as tree/structure when stationary | `Ability.Alliance.Mirage.PhantomDisguise` | VERIFIED |
| **Alliance** | `AL_CryoCopter` | Криогенный луч | Sustained Single | 0s | Freezes target, rendering fragile | `Ability.Alliance.CryoCopter.FreezeBeam` | VERIFIED |
| **Alliance** | `AL_Hero_Hart` | Орбитальный целеуказатель | Area Ground | 90s | Calls precision orbital strike | `Ability.Alliance.Hero.OrbitalTargeting` | VERIFIED |
| **Coalition** | `CO_Vanguard` | Резонансный щит | Self Passive | 0s | Shared barrier with adjacent units | `Ability.Coalition.Vanguard.ResonanceShield` | VERIFIED |
| **Coalition** | `CO_PhaseArcher` | Фазовый выстрел | Directional | 18s | Pierces through multiple enemies | `Ability.Coalition.PhaseArcher.PhaseShot` | VERIFIED |
| **Coalition** | `CO_Bastion` | Бастионное поле | Area Toggle | 10s | Projects shield sphere for allies | `Ability.Coalition.Bastion.Field` | VERIFIED |
| **Coalition** | `CO_Hero_Mei` | Нефритовая буря | Area Ground | 75s | High-damage plasma tempest | `Ability.Coalition.Hero.JadeTempest` | VERIFIED |
| **Chrono** | `CH_EchoRifleman` | Скачок в эхо | Instant Self | 12s | Teleports 500 units backward | `Ability.Chrono.Echo.Blink` | VERIFIED |
| **Chrono** | `CH_Rewinder` | Хроно-перемотка | Single Ally/Self | 30s | Restores HP and position 5s prior | `Ability.Chrono.Rewinder.TemporalRewind` | VERIFIED |
| **Chrono** | `CH_StasisProjector` | Купол стазиса | Area Ground | 40s | Pauses time for all units in sphere for 8s | `Ability.Chrono.Stasis.Dome` | VERIFIED |
| **Chrono** | `CH_Hero_Voss` | Стирание парадокса | Single Target | 90s | Erases target from timeline | `Ability.Chrono.Hero.ParadoxErasure` | VERIFIED |
