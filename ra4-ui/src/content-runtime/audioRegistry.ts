export interface AudioEntry {
  id: string;
  name: string;
  category: 'VOICE' | 'EVA' | 'SFX' | 'MUSIC';
  factionId?: string;
  urlOgg: string;
  urlMp3: string;
}

const SOVIET_UNITS = [
  { id: 'SU_RubezhRifleman', name: 'Мотострелок МС-12 «Рубеж»' },
  { id: 'SU_ZapalGrenadier', name: 'Штурмовик ОШ-4 «Запал»' },
  { id: 'SU_ZaslonAATeam', name: 'Зенитный расчёт ПЗК-9 «Заслон»' },
  { id: 'SU_MasterEngineer', name: 'Инженер-сапёр ИС-3 «Мастер»' },
  { id: 'SU_RazryadTrooper', name: 'Электроштурмовик ЭШ-8 «Разряд»' },
  { id: 'SU_VektorOfficer', name: 'Офицер связи КС-6 «Вектор»' },
  { id: 'SU_Hero_Morozova', name: 'Герой Морозова' },
  { id: 'SU_GranitMBT', name: 'Средний танк Т-90 «Гранит»' },
  { id: 'SU_BogatyrOreCarrier', name: 'Рудовоз Б-4 «Богатырь»' },
  { id: 'SU_RysScout', name: 'Бронеездок БРМ-1 «Рысь»' },
  { id: 'SU_GromoboyRam', name: 'Таран БМР-2 «Громобой»' },
  { id: 'SU_VoevodaHeavyTank', name: 'Тяжёлый танк Т-100 «Воевода»' },
  { id: 'SU_ZarevoMLRS', name: 'РСЗО БМ-21 «Зарево»' },
  { id: 'SU_KrechetInterceptor', name: 'Истребитель МиГ-35 «Кречет»' },
  { id: 'SU_KorshunGunship', name: 'Ударный вертолёт Ми-24 «Коршун»' },
  { id: 'SU_GromadaAirship', name: 'Тяжёлый дирижабль «Громада»' },
  { id: 'SU_BuranPatrolBoat', name: 'Катер «Буран»' },
  { id: 'SU_MorokSubmarine', name: 'Подводная лодка «Морок»' },
  { id: 'SU_SvyatogorCruiser', name: 'Ракетный крейсер «Святогор»' },
];

const EVENTS = [
  { key: 'Selected', label: 'Выбор' },
  { key: 'Move', label: 'Движение' },
  { key: 'Attack', label: 'Огонь' },
  { key: 'Ability', label: 'Способность' },
  { key: 'Damaged', label: 'Урон' },
  { key: 'Death', label: 'Гибель' },
  { key: 'Elite', label: 'Элита' },
  { key: 'Idle', label: 'Ожидание' },
];

const generatedUnitBarks: AudioEntry[] = SOVIET_UNITS.flatMap((unit) =>
  EVENTS.map((event) => ({
    id: `vo_su_${unit.id.toLowerCase()}_${event.key.toLowerCase()}`,
    name: `${unit.name}: ${event.label}`,
    category: 'VOICE' as const,
    factionId: 'USSR',
    urlOgg: `/assets/audio/voice/VO_RU_${unit.id}_${event.key}_01.ogg`,
    urlMp3: `/assets/audio/voice/VO_RU_${unit.id}_${event.key}_01.mp3`,
  }))
);

export const AUDIO_REGISTRY: AudioEntry[] = [
  // EVA Announcer
  { id: 'eva_match_start', name: 'EVA: Сражение началось', category: 'EVA', urlOgg: '/assets/audio/voice/EVA_MatchStart.ogg', urlMp3: '/assets/audio/voice/EVA_MatchStart.mp3' },
  { id: 'eva_building_complete', name: 'EVA: Строительство завершено', category: 'EVA', urlOgg: '/assets/audio/voice/EVA_BuildingComplete.ogg', urlMp3: '/assets/audio/voice/EVA_BuildingComplete.mp3' },
  { id: 'eva_unit_ready', name: 'EVA: Юнит готов', category: 'EVA', urlOgg: '/assets/audio/voice/EVA_UnitReady.ogg', urlMp3: '/assets/audio/voice/EVA_UnitReady.mp3' },
  { id: 'eva_base_under_attack', name: 'EVA: Внимание, база под атакой!', category: 'EVA', urlOgg: '/assets/audio/voice/EVA_BaseUnderAttack.ogg', urlMp3: '/assets/audio/voice/EVA_BaseUnderAttack.mp3' },
  { id: 'eva_insufficient_funds', name: 'EVA: Недостаточно средств', category: 'EVA', urlOgg: '/assets/audio/voice/EVA_InsufficientFunds.ogg', urlMp3: '/assets/audio/voice/EVA_InsufficientFunds.mp3' },

  // Soviet Unit Voice Lines (19 units x 8 events = 152 voice barks)
  ...generatedUnitBarks,

  // Alliance Voice Barks
  { id: 'vo_al_bulwark_selected', name: 'Танк Бастион Альянса: Выбор', category: 'VOICE', factionId: 'ALLIANCE', urlOgg: '/assets/audio/voice/VO_RU_AL_BulwarkMBT_Selected_01.ogg', urlMp3: '/assets/audio/voice/VO_RU_AL_BulwarkMBT_Selected_01.mp3' },
  { id: 'vo_al_chrono_collector_selected', name: 'Хроно-Сборщик: Выбор', category: 'VOICE', factionId: 'ALLIANCE', urlOgg: '/assets/audio/voice/VO_RU_AL_ChronoCollector_Selected_01.ogg', urlMp3: '/assets/audio/voice/VO_RU_AL_ChronoCollector_Selected_01.mp3' },

  // Music Tracks
  { id: 'music_main_theme', name: 'Red Alert 4 Main Theme', category: 'MUSIC', urlOgg: '/assets/audio/music/RA4_MainMenu_Theme.ogg', urlMp3: '/assets/audio/music/RA4_MainMenu_Theme.mp3' },
];

export class AudioRegistry {
  private static instance: AudioRegistry;
  private entries: Map<string, AudioEntry> = new Map();

  constructor() {
    AUDIO_REGISTRY.forEach((entry) => this.entries.set(entry.id, entry));
  }

  public static getInstance(): AudioRegistry {
    if (!AudioRegistry.instance) {
      AudioRegistry.instance = new AudioRegistry();
    }
    return AudioRegistry.instance;
  }

  public get(id: string): AudioEntry | undefined {
    return this.entries.get(id);
  }

  public getAll(): AudioEntry[] {
    return Array.from(this.entries.values());
  }
}
