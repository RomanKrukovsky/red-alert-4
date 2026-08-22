import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';

export const LoadingScreen: React.FC = () => {
  const navigate = useNavigate();
  const [progress, setProgress] = useState(72);

  useEffect(() => {
    const timer = setInterval(() => {
      setProgress(p => {
        if (p >= 100) {
          clearInterval(timer);
          setTimeout(() => {
            navigate('/hud?mode=eurasian-ground');
          }, 400);
          return 100;
        }
        return p + 2;
      });
    }, 150);
    return () => clearInterval(timer);
  }, [navigate]);

  return (
    <div
      onClick={() => navigate('/hud?mode=eurasian-ground')}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/remaster/10_mission_loading_eurasian.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '26px 40px 20px 40px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif",
        cursor: 'pointer'
      }}
    >
      {/* Top Centered Title */}
      <div style={{ textAlign: 'center', zIndex: 10 }}>
        <div style={{ transform: 'scale(0.55)', transformOrigin: 'top center' }}>
          <BrandLogo />
        </div>
        <div style={{
          fontFamily: "'Oswald', sans-serif",
          color: '#ffffff',
          fontSize: '2.9rem',
          fontWeight: 900,
          letterSpacing: '10px',
          marginTop: '-8px',
          textShadow: '0 4px 24px rgba(0,0,0,0.95), 0 0 40px rgba(176,108,255,0.35)'
        }}>
          ЗАГРУЗКА МИССИИ
        </div>
        <div style={{ color: '#cdd3dd', fontSize: '15px', letterSpacing: '6px', marginTop: '6px' }}>
          ОПЕРАЦИЯ «ТИХИЙ РЕЛЕЙ»
        </div>
      </div>

      {/* Middle: left summary panel */}
      <div style={{ flex: 1, display: 'flex', alignItems: 'center' }}>
        <div style={{
          width: '360px',
          padding: '18px 20px',
          background: 'linear-gradient(180deg, rgba(10,8,16,0.93), rgba(6,5,10,0.97))',
          border: '1px solid rgba(176,108,255,0.66)',
          borderRadius: '6px'
        }}>
          <div style={{ color: '#b06cff', fontSize: '12px', fontWeight: 700, letterSpacing: '2px', marginBottom: '6px' }}>
            СВОДКА
          </div>
          <p style={{ color: '#c3c8d2', fontSize: '12px', lineHeight: 1.6, margin: '0 0 14px 0' }}>
            Передовой отряд «Комета» выдвигается к горному коридору. Противник удерживает узлы связи и контролирует перевал. Проведите броне группу и подавите ретрансляцию.
          </p>

          <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '12px' }}>
            <div style={{ color: '#b06cff', fontSize: '12px', fontWeight: 700, letterSpacing: '2px', marginBottom: '8px' }}>
              ЦЕЛИ
            </div>
            {[
              { icon: '🛡', label: 'Удержать 3 узла связи' },
              { icon: '📡', label: 'Сохранить мобильный комплекс РЭБ' },
              { icon: '🚚', label: 'Обеспечить проход колонны' }
            ].map(o => (
              <div key={o.label} style={{ display: 'flex', alignItems: 'center', gap: '10px', marginBottom: '7px', fontSize: '12px', color: '#c3c8d2' }}>
                <span style={{
                  width: '22px', height: '22px', flexShrink: 0,
                  clipPath: 'polygon(50% 0, 93% 25%, 93% 75%, 50% 100%, 7% 75%, 7% 25%)',
                  background: 'rgba(176,108,255,0.18)',
                  border: '1px solid rgba(176,108,255,0.6)',
                  display: 'inline-flex', alignItems: 'center', justifyContent: 'center',
                  fontSize: '11px'
                }}>{o.icon}</span>
                {o.label}
              </div>
            ))}
          </div>
        </div>
      </div>

      {/* Bottom right checklist */}
      <div style={{
        position: 'absolute',
        right: '40px',
        bottom: '110px',
        width: '250px',
        display: 'flex',
        flexDirection: 'column',
        gap: '7px',
        zIndex: 10
      }}>
        {[
          { label: 'КАРТА ЗАГРУЖЕНА', done: true },
          { label: 'БОЕВЫЕ ГРУППЫ: ГОТОВЫ', done: true },
          { label: 'СВЯЗЬ: ШИФРУЕТСЯ', done: false }
        ].map(item => (
          <div key={item.label} style={{
            display: 'flex',
            alignItems: 'center',
            gap: '9px',
            padding: '7px 12px',
            background: 'rgba(6,5,12,0.88)',
            border: '1px solid rgba(255,255,255,0.14)',
            borderRadius: '3px',
            fontSize: '11px',
            letterSpacing: '1px',
            color: item.done ? '#57e89a' : '#e0c25c'
          }}>
            <span>{item.done ? '✓' : '◌'}</span>
            {item.label}
          </div>
        ))}
      </div>

      {/* Progress Bar */}
      <div style={{ zIndex: 10, display: 'flex', flexDirection: 'column', gap: '9px' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '18px' }}>
          <div style={{
            flex: 1,
            height: '14px',
            background: 'rgba(4,4,8,0.9)',
            border: '1px solid rgba(176,108,255,0.7)',
            borderRadius: '3px',
            overflow: 'hidden',
            boxShadow: '0 0 16px rgba(176,108,255,0.4), inset 0 0 8px rgba(0,0,0,0.9)'
          }}>
            <div style={{
              width: `${progress}%`,
              height: '100%',
              background: 'linear-gradient(90deg, #6a3fa0, #b06cff, #d8b4ff)',
              boxShadow: '0 0 14px #b06cff'
            }} />
          </div>
          <span style={{
            fontFamily: "'Orbitron', sans-serif",
            color: '#ffffff',
            fontSize: '22px',
            fontWeight: 800,
            width: '70px',
            textAlign: 'right',
            textShadow: '0 0 14px rgba(176,108,255,0.9)'
          }}>
            {progress}%
          </span>
        </div>
        <div style={{ textAlign: 'center', color: '#aab0ba', fontSize: '11px', letterSpacing: '4px' }}>
          СИНХРОНИЗАЦИЯ ТАКТОВ
        </div>

        {/* Tip */}
        <div style={{
          alignSelf: 'center',
          maxWidth: '760px',
          padding: '7px 18px',
          background: 'rgba(6,5,12,0.85)',
          border: '1px solid rgba(255,255,255,0.14)',
          borderRadius: '3px',
          textAlign: 'center',
          fontSize: '12px',
          color: '#c3c8d2'
        }}>
          💡 СОВЕТ: РЭБ снижает дальность обнаружения противника, но требует устойчивой линии снабжения.
        </div>
      </div>
    </div>
  );
};
