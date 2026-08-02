export declare enum FactionId {
    USSR = "SU",
    ALLIANCE = "AL",
    ORIENTAL_COALITION = "CO",
    CHRONOLEGION = "CH",
    NEUTRAL = "NE"
}
export declare enum TechTier {
    T1 = 1,
    T2 = 2,
    T3 = 3
}
export declare enum ArmorType {
    LightInfantry = "LightInfantry",
    HeavyInfantry = "HeavyInfantry",
    LightVehicle = "LightVehicle",
    HeavyVehicle = "HeavyVehicle",
    SiegeVehicle = "SiegeVehicle",
    Air = "Air",
    Naval = "Naval",
    Structure = "Structure",
    Shield = "Shield"
}
export declare enum DamageType {
    Ballistic = "Ballistic",
    Shrapnel = "Shrapnel",
    ArmorPiercing = "ArmorPiercing",
    Siege = "Siege",
    Electric = "Electric",
    Plasma = "Plasma",
    Cryo = "Cryo",
    Temporal = "Temporal",
    AntiAir = "AntiAir"
}
export declare enum UnitCategory {
    Infantry = "Infantry",
    Vehicle = "Vehicle",
    Aircraft = "Aircraft",
    Naval = "Naval",
    Hero = "Hero"
}
export declare enum BuildingCategory {
    HQ = "HQ",
    PowerPlant = "PowerPlant",
    Refinery = "Refinery",
    Barracks = "Barracks",
    Factory = "Factory",
    Airfield = "Airfield",
    NavalYard = "NavalYard",
    Radar = "Radar",
    TechLab = "TechLab",
    Defense = "Defense",
    Superweapon = "Superweapon",
    Capturable = "Capturable"
}
export declare enum CommandType {
    MOVE = "MOVE",
    ATTACK = "ATTACK",
    ATTACK_MOVE = "ATTACK_MOVE",
    STOP = "STOP",
    HOLD = "HOLD",
    PATROL = "PATROL",
    GUARD = "GUARD",
    BUILD_STRUCTURE = "BUILD_STRUCTURE",
    PRODUCE_UNIT = "PRODUCE_UNIT",
    CANCEL_PRODUCTION = "CANCEL_PRODUCTION",
    SELL_STRUCTURE = "SELL_STRUCTURE",
    REPAIR_STRUCTURE = "REPAIR_STRUCTURE",
    USE_ABILITY = "USE_ABILITY",
    CAPTURE_BUILDING = "CAPTURE_BUILDING",
    DEPOSIT_ORE = "DEPOSIT_ORE",
    SURRENDER = "SURRENDER"
}
export declare enum VeterancyRank {
    Rookie = 0,
    Veteran = 1,
    Elite = 2,
    Heroic = 3
}
export declare enum PassabilityType {
    Ground = "Ground",
    Water = "Water",
    Amphibious = "Amphibious",
    Cliff = "Cliff",
    Air = "Air"
}
export declare enum MatchState {
    LOBBY = "LOBBY",
    STARTING = "STARTING",
    IN_GAME = "IN_GAME",
    PAUSED = "PAUSED",
    FINISHED = "FINISHED"
}
export declare enum PlayerType {
    HUMAN = "HUMAN",
    AI_EASY = "AI_EASY",
    AI_MEDIUM = "AI_MEDIUM",
    AI_HARD = "AI_HARD",
    SPECTATOR = "SPECTATOR"
}
export declare function assertNever(x: never): never;
//# sourceMappingURL=enums.d.ts.map