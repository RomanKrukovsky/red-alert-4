import React from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';
import { FACTIONS, FACTION_ORDER } from '../data/factions';

export const FactionCampaignScreen: React.FC = () => {
  const { faction } = useParams<{ faction: string }>();
  const navigate = useNavigate();

  const currentFactionKey = faction && FACTIONS[faction] ? faction : 'eurasian';
  const config = FACTIONS[currentFactionKey];

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
        background: `url('${config.bgScreenshot}') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '18px 30px 20px 30px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* Top Header Strip */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '300px 1fr 220px',
        alignItems: 'start',
        zIndex: 10
      }}>
        {/* Left Hex Emblem */}
        <div
          onClick={() => navigate('/menu')}
          style={{
            width: '86px',
            height: '96px',
            clipPath: 'polygon(50% 0, 100% 25%, 100% 75%, 50% 100%, 0 75%, 0 25%)',
            background: `linear-gradient(180deg, ${config.color}44, rgba(8,6,14,0.95))`,
            border: `1px solid ${config.color}`,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            fontSize: '40px',
            color: config.color,
            textShadow: `0 0 16px ${config.color}`,
            cursor: 'pointer'
          }}
        >
          {config.crest}
        </div>

        {/* Center Brand */}
        <div>
          <BrandLogo scale={0.62} subtitle={`КАМПАНИЯ ${config.name}`} />
        </div>

        {/* Right Tool Buttons */}
        <div style={{ display: 'flex', gap: '10px', justifyContent: 'flex-end' }}>
          {[
            { icon: '⚙', to: '/video-comms' },
            { icon: '📊', to: '/strategic-map' },
            { icon: '🛡', to: '/briefing' },
            { icon: '⏻', to: '/menu' }
          ].map((btn, i) => (
            <button
              key={i}
              onClick={() => navigate(btn.to)}
              style={{
                width: '44px',
                height: '44px',
                background: 'rgba(8,7,14,0.82)',
                border: '1px solid rgba(255,255,255,0.22)',
                borderRadius: '4px',
                color: '#dfe3ea',
                fontSize: '17px',
                cursor: 'pointer',
                clipPath: 'polygon(0 6px, 6px 0, 100% 0, 100% calc(100% - 6px), calc(100% - 6px) 100%, 0 100%)'
              }}
            >
              {btn.icon}
            </button>
          ))}
        </div>
      </div>

      {/* Middle Band: Sidebar + Content */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '280px minmax(430px, 560px) 1fr',
        flex: 1,
        alignItems: 'center',
        zIndex: 5,
        marginTop: '-14px'
      }}>
        {/* Left Campaigns Sidebar */}
        <div style={{ alignSelf: 'start', marginTop: '26px' }}>
          <div style={{
            fontFamily: "'Oswald', sans-serif",
            color: '#c9cfda',
            fontSize: '13px',
            letterSpacing: '4px',
            padding: '8px 14px',
            marginBottom: '12px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.16)',
            borderRadius: '4px'
          }}>
            КАМПАНИИ
          </div>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
            {FACTION_ORDER.map(key => {
              const fac = FACTIONS[key];
              const isSelected = key === currentFactionKey;
              return (
                <button
                  key={key}
                  onClick={() => navigate(`/campaign/${key}`)}
                  style={{
                    minHeight: '64px',
                    display: 'flex',
                    alignItems: 'center',
                    gap: '14px',
                    padding: '8px 16px',
                    cursor: 'pointer',
                    textAlign: 'left',
                    fontFamily: "'Oswald', sans-serif",
                    fontSize: '15px',
                    letterSpacing: '2px',
                    fontWeight: 600,
                    lineHeight: 1.25,
                    color: isSelected ? '#ffffff' : '#b9bfca',
                    background: isSelected
                      ? `linear-gradient(90deg, ${fac.dimColor}cc 0%, rgba(10,8,16,0.92) 70%)`
                      : 'rgba(8,7,14,0.85)',
                    border: `1px solid ${isSelected ? fac.color : 'rgba(255,255,255,0.18)'}`,
                    boxShadow: isSelected ? `0 0 18px ${fac.color}88` : 'none',
                    borderRadius: '4px',
                    transition: 'all 0.15s ease'
                  }}
                >
                  <span style={{
                    fontSize: '24px',
                    color: fac.color,
                    filter: `drop-shadow(0 0 8px ${fac.color})`
                  }}>
                    {fac.crest}
                  </span>
                  <span>{fac.nameLines[0]}<br />{fac.nameLines[1]}</span>
                </button>
              );
            })}
          </div>
        </div>

        {/* Center Content Column */}
        <div style={{ alignSelf: 'start', marginTop: '26px', paddingRight: '20px' }}>
          <h1 style={{
            margin: 0,
            fontFamily: "'Oswald', sans-serif",
            fontWeight: 800,
            fontSize: '3.4rem',
            letterSpacing: '2px',
            lineHeight: 1.02,
            color: '#f2f4f8',
            textShadow: '0 4px 22px rgba(0,0,0,0.95)'
          }}>
            {config.country}: {config.campaignTitle.split(': ')[1]}
          </h1>
          <div style={{
            fontFamily: "'Oswald', sans-serif",
            color: config.color,
            fontSize: '15px',
            fontWeight: 700,
            letterSpacing: '2.5px',
            margin: '8px 0 12px 0',
            textShadow: `0 0 12px ${config.color}66`
          }}>
            {config.doctrine}
          </div>
          <p style={{
            color: '#c9cdd6',
            fontSize: '13.5px',
            lineHeight: 1.65,
            maxWidth: '520px',
            textShadow: '0 2px 6px rgba(0,0,0,0.9)'
          }}>
            {config.description}
          </p>

          {/* Progress Panel */}
          <div style={{
            marginTop: '26px',
            minWidth: '380px',
            maxWidth: '480px',
            padding: '16px 20px',
            background: 'linear-gradient(180deg, rgba(10,8,16,0.92), rgba(6,5,10,0.96))',
            border: `1px solid ${config.color}88`,
            borderRadius: '6px',
            boxShadow: `inset 0 0 24px ${config.color}14, 0 8px 24px rgba(0,0,0,0.7)`
          }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline', marginBottom: '8px' }}>
              <span style={{
                fontFamily: "'Oswald', sans-serif",
                color: config.color,
                fontSize: '14px',
                fontWeight: 700,
                letterSpacing: '2px'
              }}>
                ПРОГРЕСС КАМПАНИИ
              </span>
              <strong style={{ color: '#ffffff', fontSize: '15px' }}>{config.progressPercent}%</strong>
            </div>
            <div style={{
              width: '100%',
              height: '9px',
              background: 'rgba(0,0,0,0.8)',
              border: '1px solid rgba(255,255,255,0.14)',
              borderRadius: '3px',
              overflow: 'hidden',
              marginBottom: '14px'
            }}>
              <div style={{
                width: `${config.progressPercent}%`,
                height: '100%',
                background: `linear-gradient(90deg, ${config.dimColor}, ${config.color})`,
                boxShadow: `0 0 10px ${config.color}`
              }} />
            </div>

            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#aeb4c0', padding: '5px 0', borderTop: '1px solid rgba(255,255,255,0.08)' }}>
              <span>МИССИЙ ЗАВЕРШЕНО</span>
              <strong style={{ color: '#fff' }}>{config.missionsCompleted}</strong>
            </div>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '13px', color: '#aeb4c0', padding: '5px 0', borderTop: '1px solid rgba(255,255,255,0.08)' }}>
              <span>СЛОЖНОСТЬ</span>
              <strong style={{ color: '#fff', letterSpacing: '1px' }}>{config.difficulty}</strong>
            </div>
            <div
              onClick={() => navigate('/strategic-map')}
              style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                fontSize: '13px',
                color: config.color,
                fontWeight: 700,
                letterSpacing: '1.5px',
                padding: '9px 0 2px 0',
                borderTop: '1px solid rgba(255,255,255,0.08)',
                cursor: 'pointer'
              }}
            >
              <span>{config.currentChapter}</span>
              <span>›</span>
            </div>
          </div>
        </div>
      </div>

      {/* Bottom Action Row */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '150px 1fr 1.3fr 1fr',
        gap: '18px',
        alignItems: 'center',
        zIndex: 10
      }}>
        <button
          onClick={() => navigate('/campaign-select')}
          style={{
            height: '52px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '15px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer',
            clipPath: 'polygon(0 8px, 10px 0, 100% 0, 100% calc(100% - 8px), calc(100% - 10px) 100%, 0 100%)'
          }}
        >
          ‹&nbsp;&nbsp;НАЗАД
        </button>

        <button
          onClick={() => navigate(`/campaign/${currentFactionKey}`)}
          style={{
            height: '52px',
            background: 'rgba(8,7,14,0.85)',
            border: `1px solid ${config.color}77`,
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '15px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer',
            clipPath: 'polygon(0 8px, 10px 0, 100% 0, 100% calc(100% - 8px), calc(100% - 10px) 100%, 0 100%)'
          }}
        >
          НОВАЯ КАМПАНИЯ
        </button>

        <button
          onClick={handleLaunchMission}
          style={{
            height: '58px',
            background: `linear-gradient(180deg, ${config.color} 0%, ${config.dimColor} 100%)`,
            border: `1px solid ${config.color}`,
            borderRadius: '4px',
            color: '#0b0712',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '19px',
            fontWeight: 800,
            letterSpacing: '3px',
            cursor: 'pointer',
            boxShadow: `0 0 28px ${config.color}aa, inset 0 1px 0 rgba(255,255,255,0.35)`,
            clipPath: 'polygon(0 10px, 12px 0, 100% 0, 100% calc(100% - 10px), calc(100% - 12px) 100%, 0 100%)'
          }}
        >
          ПРОДОЛЖИТЬ&nbsp;&nbsp;≫
        </button>

        <button
          onClick={() => navigate('/strategic-map')}
          style={{
            height: '52px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '15px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer',
            clipPath: 'polygon(0 8px, 10px 0, 100% 0, 100% calc(100% - 8px), calc(100% - 10px) 100%, 0 100%)'
          }}
        >
          ВЫБОР ГЛАВЫ
        </button>
      </div>
    </div>
  );
};
