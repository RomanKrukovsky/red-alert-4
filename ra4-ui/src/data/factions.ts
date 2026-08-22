export interface FactionConfig {
  id: string;
  name: string;
  shortName: string;
  nameLines: string[];
  country: string;
  crest: string;
  color: string;
  dimColor: string;
  themeClass: string;
  campaignTitle: string;
  doctrine: string;
  description: string;
  progressPercent: number;
  missionsCompleted: string;
  difficulty: string;
  currentChapter: string;
  bgScreenshot: string;
}

export const FACTIONS: Record<string, FactionConfig> = {
  eurasian: {
    id: 'eurasian',
    name: 'ЕВРАЗИЙСКИЙ ПАКТ',
    shortName: 'ЕВРАЗИЙСКИЙ',
    nameLines: ['ЕВРАЗИЙСКИЙ', 'ПАКТ'],
    country: 'РОССИЯ',
    crest: '❖',
    color: '#b06cff',
    dimColor: '#6a3fa0',
    themeClass: 'theme-eurasian',
    campaignTitle: 'РОССИЯ: ЛИНИЯ РАЗЛОМА',
    doctrine: 'РЭБ • РАКЕТНЫЕ ВОЙСКА • ТЯЖЁЛАЯ ОБОРОНА',
    description: 'Когда небо глохнет от помех, а даль бьёт без предупреждения, победа достаётся тем, кто видит сквозь туман войны. Россия строит непроницаемую оборону, ломает сети противника и наносит ответный удар, когда он меньше всего этого ждёт.',
    progressPercent: 58,
    missionsCompleted: '7 / 12',
    difficulty: 'ВЕТЕРАН',
    currentChapter: 'ГЛАВА 4: БЕЛЫЙ ШУМ',
    bgScreenshot: '/remaster/03_campaign_eurasian_russia.png'
  },
  atlantic: {
    id: 'atlantic',
    name: 'АТЛАНТИЧЕСКИЙ АЛЬЯНС',
    shortName: 'АТЛАНТИЧЕСКИЙ',
    nameLines: ['АТЛАНТИЧЕСКИЙ', 'АЛЬЯНС'],
    country: 'США',
    crest: '⬢',
    color: '#3f8dff',
    dimColor: '#1648a0',
    themeClass: 'theme-atlantic',
    campaignTitle: 'США: ДАЛЬНИЙ РУБЕЖ',
    doctrine: 'ЭКСПЕДИЦИЯ • АВИАЦИЯ • ЦЕНТР КООРДИНАЦИИ',
    description: 'На далёком рубеже решается исход противостояния. Мы проецируем силу, поддерживаем союзников и обеспечиваем свободу морских путей в эпоху нестабильного мира.',
    progressPercent: 42,
    missionsCompleted: '5 / 12',
    difficulty: 'НОРМАЛЬНО',
    currentChapter: 'ГЛАВА 3: ЛИНИЯ ПРИЛИВОВ',
    bgScreenshot: '/remaster/04_campaign_atlantic_usa.png'
  },
  eastern: {
    id: 'eastern',
    name: 'ВОСТОЧНАЯ КОАЛИЦИЯ',
    shortName: 'ВОСТОЧНАЯ',
    nameLines: ['ВОСТОЧНАЯ', 'КОАЛИЦИЯ'],
    country: 'КИТАЙ',
    crest: '✦',
    color: '#2fd98a',
    dimColor: '#0f5c2e',
    themeClass: 'theme-eastern',
    campaignTitle: 'КИТАЙ: НЕФТОВАЯ СЕТЬ',
    doctrine: 'ДРОНЫ • АВТОМАТИЗАЦИЯ • МАССОВОЕ ПРОИЗВОДСТВО',
    description: 'Нефтяная артерия империи должна биться ровно. Автоматизированные производства и рои дронов гарантируют, что экономика коалиции не остановится ни на минуту.',
    progressPercent: 63,
    missionsCompleted: '8 / 12',
    difficulty: 'ВЕТЕРАН',
    currentChapter: 'ГЛАВА 5: РОЙ НАД ДЕЛЬТОЙ',
    bgScreenshot: '/remaster/05_campaign_eastern_china.png'
  },
  pacific: {
    id: 'pacific',
    name: 'ТИХООКЕАНСКИЙ ПАКТ',
    shortName: 'ТИХООКЕАНСКИЙ',
    nameLines: ['ТИХООКЕАНСКИЙ', 'ПАКТ'],
    country: 'ЯПОНИЯ',
    crest: '◈',
    color: '#2fd4c8',
    dimColor: '#12666c',
    themeClass: 'theme-pacific',
    campaignTitle: 'ЯПОНИЯ: ДУГА ШТОРМА',
    doctrine: 'БЕРЕГОВАЯ ОБОРОНА • ПВО • РОБОТЕХНИКА',
    description: 'Островная держава превращает береговую линию в неприступную дугу. Роботехника и системы ПВО нового поколения встретят любую волну шторма.',
    progressPercent: 37,
    missionsCompleted: '4 / 12',
    difficulty: 'НОРМАЛЬНО',
    currentChapter: 'ГЛАВА 3: ПЕРВАЯ ВОЛНА',
    bgScreenshot: '/remaster/06_campaign_pacific_japan.png'
  },
  independent: {
    id: 'independent',
    name: 'НЕЗАВИСИМЫЕ ДЕРЖАВЫ',
    shortName: 'НЕЗАВИСИМЫЕ',
    nameLines: ['НЕЗАВИСИМЫЕ', 'ДЕРЖАВЫ'],
    country: 'ИРАН',
    crest: '◉',
    color: '#e8a13d',
    dimColor: '#8a5c1c',
    themeClass: 'theme-independent',
    campaignTitle: 'ИРАН: БРОНЯ И АСИММЕТРИЧНАЯ ВОЙНА',
    doctrine: 'ГОРНАЯ ВОЙНА • АСИММЕТРИЧНЫЙ ОТВЕТ • МОБИЛЬНЫЕ УЗЛЫ',
    description: 'Иран делает ставку на асимметричную войну: горные укрепления, мобильные ракетные комплексы и сеть автономных узлов, которые невозможно вычислить с первого удара.',
    progressPercent: 24,
    missionsCompleted: '3 / 10',
    difficulty: 'СЛОЖНО',
    currentChapter: 'ОПЕРАЦИЯ «САТУРАЦИОННЫЙ УДАР»',
    bgScreenshot: '/remaster/19_campaign_independent_iran.png'
  }
};

export const FACTION_ORDER = ['eurasian', 'atlantic', 'eastern', 'pacific', 'independent'];

/* Front legend colors used on the main-menu "front summary" card */
export const FRONT_LEGEND = [
  { name: 'ЕВРАЗИЙСКИЙ ПАКТ', color: '#b06cff', status: 'НАСТУПЛЕНИЕ' },
  { name: 'АТЛАНТИЧЕСКИЙ АЛЬЯНС', color: '#3f8dff', status: 'ОБОРОНА' },
  { name: 'ВОСТОЧНАЯ КОАЛИЦИЯ', color: '#2fd98a', status: 'ПРОИЗВОДСТВО' },
  { name: 'ТИХООКЕАНСКИЙ ПАКТ', color: '#2fd4c8', status: 'ПАТРУЛЬ' },
  { name: 'НЕЗАВИСИМЫЕ ДЕРЖАВЫ', color: '#e8a13d', status: 'НЕЙТРАЛИТЕТ' }
];
