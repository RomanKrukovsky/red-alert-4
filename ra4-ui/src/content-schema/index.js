import { z } from 'zod';
import { ArmorType, BuildingCategory, DamageType, FactionId, PassabilityType, TechTier, UnitCategory } from '@ra4/shared-types';
export const WeaponSpecSchema = z.object({
    id: z.string(),
    name: z.string(),
    damageType: z.nativeEnum(DamageType),
    baseDamage: z.number().positive(),
    range: z.number().nonnegative(),
    cooldownTicks: z.number().int().positive(),
    splashRadius: z.number().nonnegative().default(0),
    minRange: z.number().nonnegative().default(0),
    projectileSpeed: z.number().nonnegative().default(0),
    targetsAir: z.boolean().default(false),
    targetsGround: z.boolean().default(true),
    targetsNaval: z.boolean().default(true)
});
export const UnitSpecSchema = z.object({
    id: z.string(),
    legacyAlias: z.string().optional(),
    name: z.string(),
    factionId: z.nativeEnum(FactionId),
    category: z.nativeEnum(UnitCategory),
    tier: z.nativeEnum(TechTier),
    cost: z.number().positive(),
    buildTimeSeconds: z.number().positive(),
    commandCapCost: z.number().int().positive(),
    hp: z.number().positive(),
    shield: z.number().nonnegative().default(0),
    armorType: z.nativeEnum(ArmorType),
    speed: z.number().positive(),
    sightRange: z.number().positive(),
    weaponId: z.string().optional(),
    secondaryWeaponId: z.string().optional(),
    abilities: z.array(z.string()).default([]),
    prerequisites: z.array(z.string()).default([]),
    passability: z.nativeEnum(PassabilityType).default(PassabilityType.Ground),
    harvesterCapacity: z.number().positive().optional()
});
export const BuildingSpecSchema = z.object({
    id: z.string(),
    legacyAlias: z.string().optional(),
    name: z.string(),
    factionId: z.nativeEnum(FactionId),
    category: z.nativeEnum(BuildingCategory),
    tier: z.nativeEnum(TechTier),
    cost: z.number().nonnegative(),
    buildTimeSeconds: z.number().nonnegative(),
    commandCapGranted: z.number().int().nonnegative().default(0),
    powerProduced: z.number().int().nonnegative().default(0),
    powerConsumed: z.number().int().nonnegative().default(0),
    hp: z.number().positive(),
    shield: z.number().nonnegative().default(0),
    armorType: z.nativeEnum(ArmorType),
    sightRange: z.number().positive(),
    weaponId: z.string().optional(),
    garrisonCapacity: z.number().int().nonnegative().default(0),
    prerequisites: z.array(z.string()).default([]),
    gridWidth: z.number().int().positive().default(2),
    gridHeight: z.number().int().positive().default(2),
    producesCategory: z.nativeEnum(UnitCategory).optional()
});
export const FactionSpecSchema = z.object({
    id: z.nativeEnum(FactionId),
    name: z.string(),
    description: z.string(),
    resourceName: z.string(),
    resourceDescription: z.string(),
    hqBuildingId: z.string(),
    powerBuildingId: z.string(),
    refineryBuildingId: z.string(),
    barracksBuildingId: z.string(),
    factoryBuildingId: z.string(),
    techBuildingId: z.string(),
    defenseBuildingId: z.string()
});
export const ResourceNodeMapEntrySchema = z.object({
    id: z.string(),
    x: z.number().int().nonnegative(),
    y: z.number().int().nonnegative(),
    isRich: z.boolean(),
    creditsRemaining: z.number().int().positive()
});
export const NeutralStructureMapEntrySchema = z.object({
    id: z.string(),
    type: z.enum(['OIL_DERRICK', 'REPAIR_DEPOT', 'OBSERVATION_POST']),
    x: z.number().int().nonnegative(),
    y: z.number().int().nonnegative()
});
export const SpawnPointMapEntrySchema = z.object({
    index: z.number().int().nonnegative(),
    x: z.number().int().nonnegative(),
    y: z.number().int().nonnegative()
});
export const MapDefinitionSchema = z.object({
    id: z.string(),
    name: z.string(),
    width: z.number().int().positive(),
    height: z.number().int().positive(),
    maxPlayers: z.number().int().positive(),
    spawnPoints: z.array(SpawnPointMapEntrySchema),
    resourceNodes: z.array(ResourceNodeMapEntrySchema),
    neutralStructures: z.array(NeutralStructureMapEntrySchema),
    heightMap: z.array(z.array(z.number())),
    passabilityGrid: z.array(z.array(z.number()))
});
export const ContentDatabaseSchema = z.object({
    factions: z.array(FactionSpecSchema),
    buildings: z.array(BuildingSpecSchema),
    units: z.array(UnitSpecSchema),
    weapons: z.array(WeaponSpecSchema),
    maps: z.array(MapDefinitionSchema)
});
//# sourceMappingURL=index.js.map