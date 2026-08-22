import React, { useState } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';

interface BuildItem {
  id: string;
  name: string;
  cost: number;
  icon: string;
}
interface BuildCategory {
  name: string;
  items: BuildItem[];
}
interface HudModeConfig {
  screenshot: string;
  themeClass: string;
  color: string;
  dim: string;
  crest: string;
  factionLabel: string;
  objectives: string[];
  alert?: string;
  superweapon?: boolean;
  unit: { name: string; type: string; armor: number; hp: number; hpMax: number };
  commands: { icon: string; label: string }[];
  categories: BuildCategory[];
}

const MODE_CONFIGS: Record<string, HudModeConfig> = {
  'eurasian-ground': {
    screenshot: '/remaster/12_battle_hud_eurasian_ground.png',
    themeClass: 'theme-eurasian',
    color: '#b06cff',
    dim: '#6a3fa0',
    crest: '❖',
    factionLabel: 'ЕВРАЗИЙСКИЙ ПАКТ',
    objectives: ['Прорвать линию обороны', 'Захватить железнодорожный узел'],
    alert: 'ВРАЖЕСКАЯ ПВО ПОДАВЛЕНА',
    unit: { name: 'Т-34М «БАРС»', type: 'ТЯЖЁЛЫЙ ТАНК', armor: 82, hp: 1850, hpMax: 1850 },
    commands: [
      { icon: '⇥', label: 'ДВИЖЕНИЕ' },
      { icon: '◎', label: 'АТАКА' },
      { icon: '⏹', label: 'ОСТАНОВИТЬ' },
      { icon: '☁', label: 'ДЫМ' },
      { icon: '📡', label: 'РЭБ-МЕТКА' }
    ],
    categories: [
      {
        name: 'ЗДАНИЯ',
        items: [
          { id: 'e-b1', name: 'КОМАНДНЫЙ ЦЕНТР', cost: 1500, icon: '🏛' },
          { id: 'e-b2', name: 'ЭНЕРГОСТАНЦИЯ', cost: 800, icon: '⚡' },
          { id: 'e-b3', name: 'ЗАВОД БПЛА', cost: 1200, icon: '🏭' },
          { id: 'e-b4', name: 'РЕМ. КОМПЛЕКС', cost: 1000, icon: '🔧' }
        ]
      },
      {
        name: 'БРОНЕТЕХНИКА',
        items: [
          { id: 'e-t1', name: 'Т-34М «БАРС»', cost: 900, icon: '🛡' },
          { id: 'e-t2', name: 'БМПТ «ВОЛК»', cost: 650, icon: '🚜' },
          { id: 'e-t3', name: 'БТР-МД «РЫСЬ»', cost: 450, icon: '🚙' },
          { id: 'e-t4', name: 'ИНЖ. МАШИНА', cost: 500, icon: '⚙' }
        ]
      },
      {
        name: 'АРТИЛЛЕРИЯ',
        items: [
          { id: 'e-a1', name: 'САУ 2С35 «КОАЛИЦИЯ»', cost: 1600, icon: '🎯' },
          { id: 'e-a2', name: 'РСЗО «СМЕРЧ-2»', cost: 1200, icon: '🚀' },
          { id: 'e-a3', name: 'МИНОМЁТ 2С42', cost: 600, icon: '💥' },
          { id: 'e-a4', name: 'ТОС-2 «ТОСОЧКА»', cost: 1000, icon: '☄' }
        ]
      },
      {
        name: 'ПВО',
        items: [
          { id: 'e-p1', name: 'ЗРК «ВИТЯЗЬ»', cost: 1000, icon: '🗼' },
          { id: 'e-p2', name: 'ПАНЦИРЬ-СМД', cost: 900, icon: '🔫' },
          { id: 'e-p3', name: 'ТОР-М2КМ', cost: 750, icon: '📡' },
          { id: 'e-p4', name: 'РАДАР «ВОРОНЕЖ»', cost: 1200, icon: '📶' }
        ]
      }
    ]
  },
  'atlantic-naval': {
    screenshot: '/remaster/13_battle_hud_atlantic_naval.png',
    themeClass: 'theme-atlantic',
    color: '#3f8dff',
    dim: '#1648a0',
    crest: '⬢',
    factionLabel: 'АТЛАНТИЧЕСКИЙ АЛЬЯНС',
    objectives: ['Удержать морской коридор', 'Не допустить высадку десанта'],
    alert: 'АВИАНОСНАЯ ГРУППА В БОЮ',
    unit: { name: 'ЭСМИНЕЦ «СВОБОДА»', type: 'Эсминец класса «Арли Бёрк»', armor: 76, hp: 3600, hpMax: 3600 },
    commands: [
      { icon: '⇥', label: 'КУРС' },
      { icon: '◎', label: 'ЗАЛП' },
      { icon: '⏹', label: 'СТОП' },
      { icon: '🌫', label: 'МАСКИРОВКА' },
      { icon: '✈', label: 'ПАЛУБА' }
    ],
    categories: [
      {
        name: 'ФЛОТ',
        items: [
          { id: 'a-f1', name: 'ЭСМИНЕЦ «БЁРК»', cost: 1800, icon: '🚢' },
          { id: 'a-f2', name: 'РАКЕТНЫЙ КАТЕР', cost: 700, icon: '⛴' },
          { id: 'a-f3', name: 'ПЛ «СИ ВУЛФ»', cost: 2400, icon: '🌊' },
          { id: 'a-f4', name: 'ДЕСАНТНЫЙ КОРАБЛЬ', cost: 1500, icon: '⚓' }
        ]
      },
      {
        name: 'АВИАЦИЯ',
        items: [
          { id: 'a-v1', name: 'F/A-XX «РАПТОР»', cost: 1600, icon: '✈' },
          { id: 'a-v2', name: '«ГРОУЛЕР» РЭБ', cost: 1100, icon: '📻' },
          { id: 'a-v3', name: 'MH-60 «СИХОК»', cost: 800, icon: '🚁' },
          { id: 'a-v4', name: 'ДРОН MQ-25', cost: 950, icon: '🛩' }
        ]
      },
      {
        name: 'ОБОРОНА',
        items: [
          { id: 'a-d1', name: 'ПЛАТФОРМА «ИДЖИС»', cost: 1400, icon: '🗼' },
          { id: 'a-d2', name: 'МИННОЕ ПОЛЕ', cost: 500, icon: '💣' },
          { id: 'a-d3', name: 'БЕРЕГОВАЯ БАТАРЕЯ', cost: 1000, icon: '🎯' },
          { id: 'a-d4', name: 'РАДАР ДРЛО', cost: 1200, icon: '📶' }
        ]
      }
    ]
  },
  'eastern-base': {
    screenshot: '/remaster/14_battle_hud_eastern_base.png',
    themeClass: 'theme-eastern',
    color: '#2fd98a',
    dim: '#0f5c2e',
    crest: '✦',
    factionLabel: 'ВОСТОЧНАЯ КОАЛИЦИЯ',
    objectives: ['Отстроить автозавод', 'Развернуть рой дронов'],
    alert: 'ПРОИЗВОДСТВО УСКОРЕНО',
    unit: { name: 'УМНЫЙ АВТОЗАВОД', type: 'Производственный комплекс', armor: 91, hp: 4200, hpMax: 4200 },
    commands: [
      { icon: '⚙', label: 'КОНВЕЙЕР' },
      { icon: '◎', label: 'ЗАКАЗ' },
      { icon: '⏹', label: 'ПАУЗА' },
      { icon: '♻', label: 'РЕСУРСЫ' },
      { icon: '🐝', label: 'РОЙ' }
    ],
    categories: [
      {
        name: 'ЗДАНИЯ',
        items: [
          { id: 'c-b1', name: 'ЦЕНТР УПРАВЛЕНИЯ', cost: 1400, icon: '🏛' },
          { id: 'c-b2', name: 'АВТОЗАВОД', cost: 1800, icon: '🏭' },
          { id: 'c-b3', name: 'СОЛНЕЧНЫЙ КУПОЛ', cost: 700, icon: '☀' },
          { id: 'c-b4', name: 'ЛОГИСТИЧЕСКИЙ ХАБ', cost: 900, icon: '📦' }
        ]
      },
      {
        name: 'ДРОНЫ',
        items: [
          { id: 'c-d1', name: 'РОЙ «СТРИЖ»', cost: 800, icon: '🐝' },
          { id: 'c-d2', name: 'УДАРНЫЙ D-7', cost: 1100, icon: '🛩' },
          { id: 'c-d3', name: 'РАЗВЕДЧИК', cost: 450, icon: '👁' },
          { id: 'c-d4', name: 'ТЯЖЁЛЫЙ ДРОН', cost: 1300, icon: '🛸' }
        ]
      },
      {
        name: 'ТЕХНИКА',
        items: [
          { id: 'c-t1', name: 'ТАНК Т-99А', cost: 1050, icon: '🛡' },
          { id: 'c-t2', name: 'БМП «ТИГР»', cost: 550, icon: '🚜' },
          { id: 'c-t3', name: 'РСЗО «ЖУРАВЛЬ»', cost: 1150, icon: '🚀' },
          { id: 'c-t4', name: 'МИНОМЁТ', cost: 600, icon: '💥' }
        ]
      },
      {
        name: 'ПВО',
        items: [
          { id: 'c-p1', name: 'HQ-22', cost: 950, icon: '🗼' },
          { id: 'c-p2', name: 'ЛАЗЕР L-30', cost: 1250, icon: '🔆' },
          { id: 'c-p3', name: 'РАДАР «НЕБО»', cost: 1100, icon: '📶' },
          { id: 'c-p4', name: 'ЗСУ «ГЭПАРД-2»', cost: 700, icon: '🔫' }
        ]
      }
    ]
  },
  'pacific-air': {
    screenshot: '/remaster/15_battle_hud_pacific_air.png',
    themeClass: 'theme-pacific',
    color: '#2fd4c8',
    dim: '#12666c',
    crest: '◈',
    factionLabel: 'ТИХООКЕАНСКИЙ ПАКТ',
    objectives: ['Удержать господство в воздухе', 'Не допустить прорыва к базе'],
    alert: 'ЭСКАДРИЛЬЯ НА ПОЗИЦИЯХ',
    unit: { name: 'ЭСКАДРИЛЬЯ «СОРОКА»', type: 'Многоцелевые истребители 5-го поколения', armor: 64, hp: 900, hpMax: 900 },
    commands: [
      { icon: '⇥', label: 'КУРС' },
      { icon: '◎', label: 'ПУШКА' },
      { icon: '🚀', label: 'РАКЕТЫ' },
      { icon: '🌫', label: 'ЛОВУШКИ' },
      { icon: '↺', label: 'ВОЗВРАТ' }
    ],
    categories: [
      {
        name: 'АЭРОДРОМ',
        items: [
          { id: 'p-b1', name: 'ВЗЛЁТНАЯ ПОЛОСА', cost: 1200, icon: '🛫' },
          { id: 'p-b2', name: 'АНГАР', cost: 1000, icon: '🏚' },
          { id: 'p-b3', name: 'РАДАР ПОСАДКИ', cost: 850, icon: '📶' },
          { id: 'p-b4', name: 'ЦУО', cost: 1300, icon: '🏛' }
        ]
      },
      {
        name: 'АВИАЦИЯ',
        items: [
          { id: 'p-v1', name: 'F-3 «СИНСИН»', cost: 1700, icon: '✈' },
          { id: 'p-v2', name: 'ШТУРМОВИК P-2', cost: 1100, icon: '🎯' },
          { id: 'p-v3', name: 'РЭБ-САМОЛЁТ', cost: 1200, icon: '📻' },
          { id: 'p-v4', name: 'ТРАНСПОРТ C-2', cost: 900, icon: '🛩' }
        ]
      },
      {
        name: 'РОБОТЫ',
        items: [
          { id: 'p-r1', name: 'МЕХ «КАБУТО»', cost: 1500, icon: '🤖' },
          { id: 'p-r2', name: 'РАЗВЕД-ПАУК', cost: 650, icon: '🕷' },
          { id: 'p-r3', name: 'ДРОН-ПОДВОДНИК', cost: 800, icon: '🌊' },
          { id: 'p-r4', name: 'СБОРЩИК', cost: 500, icon: '⚙' }
        ]
      },
      {
        name: 'ПВО',
        items: [
          { id: 'p-p1', name: 'AEGIS ASHORE', cost: 1400, icon: '🗼' },
          { id: 'p-p2', name: 'PAC-4', cost: 900, icon: '🚀' },
          { id: 'p-p3', name: 'ЛАЗЕР «ЯТА»', cost: 1150, icon: '🔆' },
          { id: 'p-p4', name: 'РЛС «МУЦУ»', cost: 1000, icon: '📶' }
        ]
      }
    ]
  },
  'independent-iran': {
    screenshot: '/remaster/16_battle_hud_independent_iran.png',
    themeClass: 'theme-independent',
    color: '#e8a13d',
    dim: '#8a5c1c',
    crest: '◉',
    factionLabel: 'НЕЗАВИСИМЫЕ ДЕРЖАВЫ',
    objectives: ['Удержать горные тоннели', 'Не выдать позиции до залпа'],
    superweapon: true,
    unit: { name: 'МИССИЛЬНЫЙ УЗЕЛ «МИРАЖ»', type: 'Координированный удар', armor: 88, hp: 2400, hpMax: 2400 },
    commands: [
      { icon: '🕳', label: 'СПУСК' },
      { icon: '📡', label: 'ЛОЖНЫЙ СИГНАЛ' },
      { icon: '↔', label: 'СМЕНА ПОЗИЦИИ' },
      { icon: '🔺', label: 'ТРЕУГОЛЬНИК' },
      { icon: '🚀', label: 'ПУСК' }
    ],
    categories: [
      {
        name: 'БАЗА',
        items: [
          { id: 'i-b1', name: 'ПОДЗЕМНЫЙ ЦЕХ', cost: 1300, icon: '🕳' },
          { id: 'i-b2', name: 'ГЕНЕРАТОРНАЯ', cost: 750, icon: '⚡' },
          { id: 'i-b3', name: 'СКЛАД РАКЕТ', cost: 1100, icon: '📦' },
          { id: 'i-b4', name: 'КАМУФЛИРОВАННЫЙ ВЫХОД', cost: 600, icon: '🚪' }
        ]
      },
      {
        name: 'МОБ. ГРУППЫ',
        items: [
          { id: 'i-m1', name: 'ПУСКОВАЯ УСТАНОВКА', cost: 1250, icon: '🚚' },
          { id: 'i-m2', name: 'ТЕХНИЧКА', cost: 500, icon: '🔧' },
          { id: 'i-m3', name: 'РАЗВЕД-ПАТРУЛЬ', cost: 400, icon: '👁' },
          { id: 'i-m4', name: 'ПВО «ГЕРБ-3»', cost: 850, icon: '🗼' }
        ]
      },
      {
        name: 'РАКЕТЫ',
        items: [
          { id: 'i-r1', name: 'ФАТЕГ-110', cost: 1000, icon: '🚀' },
          { id: 'i-r2', name: 'КРЫЛАТАЯ «ПАВЕ»', cost: 1400, icon: '☄' },
          { id: 'i-r3', name: 'ПТРК «ДЕЛАВИЯ»', cost: 700, icon: '🎯' },
          { id: 'i-r4', name: 'БАЛЛИСТИЧЕСКАЯ', cost: 2200, icon: '💥' }
        ]
      }
    ]
  },
  'eurasian-base': {
    screenshot: '/remaster/17_battle_hud_eurasian_base.png',
    themeClass: 'theme-eurasian',
    color: '#b06cff',
    dim: '#6a3fa0',
    crest: '❖',
    factionLabel: 'ЕВРАЗИЙСКИЙ ПАКТ',
    objectives: ['Обеспечить живучесть узла РЭБ', 'Восстановить линию снабжения'],
    alert: 'ТРЕВОГА: НАЛЁТ ПРОТИВНИКА',
    unit: { name: 'РЕЛЕЙНО-КОМАНДНЫЙ УЗЕЛ «БЕЛЫЙ ШУМ»', type: 'Центр управления РЭБ', armor: 95, hp: 5000, hpMax: 5000 },
    commands: [
      { icon: '⚙', label: 'МОДЕРНИЗАЦИЯ' },
      { icon: '📡', label: 'ПЕРЕХВАТ' },
      { icon: '🔧', label: 'РЕМОНТ' },
      { icon: '⏱', label: 'МАКСИМАЛЬНАЯ' }
    ],
    categories: [
      {
        name: 'ЗДАНИЯ',
        items: [
          { id: 'eb-b1', name: 'РЕЛЕЙНАЯ МАЧТА', cost: 1100, icon: '📶' },
          { id: 'eb-b2', name: 'ГЕНЕРАТОР', cost: 800, icon: '⚡' },
          { id: 'eb-b3', name: 'БУНКЕР', cost: 1300, icon: '🏚' },
          { id: 'eb-b4', name: 'ДЕПО', cost: 900, icon: '📦' }
        ]
      },
      {
        name: 'ТЕХНИКА',
        items: [
          { id: 'eb-t1', name: 'МОБ. ТЕАТР РЭБ', cost: 1350, icon: '🚚' },
          { id: 'eb-t2', name: 'ТРАНСПОРТ 6х6', cost: 600, icon: '🚙' },
          { id: 'eb-t3', name: 'ТЯГАЧ', cost: 500, icon: '⚙' },
          { id: 'eb-t4', name: 'ЗЕН. ТАНК', cost: 950, icon: '🛡' }
        ]
      },
      {
        name: 'ПВО',
        items: [
          { id: 'eb-p1', name: 'С-500 «ПРОМЕТЕЙ»', cost: 1800, icon: '🗼' },
          { id: 'eb-p2', name: 'ПАНЦИРЬ-С1', cost: 900, icon: '🔫' },
          { id: 'eb-p3', name: 'ТОР-М2', cost: 800, icon: '📡' },
          { id: 'eb-p4', name: 'РЛС КРУГОВАЯ', cost: 1100, icon: '📶' }
        ]
      }
    ]
  },
  'pacific-base': {
    screenshot: '/remaster/18_battle_hud_pacific_base.png',
    themeClass: 'theme-pacific',
    color: '#2fd4c8',
    dim: '#12666c',
    crest: '◈',
    factionLabel: 'ТИХООКЕАНСКИЙ ПАКТ',
    objectives: ['Развернуть оборону побережья', 'Провести разведку архипелага'],
    alert: 'УГРОЗА ПРОТИВНИКА ОБНАРУЖЕНА',
    unit: { name: 'РАЗВЕДЫВАТЕЛЬНЫЙ ЦЕНТР «АЙЛАНД»', type: 'Береговой узел наблюдения', armor: 90, hp: 3800, hpMax: 3800 },
    commands: [
      { icon: '👁', label: 'РАЗВЕДКА' },
      { icon: '🔧', label: 'ИНЖЕНЕРЫ' },
      { icon: '🚀', label: 'ПВО' },
      { icon: '⚙', label: 'РОБЫ' }
    ],
    categories: [
      {
        name: 'ЗДАНИЯ',
        items: [
          { id: 'pb-b1', name: 'ЦЕНТР НАБЛЮДЕНИЯ', cost: 1200, icon: '🏛' },
          { id: 'pb-b2', name: 'ПРИЧАЛ', cost: 900, icon: '⚓' },
          { id: 'pb-b3', name: 'ГЕНЕРАТОР', cost: 700, icon: '⚡' },
          { id: 'pb-b4', name: 'УКРЕПЛЕНИЕ', cost: 1000, icon: '🧱' }
        ]
      },
      {
        name: 'ФЛОТ',
        items: [
          { id: 'pb-f1', name: 'ПАТРУЛЬНЫЙ КАТЕР', cost: 700, icon: '⛴' },
          { id: 'pb-f2', name: 'РОБОТ-ТРАЛ', cost: 800, icon: '🤖' },
          { id: 'pb-f3', name: 'ЭСКОРТ «МИНАЗУ»', cost: 1500, icon: '🚢' },
          { id: 'pb-f4', name: 'ДЕСАНТНЫЙ МОДУЛЬ', cost: 1100, icon: '🌊' }
        ]
      },
      {
        name: 'ПВО',
        items: [
          { id: 'pb-p1', name: 'БЕРЕГОВАЯ БАТАРЕЯ', cost: 1200, icon: '🎯' },
          { id: 'pb-p2', name: 'PAC-4', cost: 900, icon: '🚀' },
          { id: 'pb-p3', name: 'РЛС «ФУДЗИ»', cost: 1000, icon: '📶' },
          { id: 'pb-p4', name: 'ЛАЗЕРНЫЙ КОМПЛЕКС', cost: 1250, icon: '🔆' }
        ]
      }
    ]
  }
};

export const InGameHUD: React.FC = () => {
  const [searchParams] = useSearchParams();
  const navigate = useNavigate();
  const mode = searchParams.get('mode') || 'eurasian-ground';

  const cfg = MODE_CONFIGS[mode] || MODE_CONFIGS['eurasian-ground'];

  const [activeTab, setActiveTab] = useState(cfg.categories[0].name);
  const [productionQueue, setProductionQueue] = useState<{ id: string; name: string; progress: number }[]>([]);
  const [selectedGroup, setSelectedGroup] = useState(1);

  const handleQueueItem = (item: BuildItem) => {
    setProductionQueue(q => [...q, { id: Date.now().toString(), name: item.name, progress: 15 }]);
  };

  const handleRemoveQueueItem = (id: string) => {
    setProductionQueue(q => q.filter(item => item.id !== id));
  };

  return (
    <div
      className={cfg.themeClass}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('${cfg.screenshot}') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '12px 14px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* ===== TOP STRIP ===== */}
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', zIndex: 20 }}>
        {/* Top-left objectives */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', width: '330px' }}>
          <div className="ra4-panel" style={{ padding: '11px 15px' }}>
            <div style={{
              fontFamily: "'Oswald', sans-serif",
              color: cfg.color,
              fontSize: '13px',
              fontWeight: 700,
              letterSpacing: '2px',
              marginBottom: '8px'
            }}>
              ОСНОВНЫЕ ЗАДАЧИ
            </div>
            {cfg.objectives.map((o, i) => (
              <div key={i} style={{ display: 'flex', alignItems: 'center', gap: '9px', marginBottom: '5px' }}>
                <span style={{
                  width: '13px', height: '13px', flexShrink: 0,
                  borderRadius: '50%',
                  border: `1.5px solid ${i === 0 ? cfg.color : '#8b93a2'}`,
                  background: i === 0 ? `${cfg.color}44` : 'transparent'
                }} />
                <span style={{ fontSize: '12px', color: '#dfe3ea' }}>{o}</span>
              </div>
            ))}
          </div>

          {/* Alert badge */}
          {cfg.alert && (
            <div className="alert-pulse clip-bevel-sm" style={{
              alignSelf: 'flex-start',
              display: 'flex',
              alignItems: 'center',
              gap: '9px',
              padding: '8px 16px',
              background: `linear-gradient(90deg, ${cfg.dim}, rgba(6,5,12,0.92))`,
              border: `1px solid ${cfg.color}`,
              borderRadius: '3px',
              boxShadow: `0 0 16px ${cfg.color}77`
            }}>
              <span style={{ fontSize: '15px' }}>{cfg.superweapon ? '🚀' : '🛡'}</span>
              <strong style={{ color: '#ffffff', fontFamily: "'Oswald', sans-serif", fontSize: '12.5px', letterSpacing: '1.5px' }}>
                {cfg.alert}
              </strong>
            </div>
          )}

          {/* Superweapon countdown (iran mode) */}
          {cfg.superweapon && (
            <div className="alert-pulse" style={{
              alignSelf: 'flex-start',
              textAlign: 'center',
              padding: '8px 26px',
              background: 'linear-gradient(90deg, #8a1408, #ff3c28)',
              border: '1px solid #ff8a75',
              borderRadius: '4px',
              boxShadow: '0 0 24px rgba(255,60,40,0.8)'
            }}>
              <div style={{ fontFamily: "'Oswald', sans-serif", color: '#fff', fontWeight: 800, fontSize: '13px', letterSpacing: '2px' }}>
                САТУРАЦИОННЫЙ УДАР НАЧАТ
              </div>
              <div style={{ fontFamily: "'Orbitron', sans-serif", color: '#ffd9a0', fontSize: '24px', fontWeight: 900, lineHeight: 1.1 }}>
                00:18
              </div>
            </div>
          )}
        </div>

        {/* Top-right resources + minimap + tool strip */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', alignItems: 'flex-end' }}>
          <div style={{ display: 'flex', gap: '8px', alignItems: 'stretch' }}>
            <div className="ra4-panel" style={{ padding: '7px 14px', display: 'flex', gap: '18px', alignItems: 'center' }}>
              <span style={{ color: cfg.color, fontSize: '13.5px', fontWeight: 700 }}>💠 23 450</span>
              <span style={{ color: '#57e89a', fontSize: '13.5px', fontWeight: 700 }}>⚡ 17 820</span>
              <span style={{ color: '#ffd76a', fontSize: '13.5px', fontWeight: 700 }}>🔶 9 680</span>
              <span style={{ color: '#dfe3ea', fontSize: '13.5px', fontWeight: 700 }}>👥 186 / 200</span>
            </div>
            <div style={{ display: 'flex', gap: '6px' }}>
              {['▦', '⚠', '⚙'].map(ic => (
                <button
                  key={ic}
                  onClick={() => navigate('/menu')}
                  style={{
                    width: '34px',
                    background: 'rgba(8,7,14,0.85)',
                    border: '1px solid rgba(255,255,255,0.2)',
                    borderRadius: '4px',
                    color: '#cfd4dd',
                    cursor: 'pointer',
                    fontSize: '14px'
                  }}
                >
                  {ic}
                </button>
              ))}
            </div>
          </div>

          {/* Minimap + tool strip row */}
          <div style={{ display: 'flex', gap: '8px', alignItems: 'stretch' }}>
            {/* Vertical tools between field and minimap */}
            <div style={{
              display: 'flex',
              flexDirection: 'column',
              gap: '6px',
              justifyContent: 'center'
            }}>
              {['◎', '🛡', '👁', '▦'].map((ic, i) => (
                <button
                  key={ic + i}
                  onClick={() => navigate('/briefing')}
                  style={{
                    width: '38px',
                    height: '38px',
                    background: i === 0 ? `${cfg.color}33` : 'rgba(8,7,14,0.85)',
                    border: `1px solid ${i === 0 ? cfg.color : 'rgba(255,255,255,0.2)'}`,
                    borderRadius: '4px',
                    color: i === 0 ? cfg.color : '#aab0bc',
                    cursor: 'pointer',
                    fontSize: '15px'
                  }}
                >
                  {ic}
                </button>
              ))}
            </div>

            {/* Minimap */}
            <div className="ra4-panel" style={{ width: '270px', height: '170px', padding: '6px', position: 'relative' }}>
              <div style={{
                width: '100%', height: '100%',
                borderRadius: '3px',
                background:
                  `radial-gradient(circle at 40% 40%, ${cfg.color}30, transparent 50%), radial-gradient(circle at 70% 70%, rgba(255,80,60,0.28), transparent 45%), linear-gradient(150deg, #14182a, #0a0c16)`,
                position: 'relative',
                overflow: 'hidden'
              }}>
                <svg viewBox="0 0 100 100" style={{ position: 'absolute', inset: 0, width: '100%', height: '100%' }}>
                  <path d="M10,80 L30,55 L55,65 L80,40 L95,50" stroke={`${cfg.color}aa`} strokeWidth="0.8" fill="none" strokeDasharray="3 2" />
                  <path d="M20,20 L45,35 L60,25 L85,30" stroke="rgba(255,255,255,0.25)" strokeWidth="0.6" fill="none" />
                  <polygon points="55,55 85,48 90,72 62,78" fill={`${cfg.color}22`} stroke={cfg.color} strokeWidth="0.7" />
                  {[30, 38, 46].map((y, i) => (
                    <circle key={i} cx={20 + i * 8} cy={y} r="1.4" fill="#ff5c47">
                      <animate attributeName="opacity" values="1;0.2;1" dur={`${1.2 + i * 0.4}s`} repeatCount="indefinite" />
                    </circle>
                  ))}
                  <circle cx="66" cy="60" r="2" fill={cfg.color}>
                    <animate attributeName="opacity" values="1;0.3;1" dur="1s" repeatCount="indefinite" />
                  </circle>
                </svg>
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* ===== MIDDLE SPACER ===== */}
      <div style={{ flex: 1 }} />

      {/* ===== RIGHT BUILD SIDEBAR ===== */}
      <div style={{
        position: 'absolute',
        right: '14px',
        top: '210px',
        bottom: '230px',
        width: '372px',
        zIndex: 15,
        display: 'flex',
        flexDirection: 'column',
        minHeight: 0
      }}>
        {/* Tabs */}
        <div style={{ display: 'flex' }}>
          {cfg.categories.map(c => (
            <button
              key={c.name}
              onClick={() => setActiveTab(c.name)}
              style={{
                flex: 1,
                height: '36px',
                background: activeTab === c.name
                  ? `linear-gradient(180deg, ${cfg.color}44, ${cfg.dim}66)`
                  : 'rgba(8,7,14,0.88)',
                border: `1px solid ${activeTab === c.name ? cfg.color : 'rgba(255,255,255,0.18)'}`,
                borderBottom: 'none',
                borderRadius: '4px 4px 0 0',
                color: activeTab === c.name ? '#ffffff' : '#98a0ae',
                fontFamily: "'Oswald', sans-serif",
                fontSize: '11.5px',
                fontWeight: 700,
                letterSpacing: '1px',
                cursor: 'pointer'
              }}
            >
              {c.name}
            </button>
          ))}
        </div>

        {/* Category body */}
        <div style={{
          flex: 1,
          minHeight: 0,
          overflowY: 'auto',
          background: 'linear-gradient(180deg, rgba(10,8,16,0.94), rgba(6,5,10,0.97))',
          border: `1px solid ${cfg.color}55`,
          borderTop: `1px solid ${cfg.color}aa`,
          borderRadius: '0 0 4px 4px',
          padding: '10px'
        }}>
          <div style={{ color: cfg.color, fontSize: '12px', fontWeight: 700, letterSpacing: '1px', marginBottom: '8px' }}>
            ＋ {activeTab}
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '7px' }}>
            {(cfg.categories.find(c => c.name === activeTab)?.items || []).map(item => (
              <button
                key={item.id}
                onClick={() => handleQueueItem(item)}
                style={{
                  background: 'rgba(20,16,32,0.9)',
                  border: `1px solid rgba(255,255,255,0.16)`,
                  borderRadius: '3px',
                  padding: '7px 5px 6px 5px',
                  cursor: 'pointer',
                  display: 'flex',
                  flexDirection: 'column',
                  alignItems: 'center',
                  gap: '4px',
                  transition: 'border-color 0.15s'
                }}
              >
                <span style={{ fontSize: '19px', filter: `drop-shadow(0 0 6px ${cfg.color}88)` }}>{item.icon}</span>
                <span style={{
                  color: '#e8ebf0',
                  fontSize: '8.5px',
                  fontWeight: 600,
                  textAlign: 'center',
                  lineHeight: 1.2,
                  whiteSpace: 'normal'
                }}>
                  {item.name}
                </span>
                <span style={{ color: '#ffd76a', fontSize: '9px', fontWeight: 700 }}>◉ {item.cost.toLocaleString('ru-RU')}</span>
              </button>
            ))}
          </div>

          {/* Production queue inside sidebar */}
          {productionQueue.length > 0 && (
            <div style={{ marginTop: '10px', borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '8px' }}>
              <div style={{ color: cfg.color, fontSize: '10px', fontWeight: 700, letterSpacing: '1px', marginBottom: '6px' }}>
                ОЧЕРЕДЬ ({productionQueue.length})
              </div>
              {productionQueue.map(q => (
                <div key={q.id} style={{ display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '5px' }}>
                  <span style={{ flex: 1, color: '#cdd3dd', fontSize: '10px', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>
                    {q.name}
                  </span>
                  <div style={{ width: '70px', height: '4px', background: 'rgba(255,255,255,0.1)', borderRadius: '2px', overflow: 'hidden' }}>
                    <div style={{ width: `${q.progress}%`, height: '100%', background: cfg.color }} />
                  </div>
                  <span onClick={() => handleRemoveQueueItem(q.id)} style={{ color: '#ff5c47', cursor: 'pointer', fontSize: '10px', fontWeight: 800 }}>✕</span>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {/* ===== BOTTOM STRIP ===== */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '330px 1fr',
        gap: '16px',
        alignItems: 'end',
        zIndex: 20
      }}>
        {/* Bottom-left: control groups + unit grid */}
        <div style={{
          background: 'linear-gradient(180deg, rgba(10,8,16,0.94), rgba(6,5,10,0.97))',
          border: `1px solid ${cfg.color}55`,
          borderRadius: '6px',
          padding: '10px'
        }}>
          {/* Group hotkeys */}
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(10, 1fr)', gap: '4px', marginBottom: '8px' }}>
            {[1, 2, 3, 4, 5, 6, 7, 8, 9, 0].map(g => (
              <div
                key={g}
                onClick={() => setSelectedGroup(g)}
                style={{
                  textAlign: 'center',
                  padding: '3px 0',
                  background: selectedGroup === g ? cfg.color : 'rgba(255,255,255,0.07)',
                  color: selectedGroup === g ? '#0b0712' : '#8b93a2',
                  fontSize: '11px',
                  fontWeight: 800,
                  borderRadius: '2px',
                  cursor: 'pointer'
                }}
              >
                {g}
              </div>
            ))}
          </div>

          {/* Unit selection grid */}
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(5, 1fr)', gap: '5px' }}>
            {Array.from({ length: 10 }).map((_, i) => (
              <div key={i} style={{
                aspectRatio: '1.35',
                background: 'rgba(255,255,255,0.05)',
                border: '1px solid rgba(255,255,255,0.12)',
                borderRadius: '3px',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontSize: '15px',
                opacity: i < 6 ? 1 : 0.35,
                cursor: 'pointer',
                position: 'relative'
              }}>
                {['🛡', '🚜', '🚙', '⚙', '🚀'][i % 5]}
                <span style={{
                  position: 'absolute',
                  top: '1px',
                  left: '3px',
                  color: cfg.color,
                  fontSize: '8px',
                  fontWeight: 800
                }}>
                  {[5, 3, 2, 3, 8][i % 5]}
                </span>
              </div>
            ))}
          </div>
        </div>

        {/* Bottom-center: selected unit inspector */}
        <div style={{
          maxWidth: '760px',
          justifySelf: 'center',
          width: '100%',
          background: 'linear-gradient(180deg, rgba(10,8,16,0.94), rgba(6,5,10,0.97))',
          border: `1px solid ${cfg.color}77`,
          borderRadius: '6px',
          boxShadow: `inset 0 0 30px ${cfg.color}0d`,
          padding: '12px 16px',
          display: 'flex',
          flexDirection: 'column',
          gap: '9px'
        }}>
          <div style={{ display: 'flex', gap: '14px', alignItems: 'stretch' }}>
            {/* Portrait */}
            <div style={{
              width: '96px',
              borderRadius: '4px',
              border: `1px solid ${cfg.color}66`,
              background: `linear-gradient(160deg, ${cfg.dim}88, rgba(6,5,12,0.95))`,
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              fontSize: '40px'
            }}>
              🛡
            </div>

            {/* Info + bars */}
            <div style={{ flex: 1, display: 'flex', flexDirection: 'column', gap: '6px' }}>
              <div>
                <div style={{ fontFamily: "'Oswald', sans-serif", color: '#ffffff', fontSize: '16px', fontWeight: 800, letterSpacing: '1px' }}>
                  {cfg.unit.name}
                </div>
                <div style={{ color: '#98a0ae', fontSize: '11px' }}>
                  {cfg.unit.type}
                </div>
              </div>

              <div>
                <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '10px', color: '#98a0ae', marginBottom: '2px' }}>
                  <span>БРОНЯ <strong style={{ color: '#57e89a' }}>{cfg.unit.armor}%</strong></span>
                  <span>{cfg.unit.hp} / {cfg.unit.hpMax}</span>
                </div>
                <div style={{ height: '7px', background: 'rgba(0,0,0,0.8)', borderRadius: '2px', overflow: 'hidden', border: '1px solid rgba(255,255,255,0.12)' }}>
                  <div style={{
                    width: `${Math.round((cfg.unit.hp / cfg.unit.hpMax) * 100)}%`,
                    height: '100%',
                    background: 'linear-gradient(90deg, #1fae52, #57e89a)'
                  }} />
                </div>
              </div>

              {/* Ammo slots */}
              <div style={{ display: 'flex', gap: '6px', marginTop: '2px' }}>
                {[
                  { count: 2, icon: '▣' },
                  { count: 3, icon: '▤' },
                  { count: 2, icon: '▥' },
                  { count: 0, icon: '▢' }
                ].map((a, i) => (
                  <div key={i} style={{
                    minWidth: '44px',
                    textAlign: 'center',
                    padding: '4px 6px',
                    background: 'rgba(255,255,255,0.05)',
                    border: '1px solid rgba(255,255,255,0.14)',
                    borderRadius: '3px',
                    color: a.count > 0 ? '#ffd76a' : '#555c68',
                    fontSize: '10px'
                  }}>
                    {a.icon}<br /><strong>{a.count}</strong>
                  </div>
                ))}
                <div style={{
                  marginLeft: 'auto',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  width: '54px',
                  borderRadius: '3px',
                  border: `1px solid ${cfg.color}88`,
                  background: `${cfg.color}22`,
                  fontSize: '22px',
                  color: cfg.color
                }}>
                  {cfg.crest}
                </div>
              </div>
            </div>
          </div>

          {/* Command buttons */}
          <div style={{ display: 'grid', gridTemplateColumns: `repeat(${cfg.commands.length}, 1fr)`, gap: '7px' }}>
            {cfg.commands.map(cmd => (
              <button
                key={cmd.label}
                className="clip-bevel-sm"
                style={{
                  height: '46px',
                  background: 'rgba(20,16,32,0.9)',
                  border: `1px solid ${cfg.color}66`,
                  borderRadius: '3px',
                  color: '#ffffff',
                  cursor: 'pointer',
                  display: 'flex',
                  flexDirection: 'column',
                  alignItems: 'center',
                  justifyContent: 'center',
                  gap: '3px'
                }}
              >
                <span style={{ fontSize: '15px' }}>{cmd.icon}</span>
                <span style={{ fontSize: '8.5px', fontWeight: 700, letterSpacing: '0.5px' }}>{cmd.label}</span>
              </button>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
};
