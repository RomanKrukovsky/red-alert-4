# RAAI - Red Alert AI System for Unreal Engine 5.8

A complete, free AI architecture for Red Alert-style RTS games built entirely with UE5.8 built-in systems.

## Architecture Overview

```
Strategic AI (C++)
├── HTN Planner (FAIHTNPlanner) - Multi-step planning
├── Utility AI (FAIUtilityScorer) - Decision scoring
├── Economy Manager (FAIEconomyManager) - Resources, income, power
├── Base Planner (FAIBasePlanner) - Building placement, build queue
├── Production Manager (FAIProductionManager) - Unit training queue
└── Intelligence Manager (FAIIntelManager) - Fog of war, probabilistic estimates, threat maps

Tactical AI (Behavior Trees + EQS + StateTree)
├── Army Commander - Group coordination
├── Squad AI - Formation, retreat, reinforce
└── EQS - Position/target selection for commanders only

Unit AI (StateTree + MassEntity)
├── StateTree - Individual unit behaviors
├── MassEntity - Mass infantry/drones (100-500+ units)
└── NavMesh + RVO - Movement & avoidance
```

## Quick Start

### 1. Enable Required Plugins
In your `.uproject` or via Editor > Plugins:
```json
{
  "Plugins": [
    { "Name": "MassEntity", "Enabled": true },
    { "Name": "MassAI", "Enabled": true },
    { "Name": "StateTreeModule", "Enabled": true },
    { "Name": "StateTreeAI", "Enabled": true },
    { "Name": "GameplayAbilities", "Enabled": true },
    { "Name": "GameplayTags", "Enabled": true }
  ]
}
```

### 2. Add Module Dependency
In your `Build.cs`:
```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "RAAI", "AIModule", "GameplayTasks", "NavigationSystem",
    "MassEntity", "MassAI", "StateTreeModule", "GameplayTags"
});
```

### 3. Place AI Director in Level
```cpp
// In your GameMode or LevelScriptActor BeginPlay:
AAIDirector* Director = GetWorld()->SpawnActor<AAIDirector>();
Director->InitializeDirector(EAIArchetype::USSR_HeavyAssault, 1);
```

Or via Blueprint:
1. Add `AAIDirector` to level
2. Set `Archetype` (USSR_HeavyAssault, Allied_ReconAir, Empire_TechTransform, Cautious, Aggressive, Guerrilla)
3. Set `PlayerIndex` (1 for AI player)

### 4. Register Units & Structures
```cpp
// In your unit/structure BeginPlay:
AAIDirector* Director = URAAIBlueprintLibrary::GetAIDirector(this);
if (Director)
{
    // For units
    URAAIBlueprintLibrary::RegisterUnit(Director, this, FGameplayTag::RequestGameplayTag("Unit.Tank.Medium"));

    // For structures
    URAAIBlueprintLibrary::RegisterStructure(Director, this, FGameplayTag::RequestGameplayTag("Structure.WarFactory"));
}
```

### 5. Report Enemy Sightings
```cpp
// From your vision/sensor system:
URAAIBlueprintLibrary::ReportEnemySighting(Director, EnemyLocation, FGameplayTag::RequestGameplayTag("Unit.Tank.Heavy"), 3, 0.9f);
URAAIBlueprintLibrary::ReportEnemyStructure(Director, StructureLocation, FGameplayTag::RequestGameplayTag("Structure.PowerPlant"), 0.8f);
```

## Archetypes

| Archetype | Playstyle | Priority Goals |
|-----------|-----------|----------------|
| `USSR_HeavyAssault` | Heavy armor push | Build WarFactory → Heavy Tanks → Attack |
| `Allied_ReconAir` | Air superiority | Build Airfield → Aircraft → Harass economy |
| `Empire_TechTransform` | Tech rush | Build TechCenter → Transform units → Defend tech |
| `Cautious` | Defensive | Build defenses → Accumulate → Counter-attack |
| `Aggressive` | Early rush | Early tanks → Constant pressure → Deny expansion |
| `Guerrilla` | Harassment | Fast units → Ambush harvesters → Hit & run |

## Gameplay Tags Reference

### Resources
- `Resource.Credits` - Main currency
- `Resource.Ore` - Raw ore
- `Resource.Power` - Current power usage
- `Resource.PowerCapacity` - Max power

### Structures
- `Structure.PowerPlant`
- `Structure.Refinery`
- `Structure.Barracks`
- `Structure.WarFactory`
- `Structure.Airfield`
- `Structure.TechCenter`
- `Structure.Defense.Turret`
- `Structure.Defense.AA`

### Units
- `Unit.Infantry`
- `Unit.Engineer`
- `Unit.Tank.Light`
- `Unit.Tank.Medium`
- `Unit.Tank.Heavy`
- `Unit.Artillery`
- `Unit.AntiAir`
- `Unit.Harvester`
- `Unit.Aircraft`
- `Unit.Transport`
- `Unit.Transform`
- `Unit.Fast`

### Strategic Goals
- `Strategic.Goal.BuildHeavyArmor`
- `Strategic.Goal.ControlOreFields`
- `Strategic.Goal.DestroyEnemyPower`
- `Strategic.Goal.BuildAirForce`
- `Strategic.Goal.ScoutEnemyBase`
- `Strategic.Goal.HarassEnemyEconomy`
- `Strategic.Goal.TechUpFast`
- `Strategic.Goal.BuildTransformUnits`
- `Strategic.Goal.DefendTechStructures`
- `Strategic.Goal.BuildDefenses`
- `Strategic.Goal.AccumulateResources`
- `Strategic.Goal.WaitForMistake`
- `Strategic.Goal.EarlyRush`
- `Strategic.Goal.ConstantPressure`
- `Strategic.Goal.DenyExpansion`
- `Strategic.Goal.Harassment`
- `Strategic.Goal.Ambushes`
- `Strategic.Goal.HitAndRun`

### Orders (for Army Groups)
- `Order.Idle`
- `Order.AttackMove`
- `Order.Defend`
- `Order.Scout`
- `Order.Patrol`
- `Order.Retreat`
- `Order.Reinforce`

## Blueprint API

All functionality exposed via `URAAIBlueprintLibrary`:

```cpp
// Economy
GetResourceAmount(Director, Resource.Credits)
CanAfford(Director, Resource.Credits, 1000)
GetNetIncome(Director, Resource.Credits)
IsPowerPositive(Director)
GetPowerRatio(Director)

// Intelligence
GetKnownEnemyUnitCount(Director, Unit.Tank)
GetKnownEnemyStructureCount(Director, Structure.PowerPlant)
IsEnemyPowerDown(Director)
GetEnemyPressureLevel(Director)
GetBaseWeaknessScore(Director)
AreHarvestersUnderAttack(Director)
IsExpansionSafe(Director)
GetEnemyProximityToBase(Director)

// Production
GetQueuedUnitCount(Director, Unit.Tank.Medium)
GetQueueProgress(Director, Unit.Tank.Heavy)
CanProduce(Director, Unit.Aircraft)

// Base
HasStructure(Director, Structure.WarFactory)
HasAvailableOreField(Director)
FindBestBuildLocation(Director, Structure.Defense.Turret)
```

## Extending the AI

### Add Custom HTN Methods
```cpp
// In your game module startup:
FAIHTNPlanner& Planner = Director->HTNPlanner;
Planner.RegisterMethod(FHTNMethod{
    FGameplayTag::RequestGameplayTag("Strategic.Goal.CustomGoal"),
    {
        FHTNTask{EHTNTaskType::BuildStructure, FGameplayTag::RequestGameplayTag("Structure.Custom")},
        FHTNTask{EHTNTaskType::TrainUnits, FGameplayTag::RequestGameplayTag("Unit.Custom"), 5}
    },
    2000.0f,
    {FGameplayTag::RequestGameplayTag("Structure.Prerequisite")}
});
```

### Add Custom Utility Factors
```cpp
FAIUtilityScorer& Scorer = Director->UtilityScorer;
Scorer.RegisterOption(FUtilityOption{
    FGameplayTag::RequestGameplayTag("Action.CustomAction"),
    {
        {FGameplayTag::RequestGameplayTag("Factor.CustomFactor"), 2.0f, 0.0f, 1.0f},
        {FGameplayTag::RequestGameplayTag("Factor.CreditAvailability"), 1.0f, 0.0f, 1.0f}
    },
    0.5f
});
```

### Custom Archetype
```cpp
// Extend EAIArchetype enum or use data-driven approach:
void AMyGameMode::SetupCustomAI(AAIDirector* Director)
{
    Director->Archetype = EAIArchetype::Aggressive; // Base
    Director->SetStrategicGoal(FGameplayTag::RequestGameplayTag("Strategic.Goal.Custom"), 10.0f);
    // Modify utility weights for personality
    Director->UtilityScorer.SetFactorWeight(
        FGameplayTag::RequestGameplayTag("Action.AttackEnemyBase"),
        FGameplayTag::RequestGameplayTag("Factor.ArmyStrength"), 3.0f);
}
```

## Performance Notes

- **Strategic AI**: Runs every 2 seconds (configurable `StrategicUpdateInterval`)
- **Tactical AI**: Runs every 0.5 seconds (configurable `TacticalUpdateInterval`)
- **MassEntity**: Use for 50+ identical units (infantry, drones)
- **EQS**: Only for squad commanders, NOT per-unit
- **StateTree**: Per-unit behavior, highly optimized

## Debugging

Enable logging:
```cpp
// In DefaultEngine.ini:
[Core.Log]
LogRAAI=Verbose
```

Or in code:
```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogRAAI, Log, All);
```

## Files Structure

```
Source/RAAI/
├── Public/
│   ├── AI/AAIDirector.h
│   ├── Planning/FAIHTNPlanner.h
│   ├── Planning/FAIUtilityScorer.h
│   ├── Managers/FAIEconomyManager.h
│   ├── Managers/FAIBasePlanner.h
│   ├── Managers/FAIProductionManager.h
│   ├── Intelligence/FAIIntelManager.h
│   └── RAAIBlueprintLibrary.h
└── Private/
    ├── AI/AAIDirector.cpp
    ├── Planning/FAIHTNPlanner.cpp
    ├── Planning/FAIUtilityScorer.cpp
    ├── Managers/FAIEconomyManager.cpp
    ├── Managers/FAIBasePlanner.cpp
    ├── Managers/FAIProductionManager.cpp
    ├── Intelligence/FAIIntelManager.cpp
    └── RAAIBlueprintLibrary.cpp
```

## Next Steps for Production

1. **StateTree Setup**: Create StateTree assets for Unit, Squad, Harvester, Aircraft
2. **Behavior Trees**: Build BTs for Army Commander (uses EQS)
3. **EQS Queries**: Create tests for SafeArtilleryPos, WeakBaseSpot, DropZone, AAPlacement
4. **MassEntity**: Define fragments & processors for mass infantry
5. **Navigation**: Configure NavMesh, NavModifiers for structures
6. **Multiplayer**: Replicate Director state via GameplayTags/NetSerialize

## License

MIT - Free to use in any project.