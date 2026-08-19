import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';

export const LoadingScreen: React.FC = () => {
  const navigate = useNavigate();
  const [progress, setProgress] = useState(72);

  useEffect(() => {
    const timer = setInterval(() => {
      setProgress(p => {
        if (p >= 100) {
          clearInterval(timer);
          setTimeout(() => {
            navigate('/hud?mode=ussr-base');
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
      onClick={() => navigate('/hud?mode=ussr-base')}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/screenshots/19.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '24px 40px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Oswald', sans-serif",
        cursor: 'pointer'
      }}
    >
      {/* Top Header */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderBottom: '1px solid rgba(255,50,50,0.3)',
        paddingBottom: '10px',
        zIndex: 10
      }}>
        <div>
          <div style={{ color: '#888', fontSize: '10px', letterSpacing: '3px' }}>COMMAND & CONQUER™</div>
          <div style={{ color: '#ff2222', fontSize: '24px', fontWeight: 800, letterSpacing: '2px', lineHeight: 1 }}>
            RED ALERT 4
          </div>
        </div>

        <div style={{ textAlign: 'right' }}>
          <div style={{ color: '#ff3333', fontSize: '13px', letterSpacing: '2px', fontWeight: 700 }}>
            ЗАГРУЗКА МИССИИ
          </div>
          <div style={{ color: '#ffffff', fontSize: '18px', fontWeight: 800 }}>
            ОПЕРАЦИЯ «КИЕВ-86» ★
          </div>
        </div>
      </div>

      {/* Main Body with Left Intel Overlay */}
      <div style={{
        display: 'flex',
        flex: 1,
        alignItems: 'center',
        zIndex: 5,
        margin: '20px 0'
      }}>
        <div className="ra4-panel clip-bevel-md" style={{
          width: '380px',
          padding: '20px',
          border: '1px solid #ff3333',
          background: 'rgba(15, 8, 10, 0.92)',
          display: 'flex',
          flexDirection: 'column',
          gap: '14px'
        }}>
          {/* Summary */}
          <div>
            <div style={{ color: '#ff3333', fontSize: '12px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '4px' }}>
              СВОДКА:
            </div>
            <p style={{ color: '#ccc', fontSize: '12px', lineHeight: 1.5, fontFamily: "'Inter', sans-serif", margin: 0 }}>
              Американцы укрепили позиции в Киеве, превратив город в плацдарм для дальнейшего наступления вглубь территории СССР. Наша цель — выбить противника, уничтожить их командный центр и восстановить контроль.
            </p>
          </div>

          {/* Objectives */}
          <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px' }}>
            <div style={{ color: '#ff3333', fontSize: '12px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '6px' }}>
              ЦЕЛИ:
            </div>
            <ul style={{ listStyle: 'none', padding: 0, margin: 0, display: 'flex', flexDirection: 'column', gap: '4px', fontSize: '12px', fontFamily: "'Inter', sans-serif", color: '#ddd' }}>
              <li>★ Уничтожить командный центр США</li>
              <li>★ Ликвидировать генерала Хейса</li>
              <li>★ Захватить центральный район Киева</li>
              <li>★ Эвакуировать инженеров</li>
            </ul>
          </div>

          {/* Tactical Advice Box */}
          <div style={{
            borderTop: '1px solid rgba(255,255,255,0.1)',
            paddingTop: '10px',
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
            background: 'rgba(40, 10, 10, 0.5)',
            padding: '8px',
            borderRadius: '4px',
            border: '1px solid rgba(255,50,50,0.3)'
          }}>
            <span style={{ color: '#ff2222', fontSize: '20px' }}>★</span>
            <div>
              <div style={{ color: '#ff3333', fontSize: '11px', fontWeight: 700 }}>СОВЕТ:</div>
              <div style={{ color: '#aaa', fontSize: '11px', fontFamily: "'Inter', sans-serif" }}>
                Используйте штурмовых инженеров для захвата вражеских построек и техники.
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Bottom Loading Progress Bar */}
      <div style={{
        display: 'flex',
        flexDirection: 'column',
        gap: '8px',
        zIndex: 10
      }}>
        {/* Bar */}
        <div style={{
          display: 'flex',
          alignItems: 'center',
          gap: '16px'
        }}>
          <div style={{
            flex: 1,
            height: '10px',
            background: 'rgba(0,0,0,0.85)',
            borderRadius: '4px',
            overflow: 'hidden',
            border: '1px solid #ff3333',
            boxShadow: '0 0 10px rgba(255,0,0,0.4)'
          }}>
            <div style={{
              width: `${progress}%`,
              height: '100%',
              background: 'linear-gradient(90deg, #ff2222, #ff8800, #ffdd00)',
              boxShadow: '0 0 15px #ff2222'
            }} />
          </div>
          <span style={{ color: '#ffffff', fontSize: '20px', fontWeight: 800, width: '60px', textAlign: 'right' }}>
            {progress}%
          </span>
        </div>

        {/* Tip Text */}
        <div style={{ textAlign: 'center', color: '#ff4444', fontSize: '13px', letterSpacing: '1px' }}>
          <strong>Подсказка:</strong> Используйте инженеров для захвата вражеских зданий. (Кликните для пропуска)
        </div>
      </div>
    </div>
  );
};
