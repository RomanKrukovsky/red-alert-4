# Ability Implementation Matrix

Abilities are loaded from the bible as text descriptions. Each unit card has
an `abilities` array with the canonical description. Runtime GAS implementation
is pending — the data is loaded and accessible via `EntityDef::Abilities`.

| Unit ID | Abilities (from bible) | Runtime Status |
|---------|------------------------|----------------|
| SU_RubezhRifleman | “Dig in”: 2 sec preparation, +35% defense and -40% speed before canceling | Data loaded, GAS pending |
| SU_ZapalGrenadier | “Thermobaric charge”: knocks out the garrison and sets the building on fire; 28 sec | Data loaded, GAS pending |
| SU_ZaslonAATeam | “Aerial Ambush”: camouflages for 6 sec and receives +40% first salvo | Data loaded, GAS pending |
| SU_MasterEngineer | “Field repair”: restores 300 HP to vehicles in 8 seconds; "Capture": occupies neutral and enemy Buildings | Data loaded, GAS pending |
| SU_RazryadTrooper | “Overload”: chain lightning for 4 targets; 30 sec | Data loaded, GAS pending |
| SU_VektorOfficer | “Order No. 1”: allied Infantry in the radius receives +20% Damaged and immunity to suppression for 10 sec; 35 sec; Passively speeds up gaining Mobilization | Data loaded, GAS pending |
| SU_BogatyrOreCarrier | “Emergency Armor”: for 8 sec receives -40% of incoming damage; 50 sec | Data loaded, GAS pending |
| SU_RysScout | “Jump via obstacle”: short jump, 18 sec | Data loaded, GAS pending || SU_GranitMBT | “Bram”: accelerates and throws away light vehicles; 26 sec | Data loaded, GAS pending |
| SU_ZarevoMLRS | “Fire Square”: a salvo over a large area, leaving a fire; 38 sec | Data loaded, GAS pending |
| SU_GromoboyRam | “Ground Discharge”: a cone-shaped electric shock and a brief stun of vehicles; 32 sec | Data loaded, GAS pending |
| SU_VoevodaHeavyTank | “Siege Mode”: -60% speed, +30% range and Armor; 4 sec deployment | Data loaded, GAS pending |
| SU_KrechetInterceptor | “Fast and Furious”: +40% speed for 6 sec; 25 sec | Data loaded, GAS pending |
| SU_KorshunGunship | "Landing": transports 6 infantry; Circle of Fire: freezes and increases fire for 8 sec | Data loaded, GAS pending |
| SU_GromadaAirship | “Full Throttle”: +60% speed for 10 sec, then takes 20% damage; 50 sec | Data loaded, GAS pending |
| SU_BuranPatrolBoat | “Power Grid”: places an electric mine on the water; 25 sec | Data loaded, GAS pending |
| SU_MorokSubmarine | “Silent Move”: increased camouflage for 12 sec; 35 sec | Data loaded, GAS pending || SU_SvyatogorCruiser | "Barrage": 6 missiles over a wide area; 50 sec | Data loaded, GAS pending |
| SU_Hero_Morozova | Suppression Field: Enemies in the area lose speed and accuracy; 40 sec; “Team Impulse”: Allies instantly receive 20 Mobilization; 60 sec (+1 more) | Data loaded, GAS pending |
| AL_SentinelRifleman | "Flash charge": reduces enemy accuracy; 24 sec | Data loaded, GAS pending |
| AL_LancerTeam | “Laser tag”: the target receives +20% damage from all allies for 8 sec; 30 sec | Data loaded, GAS pending |
| AL_FieldEngineer | “Repair swarm”: drones repair equipment from a distance; 35 sec | Data loaded, GAS pending |
| AL_LongwatchSniper | “Hidden Observer”: camouflages himself motionless and increases visibility; 4 sec | Data loaded, GAS pending |
| AL_LifelineMedic | “Stabilization”: returns 40% HP to allied infantry for 6 sec; 30 sec | Data loaded, GAS pending |
| AL_FrostlineSpecialist | “Full Freeze”: immobilizes the target for 4 sec; 34 sec | Data loaded, GAS pending || AL_PioneerHarvester | “Expand outpost”: becomes a small repair and construction site; re-folding 10 sec | Data loaded, GAS pending |
| AL_KestrelScout | "Active Scan": reveals hidden goals; 20 sec | Data loaded, GAS pending |
| AL_BulwarkMBT | “Target Designator”: reduces the target’s armor by 20% for 8 sec; 28 sec | Data loaded, GAS pending |
| AL_OracleArtillery | “Synchronized salvo”: a powerful shot after 3 seconds of aiming; 36 sec | Data loaded, GAS pending |
| AL_RefractionTank | “Optical camouflage”: when stationary, becomes invisible; first shot +35% Damaged | Data loaded, GAS pending |
| AL_WardShieldCarrier | “Projection”: creates a directed shield for 12 sec; 32 sec | Data loaded, GAS pending |
| AL_CitadelTank | “Active defense”: intercepts 6 missiles/shells; 40 sec | Data loaded, GAS pending |
| AL_ShrikeInterceptor | “Interception”: instantly accelerates towards the selected air target; 24 sec | Data loaded, GAS pending |
| AL_VectorVTOL | “Vertical Ambush”: hangs behind the terrain and gets +25% first salvo | Data loaded, GAS pending || AL_NightveilBomber | "Shadow Mode": not detected by normal radar before reset; 45 sec | Data loaded, GAS pending |
| AL_MantaPatrolCraft | “Radio Suppression”: disables the Weapons of one ship for 5 seconds; 28 sec | Data loaded, GAS pending |
| AL_ResoluteDestroyer | “Sonar Pulse”: reveals submarines in the area; 30 sec | Data loaded, GAS pending |
| AL_HorizonCarrier | “Full Air Pack”: launches 8 drones in an area; 55 sec | Data loaded, GAS pending |
| AL_Hero_Hart | “Ghost Protocol”: complete disguise for 10 seconds; 45 sec; “Hack”: temporarily disables an enemy building or equipment; 50 sec (+1 more) | Data loaded, GAS pending |
| CO_QianweiRifleman | “Building”: next to two fighters, “Qianwei” receives +15% defense | Data loaded, GAS pending |
| CO_VajraLancer | “Impulse lunge”: a short jerk and disables light equipment for 2 seconds; 24 sec | Data loaded, GAS pending |
| CO_JieTechnician | “Link Node”: temporarily connects an isolated building to the network; "Repair Swarm": equipment repair | Data loaded, GAS pending || CO_ShengongMarksman | “Shield Break”: the next shot ignores the shield; 28 sec | Data loaded, GAS pending |
| CO_SanjivaniMedic | “Protective Shell”: gives 200 shield for 10 sec; 32 sec | Data loaded, GAS pending |
| CO_RakshaGuard | “Reflection”: 4 sec reflects 40% of ranged damage; 35 sec | Data loaded, GAS pending |
| CO_YuanCollector | “Energy Communication”: next to buildings gives +10 Synchronization | Data loaded, GAS pending |
| CO_KamakiriWalker | "Wall Step": overcomes small ledges and barricades | Data loaded, GAS pending |
| CO_QinglongMBT | “Shield Link”: connects the shields of adjacent tanks, distributing Damaged | Data loaded, GAS pending |
| CO_MonsoonArtillery | "Monsoon Front": turns around, gains +25% range and splits the projectile into three ammo | Data loaded, GAS pending |
| CO_SeimonShieldCarrier | “Dome”: creates a circular shield for 12 sec; 38 sec; Passive +10 Sync near 4+ units | Data loaded, GAS pending |
| CO_AiravataWalker | "Sky Leap": jumps over the front line and strikes with landing; 42 sec | Data loaded, GAS pending || CO_TianmenFortress | "Team Mode": stops, +20 Sync and repairs allies | Data loaded, GAS pending |
| CO_KawasemiDrone | Network Beacon: Increases Synchronization of visible allies | Data loaded, GAS pending |
| CO_LeiheGunship | “Protective Wing”: gives the allied group 150 shield; 35 sec | Data loaded, GAS pending |
| CO_AgnipakshaBomber | “Rebirth”: once per life with fatal damage returns to base with 25% HP | Data loaded, GAS pending |
| CO_KazekiriCorvette | “Cutting maneuver”: dash along the target, reduces its accuracy | Data loaded, GAS pending |
| CO_XuanwuCruiser | “Stabilized Shot”: penetrates several targets along a line; 38 sec | Data loaded, GAS pending |
| CO_SamudraCarrier | “Immersed Launch”: releases a mixed swarm without opening for 6 sec | Data loaded, GAS pending |
| CO_Hero_Mei | “Perfect Formation”: instantly gives the group maximum formation bonuses for 12 sec; Shield Transfer: Redirects allies' shields to the selected target; 40 sec (+1 more) | Data loaded, GAS pending || CH_ResonanceRifleman | Every fourth salvo is repeated via 0.6 sec with 50% damage | Data loaded, GAS pending |
| CH_PunctureLancer | “Phase Step”: becomes invulnerable for 1 second and passes through Units; 22 sec, 8 stability | Data loaded, GAS pending |
| CH_CausalityEngineer | “Rewind repairs”: returns the State building 6 seconds back; 40 sec, 15 stability | Data loaded, GAS pending |
| CH_ReversalMedic | “State return”: the ally returns the HP he had 5 seconds ago; 32 sec, 12 stability | Data loaded, GAS pending |
| CH_AporiaSniper | “Delayed Death”: Damaged triggers via 4 sec and doubles if the target receives a second shot; 30 sec | Data loaded, GAS pending |
| CH_CensorOperative | “Nullify”: disables the target’s active abilities for 8 sec; 38 sec, 18 stability; Can camouflage outside of combat | Data loaded, GAS pending |
| CH_ProbabilistHarvester | "Quantum Return": teleports to the recycler; 45 sec, 15 stability | Data loaded, GAS pending |
| CH_ParallaxScout | “Jump”: short-distance teleport; 12 sec, 6 stability | Data loaded, GAS pending || CH_TimelineTank | “Temporary Shell”: records Damaged for 6 sec, then returns 40% of lost HP; 34 sec, 14 stability | Data loaded, GAS pending |
| CH_DeltaDelayArtillery | “Delay Field”: the area slows down enemies by 50% for 8 sec; 40 sec, 18 stability | Data loaded, GAS pending |
| CH_PauseProjector | “Full stasis”: switches the target out of combat for 5 seconds; 42 sec, 22 stability | Data loaded, GAS pending |
| CH_EraEngine | “Afterimage”: creates a copy with 45% of the characteristics for 15 sec; 55 sec, 30 stability | Data loaded, GAS pending |
| CH_GapInterceptor | “Course break”: instantly changes position behind the target; 22 sec, 8 stability | Data loaded, GAS pending |
| CH_TrailGunship | “False swarm”: creates 3 non-attacking copies that disrupt targeting; 34 sec, 12 stability | Data loaded, GAS pending |
| CH_CriticalPointBomber | “Backward Wave”: after the explosion, enemies are attracted to the center; 48 sec, 24 stability | Data loaded, GAS pending |
| CH_IsobathFrigate | Tide Mark: Marks an area; allied ships gain +15% speed | Data loaded, GAS pending || CH_BathysSubmarine | "Deep Leap": teleport under water; 30 sec, 14 stability | Data loaded, GAS pending |
| CH_AttractorArk | “Sea Portal”: teleports before 6 allied ships to itself; 65 sec, 35 stability | Data loaded, GAS pending |
| CH_Hero_Voss | “State archive”: records the State of the group and can return it within 12 seconds; “Event Ban”: cancels one use of an enemy ability; 60 sec (+1 more) | Data loaded, GAS pending |
**Total: 78 units with abilities**