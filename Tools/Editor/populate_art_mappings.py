import unreal

DA_PATH = "/Game/RA4/Art/Generated/DA_RA4_ArtMappings"
A = "/Game/RA4/Art"  # root

# content id -> static mesh path (authored first, blockout fallback)
UNIT_MAP = {
    # --- Soviet ---
    "unit.sov.combat_engineer": f"{A}/Blockout/Soviet/SM_Soviet_SU_MasterEngineer_Blockout",
    "unit.all.combat_engineer": f"{A}/Blockout/Alliance/SM_Alliance_AL_FieldEngineer_Blockout",
    "unit.ec.combat_engineer":  f"{A}/Blockout/Coalition/SM_Coalition_CO_QianweiRifleman_Blockout",
    "unit.cl.combat_engineer":  f"{A}/Blockout/Chronolegion/SM_Chronolegion_CH_CausalityEngineer_Blockout",
    "unit.sov.mcv":             f"{A}/Blockout/Soviet/SM_Soviet_SU_MCV_MobileYard_Blockout",
    "unit.sov.conscript":       f"{A}/Blockout/Soviet/SM_Soviet_SU_RubezhRifleman_Blockout",
    "unit.sov.rocket_trooper":  f"{A}/Blockout/Soviet/SM_Soviet_SU_RazryadTrooper_Blockout",
    "unit.sov.heavy_tank":      f"{A}/Units/Soviet/SM_Soviet_SU_GranitMBT",
    "unit.sov.ore_harvester":   f"{A}/Units/Soviet/SM_Soviet_SU_BogatyrOreCarrier",
    "unit.sov.zarevo_mlrs":     f"{A}/Units/Soviet/SM_Soviet_SU_ZarevoMLRS",
    "unit.sov.mig_bomber":      f"{A}/Blockout/Soviet/SM_Soviet_SU_KrechetInterceptor_Blockout",
    # --- Alliance ---
    "unit.all.mcv":             f"{A}/Blockout/Alliance/SM_Alliance_AL_MCV_MobileNode_Blockout",
    "unit.all.rifleman":        f"{A}/Blockout/Alliance/SM_Alliance_AL_SentinelRifleman_Blockout",
    "unit.all.missile_infantry":f"{A}/Blockout/Alliance/SM_Alliance_AL_LancerTeam_Blockout",
    "unit.all.light_tank":      f"{A}/Units/Alliance/SM_Alliance_AL_BulwarkMBT",
    "unit.all.ore_harvester":   f"{A}/Units/Alliance/SM_Alliance_AL_PioneerHarvester",
    "unit.all.oracle_artillery":f"{A}/Units/Alliance/SM_Alliance_AL_OracleArtillery",
    "unit.all.harrier_jet":     f"{A}/Blockout/Alliance/SM_Alliance_AL_ShrikeInterceptor_Blockout",
    # --- Coalition (ec) ---
    "unit.ec.mcv":              f"{A}/Blockout/Coalition/SM_Coalition_CO_MCV_MobileNode_Blockout",
    "unit.ec.qianwei_rifleman": f"{A}/Blockout/Coalition/SM_Coalition_CO_QianweiRifleman_Blockout",
    "unit.ec.qinglong_mbt":     f"{A}/Units/Coalition/SM_Coalition_CO_QinglongMBT",
    "unit.ec.harmony_harvester":f"{A}/Units/Coalition/SM_Coalition_CO_YuanCollector",
    "unit.ec.longbow_artillery":f"{A}/Units/Coalition/SM_Coalition_CO_MonsoonArtillery",
    "unit.ec.vajra_lancer":     f"{A}/Units/Coalition/SM_Coalition_CO_KamakiriWalker",
    "unit.ec.phoenix_gunship":  f"{A}/Blockout/Coalition/SM_Coalition_CO_KamakiriWalker_Blockout",
    # --- Chronolegion (cl) ---
    "unit.cl.mcv":              f"{A}/Blockout/Chronolegion/SM_Chronolegion_CH_MCV_MobileArk_Blockout",
    "unit.cl.resonance_rifleman": f"{A}/Blockout/Chronolegion/SM_Chronolegion_CH_CensorOperative_Blockout",
    "unit.cl.timeline_tank":    f"{A}/Units/Chronolegion/SM_Chronolegion_CH_TimelineTank",
    "unit.cl.echo_harvester":   f"{A}/Units/Chronolegion/SM_Chronolegion_CH_ProbabilistHarvester",
    "unit.cl.entropy_mortar":   f"{A}/Units/Chronolegion/SM_Chronolegion_CH_DeltaDelayArtillery",
    "unit.cl.phase_striker":    f"{A}/Units/Chronolegion/SM_Chronolegion_CH_ParallaxScout",
    "unit.cl.paradox_lancer":   f"{A}/Blockout/Chronolegion/SM_Chronolegion_CH_PunctureLancer_Blockout",
}

FACTION_SHORT = {"Soviet": "SU", "Alliance": "AL", "Coalition": "CO", "Chronolegion": "CH"}

def B(faction, authored, blockout):
    if authored:
        return f"{A}/Buildings/{faction}/SM_{faction}_{authored}"
    return f"{A}/Blockout/{faction}/SM_{faction}_{FACTION_SHORT[faction]}_{blockout}_Blockout"

BUILDING_MAP = {
    "building.sov.construction_yard": B("Soviet", "SU_ConYard", None),
    "building.sov.tesla_reactor":     B("Soviet", "SU_PowerPlant", None),
    "building.sov.ore_refinery":      B("Soviet", "SU_Refinery", None),
    "building.sov.war_factory":       B("Soviet", "SU_WarFactory", None),
    "building.sov.barracks":          B("Soviet", "SU_Barracks", None),
    "building.sov.gun_turret":        B("Soviet", None, "GunTurret"),
    "building.sov.flak_turret":       B("Soviet", None, "AATurret"),
    "building.sov.radar_complex":     B("Soviet", None, "Radar"),
    "building.all.construction_yard": B("Alliance", "AL_ConYard", None),
    "building.all.power_plant":       B("Alliance", "AL_PowerPlant", None),
    "building.all.ore_refinery":      B("Alliance", "AL_Refinery", None),
    "building.all.war_factory":       B("Alliance", "AL_WarFactory", None),
    "building.all.barracks":          B("Alliance", "AL_Barracks", None),
    "building.all.patriot_battery":   B("Alliance", None, "GunTurret"),
    "building.all.pillbox":           B("Alliance", None, "GunTurret"),
    "building.all.radar_complex":     B("Alliance", None, "Radar"),
    "building.all.aegis_lance":       B("Alliance", None, "PrismTower"),
    "building.ec.construction_yard":  B("Coalition", "CO_ConYard", None),
    "building.ec.solar_collector":    B("Coalition", "CO_PowerPlant", None),
    "building.ec.ore_synthesizer":    B("Coalition", "CO_Refinery", None),
    "building.ec.war_factory":        B("Coalition", "CO_WarFactory", None),
    "building.ec.barracks":           B("Coalition", "CO_Barracks", None),
    "building.ec.aa_tower":           B("Coalition", None, "AATurret"),
    "building.ec.defense_tower":      B("Coalition", None, "DefenseTower"),
    "building.ec.radar_array":        B("Coalition", None, "Radar"),
    "building.ec.helipad":            B("Coalition", None, "Helipad"),
    "building.ec.harmony_sanctuary":  B("Coalition", None, "HarmonySanctuary"),
    "building.cl.construction_yard":  B("Chronolegion", "CH_ConYard", None),
    "building.cl.decay_reactor":      B("Chronolegion", "CH_PowerPlant", None),
    "building.cl.rift_pad":           B("Chronolegion", "CH_Refinery", None),
    "building.cl.war_factory":        B("Chronolegion", "CH_WarFactory", None),
    "building.cl.barracks":           B("Chronolegion", "CH_Barracks", None),
    "building.cl.temporal_turret":    B("Chronolegion", None, "EchoTurret"),
    "building.cl.warp_flak":          B("Chronolegion", None, "AATurret"),
    "building.cl.chronoscan_array":   B("Chronolegion", None, "Radar"),
    "building.cl.chronosphere":       B("Chronolegion", None, "PauseProjector"),
    "building.cl.causality_center":   B("Chronolegion", None, "CausalityAnchor"),
}

def asset_exists(obj_path):
    try:
        return unreal.load_asset(obj_path) is not None
    except Exception:
        return False

da = unreal.load_asset(DA_PATH + "." + "DA_RA4_ArtMappings")
if da is None:
    unreal.log_warning("[ArtPopulate] DA missing")
    raise SystemExit(1)

units_map = da.get_editor_property("Units")
buildings_map = da.get_editor_property("Buildings")

added_units = 0
missing_units = []
for cid, mesh in UNIT_MAP.items():
    if not asset_exists(mesh):
        missing_units.append((cid, mesh))
        continue
    entry = unreal.RA4UnitArtDefinition()
    entry.set_editor_property("static_mesh", unreal.load_asset(mesh))
    units_map[cid] = entry
    added_units += 1

added_bld = 0
missing_bld = []
for cid, mesh in BUILDING_MAP.items():
    if not asset_exists(mesh):
        missing_bld.append((cid, mesh))
        continue
    entry = unreal.RA4BuildingArtDefinition()
    entry.set_editor_property("stage4_active_mesh", unreal.load_asset(mesh))
    buildings_map[cid] = entry
    added_bld += 1

da.set_editor_property("Units", units_map)
da.set_editor_property("Buildings", buildings_map)
unreal.EditorAssetLibrary.save_loaded_asset(da)

unreal.log_warning(f"[ArtPopulate] units mapped: {added_units}, buildings mapped: {added_bld}")
for cid, m in missing_units:
    unreal.log_warning(f"[ArtPopulate] MISSING unit mesh for {cid}: {m}")
for cid, m in missing_bld:
    unreal.log_warning(f"[ArtPopulate] MISSING building mesh for {cid}: {m}")
print(f"DONE units={added_units} buildings={added_bld} missing_u={len(missing_units)} missing_b={len(missing_bld)}")
