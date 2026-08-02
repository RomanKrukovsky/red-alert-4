import { ArmorType, FactionId, TechTier, UnitCategory, BuildingCategory, VeterancyRank } from './enums.js';
export interface ScaledVector2 {
    x: number;
    y: number;
}
export interface FixedBounds {
    minX: number;
    minY: number;
    maxX: number;
    maxY: number;
}
export interface PlayerEconomyState {
    credits: number;
    powerProduced: number;
    powerConsumed: number;
    powerLow: boolean;
    commandCapUsed: number;
    commandCapMax: number;
    factionResource: number;
    techTier: TechTier;
    hasHQ: boolean;
}
export interface ProductionQueueItem {
    id: string;
    itemType: 'UNIT' | 'BUILDING';
    specId: string;
    costTotal: number;
    costPaid: number;
    progressTicks: number;
    totalTicks: number;
}
export interface EntityStateSnapshot {
    id: number;
    specId: string;
    factionId: FactionId;
    playerIndex: number;
    category: UnitCategory | BuildingCategory;
    position: ScaledVector2;
    rotation: number;
    hp: number;
    maxHp: number;
    shield: number;
    maxShield: number;
    armorType: ArmorType;
    veterancy: VeterancyRank;
    isBuilding: boolean;
    isPowered: boolean;
    isDisabled: boolean;
    targetEntityId?: number;
    moveTarget?: ScaledVector2;
    currentOre: number;
    maxOre: number;
    productionQueue: ProductionQueueItem[];
}
export interface WorldSnapshot {
    tick: number;
    checksum: number;
    seed: number;
    entities: EntityStateSnapshot[];
    players: PlayerEconomyState[];
}
//# sourceMappingURL=simulation.d.ts.map