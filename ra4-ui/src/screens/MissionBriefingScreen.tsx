import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';
import { FACTIONS } from '../data/factions';

export const MissionBriefingScreen: React.FC = () => {
  const navigate = useNavigate();
  const fac = FACTIONS.eurasian;
  const [checkedObjectives, setCheckedObjectives] = useState([true, false, false]);

  const toggleObjective = (index: number) => {
    const next = [...checkedObjectives];
    next[index] = !next[index];
    setCheckedObjectives(next);
  };

  return (
    <div
      className={fac.themeClass}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/remaster/08_operation_briefing_eurasian.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '16px 30px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* ===== Top Header ===== */}
      <div style={{ display: 'grid', gridTemplateColumns: '1fr auto 1fr', alignItems: 'start', zIndex: 10 }}>
        <div>
          <BrandLogo scale={0.44} />
        </div>

        <div style={{
          fontFamily: "'Oswald', sans-serif",
          color: '#ffffff',
          fontSize: '19px',
          fontWeight: 800,
          letterSpacing: '6px',
          textShadow: `0 0 18px ${fac.color}88`
        }}>
          БРИФИНГ ОПЕРАЦИИ
        </div>

        {/* Commander Card */}
        <div style={{ display: 'flex', justifyContent: 'flex-end' }}>
          <div style={{
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
            padding: '7px 14px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.18)',
            borderRadius: '4px'
          }}>
            <div style={{
              width: '38px', height: '44px', borderRadius: '3px',
              border: `1px solid ${fac.color}66`,
              background: 'linear-gradient(180deg,#2a2038,#0d0a16)',
              display: 'flex', alignItems: 'center', justifyContent: 'center',
              fontSize: '18px', color: fac.color
            }}>⚔</div>
            <div>
              <div style={{ color: '#fff', fontSize: '12px', fontWeight: 700 }}>КОМАНДИР ИРИНА ВОЛКОВА</div>
              <div style={{ color: '#9aa2b0', fontSize: '10px' }}>ОПЕРАТИВНАЯ ГРУППА «СЕВЕР»</div>
            </div>
          </div>
        </div>
      </div>

      {/* ===== Main 3-Column Grid ===== */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '330px 1fr 330px',
        gap: '20px',
        flex: 1,
        margin: '14px 0',
        zIndex: 5,
        alignItems: 'stretch'
      }}>
        {/* Left Operation Panel */}
        <div style={{
          alignSelf: 'stretch',
          maxHeight: '100%',
          overflow: 'hidden auto',
          padding: '18px',
          background: 'linear-gradient(180deg, rgba(10,8,16,0.93), rgba(6,5,10,0.97))',
          border: `1px solid ${fac.color}66`,
          borderRadius: '6px',
          boxShadow: 'inset 0 0 24px rgba(176,108,255,0.08)'
        }}>
          <div style={{ display: 'flex', gap: '10px', alignItems: 'baseline' }}>
            <span style={{ color: fac.color, fontSize: '11px', letterSpacing: '3px', fontWeight: 700 }}>ОПЕРАЦИЯ</span>
          </div>
          <h2 style={{ fontFamily: "'Oswald', sans-serif", color: '#ffffff', fontSize: '27px', fontWeight: 800, margin: '3px 0 5px 0', letterSpacing: '1px' }}>
            ТИХИЙ РЕЛЕЙ
          </h2>
          <div style={{ color: '#aab0bc', fontSize: '12px', marginBottom: '12px', letterSpacing: '1px' }}>
            СЕКТОР: ГОРНЫЙ КОРИДОР
          </div>

          <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px', marginBottom: '10px' }}>
            <div style={{ color: fac.color, fontSize: '12px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '8px' }}>
              ЦЕЛИ ОПЕРАЦИИ
            </div>
            {['Пробиться к узлу связи', 'Подавить противника', 'Обеспечить мобильный комплекс РЭБ'].map((obj, i) => (
              <div key={i} onClick={() => toggleObjective(i)} style={{
                display: 'flex',
                alignItems: 'center',
                gap: '9px',
                fontSize: '12px',
                color: checkedObjectives[i] ? '#ffffff' : '#a8aeb8',
                cursor: 'pointer',
                marginBottom: '7px'
              }}>
                <span style={{
                  width: '13px', height: '13px',
                  clipPath: 'polygon(50% 0, 93% 25%, 93% 75%, 50% 100%, 7% 75%, 7% 25%)',
                  background: checkedObjectives[i] ? `${fac.color}55` : 'rgba(255,255,255,0.07)',
                  border: `1px solid ${fac.color}`,
                  display: 'inline-flex', alignItems: 'center', justifyContent: 'center',
                  fontSize: '8px', color: '#fff', flexShrink: 0
                }}>{checkedObjectives[i] ? '✓' : ''}</span>
                {obj}
              </div>
            ))}
          </div>

          <div style={{
            marginTop: 'auto',
            borderTop: '1px solid rgba(255,255,255,0.1)',
            paddingTop: '12px'
          }}>
            <div style={{ color: fac.color, fontSize: '12px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '8px' }}>
              ПОДКРЕПЛЕНИЯ
            </div>
            {[
              { icon: '🛰', label: 'СПУТНИКОВЫЙ ПРОЛЁТ', count: '3' },
              { icon: '🌫', label: 'ДЫМОВАЯ ЗАВЕСА', count: '2' },
              { icon: '🎖', label: 'РЕЗЕРВНАЯ БРИГАДА', count: '2' }
            ].map(r => (
              <div key={r.label} style={{
                height: '46px',
                display: 'flex',
                alignItems: 'center',
                gap: '10px',
                padding: '0 10px',
                background: 'rgba(255,255,255,0.05)',
                border: '1px solid rgba(255,255,255,0.12)',
                borderRadius: '3px',
                marginBottom: '6px',
                fontSize: '11px',
                color: '#b0b6c0'
              }}>
                <span style={{ fontSize: '17px' }}>{r.icon}</span>
                <span style={{ flex: 1 }}>{r.label}</span>
                <strong style={{ color: '#fff' }}>✕{r.count}</strong>
              </div>
            ))}
          </div>
        </div>

        {/* Center Video Feed */}
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'flex-end',
          minHeight: 0,
          position: 'relative'
        }}>
          <div style={{
            position: 'absolute',
            top: 0,
            left: '50%',
            transform: 'translateX(-50%)',
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
            padding: '4px 14px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.2)',
            borderRadius: '3px',
            fontSize: '10px',
            color: '#aab0ba',
            letterSpacing: '2px'
          }}>
            <span style={{ color: '#57e89a' }}>◉</span> ПРЯМОЙ ЭФИР / ЗАПИСЬ
            <span style={{ color: fac.color, fontWeight: 700 }}>КАНАЛ 7/12</span>
          </div>

          {/* Commander Nameplate */}
          <div style={{
            alignSelf: 'center',
            display: 'flex',
            alignItems: 'center',
            gap: '12px',
            padding: '8px 26px',
            background: 'linear-gradient(180deg, rgba(12,9,22,0.95), rgba(7,5,12,0.98))',
            border: `1px solid ${fac.color}88`,
            borderRadius: '4px',
            boxShadow: `0 0 22px ${fac.color}66`,
            marginBottom: '14px'
          }}>
            <span style={{ fontSize: '20px', color: fac.color }}>⚔</span>
            <div>
              <div style={{ color: '#9aa2b0', fontSize: '10px', letterSpacing: '2px' }}>КОМАНДИР ГРУППЫ «СЕВЕР»</div>
              <div style={{ color: '#ffffff', fontFamily: "'Oswald', sans-serif", fontSize: '16px', fontWeight: 800, letterSpacing: '1px' }}>ИРИНА ВОЛКОВА</div>
            </div>
          </div>
        </div>

        {/* Right Intel Panel */}
        <div style={{
          alignSelf: 'stretch',
          padding: '18px',
          background: 'linear-gradient(180deg, rgba(10,8,16,0.93), rgba(6,5,10,0.97))',
          border: `1px solid ${fac.color}66`,
          borderRadius: '6px',
          display: 'flex',
          flexDirection: 'column',
          gap: '12px'
        }}>
          <div>
            <div style={{ color: fac.color, fontSize: '11px', letterSpacing: '3px', fontWeight: 700, marginBottom: '8px' }}>РАЗВЕДДАННЫЕ</div>
            <div style={{
              height: '110px',
              borderRadius: '3px',
              border: '1px solid rgba(255,255,255,0.14)',
              background:
                'radial-gradient(circle at 25% 35%, rgba(176,108,255,0.25), transparent 45%), radial-gradient(circle at 70% 65%, rgba(63,141,255,0.18), transparent 40%), rgba(8,7,14,0.9)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              fontSize: '10px',
              color: '#8b93a2'
            }}>
              [ТАКТКАРТА СЕКТОРА]
            </div>
          </div>

          {/* Enemy Assessment */}
          <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '12px' }}>
            <div style={{ color: fac.color, fontSize: '11px', letterSpacing: '3px', fontWeight: 700, marginBottom: '4px' }}>ОЦЕНКА ПРОТИВНИКА</div>
            <div style={{
              fontFamily: "'Oswald', sans-serif",
              color: '#ff5c47',
              fontSize: '34px',
              fontWeight: 900,
              letterSpacing: '4px',
              textShadow: '0 0 18px rgba(255,60,40,0.75)'
            }}>ВЫСОКАЯ</div>
            <div style={{ marginTop: '8px', display: 'flex', flexDirection: 'column', gap: '5px' }}>
              {[
                { label: 'РЭБ', value: 72 },
                { label: 'БРОНЯ', value: 58 },
                { label: 'ПЕХОТА', value: 64 },
                { label: 'АВИАЦИЯ', value: 42 }
              ].map(s => (
                <div key={s.label} style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
                  <span style={{ width: '52px', color: '#98a0ac', fontSize: '9px', letterSpacing: '1px' }}>{s.label}</span>
                  <div style={{ flex: 1, height: '5px', background: 'rgba(255,255,255,0.08)', borderRadius: '2px', overflow: 'hidden' }}>
                    <div style={{ width: `${s.value}%`, height: '100%', background: `linear-gradient(90deg, ${fac.dimColor}, #ff5c47)` }} />
                  </div>
                </div>
              ))}
            </div>
          </div>

          <button
            onClick={() => navigate('/loading')}
            className="clip-bevel-sm"
            style={{
              marginTop: 'auto',
              width: '100%',
              height: '54px',
              background: `linear-gradient(180deg, ${fac.color}, ${fac.dimColor})`,
              border: `1px solid ${fac.color}`,
              borderRadius: '4px',
              color: '#0b0712',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '19px',
              fontWeight: 800,
              letterSpacing: '3px',
              cursor: 'pointer',
              boxShadow: `0 0 26px ${fac.color}99`,
              clipPath: 'polygon(0 9px, 11px 0, 100% 0, 100% calc(100% - 9px), calc(100% - 11px) 100%, 0 100%)'
            }}
          >
            ПРОДОЛЖИТЬ&nbsp;&nbsp;≫
          </button>
        </div>
      </div>

      {/* ===== Bottom Bar ===== */}
      <div style={{ display: 'grid', gridTemplateColumns: '150px 1fr auto auto', gap: '14px', alignItems: 'center', zIndex: 10 }}>
        <button
          onClick={() => navigate('/strategic-map')}
          style={{
            height: '42px',
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
        <div />
        <button
          onClick={() => navigate('/video-comms')}
          style={{
            height: '42px',
            padding: '0 22px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '13px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer'
          }}
        >
          ⚙&nbsp;&nbsp;СНАРЯЖЕНИЕ
        </button>
        <button
          onClick={() => navigate('/menu')}
          style={{
            height: '42px',
            padding: '0 22px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '13px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer'
          }}
        >
          ⚙&nbsp;&nbsp;НАСТРОЙКИ
        </button>
      </div>
    </div>
  );
};
