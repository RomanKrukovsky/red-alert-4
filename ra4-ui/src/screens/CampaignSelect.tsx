import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';
import { FACTIONS, FACTION_ORDER } from '../data/factions';

export const CampaignSelect: React.FC = () => {
  const navigate = useNavigate();
  const [selectedFaction, setSelectedFaction] = useState('eurasian');
  const [hoveredFaction, setHoveredFaction] = useState<string | null>(null);

  const activeId = hoveredFaction || selectedFaction;
  const activeData = FACTIONS[activeId];

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/remaster/02_main_menu.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        padding: '20px 36px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* Dark scrim */}
      <div style={{ position: 'absolute', inset: 0, background: 'linear-gradient(180deg, rgba(4,4,8,0.72), rgba(4,4,8,0.88))', zIndex: 1 }} />

      {/* Top Header */}
      <div style={{ position: 'relative', zIndex: 5, textAlign: 'center', marginBottom: '18px' }}>
        <BrandLogo scale={0.7} subtitle="ВЫБОР КАМПАНИИ" />
      </div>

      {/* Main Grid: 5 Faction Cards + Right Detail Panel */}
      <div style={{
        position: 'relative',
        zIndex: 5,
        display: 'grid',
        gridTemplateColumns: 'repeat(5, 1fr) 340px',
        gap: '14px',
        flex: 1,
        alignItems: 'stretch'
      }}>
        {FACTION_ORDER.map(key => {
          const fac = FACTIONS[key];
          const isSelected = selectedFaction === key;
          const isHovered = hoveredFaction === key;
          return (
            <div
              key={key}
              onClick={() => {
                setSelectedFaction(key);
                navigate(`/campaign/${key}`);
              }}
              onMouseEnter={() => setHoveredFaction(key)}
              onMouseLeave={() => setHoveredFaction(null)}
              style={{
                background: `linear-gradient(180deg, ${fac.dimColor}30 0%, rgba(6,5,10,0.96) 70%)`,
                border: `2px solid ${isSelected ? fac.color : (isHovered ? `${fac.color}aa` : 'rgba(255,255,255,0.12)')}`,
                boxShadow: isSelected ? `0 0 26px ${fac.color}66, inset 0 0 24px ${fac.color}18` : 'none',
                borderRadius: '6px',
                padding: '18px 16px',
                display: 'flex',
                flexDirection: 'column',
                justifyContent: 'space-between',
                cursor: 'pointer',
                transition: 'all 0.2s ease',
                transform: isSelected ? 'scale(1.02)' : 'none',
                overflow: 'hidden'
              }}
            >
              <div style={{ textAlign: 'center' }}>
                <div
                  style={{
                    margin: '26px auto 14px',
                    width: '92px',
                    height: '104px',
                    clipPath: 'polygon(50% 0, 100% 25%, 100% 75%, 50% 100%, 0 75%, 0 25%)',
                    background: `linear-gradient(180deg, ${fac.color}33, rgba(6,5,10,0.9))`,
                    border: `1px solid ${fac.color}`,
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    fontSize: '44px',
                    color: fac.color,
                    filter: `drop-shadow(0 0 14px ${fac.color})`
                  }}
                >
                  {fac.crest}
                </div>
                {fac.nameLines.map((line, i) => (
                  <div key={i} style={{
                    fontFamily: "'Oswald', sans-serif",
                    color: isSelected ? '#ffffff' : '#c3c9d3',
                    fontSize: '19px',
                    fontWeight: 700,
                    letterSpacing: '3px',
                    lineHeight: 1.35
                  }}>
                    {line}
                  </div>
                ))}
                <div style={{ color: fac.color, fontSize: '11px', letterSpacing: '2px', marginTop: '8px', fontWeight: 700 }}>
                  {fac.country}
                </div>
              </div>

              {/* Progress footer */}
              <div style={{ marginTop: '18px' }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '11px', color: '#98a0ac', marginBottom: '4px' }}>
                  <span>ПРОГРЕСС</span>
                  <strong style={{ color: '#fff' }}>{fac.progressPercent}%</strong>
                </div>
                <div style={{
                  height: '6px',
                  background: 'rgba(0,0,0,0.75)',
                  border: '1px solid rgba(255,255,255,0.12)',
                  borderRadius: '2px',
                  overflow: 'hidden'
                }}>
                  <div style={{ width: `${fac.progressPercent}%`, height: '100%', background: fac.color, boxShadow: `0 0 8px ${fac.color}` }} />
                </div>
              </div>
            </div>
          );
        })}

        {/* Right Detail Panel */}
        <div className="ra4-panel" style={{
          padding: '22px 20px',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          border: `1px solid ${activeData.color}`
        }}>
          <div>
            <div style={{ color: activeData.color, fontSize: '12px', fontWeight: 700, letterSpacing: '2px' }}>
              О ВЫБРАННОЙ КАМПАНИИ
            </div>
            <h2 style={{ color: '#ffffff', fontFamily: "'Oswald', sans-serif", fontSize: '24px', margin: '8px 0 4px 0', fontWeight: 800, lineHeight: 1.15 }}>
              {activeData.campaignTitle}
            </h2>
            <div style={{ color: activeData.color, fontSize: '11px', letterSpacing: '1.5px', fontWeight: 700, marginBottom: '12px' }}>
              {activeData.doctrine}
            </div>
            <p style={{ color: '#b9bec8', fontSize: '12.5px', lineHeight: 1.6 }}>
              {activeData.description}
            </p>

            <div style={{ marginTop: '18px', borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '14px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#b9bec8', marginBottom: '7px' }}>
                <span>МИССИЙ ЗАВЕРШЕНО:</span>
                <strong style={{ color: '#fff' }}>{activeData.missionsCompleted}</strong>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#b9bec8', marginBottom: '7px' }}>
                <span>СЛОЖНОСТЬ:</span>
                <strong style={{ color: activeData.color }}>{activeData.difficulty}</strong>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#b9bec8' }}>
                <span>ТЕКУЩАЯ ГЛАВА:</span>
                <strong style={{ color: '#fff' }}>{activeData.currentChapter}</strong>
              </div>
            </div>
          </div>

          <button
            onClick={() => navigate(`/campaign/${selectedFaction}`)}
            className="clip-bevel-sm"
            style={{
              background: `linear-gradient(180deg, ${activeData.color}, ${activeData.dimColor})`,
              border: `1px solid ${activeData.color}`,
              color: '#0b0712',
              padding: '14px',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '17px',
              fontWeight: 800,
              letterSpacing: '2px',
              cursor: 'pointer',
              boxShadow: `0 0 20px ${activeData.color}88`,
              marginTop: '16px'
            }}
          >
            ВОЙТИ В КАМПАНИЮ ≫
          </button>
        </div>
      </div>

      {/* Bottom Bar */}
      <div style={{
        position: 'relative',
        zIndex: 5,
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        paddingTop: '12px'
      }}>
        <button
          onClick={() => navigate('/menu')}
          className="clip-bevel-sm"
          style={{
            padding: '9px 26px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '14px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer'
          }}
        >
          ‹&nbsp;&nbsp;НАЗАД
        </button>

        <button
          onClick={() => navigate('/skirmish')}
          className="clip-bevel-sm"
          style={{
            padding: '9px 26px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '14px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer'
          }}
        >
          СЕТЕВОЙ БОЙ
        </button>
      </div>
    </div>
  );
};
