import { ArmorType, BuildingCategory, DamageType, FactionId, PassabilityType, TechTier, UnitCategory } from './enums.js';
export interface WeaponSpec {
    id: string;
    name: string;
    damageType: DamageType;
    baseDamage: number;
    range: number;
    cooldownTicks: number;
    splashRadius: number;
    minRange: number;
    projectileSpeed: number;
    targetsAir: boolean;
    targetsGround: boolean;
    targetsNaval: boolean;
}
export interface UnitSpec {
    id: string;
    legacyAlias?: string;
    name: string;
    factionId: FactionId;
    category: UnitCategory;
    tier: TechTier;
    cost: number;
    buildTimeSeconds: number;
    commandCapCost: number;
    hp: number;
    shield: number;
    armorType: ArmorType;
    speed: number;
    sightRange: number;
    weaponId?: string;
    secondaryWeaponId?: string;
    abilities: string[];
    prerequisites: string[];
    passability: PassabilityType;
    harvesterCapacity?: number;
}
export interface BuildingSpec {
    id: string;
    legacyAlias?: string;
    name: string;
    factionId: FactionId;
    category: BuildingCategory;
    tier: TechTier;
    cost: number;
    buildTimeSeconds: number;
    commandCapGranted: number;
    powerProduced: number;
    powerConsumed: number;
    hp: number;
    shield: number;
    armorType: ArmorType;
    sightRange: number;
    weaponId?: string;
    garrisonCapacity: number;
    prerequisites: string[];
    gridWidth: number;
    gridHeight: number;
    producesCategory?: UnitCategory;
}
export interface FactionSpec {
    id: FactionId;
    name: string;
    description: string;
    resourceName: string;
    resourceDescription: string;
    hqBuildingId: string;
    powerBuildingId: string;
    refineryBuildingId: string;
    barracksBuildingId: string;
    factoryBuildingId: string;
    techBuildingId: string;
    defenseBuildingId: string;
}
export interface ResourceNodeMapEntry {
    id: string;
    x: number;
    y: number;
    isRich: boolean;
    creditsRemaining: number;
}
export interface NeutralStructureMapEntry {
    id: string;
    type: 'OIL_DERRICK' | 'REPAIR_DEPOT' | 'OBSERVATION_POST';
    x: number;
    y: number;
}
export interface SpawnPointMapEntry {
    index: number;
    x: number;
    y: number;
}
export interface MapDefinition {
    id: string;
    name: string;
    width: number;
    height: number;
    maxPlayers: number;
    spawnPoints: SpawnPointMapEntry[];
    resourceNodes: ResourceNodeMapEntry[];
    neutralStructures: NeutralStructureMapEntry[];
    heightMap: number[][];
    passabilityGrid: number[][];
}
//# sourceMappingURL=content.d.ts.map