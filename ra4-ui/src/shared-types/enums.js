export var FactionId;
(function (FactionId) {
    FactionId["USSR"] = "SU";
    FactionId["ALLIANCE"] = "AL";
    FactionId["ORIENTAL_COALITION"] = "CO";
    FactionId["CHRONOLEGION"] = "CH";
    FactionId["NEUTRAL"] = "NE";
})(FactionId || (FactionId = {}));
export var TechTier;
(function (TechTier) {
    TechTier[TechTier["T1"] = 1] = "T1";
    TechTier[TechTier["T2"] = 2] = "T2";
    TechTier[TechTier["T3"] = 3] = "T3";
})(TechTier || (TechTier = {}));
export var ArmorType;
(function (ArmorType) {
    ArmorType["LightInfantry"] = "LightInfantry";
    ArmorType["HeavyInfantry"] = "HeavyInfantry";
    ArmorType["LightVehicle"] = "LightVehicle";
    ArmorType["HeavyVehicle"] = "HeavyVehicle";
    ArmorType["SiegeVehicle"] = "SiegeVehicle";
    ArmorType["Air"] = "Air";
    ArmorType["Naval"] = "Naval";
    ArmorType["Structure"] = "Structure";
    ArmorType["Shield"] = "Shield";
})(ArmorType || (ArmorType = {}));
export var DamageType;
(function (DamageType) {
    DamageType["Ballistic"] = "Ballistic";
    DamageType["Shrapnel"] = "Shrapnel";
    DamageType["ArmorPiercing"] = "ArmorPiercing";
    DamageType["Siege"] = "Siege";
    DamageType["Electric"] = "Electric";
    DamageType["Plasma"] = "Plasma";
    DamageType["Cryo"] = "Cryo";
    DamageType["Temporal"] = "Temporal";
    DamageType["AntiAir"] = "AntiAir";
})(DamageType || (DamageType = {}));
export var UnitCategory;
(function (UnitCategory) {
    UnitCategory["Infantry"] = "Infantry";
    UnitCategory["Vehicle"] = "Vehicle";
    UnitCategory["Aircraft"] = "Aircraft";
    UnitCategory["Naval"] = "Naval";
    UnitCategory["Hero"] = "Hero";
})(UnitCategory || (UnitCategory = {}));
export var BuildingCategory;
(function (BuildingCategory) {
    BuildingCategory["HQ"] = "HQ";
    BuildingCategory["PowerPlant"] = "PowerPlant";
    BuildingCategory["Refinery"] = "Refinery";
    BuildingCategory["Barracks"] = "Barracks";
    BuildingCategory["Factory"] = "Factory";
    BuildingCategory["Airfield"] = "Airfield";
    BuildingCategory["NavalYard"] = "NavalYard";
    BuildingCategory["Radar"] = "Radar";
    BuildingCategory["TechLab"] = "TechLab";
    BuildingCategory["Defense"] = "Defense";
    BuildingCategory["Superweapon"] = "Superweapon";
    BuildingCategory["Capturable"] = "Capturable";
})(BuildingCategory || (BuildingCategory = {}));
export var CommandType;
(function (CommandType) {
    CommandType["MOVE"] = "MOVE";
    CommandType["ATTACK"] = "ATTACK";
    CommandType["ATTACK_MOVE"] = "ATTACK_MOVE";
    CommandType["STOP"] = "STOP";
    CommandType["HOLD"] = "HOLD";
    CommandType["PATROL"] = "PATROL";
    CommandType["GUARD"] = "GUARD";
    CommandType["BUILD_STRUCTURE"] = "BUILD_STRUCTURE";
    CommandType["PRODUCE_UNIT"] = "PRODUCE_UNIT";
    CommandType["CANCEL_PRODUCTION"] = "CANCEL_PRODUCTION";
    CommandType["SELL_STRUCTURE"] = "SELL_STRUCTURE";
    CommandType["REPAIR_STRUCTURE"] = "REPAIR_STRUCTURE";
    CommandType["USE_ABILITY"] = "USE_ABILITY";
    CommandType["CAPTURE_BUILDING"] = "CAPTURE_BUILDING";
    CommandType["DEPOSIT_ORE"] = "DEPOSIT_ORE";
    CommandType["SURRENDER"] = "SURRENDER";
})(CommandType || (CommandType = {}));
export var VeterancyRank;
(function (VeterancyRank) {
    VeterancyRank[VeterancyRank["Rookie"] = 0] = "Rookie";
    VeterancyRank[VeterancyRank["Veteran"] = 1] = "Veteran";
    VeterancyRank[VeterancyRank["Elite"] = 2] = "Elite";
    VeterancyRank[VeterancyRank["Heroic"] = 3] = "Heroic";
})(VeterancyRank || (VeterancyRank = {}));
export var PassabilityType;
(function (PassabilityType) {
    PassabilityType["Ground"] = "Ground";
    PassabilityType["Water"] = "Water";
    PassabilityType["Amphibious"] = "Amphibious";
    PassabilityType["Cliff"] = "Cliff";
    PassabilityType["Air"] = "Air";
})(PassabilityType || (PassabilityType = {}));
export var MatchState;
(function (MatchState) {
    MatchState["LOBBY"] = "LOBBY";
    MatchState["STARTING"] = "STARTING";
    MatchState["IN_GAME"] = "IN_GAME";
    MatchState["PAUSED"] = "PAUSED";
    MatchState["FINISHED"] = "FINISHED";
})(MatchState || (MatchState = {}));
export var PlayerType;
(function (PlayerType) {
    PlayerType["HUMAN"] = "HUMAN";
    PlayerType["AI_EASY"] = "AI_EASY";
    PlayerType["AI_MEDIUM"] = "AI_MEDIUM";
    PlayerType["AI_HARD"] = "AI_HARD";
    PlayerType["SPECTATOR"] = "SPECTATOR";
})(PlayerType || (PlayerType = {}));
export function assertNever(x) {
    throw new Error(`Unhandled discriminate variant: ${JSON.stringify(x)}`);
}
//# sourceMappingURL=enums.js.map