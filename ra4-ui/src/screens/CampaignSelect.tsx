import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';

interface FactionCardData {
  id: string;
  name: string;
  badge: string;
  tagline: string;
  description: string;
  missionsCompleted: string;
  extraObjectives: string;
  difficulty: string;
  color: string;
  bgScreenshot: string;
}

const FACTIONS: Record<string, FactionCardData> = {
  ussr: {
    id: 'ussr',
    name: 'СССР',
    badge: '★',
    tagline: 'СЛАВА РОДИНЕ. БУДУЩЕЕ ЗА НАМИ.',
    description: 'Возглавьте возрождённый Советский Союз в борьбе за мировое господство. Мощь, дисциплина и стальная воля помогут сокрушить врагов революции.',
    missionsCompleted: '06 / 18',
    extraObjectives: '09 / 36',
    difficulty: 'ВЕТЕРАН',
    color: '#ff2222',
    bgScreenshot: '/screenshots/4.png'
  },
  allies: {
    id: 'allies',
    name: 'АЛЬЯНС',
    badge: '🦅',
    tagline: 'СВОБОДА. ЕДИНСТВО. ТЕХНОЛОГИИ.',
    description: 'Альянс стоит на страже свободы и процветания. Перед лицом новой угрозы мы объединяем нации, передовые технологии и волю для обеспечения мира.',
    missionsCompleted: '04 / 16',
    extraObjectives: '08 / 32',
    difficulty: 'НОРМАЛЬНО',
    color: '#0088ff',
    bgScreenshot: '/screenshots/5.png'
  },
  ec: {
    id: 'ec',
    name: 'ВОСТОЧНАЯ КОАЛИЦИЯ',
    badge: '🐉',
    tagline: 'МУДРОСТЬ ДРАКОНА. СИЛА ПРОГРЕССА.',
    description: 'Восточная коалиция объединяет передовые нации Азии для защиты суверенитета и построения гармоничного многополярного мира.',
    missionsCompleted: '08 / 20',
    extraObjectives: '12 / 40',
    difficulty: 'ВЕТЕРАН',
    color: '#00ff66',
    bgScreenshot: '/screenshots/6.png'
  },
  chrono: {
    id: 'chrono',
    name: 'ХРОНОЛЕГИОН',
    badge: '⏳',
    tagline: 'ВЛАСТЬ НАД ВРЕМЕНЕМ. ГОСПОДСТВО НАД ВСЕЛЕННОЙ.',
    description: 'Они пришли не из этого времени. Хронолегион существует вне линейности, исправляя ошибки прошлого и переписывая будущее.',
    missionsCompleted: '02 / 14',
    extraObjectives: '05 / 28',
    difficulty: 'МАСТЕР',
    color: '#aa00ff',
    bgScreenshot: '/screenshots/7.png'
  }
};

export const CampaignSelect: React.FC = () => {
  const navigate = useNavigate();
  const [selectedFaction, setSelectedFaction] = useState('ussr');
  const [hoveredFaction, setHoveredFaction] = useState<string | null>(null);

  const activeId = hoveredFaction || selectedFaction;
  const activeData = FACTIONS[activeId];

  const handleLaunch = () => {
    navigate(`/campaign/${selectedFaction}`);
  };

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/screenshots/3.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '16px 36px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Oswald', sans-serif"
      }}
    >
      {/* Top Navigation Bar */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderBottom: '1px solid rgba(255,50,50,0.3)',
        paddingBottom: '10px',
        zIndex: 10
      }}>
        {/* Brand */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '15px' }}>
          <div>
            <div style={{ color: '#aaa', fontSize: '11px', letterSpacing: '3px' }}>COMMAND & CONQUER™</div>
            <div style={{ color: '#ff2222', fontSize: '20px', fontWeight: 800, letterSpacing: '2px', lineHeight: 1 }}>
              RED ALERT 4
            </div>
          </div>
        </div>

        {/* Breadcrumb Nav Tabs */}
        <div style={{ display: 'flex', gap: '8px' }}>
          {['ГЛАВНАЯ', 'КАМПАНИЯ', 'СЕТЕВАЯ ИГРА', 'ИСПЫТАНИЯ', 'КАЗАРМА', 'НАСТРОЙКИ'].map((tab, idx) => {
            const isActive = tab === 'КАМПАНИЯ';
            return (
              <button
                key={tab}
                onClick={() => {
                  if (tab === 'ГЛАВНАЯ') navigate('/menu');
                  if (tab === 'СЕТЕВАЯ ИГРА') navigate('/skirmish');
                  if (tab === 'НАСТРОЙКИ') navigate('/video-comms');
                }}
                style={{
                  background: isActive ? 'linear-gradient(180deg, #991616, #400606)' : 'rgba(20,15,15,0.7)',
                  border: `1px solid ${isActive ? '#ff3333' : 'rgba(255,255,255,0.1)'}`,
                  color: isActive ? '#ffffff' : '#aaa',
                  padding: '6px 16px',
                  borderRadius: '3px',
                  fontSize: '13px',
                  letterSpacing: '1.5px',
                  fontWeight: 600,
                  cursor: 'pointer'
                }}
              >
                {tab}
              </button>
            );
          })}
        </div>

        {/* User Profile */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
          <div style={{
            width: '32px',
            height: '32px',
            borderRadius: '4px',
            background: '#330a0a',
            border: '1px solid #ff3333',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            color: '#ff2222'
          }}>
            ★
          </div>
          <div>
            <div style={{ color: '#ff4444', fontSize: '13px', fontWeight: 700 }}>Товарищ Командир</div>
            <div style={{ color: '#888', fontSize: '11px' }}>УРОВЕНЬ 47</div>
          </div>
        </div>
      </div>

      {/* Screen Title */}
      <div style={{ textAlign: 'center', margin: '10px 0', zIndex: 5 }}>
        <h2 style={{
          color: '#ffffff',
          fontSize: '2.8rem',
          fontWeight: 800,
          letterSpacing: '6px',
          margin: 0,
          textShadow: '0 0 20px rgba(255,50,50,0.6)'
        }}>
          ВЫБОР КАМПАНИИ
        </h2>
        <div style={{ color: '#ff2222', fontSize: '16px', marginTop: '2px' }}>★</div>
      </div>

      {/* Main Grid: 4 Faction Cards + Right Detail Panel */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(4, 1fr) 360px',
        gap: '16px',
        flex: 1,
        alignItems: 'stretch',
        zIndex: 5,
        marginBottom: '10px'
      }}>
        {/* 4 Faction Cards */}
        {Object.values(FACTIONS).map(fac => {
          const isSelected = selectedFaction === fac.id;
          const isHovered = hoveredFaction === fac.id;
          return (
            <div
              key={fac.id}
              onClick={() => setSelectedFaction(fac.id)}
              onMouseEnter={() => setHoveredFaction(fac.id)}
              onMouseLeave={() => setHoveredFaction(null)}
              className="clip-bevel-md"
              style={{
                background: `linear-gradient(180deg, rgba(20,10,15,0.7) 0%, rgba(10,5,8,0.95) 100%)`,
                border: `2px solid ${isSelected ? fac.color : (isHovered ? 'rgba(255,255,255,0.4)' : 'rgba(255,255,255,0.12)')}`,
                boxShadow: isSelected ? `0 0 25px ${fac.color}, inset 0 0 15px rgba(0,0,0,0.8)` : 'none',
                borderRadius: '8px',
                padding: '16px',
                display: 'flex',
                flexDirection: 'column',
                justifyContent: 'space-between',
                cursor: 'pointer',
                transition: 'all 0.2s ease',
                transform: isSelected ? 'scale(1.02)' : 'none',
                position: 'relative',
                overflow: 'hidden'
              }}
            >
              {/* Card Top Crest */}
              <div style={{ textAlign: 'center', marginTop: '20px' }}>
                <div style={{
                  fontSize: '44px',
                  color: fac.color,
                  filter: `drop-shadow(0 0 15px ${fac.color})`,
                  marginBottom: '10px'
                }}>
                  {fac.badge}
                </div>
              </div>

              {/* Card Bottom Title */}
              <div style={{
                textAlign: 'center',
                padding: '12px 0',
                borderTop: `1px solid ${isSelected ? fac.color : 'rgba(255,255,255,0.1)'}`,
                background: 'linear-gradient(180deg, transparent, rgba(0,0,0,0.6))'
              }}>
                <h3 style={{
                  color: isSelected ? '#ffffff' : '#ccc',
                  fontSize: '22px',
                  fontWeight: 700,
                  letterSpacing: '3px',
                  margin: 0
                }}>
                  {fac.name}
                </h3>
              </div>
            </div>
          );
        })}

        {/* Right Info & Launch Panel */}
        <div className="ra4-panel clip-bevel-md" style={{
          padding: '24px 20px',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          border: `1px solid ${activeData.color}`
        }}>
          <div>
            <div style={{ color: activeData.color, fontSize: '13px', fontWeight: 700, letterSpacing: '2px' }}>
              О ВЫБРАННОЙ КАМПАНИИ
            </div>
            <h2 style={{ color: '#ffffff', fontSize: '28px', margin: '8px 0 4px 0', fontWeight: 800 }}>
              {activeData.name}
            </h2>
            <div style={{ color: activeData.color, fontSize: '12px', letterSpacing: '1px', fontWeight: 600, marginBottom: '14px' }}>
              {activeData.tagline}
            </div>
            <p style={{ color: '#bbb', fontSize: '13px', lineHeight: 1.6, fontFamily: "'Inter', sans-serif" }}>
              {activeData.description}
            </p>

            <div style={{ marginTop: '24px', borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '16px' }}>
              <div style={{ color: activeData.color, fontSize: '13px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '12px' }}>
                ПРОГРЕСС КАМПАНИИ
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#ccc', marginBottom: '6px' }}>
                <span>МИССИИ ПРОЙДЕНО:</span>
                <strong style={{ color: '#fff' }}>{activeData.missionsCompleted}</strong>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#ccc', marginBottom: '6px' }}>
                <span>ДОП. ЗАДАНИЯ:</span>
                <strong style={{ color: '#fff' }}>{activeData.extraObjectives}</strong>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#ccc', marginBottom: '6px' }}>
                <span>СЛОЖНОСТЬ:</span>
                <strong style={{ color: activeData.color }}>{activeData.difficulty}</strong>
              </div>
            </div>
          </div>

          <button
            onClick={handleLaunch}
            className="clip-bevel-sm"
            style={{
              background: `linear-gradient(180deg, ${activeData.color}, #110505)`,
              border: `1px solid ${activeData.color}`,
              color: '#ffffff',
              padding: '14px',
              fontSize: '17px',
              fontWeight: 800,
              letterSpacing: '2px',
              cursor: 'pointer',
              boxShadow: `0 0 20px ${activeData.color}`,
              marginTop: '16px'
            }}
          >
            ПРОДОЛЖИТЬ КАМПАНИЮ
          </button>
        </div>
      </div>

      {/* Bottom Bar */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderTop: '1px solid rgba(255,255,255,0.1)',
        paddingTop: '8px',
        zIndex: 10
      }}>
        <div style={{ display: 'flex', gap: '10px' }}>
          <button
            onClick={() => navigate('/menu')}
            className="ra4-btn-ussr clip-bevel-sm"
            style={{ padding: '8px 20px', fontSize: '14px' }}
          >
            ‹ НАЗАД
          </button>
          <button
            onClick={() => navigate('/strategic-map')}
            className="ra4-btn-ussr clip-bevel-sm"
            style={{ padding: '8px 20px', fontSize: '14px' }}
          >
            ОБУЧЕНИЕ
          </button>
        </div>

        <div style={{ color: '#666', fontSize: '13px', letterSpacing: '4px' }}>
          1927 — ★ — 2047
        </div>

        <div style={{ display: 'flex', gap: '8px', color: '#888', fontSize: '14px' }}>
          <span>📊</span>
          <span>🎖</span>
          <span>👥</span>
          <span style={{ color: '#fff', fontWeight: 700 }}>12</span>
        </div>
      </div>
    </div>
  );
};
