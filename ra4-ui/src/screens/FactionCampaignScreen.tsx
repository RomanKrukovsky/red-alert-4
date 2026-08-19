import React, { useState } from 'react';
import { useParams, useNavigate, useSearchParams } from 'react-router-dom';

interface FactionConfig {
  name: string;
  subname: string;
  tagline: string;
  commander: string;
  quote: string;
  bgScreenshot: string;
  themeClass: string;
  accentColor: string;
  progressPercent: number;
  missionsCompleted: string;
  currentMissionName: string;
  difficulty: string;
  lore: string;
  chapters: { num: string; name: string; completed: boolean; current?: boolean }[];
  traits?: { title: string; desc: string }[];
  urgentMsg: string;
}

const FACTION_DATA: Record<string, FactionConfig> = {
  ussr: {
    name: 'СССР',
    subname: 'СОВЕТСКИЙ СОЮЗ',
    tagline: 'СЛАВА СОВЕТСКОМУ СОЮЗУ!',
    commander: 'МАРШАЛ ВИКТОР СОКОЛОВ',
    quote: '«ПОБЕДА БУДЕТ ЗА НАМИ!»',
    bgScreenshot: '/screenshots/4.png',
    themeClass: 'theme-ussr',
    accentColor: '#ff2222',
    progressPercent: 58,
    missionsCompleted: '14 / 24',
    currentMissionName: 'МИССИЯ 15: ЛЕДЯНОЙ ШТОРМ',
    difficulty: 'ВЕТЕРАН',
    lore: 'Враг у наших границ. Империалисты и предатели стремятся задушить нашу Родину в огне и лжи. Только дисциплина, сталь и вера в дело Ленина приведут нас к окончательной победе. Товарищ, судьба мира в твоих приказах!',
    chapters: [
      { num: 'I', name: 'Красный рассвет', completed: true },
      { num: 'II', name: 'Берлинский рубеж', completed: true },
      { num: 'III', name: 'Ледяной шторм', completed: false, current: true },
      { num: 'IV', name: 'Пепел Атлантики', completed: false },
      { num: 'V', name: 'Окончательный ответ', completed: false }
    ],
    urgentMsg: 'РАЗВЕДКА СООБЩАЕТ: СИЛЫ АЛЬЯНСА КОНЦЕНТРИРУЮТСЯ НА СЕВЕРЕ.'
  },
  allies: {
    name: 'АЛЬЯНС',
    subname: 'СОЕДИНЁННЫЕ ШТАТЫ И СОЮЗНИКИ',
    tagline: 'ВЕРНОСТЬ. ЕДИНСТВО. ПОБЕДА.',
    commander: 'ПРЕЗИДЕНТ ЭЛЕАНОР УОРД',
    quote: '«СВОБОДА НЕ ДАЁТСЯ ДАРОМ — МЫ ЕЁ ЗАЩИТИМ.»',
    bgScreenshot: '/screenshots/5.png',
    themeClass: 'theme-allies',
    accentColor: '#0088ff',
    progressPercent: 37,
    missionsCompleted: '06 / 16',
    currentMissionName: 'ГЛАВА 3: ЛЕДЯНОЙ РАССВЕТ',
    difficulty: 'НОРМАЛЬНО',
    lore: 'Альянс стоит на страже свободы и процветания. Перед лицом новой угрозы мы объединяем нации, передовые технологии и волю, чтобы обеспечить мир и стабильность в неопределённом мире.',
    chapters: [
      { num: 'I', name: 'Первый контакт', completed: true },
      { num: 'II', name: 'Щит свободы', completed: true },
      { num: 'III', name: 'Ледяной рассвет', completed: false, current: true },
      { num: 'IV', name: 'Операция «Буря»', completed: false },
      { num: 'V', name: 'Морской бастион', completed: false },
      { num: 'VI', name: 'Освобождение', completed: false },
      { num: 'VII', name: 'Железный занавес', completed: false },
      { num: 'VIII', name: 'Мировой мир', completed: false }
    ],
    urgentMsg: 'РАЗВЕДКА СООБЩАЕТ О НОВЫХ ПЕРЕДВИЖЕНИЯХ ПРОТИВНИКА В АРКТИЧЕСКОМ РЕГИОНЕ.'
  },
  ec: {
    name: 'ВОСТОЧНАЯ КОАЛИЦИЯ',
    subname: 'ИМПЕРИЯ ВОСТОКА',
    tagline: 'МУДРОСТЬ ДРАКОНА. СИЛА ПРОГРЕССА.',
    commander: 'ВЕРХОВНЫЙ ГЕНЕРАЛ ГАО',
    quote: '«БУДУЩЕЕ ПРИНАДЛЕЖИТ ТЕМ, КТО ЕГО СОЗДАЁТ.»',
    bgScreenshot: '/screenshots/6.png',
    themeClass: 'theme-ec',
    accentColor: '#00ff66',
    progressPercent: 63,
    missionsCompleted: '17 / 27',
    currentMissionName: 'МИССИЯ 18: НЕБЕСНЫЙ ЩИТ',
    difficulty: 'ВЕТЕРАН',
    lore: 'Восточная коалиция объединяет передовые нации Азии для защиты суверенитета и построения справедливого многополярного мира. Дисциплина, инновации и единство — ключ к будущему без войны и угнетения.',
    chapters: [
      { num: 'I', name: 'Пробуждение', completed: true },
      { num: 'II', name: 'Шёлковый путь', completed: true },
      { num: 'III', name: 'Восход дракона', completed: false, current: true },
      { num: 'IV', name: 'Небесный щит', completed: false },
      { num: 'V', name: 'Гармония огня', completed: false },
      { num: 'VI', name: 'Нефритовый триумф', completed: false }
    ],
    traits: [
      { title: 'ГАРМОНИЯ РЕСУРСОВ', desc: 'Все здания получают бонус при сочетании разных построек.' },
      { title: 'ДРОН-СЕТИ', desc: 'Уникальные боевые дроны и разведывательные рои.' },
      { title: 'РАСПРЕДЕЛЁННОЕ ПРОИЗВОДСТВО', desc: 'Фабрики возле ресурсов приносят дополнительный доход.' },
      { title: 'ТЕХНОЛОГИИ БУДУЩЕГО', desc: 'Передовые разработки и энергетическое нанооружие.' }
    ],
    urgentMsg: 'КОАЛИЦИОННЫЕ СИЛЫ УСПЕШНО ИСПЫТАЛИ ПРОТОТИП ЭНЕРГЕТИЧЕСКОГО ЩИТА.'
  },
  chrono: {
    name: 'ХРОНОЛЕГИОН',
    subname: 'ПОВЕЛИТЕЛИ ТЕМПОРАЛЬНОСТИ',
    tagline: 'ВЛАСТЬ НАД ВРЕМЕНЕМ. ГОСПОДСТВО НАД ВСЕЛЕННОЙ.',
    commander: 'ВЕСТНИК ВРЕМЕНИ / АРХОНТ',
    quote: '«ВРЕМЯ НЕ ЖДЁТ — ОНО ПОДЧИНЯЕТСЯ.»',
    bgScreenshot: '/screenshots/7.png',
    themeClass: 'theme-chrono',
    accentColor: '#aa00ff',
    progressPercent: 25,
    missionsCompleted: '04 / 16',
    currentMissionName: 'ГЛАВА 1: РАЗЛОМ ВРЕМЕНИ',
    difficulty: 'ВЕТЕРАН',
    lore: 'Они пришли не из этого времени. Хронолегион существует вне линейности, наблюдая за историей и вмешиваясь в неё. Их цель — не завоевание, а исправление. Те, кто стоит на их пути, будут стёрты из всех времён.',
    chapters: [
      { num: 'I', name: 'Разлом времени', completed: false, current: true },
      { num: 'II', name: 'Темпоральный якорь', completed: false },
      { num: 'III', name: 'Хроношторм', completed: false },
      { num: 'IV', name: 'Парадокс прошлого', completed: false },
      { num: 'V', name: 'Вечность', completed: false }
    ],
    urgentMsg: 'ХРОНОПРОТОКОЛ АКТИВЕН: ВРЕМЕННЫЕ АНОМАЛИИ ЗАФИКСИРОВАНЫ ПО ВСЕМУ МИРУ.'
  }
};

export const FactionCampaignScreen: React.FC = () => {
  const { faction } = useParams<{ faction: string }>();
  const [searchParams] = useSearchParams();
  const navigate = useNavigate();

  const currentFactionKey = faction && FACTION_DATA[faction] ? faction : 'ussr';
  const isDetailView = searchParams.get('view') === 'detail';
  const config = FACTION_DATA[currentFactionKey];

  const [selectedDifficulty, setSelectedDifficulty] = useState(config.difficulty);

  const handleLaunchMission = () => {
    navigate('/strategic-map');
  };

  return (
    <div
      className={config.themeClass}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('${isDetailView ? '/screenshots/18.png' : config.bgScreenshot}') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '20px 40px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Oswald', sans-serif"
      }}
    >
      {/* Top Header Strip */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderBottom: `1px solid rgba(255,255,255,0.15)`,
        paddingBottom: '12px',
        zIndex: 10
      }}>
        {/* Left Profile */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '14px' }}>
          <div style={{
            width: '40px',
            height: '40px',
            borderRadius: '4px',
            background: 'rgba(0,0,0,0.7)',
            border: `1px solid ${config.accentColor}`,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            color: config.accentColor,
            fontSize: '20px'
          }}>
            ★
          </div>
          <div>
            <div style={{ color: config.accentColor, fontSize: '15px', fontWeight: 700 }}>
              {config.commander}
            </div>
            <div style={{ color: '#aaa', fontSize: '11px' }}>
              УРОВЕНЬ 45 ★ ВЕТЕРАН
            </div>
          </div>
        </div>

        {/* Center Title */}
        <div style={{ textAlign: 'center' }}>
          <div style={{ color: '#aaa', fontSize: '11px', letterSpacing: '4px' }}>COMMAND & CONQUER™</div>
          <div style={{ color: config.accentColor, fontSize: '24px', fontWeight: 800, letterSpacing: '2px', lineHeight: 1 }}>
            RED ALERT 4
          </div>
        </div>

        {/* Right Tools */}
        <div style={{ display: 'flex', gap: '10px' }}>
          {['⚙', '👥', '🛡', '🏆', '⏻'].map((icon, i) => (
            <button
              key={i}
              onClick={() => {
                if (i === 4) navigate('/menu');
                if (i === 0) navigate('/video-comms');
              }}
              style={{
                width: '36px',
                height: '36px',
                background: 'rgba(0,0,0,0.6)',
                border: '1px solid rgba(255,255,255,0.15)',
                borderRadius: '4px',
                color: '#ddd',
                cursor: 'pointer'
              }}
            >
              {icon}
            </button>
          ))}
        </div>
      </div>

      {/* Main Content Area */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '260px 1fr 420px',
        gap: '30px',
        flex: 1,
        alignItems: 'center',
        zIndex: 5,
        margin: '20px 0'
      }}>
        {/* Left Faction Selector Strip */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
          <div style={{ color: '#aaa', fontSize: '12px', letterSpacing: '2px', marginBottom: '4px' }}>
            ВЫБОР КАМПАНИИ:
          </div>
          {Object.keys(FACTION_DATA).map(key => {
            const fac = FACTION_DATA[key];
            const isSelected = key === currentFactionKey;
            return (
              <button
                key={key}
                onClick={() => navigate(`/campaign/${key}`)}
                className="clip-bevel-sm"
                style={{
                  height: '52px',
                  background: isSelected
                    ? `linear-gradient(90deg, ${fac.accentColor} 0%, rgba(0,0,0,0.9) 100%)`
                    : 'rgba(15,10,12,0.85)',
                  border: `1px solid ${isSelected ? fac.accentColor : 'rgba(255,255,255,0.15)'}`,
                  color: isSelected ? '#ffffff' : '#bbb',
                  padding: '0 16px',
                  fontSize: '15px',
                  fontWeight: 700,
                  letterSpacing: '1.5px',
                  display: 'flex',
                  alignItems: 'center',
                  gap: '12px',
                  cursor: 'pointer',
                  boxShadow: isSelected ? `0 0 15px ${fac.accentColor}` : 'none'
                }}
              >
                <span>★</span>
                <span>{fac.name}</span>
              </button>
            );
          })}
        </div>

        {/* Center Main Narrative & Stats Area */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '16px', maxWidth: '640px' }}>
          <div>
            <div style={{ color: config.accentColor, fontSize: '15px', letterSpacing: '3px', fontWeight: 700 }}>
              ★ КАМПАНИЯ ★
            </div>
            <h1 style={{
              color: config.accentColor,
              fontSize: '4.5rem',
              fontWeight: 900,
              margin: '0 0 4px 0',
              lineHeight: 1,
              textShadow: `0 0 30px ${config.accentColor}`
            }}>
              {config.name}
            </h1>
            <div style={{ color: '#ffdd00', fontSize: '15px', fontWeight: 700, letterSpacing: '2px', marginBottom: '10px' }}>
              {config.tagline}
            </div>
            <p style={{
              color: '#d0d0d0',
              fontSize: '14px',
              lineHeight: 1.6,
              fontFamily: "'Inter', sans-serif",
              background: 'rgba(0,0,0,0.6)',
              padding: '14px',
              borderRadius: '4px',
              borderLeft: `4px solid ${config.accentColor}`
            }}>
              {config.lore}
            </p>
          </div>

          {/* Progress Bar & Difficulty */}
          <div className="ra4-panel clip-bevel-sm" style={{ padding: '14px 18px', border: `1px solid ${config.accentColor}` }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '6px' }}>
              <span style={{ color: '#aaa', fontSize: '12px', letterSpacing: '1px' }}>ПРОГРЕСС КАМПАНИИ:</span>
              <strong style={{ color: config.accentColor, fontSize: '14px' }}>{config.progressPercent}% ({config.missionsCompleted})</strong>
            </div>
            <div style={{
              width: '100%',
              height: '8px',
              background: 'rgba(0,0,0,0.8)',
              borderRadius: '4px',
              overflow: 'hidden',
              border: '1px solid rgba(255,255,255,0.1)'
            }}>
              <div style={{
                width: `${config.progressPercent}%`,
                height: '100%',
                background: `linear-gradient(90deg, ${config.accentColor}, #ffdd00)`
              }} />
            </div>
          </div>
        </div>

        {/* Right Action & Chapter Tracker Panel */}
        <div className="ra4-panel clip-bevel-md" style={{
          padding: '24px',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          border: `1px solid ${config.accentColor}`
        }}>
          <div>
            <div style={{ color: config.accentColor, fontSize: '13px', fontWeight: 700, letterSpacing: '2px' }}>
              ГЛАВЫ КАМПАНИИ
            </div>
            <div style={{
              display: 'flex',
              gap: '6px',
              margin: '14px 0 20px 0',
              flexWrap: 'wrap'
            }}>
              {config.chapters.map((ch, idx) => (
                <div
                  key={idx}
                  style={{
                    width: '36px',
                    height: '36px',
                    borderRadius: '4px',
                    background: ch.current
                      ? config.accentColor
                      : (ch.completed ? 'rgba(50,50,50,0.8)' : 'rgba(15,10,15,0.7)'),
                    border: `1px solid ${ch.current ? '#ffffff' : (ch.completed ? config.accentColor : 'rgba(255,255,255,0.15)')}`,
                    color: ch.current ? '#ffffff' : (ch.completed ? config.accentColor : '#666'),
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    fontWeight: 700,
                    fontSize: '13px',
                    boxShadow: ch.current ? `0 0 12px ${config.accentColor}` : 'none'
                  }}
                >
                  {ch.num}
                </div>
              ))}
            </div>

            <div style={{ color: '#fff', fontSize: '15px', fontWeight: 700, marginBottom: '6px' }}>
              {config.currentMissionName}
            </div>

            {/* Faction traits (if present) */}
            {config.traits && (
              <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', marginTop: '14px' }}>
                {config.traits.map((t, idx) => (
                  <div key={idx} style={{ fontSize: '12px', borderLeft: `2px solid ${config.accentColor}`, paddingLeft: '8px' }}>
                    <div style={{ color: config.accentColor, fontWeight: 700 }}>{t.title}</div>
                    <div style={{ color: '#aaa' }}>{t.desc}</div>
                  </div>
                ))}
              </div>
            )}
          </div>

          <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', marginTop: '20px' }}>
            <button
              onClick={handleLaunchMission}
              className="clip-bevel-sm"
              style={{
                background: `linear-gradient(180deg, ${config.accentColor} 0%, rgba(30,5,5,0.9) 100%)`,
                border: `1px solid ${config.accentColor}`,
                color: '#ffffff',
                padding: '14px',
                fontSize: '18px',
                fontWeight: 800,
                letterSpacing: '2px',
                cursor: 'pointer',
                boxShadow: `0 0 20px ${config.accentColor}`
              }}
            >
              ★ ПРОДОЛЖИТЬ
            </button>
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '10px' }}>
              <button
                onClick={handleLaunchMission}
                style={{
                  background: 'rgba(20,10,10,0.8)',
                  border: '1px solid rgba(255,255,255,0.2)',
                  color: '#fff',
                  padding: '10px',
                  fontSize: '13px',
                  fontWeight: 600,
                  cursor: 'pointer'
                }}
              >
                НОВАЯ ИГРА
              </button>
              <button
                onClick={() => navigate('/briefing')}
                style={{
                  background: 'rgba(20,10,10,0.8)',
                  border: '1px solid rgba(255,255,255,0.2)',
                  color: '#fff',
                  padding: '10px',
                  fontSize: '13px',
                  fontWeight: 600,
                  cursor: 'pointer'
                }}
              >
                БРИФИНГ
              </button>
            </div>
          </div>
        </div>
      </div>

      {/* Bottom Ticker & Navigation */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderTop: '1px solid rgba(255,255,255,0.15)',
        paddingTop: '8px',
        zIndex: 10
      }}>
        <button
          onClick={() => navigate('/campaign-select')}
          className="ra4-btn-ussr clip-bevel-sm"
          style={{ padding: '8px 24px', fontSize: '14px' }}
        >
          ‹ НАЗАД
        </button>

        <div style={{ color: config.accentColor, fontSize: '12px', letterSpacing: '2px' }}>
          {config.urgentMsg}
        </div>

        <div style={{ color: '#00ff66', fontSize: '12px' }}>
          СЕТЬ: СОЕДИНЕНО ●
        </div>
      </div>
    </div>
  );
};
