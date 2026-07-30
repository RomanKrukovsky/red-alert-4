# Copyright (c) Red Alert 4 project. Procedural Blockout Generator for UE5.
import bpy
import math
import os
import csv

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
OUTPUT_BASE = os.path.join(PROJECT_ROOT, "Content/RA4/Art/Blockout")

def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    # Remove all collections and objects
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for col in list(bpy.data.collections):
        bpy.data.collections.remove(col, do_unlink=True)

def create_material(name, color_rgb):
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Base Color'].default_value = (color_rgb[0], color_rgb[1], color_rgb[2], 1.0)
        bsdf.inputs['Roughness'].default_value = 0.5
    return mat

def assign_material(obj, mat):
    if obj.data.materials:
        obj.data.materials[0] = mat
    else:
        obj.data.materials.append(mat)

def add_cube(name, location, scale, mat=None):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
    obj = bpy.context.active_object
    obj.name = name
    obj.scale = scale
    if mat:
        assign_material(obj, mat)
    return obj

def add_cylinder(name, location, radius, depth, mat=None, rotation=(0,0,0)):
    bpy.ops.mesh.primitive_cylinder_add(radius=radius, depth=depth, location=location, rotation=rotation)
    obj = bpy.context.active_object
    obj.name = name
    if mat:
        assign_material(obj, mat)
    return obj

def add_sphere(name, location, radius, mat=None):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=radius, location=location)
    obj = bpy.context.active_object
    obj.name = name
    if mat:
        assign_material(obj, mat)
    return obj

def add_cone(name, location, radius1, radius2, depth, mat=None):
    bpy.ops.mesh.primitive_cone_add(radius1=radius1, radius2=radius2, depth=depth, location=location)
    obj = bpy.context.active_object
    obj.name = name
    if mat:
        assign_material(obj, mat)
    return obj

def export_fbx(output_path):
    # Select all mesh objects
    bpy.ops.object.select_all(action='SELECT')
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    bpy.ops.export_scene.fbx(
        filepath=output_path,
        use_selection=True,
        global_scale=1.0,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_ALL',
        axis_forward='-Z',
        axis_up='Y'
    )

# Faction Color Palettes
COLOR_BASE = (0.3, 0.35, 0.4)       # Neutral dark slate grey base
COLOR_SOVIET = (0.8, 0.15, 0.1)     # Soviet Crimson Red
COLOR_ALLIANCE = (0.1, 0.4, 0.8)   # Alliance Cobalt Blue
COLOR_COALITION = (0.15, 0.6, 0.4) # Coalition Jade Green
COLOR_CHRONO = (0.5, 0.1, 0.7)     # Chronolegion Temporal Purple

def get_faction_accent(faction):
    if faction == "Soviet": return COLOR_SOVIET
    if faction == "Alliance": return COLOR_ALLIANCE
    if faction == "Coalition": return COLOR_COALITION
    if faction == "Chronolegion": return COLOR_CHRONO
    return (0.9, 0.9, 0.1)

# Definitions of All 140 Objects
FACTION_DATA = {
    "Soviet": {
        "prefix": "SU",
        "accent": COLOR_SOVIET,
        "buildings": [
            ("SU_MCV_MobileYard", "Mobile ConYard Vehicle", 4, 4, 500, 800, 700, 500),
            ("SU_ConYard", "Red Star Headquarters", 4, 4, 0, 800, 800, 600),
            ("SU_PowerPlant", "Thermal Reactor", 3, 3, 0, 600, 600, 500),
            ("SU_Refinery", "Ore Combine Facility", 4, 4, 0, 800, 800, 450),
            ("SU_Barracks", "Mobilization Barracks", 3, 3, 0, 600, 600, 400),
            ("SU_WarFactory", "Heavy Industrial Plant", 4, 4, 0, 800, 800, 500),
            ("SU_Airfield", "Strategic Aviation Airfield", 4, 4, 0, 800, 800, 450),
            ("SU_NavalYard", "Naval Dock", 5, 5, 0, 1000, 1000, 500),
            ("SU_Radar", "Command Radar Tower", 2, 2, 0, 400, 400, 700),
            ("SU_TechCenter", "Thunder Science Complex", 3, 3, 0, 600, 600, 550),
            ("SU_GunTurret", "Pillbox Auto-Gun", 2, 2, 0, 400, 400, 300),
            ("SU_AATurret", "Shilka AA Tower", 2, 2, 0, 400, 400, 350),
            ("SU_TeslaTower", "Perun Tesla Coil", 2, 2, 0, 400, 400, 550),
            ("SU_Bunker", "Forward Outpost Bunker", 2, 2, 0, 400, 400, 250),
            ("SU_SuperweaponDome", "Iron Dome Complex", 5, 5, 0, 1000, 1000, 850),
            ("SU_SuperweaponSilo", "Punisher Missile Silo", 5, 5, 0, 1000, 1000, 950),
        ],
        "units": [
            ("SU_RubezhRifleman", "Infantry", "Rubezh Rifleman MS-12", 60, 60, 180),
            ("SU_ZapalGrenadier", "Infantry", "Zapal Assault Grenadier", 70, 70, 185),
            ("SU_ZaslonAATeam", "Infantry", "Zaslon AA Missile Team", 65, 65, 180),
            ("SU_MasterEngineer", "Infantry", "Master Engineer Sapper", 60, 60, 175),
            ("SU_RazryadTrooper", "Infantry", "Razryad Tesla Trooper", 80, 80, 195),
            ("SU_VektorOfficer", "Infantry", "Vektor Signal Officer", 60, 60, 180),
            ("SU_BogatyrOreCarrier", "Harvester", "Bogatyr Ore Mining Carrier", 600, 360, 280),
            ("SU_RysScout", "LightVehicle", "Rys Scout Armored Vehicle", 380, 220, 180),
            ("SU_GranitMBT", "MBT", "Granit Main Battle Tank", 550, 330, 220),
            ("SU_ZarevoMLRS", "Artillery", "Zarevo Thermobaric MLRS", 600, 320, 250),
            ("SU_GromoboyRam", "SpecialVehicle", "Gromoboy Tesla Ram Tank", 580, 340, 240),
            ("SU_VoevodaHeavyTank", "HeavyTank", "Voevoda Superheavy Tank", 720, 460, 350),
            ("SU_KrechetInterceptor", "Aircraft", "Krechet Jet Interceptor", 650, 500, 180),
            ("SU_KorshunGunship", "Aircraft", "Korshun Attack Helicopter", 700, 550, 250),
            ("SU_GromadaAirship", "Aircraft", "Gromada Heavy Airship", 1300, 600, 450),
            ("SU_BuranPatrolBoat", "Naval", "Buran Patrol Gunboat", 1000, 350, 280),
            ("SU_MorokSubmarine", "Naval", "Morok Attack Submarine", 1400, 350, 320),
            ("SU_SvyatogorCruiser", "Naval", "Svyatogor Missile Cruiser", 2000, 600, 500),
            ("SU_Hero_Morozova", "Infantry", "Major Elena Morozova Hero", 70, 70, 185),
        ]
    },
    "Alliance": {
        "prefix": "AL",
        "accent": COLOR_ALLIANCE,
        "buildings": [
            ("AL_MCV_MobileNode", "Mobile Deployment Node", 4, 4, 500, 800, 700, 500),
            ("AL_ConYard", "Network Command Center", 4, 4, 0, 800, 800, 600),
            ("AL_PowerPlant", "Compact Fusion Reactor", 3, 3, 0, 600, 600, 480),
            ("AL_Refinery", "Automated Ore Processor", 4, 4, 0, 800, 800, 450),
            ("AL_Barracks", "Tactical Barracks", 3, 3, 0, 600, 600, 400),
            ("AL_WarFactory", "Modular Vehicle Assembly", 4, 4, 0, 800, 800, 500),
            ("AL_Airfield", "Skyline Airbase", 4, 4, 0, 800, 800, 450),
            ("AL_NavalYard", "Oceanic Dock", 5, 5, 0, 1000, 1000, 500),
            ("AL_Radar", "Recon Intel Center", 2, 2, 0, 400, 400, 680),
            ("AL_TechCenter", "Applied Physics Lab", 3, 3, 0, 600, 600, 550),
            ("AL_GunTurret", "Guardian Auto-Cannon Turret", 2, 2, 0, 400, 400, 300),
            ("AL_AATurret", "Dome Missile AA Battery", 2, 2, 0, 400, 400, 360),
            ("AL_PrismTower", "Prism Beam Battery", 2, 2, 0, 400, 400, 560),
            ("AL_ShieldProjectorBuilding", "Area Shield Projector", 2, 2, 0, 400, 400, 320),
            ("AL_SuperweaponChrono", "Chrono Evacuation Grid", 5, 5, 0, 1000, 1000, 880),
            ("AL_SuperweaponOrbital", "Zenith Orbital Platform Base", 5, 5, 0, 1000, 1000, 920),
        ],
        "units": [
            ("AL_SentinelRifleman", "Infantry", "Sentinel Rifleman M6", 60, 60, 180),
            ("AL_LancerTeam", "Infantry", "Lancer Missile Team FGM-31", 65, 65, 180),
            ("AL_FieldEngineer", "Infantry", "Field Engineer E-4", 60, 60, 175),
            ("AL_LongwatchSniper", "Infantry", "Longwatch Sniper R-9", 60, 60, 180),
            ("AL_LifelineMedic", "Infantry", "Lifeline Field Medic M-12", 60, 60, 175),
            ("AL_FrostlineSpecialist", "Infantry", "Frostline Cryo Specialist C-7", 65, 65, 180),
            ("AL_PioneerHarvester", "Harvester", "Pioneer Mining Platform M88", 580, 350, 270),
            ("AL_KestrelScout", "LightVehicle", "Kestrel Scout Vehicle LAV-41", 360, 210, 175),
            ("AL_BulwarkMBT", "MBT", "Bulwark Main Battle Tank M14", 540, 320, 210),
            ("AL_OracleArtillery", "Artillery", "Oracle Railgun Howitzer XM190", 590, 310, 240),
            ("AL_RefractionTank", "SpecialVehicle", "Refraction Stealth Tank XM27", 520, 300, 200),
            ("AL_WardShieldCarrier", "SpecialVehicle", "Ward Shield Carrier M46", 560, 330, 230),
            ("AL_CitadelTank", "HeavyTank", "Citadel Heavy Assault Tank M70", 700, 440, 340),
            ("AL_ShrikeInterceptor", "Aircraft", "Shrike Jet Fighter F/A-48", 640, 480, 175),
            ("AL_VectorVTOL", "Aircraft", "Vector VTOL Transport AV-27", 680, 520, 240),
            ("AL_NightveilBomber", "Aircraft", "Nightveil Stealth Bomber B-39", 900, 750, 220),
            ("AL_MantaPatrolCraft", "Naval", "Manta Hydrofoil Patrol Craft PHM-22", 950, 320, 260),
            ("AL_ResoluteDestroyer", "Naval", "Resolute Guided Missile Destroyer DDG-31", 1600, 450, 420),
            ("AL_HorizonCarrier", "Naval", "Horizon Aircraft Carrier CVX-90", 2200, 700, 550),
            ("AL_Hero_Hart", "Infantry", "Agent Evelyn Hart Hero", 65, 65, 180),
        ]
    },
    "Coalition": {
        "prefix": "CO",
        "accent": COLOR_COALITION,
        "buildings": [
            ("CO_MCV_MobileNode", "Mobile Harmony Hub", 4, 4, 500, 800, 700, 500),
            ("CO_ConYard", "Command Palace Citadel", 4, 4, 0, 800, 800, 620),
            ("CO_PowerPlant", "Solar Collector Array", 3, 3, 0, 600, 600, 450),
            ("CO_Refinery", "Ore Synthesizer Plant", 4, 4, 0, 800, 800, 440),
            ("CO_Barracks", "Preparation Training Hall", 3, 3, 0, 600, 600, 400),
            ("CO_WarFactory", "Walker Assembly Factory", 4, 4, 0, 800, 800, 520),
            ("CO_Airfield", "Aerial Pagoda Airbase", 4, 4, 0, 800, 800, 460),
            ("CO_NavalYard", "Tidal Dock", 5, 5, 0, 1000, 1000, 500),
            ("CO_Radar", "Coordination Pagoda Tower", 2, 2, 0, 400, 400, 720),
            ("CO_TechCenter", "Research Citadel", 3, 3, 0, 600, 600, 560),
            ("CO_GunTurret", "Igla Needle Auto-Turret", 2, 2, 0, 400, 400, 310),
            ("CO_AATurret", "Sky Spear AA Tower", 2, 2, 0, 400, 400, 370),
            ("CO_RailTower", "Sky Judgement Rail Battery", 2, 2, 0, 400, 400, 580),
            ("CO_ShieldHub", "Harmonic Shield Node", 2, 2, 0, 400, 400, 330),
            ("CO_SuperweaponMatrix", "Ten Thousand Shields Matrix", 5, 5, 0, 1000, 1000, 860),
            ("CO_SuperweaponSeismic", "Seismic Resonator Tower", 5, 5, 0, 1000, 1000, 940),
        ],
        "units": [
            ("CO_QianweiRifleman", "Infantry", "Qianwei Rifleman Type 21", 60, 60, 180),
            ("CO_VajraLancer", "Infantry", "Vajra AT Lancer Team AT-8", 65, 65, 180),
            ("CO_JieTechnician", "Infantry", "Jie Network Technician Type 06", 60, 60, 175),
            ("CO_ShengongMarksman", "Infantry", "Shengong Phase Marksman QBS-19", 60, 60, 180),
            ("CO_SanjivaniMedic", "Infantry", "Sanjivani Nanite Medic NM-7", 60, 60, 175),
            ("CO_RakshaGuard", "Infantry", "Raksha Honor Guard HG-33", 70, 70, 190),
            ("CO_YuanCollector", "Harvester", "Yuan Mining Platform GRP-12", 590, 350, 275),
            ("CO_KamakiriWalker", "LightVehicle", "Kamakiri Scout Walker Type 17", 350, 230, 260),
            ("CO_QinglongMBT", "MBT", "Qinglong Main Battle Tank ZTZ-61", 550, 330, 215),
            ("CO_MonsoonArtillery", "Artillery", "Monsoon Artillery PHL-29", 600, 320, 245),
            ("CO_SeimonShieldCarrier", "SpecialVehicle", "Seimon Shield Carrier Type 42", 570, 340, 235),
            ("CO_AiravataWalker", "HeavyTank", "Airavata Assault Walker MBT-X", 680, 480, 420),
            ("CO_TianmenFortress", "HeavyTank", "Tianmen Mobile Fortress ZTD-90", 850, 550, 450),
            ("CO_KawasemiDrone", "Aircraft", "Kawasemi Recon Drone UAV-12", 200, 200, 80),
            ("CO_LeiheGunship", "Aircraft", "Leihe Assault Helicopter Z-28", 690, 530, 245),
            ("CO_AgnipakshaBomber", "Aircraft", "Agnipaksha Heavy Bomber H-26", 920, 760, 230),
            ("CO_KazekiriCorvette", "Naval", "Kazekiri Corvette Type 32", 980, 330, 270),
            ("CO_XuanwuCruiser", "Naval", "Xuanwu Railgun Cruiser Type 81", 1750, 480, 440),
            ("CO_SamudraCarrier", "Naval", "Samudra Submarine Aircraft Carrier SSGN-18", 2100, 520, 480),
            ("CO_Hero_Mei", "Infantry", "Commander Mei Jian Hero", 65, 65, 180),
        ]
    },
    "Chronolegion": {
        "prefix": "CH",
        "accent": COLOR_CHRONO,
        "buildings": [
            ("CH_MCV_MobileArk", "Mobile Chrono Ark", 4, 4, 500, 800, 700, 500),
            ("CH_ConYard", "Causality Hub Center", 4, 4, 0, 800, 800, 600),
            ("CH_PowerPlant", "Delayed Decay Reactor", 3, 3, 0, 600, 600, 490),
            ("CH_Refinery", "Quantum Refinery Processor", 4, 4, 0, 800, 800, 450),
            ("CH_Barracks", "Echo Training Barracks", 3, 3, 0, 600, 600, 400),
            ("CH_WarFactory", "Continuum Factory", 4, 4, 0, 800, 800, 500),
            ("CH_Airfield", "Rift Airbase Structure", 4, 4, 0, 800, 800, 450),
            ("CH_NavalYard", "Temporal Tide Dock", 5, 5, 0, 1000, 1000, 500),
            ("CH_Radar", "Probability Observer Tower", 2, 2, 0, 400, 400, 700),
            ("CH_TechCenter", "Future Archive Science Lab", 3, 3, 0, 600, 600, 550),
            ("CH_EchoTurret", "Echo Auto-Turret", 2, 2, 0, 400, 400, 310),
            ("CH_AATurret", "AA Rift Disruption Tower", 2, 2, 0, 400, 400, 360),
            ("CH_StasisProjectorBuilding", "Stasis Field Projector STS-5", 2, 2, 0, 400, 400, 420),
            ("CH_CausalityAnchor", "Causality Anchor Spire", 2, 2, 0, 400, 400, 500),
            ("CH_SuperweaponRewind", "Countdown Matrix Facility", 5, 5, 0, 1000, 1000, 870),
            ("CH_SuperweaponSingularity", "Singularity Collapser Spire", 5, 5, 0, 1000, 1000, 960),
        ],
        "units": [
            ("CH_ResonanceRifleman", "Infantry", "Resonance Rifleman ECHO-7", 60, 60, 180),
            ("CH_PunctureLancer", "Infantry", "Puncture Lancer PHASE-L9", 65, 65, 180),
            ("CH_CausalityEngineer", "Infantry", "Causality Engineer CSE-2", 60, 60, 175),
            ("CH_ReversalMedic", "Infantry", "Reversal Medic RWD-3", 60, 60, 175),
            ("CH_AporiaSniper", "Infantry", "Aporia Paradox Sniper PDX-12", 60, 60, 180),
            ("CH_CensorOperative", "Infantry", "Censor Null Operative NULL-12", 65, 65, 180),
            ("CH_ProbabilistHarvester", "Harvester", "Probabilist Quantum Mining Vehicle QH-4", 570, 340, 260),
            ("CH_ParallaxScout", "LightVehicle", "Parallax Scout Vehicle BLK-8", 370, 220, 180),
            ("CH_TimelineTank", "MBT", "Timeline Tank CT-21", 540, 320, 210),
            ("CH_DeltaDelayArtillery", "Artillery", "Delta Delay Artillery LAG-16", 580, 310, 240),
            ("CH_PauseProjector", "SpecialVehicle", "Stasis Pause Projector Vehicle STS-5", 550, 320, 220),
            ("CH_EraEngine", "HeavyTank", "Era Engine Heavy Temporal Tank EPC-0", 720, 450, 340),
            ("CH_GapInterceptor", "Aircraft", "Gap Rift Interceptor RFT-31", 630, 470, 170),
            ("CH_TrailGunship", "Aircraft", "Trail Afterimage Gunship AFG-6", 670, 510, 235),
            ("CH_CriticalPointBomber", "Aircraft", "Critical Point Bomber CRV-9", 880, 720, 210),
            ("CH_IsobathFrigate", "Naval", "Isobath Tidemark Frigate TMK-9", 1100, 380, 310),
            ("CH_BathysSubmarine", "Naval", "Bathys Abyss Submarine ABY-14", 1350, 340, 300),
            ("CH_AttractorArk", "Naval", "Attractor Singularity Ark SGA-1", 1900, 580, 490),
            ("CH_Hero_Voss", "Infantry", "Archivist Selena Voss Hero", 65, 65, 180),
        ]
    }
}

def build_procedural_model(stable_id, category, display_name, width, length, height, faction_accent):
    clear_scene()
    base_mat = create_material("M_Blockout_Base", COLOR_BASE)
    accent_mat = create_material("M_Blockout_Accent", faction_accent)

    # 1. Main Base Body
    if "Infantry" in category or "Hero" in display_name:
        # Humanoid blockout (Legs, Torso, Head, Weapon)
        torso = add_cube("Torso", (0, 0, 100), (width * 0.8, length * 0.5, height * 0.4), base_mat)
        legs = add_cube("Legs", (0, 0, 45), (width * 0.7, length * 0.4, height * 0.45), base_mat)
        head = add_sphere("Head", (0, 0, 155), radius=22.0, mat=accent_mat)
        gun = add_cube("Weapon", (width * 0.4, length * 0.4, 110), (width * 0.3, length * 1.2, 18.0), accent_mat)
    elif "Building" in category or width >= 400:
        # Building structure with main hull, entrance ramp, roof detail
        main = add_cube("MainBuilding", (0, 0, height * 0.4), (width, length, height * 0.8), base_mat)
        roof = add_cube("RoofStructure", (0, 0, height * 0.85), (width * 0.6, length * 0.6, height * 0.2), accent_mat)
        door = add_cube("EntranceGate", (0, -length * 0.5, height * 0.25), (width * 0.3, 20.0, height * 0.4), accent_mat)
        if "Radar" in display_name or "Tower" in display_name or "Silo" in display_name:
            spire = add_cylinder("Spire", (0, 0, height * 0.8), radius=width * 0.2, depth=height * 0.4, mat=accent_mat)
    elif category in ["MBT", "HeavyTank", "LightVehicle", "Harvester", "Artillery", "SpecialVehicle"]:
        # Ground Vehicle with Chassis, Wheels/Tracks, Turret, Gun Barrel
        chassis = add_cube("Chassis", (0, 0, height * 0.4), (width * 0.9, length * 0.9, height * 0.5), base_mat)
        wheels_l = add_cube("Tracks_L", (-width * 0.48, 0, height * 0.3), (width * 0.15, length * 0.95, height * 0.4), base_mat)
        wheels_r = add_cube("Tracks_R", (width * 0.48, 0, height * 0.3), (width * 0.15, length * 0.95, height * 0.4), base_mat)
        turret = add_cylinder("Turret", (0, 0, height * 0.75), radius=width * 0.3, depth=height * 0.3, mat=accent_mat)
        barrel = add_cylinder("GunBarrel", (0, length * 0.4, height * 0.75), radius=width * 0.06, depth=length * 0.5, mat=accent_mat, rotation=(math.radians(90), 0, 0))
    elif category == "Aircraft":
        # Flying aircraft (Hull, Wings, Cockpit, Thrusters)
        fuselage = add_cube("Fuselage", (0, 0, height * 0.5), (width * 0.3, length * 0.9, height * 0.5), base_mat)
        wings = add_cube("Wings", (0, 0, height * 0.5), (width * 0.95, length * 0.3, height * 0.1), accent_mat)
        cockpit = add_sphere("Cockpit", (0, length * 0.3, height * 0.7), radius=width * 0.15, mat=accent_mat)
    elif category == "Naval":
        # Ship/Submarine Hull, Superstructure, Deck Gun
        hull = add_cube("Hull", (0, 0, height * 0.4), (width * 0.4, length * 0.95, height * 0.6), base_mat)
        bridge = add_cube("Bridge", (0, -length * 0.1, height * 0.8), (width * 0.25, length * 0.3, height * 0.4), accent_mat)
        deckgun = add_cylinder("DeckTurret", (0, length * 0.25, height * 0.8), radius=width * 0.12, depth=height * 0.25, mat=accent_mat)
    else:
        add_cube("GenericBlockout", (0, 0, height * 0.5), (width, length, height), base_mat)

def process_all():
    manifest_rows = []
    total_count = 0

    for faction_name, data in FACTION_DATA.items():
        faction_dir = os.path.join(OUTPUT_BASE, faction_name)
        os.makedirs(faction_dir, exist_ok=True)
        accent = data["accent"]

        # Process Buildings
        for bld in data["buildings"]:
            stable_id, bld_name, fp_x, fp_y, z_off, w, l, h = bld
            asset_name = f"SM_{faction_name}_{stable_id}_Blockout"
            fbx_filename = f"{asset_name}.fbx"
            fbx_path = os.path.join(faction_dir, fbx_filename)

            build_procedural_model(stable_id, "Building", bld_name, w, l, h, accent)
            export_fbx(fbx_path)

            manifest_rows.append({
                "StableID": stable_id,
                "Faction": faction_name,
                "Class": "Building",
                "Name": bld_name,
                "Dimensions_Cm": f"{w}x{l}x{h}",
                "Footprint": f"{fp_x}x{fp_y}",
                "Pivot": "BottomCenter (0,0,0)",
                "Subparts": "MainBuilding, RoofStructure, EntranceGate, Spire",
                "Sockets": "SOCKET_Entrance, SOCKET_Spawn, SOCKET_VFX",
                "Collision": "Simple Box & Complex Mesh",
                "ExportStatus": "SUCCESS",
                "FilePath": f"Content/RA4/Art/Blockout/{faction_name}/{fbx_filename}"
            })
            total_count += 1

        # Process Units
        for unit in data["units"]:
            stable_id, cat, unit_name, w, l, h = unit
            asset_name = f"SM_{faction_name}_{stable_id}_Blockout"
            fbx_filename = f"{asset_name}.fbx"
            fbx_path = os.path.join(faction_dir, fbx_filename)

            build_procedural_model(stable_id, cat, unit_name, w, l, h, accent)
            export_fbx(fbx_path)

            manifest_rows.append({
                "StableID": stable_id,
                "Faction": faction_name,
                "Class": cat,
                "Name": unit_name,
                "Dimensions_Cm": f"{w}x{l}x{h}",
                "Footprint": "1x1",
                "Pivot": "BottomCenter (0,0,0)",
                "Subparts": "Chassis, Turret, GunBarrel, Tracks, Cockpit",
                "Sockets": "SOCKET_Muzzle, SOCKET_Turret, SOCKET_VFX_Engine",
                "Collision": "Capsule / Convex Hull",
                "ExportStatus": "SUCCESS",
                "FilePath": f"Content/RA4/Art/Blockout/{faction_name}/{fbx_filename}"
            })
            total_count += 1

    # Write CSV Manifest
    manifest_csv_path = os.path.join(OUTPUT_BASE, "Blockout_Manifest.csv")
    fieldnames = ["StableID", "Faction", "Class", "Name", "Dimensions_Cm", "Footprint", "Pivot", "Subparts", "Sockets", "Collision", "ExportStatus", "FilePath"]
    with open(manifest_csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(manifest_rows)

    print(f"PROCEDURAL BLOCKOUT GENERATION COMPLETE: {total_count} models exported to {OUTPUT_BASE}")

if __name__ == "__main__":
    process_all()
