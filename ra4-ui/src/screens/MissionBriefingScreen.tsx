import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';

export const MissionBriefingScreen: React.FC = () => {
  const navigate = useNavigate();
  const [checkedObjectives, setCheckedObjectives] = useState([true, false, false, false]);

  const toggleObjective = (index: number) => {
    const next = [...checkedObjectives];
    next[index] = !next[index];
    setCheckedObjectives(next);
  };

  const handleLaunch = () => {
    navigate('/loading');
  };

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/screenshots/9.png') no-repeat center center`,
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
          <div style={{ color: '#ff2222', fontSize: '22px', fontWeight: 800, letterSpacing: '2px', lineHeight: 1 }}>
            RED ALERT 4
          </div>
        </div>

        <div style={{ textAlign: 'center' }}>
          <div style={{ color: '#ffffff', fontSize: '18px', fontWeight: 800, letterSpacing: '4px' }}>
            БРИФИНГ ОПЕРАЦИИ
          </div>
          <div style={{ color: '#ff2222', fontSize: '14px' }}>★</div>
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
          <div style={{
            width: '28px',
            height: '28px',
            borderRadius: '50%',
            background: '#3a0808',
            border: '1px solid #ff3333',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            color: '#ff2222',
            fontSize: '14px'
          }}>
            ★
          </div>
          <div>
            <div style={{ color: '#fff', fontSize: '12px', fontWeight: 700 }}>ТОВАРИЩ КОМАНДИР</div>
            <div style={{ color: '#ff4444', fontSize: '10px' }}>УРОВЕНЬ 45 ★</div>
          </div>
        </div>
      </div>

      {/* Main 3-Column Briefing Room */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '400px 1fr 340px',
        gap: '24px',
        flex: 1,
        alignItems: 'stretch',
        zIndex: 5,
        margin: '12px 0'
      }}>
        {/* Left Intel & Objectives Column */}
        <div className="ra4-panel clip-bevel-md" style={{
          padding: '18px',
          border: '1px solid #ff3333',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          background: 'rgba(12, 6, 8, 0.92)'
        }}>
          <div>
            <div style={{ color: '#ff4444', fontSize: '11px', letterSpacing: '2px', fontWeight: 700 }}>
              ОПЕРАЦИЯ
            </div>
            <h2 style={{ color: '#ff2222', fontSize: '26px', fontWeight: 800, margin: '2px 0 8px 0', letterSpacing: '1px' }}>
              КРАСНЫЙ РАССВЕТ
            </h2>
            <p style={{ color: '#ccc', fontSize: '12px', lineHeight: 1.5, fontFamily: "'Inter', sans-serif", marginBottom: '14px' }}>
              Альянс стягивает войска к нашим границам, маскируя подготовку к полномасштабному вторжению. Нанесите упреждающий удар и сломайте волю врага, пока он не укрепился.
            </p>

            {/* Objectives */}
            <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px' }}>
              <div style={{ color: '#ff3333', fontSize: '12px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '8px' }}>
                ЦЕЛИ ОПЕРАЦИИ:
              </div>
              {[
                'Уничтожить командный центр Альянса',
                'Вывести из строя спутниковую связь врага',
                'Обеспечить контроль над стратегическим мостом',
                'Эвакуировать наши войска в зону сбора'
              ].map((obj, i) => (
                <div
                  key={i}
                  onClick={() => toggleObjective(i)}
                  style={{
                    display: 'flex',
                    alignItems: 'center',
                    gap: '8px',
                    fontSize: '12px',
                    color: checkedObjectives[i] ? '#ffffff' : '#aaa',
                    marginBottom: '6px',
                    cursor: 'pointer',
                    fontFamily: "'Inter', sans-serif"
                  }}
                >
                  <span style={{ color: checkedObjectives[i] ? '#ff2222' : '#555', fontSize: '14px' }}>
                    ★
                  </span>
                  <span>{obj}</span>
                </div>
              ))}
            </div>

            {/* Reconnaissance Data */}
            <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px', marginTop: '10px' }}>
              <div style={{ color: '#ff3333', fontSize: '12px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '6px' }}>
                ДАННЫЕ РАЗВЕДКИ:
              </div>
              <ul style={{ color: '#aaa', fontSize: '11px', lineHeight: 1.5, paddingLeft: '14px', fontFamily: "'Inter', sans-serif" }}>
                <li>Альянс сосредоточил силы у переправы через реку.</li>
                <li>Замечена активность тяжёлой техники и авиации.</li>
                <li>Спутниковая связь обеспечивает координацию действий.</li>
                <li>Местность благоприятна для скрытного наступления.</li>
              </ul>
            </div>
          </div>

          {/* Bottom Thumbnails & Risk Forecast */}
          <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px' }}>
            <div style={{ color: '#888', fontSize: '10px', letterSpacing: '1.5px', marginBottom: '6px' }}>
              СВОДКА РАЙОНА БОЕВЫХ ДЕЙСТВИЙ:
            </div>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '6px', marginBottom: '10px' }}>
              {['МОСТ', 'БАЗА', 'ПЕРЕВАЛ'].map((label, idx) => (
                <div
                  key={idx}
                  style={{
                    height: '48px',
                    background: 'rgba(30,10,10,0.8)',
                    border: '1px solid rgba(255,50,50,0.3)',
                    borderRadius: '2px',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    color: '#888',
                    fontSize: '9px'
                  }}
                >
                  {label}
                </div>
              ))}
            </div>

            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '11px' }}>
              <span>СЛОЖНОСТЬ: <strong style={{ color: '#ff2222' }}>ВЕТЕРАН</strong></span>
              <span>ПРОГНОЗ: <strong style={{ color: '#ff4444' }}>ПОТЕРИ ВЫСОКИЕ</strong></span>
            </div>
          </div>
        </div>

        {/* Center Marshal Viktor Sokolov Space */}
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'flex-end',
          alignItems: 'center',
          paddingBottom: '20px'
        }}>
          {/* Commander Nameplate */}
          <div className="clip-bevel-sm" style={{
            background: 'linear-gradient(180deg, rgba(30,5,5,0.95) 0%, rgba(10,2,2,0.98) 100%)',
            border: '1px solid #ff2222',
            padding: '8px 28px',
            display: 'flex',
            alignItems: 'center',
            gap: '12px',
            boxShadow: '0 0 20px rgba(255,0,0,0.6)'
          }}>
            <span style={{ color: '#ff2222', fontSize: '20px' }}>★</span>
            <div>
              <div style={{ color: '#ff3333', fontSize: '11px', letterSpacing: '2px' }}>МАРШАЛ</div>
              <div style={{ color: '#ffffff', fontSize: '18px', fontWeight: 800, letterSpacing: '1px' }}>
                ВИКТОР СОКОЛОВ
              </div>
            </div>
          </div>
        </div>

        {/* Right Tactical Signal & Code Word Column */}
        <div className="ra4-panel clip-bevel-md" style={{
          padding: '18px',
          border: '1px solid #ff3333',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          background: 'rgba(12, 6, 8, 0.92)'
        }}>
          <div>
            <div style={{ color: '#ff4444', fontSize: '11px', letterSpacing: '2px', fontWeight: 700 }}>
              РАССТАНОВКА СИЛ ПРОТИВНИКА
            </div>
            <div style={{
              height: '110px',
              background: 'rgba(20,10,12,0.8)',
              border: '1px solid rgba(255,50,50,0.3)',
              borderRadius: '4px',
              marginTop: '6px',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: '#ff3333',
              fontSize: '11px'
            }}>
              [ТАКТИЧЕСКИЙ РАДАР СЕКТОРА]
            </div>

            {/* Intercepted Radio Signals */}
            <div style={{ marginTop: '16px', borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px' }}>
              <div style={{ color: '#ff4444', fontSize: '11px', letterSpacing: '2px', fontWeight: 700, marginBottom: '6px' }}>
                ПЕРЕХВАЧЕННЫЕ ПЕРЕГОВОРЫ
              </div>

              {/* Audio waveform */}
              <div style={{ display: 'flex', gap: '3px', alignItems: 'center', height: '24px', margin: '8px 0' }}>
                {[4, 12, 22, 16, 8, 20, 24, 10, 18, 24, 14, 6, 18, 22, 10, 15, 20, 8].map((h, i) => (
                  <div
                    key={i}
                    className="audio-bar"
                    style={{
                      width: '4px',
                      height: `${h}px`,
                      background: '#ff2222',
                      borderRadius: '1px',
                      animationDelay: `${i * 0.05}s`
                    }}
                  />
                ))}
              </div>

              <div style={{
                color: '#bbb',
                fontSize: '11px',
                fontStyle: 'italic',
                lineHeight: 1.4,
                fontFamily: "'Inter', sans-serif",
                background: 'rgba(0,0,0,0.5)',
                padding: '8px',
                borderRadius: '3px',
                borderLeft: '2px solid #ff2222'
              }}>
                «...полный ввод сил по сигналу. Кодовое слово: „Свобода“... ожидаем подтверждения...»
              </div>
            </div>
          </div>

          {/* Operation Code Word */}
          <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '12px' }}>
            <div style={{ color: '#888', fontSize: '10px', letterSpacing: '2px' }}>
              КОДОВОЕ СЛОВО ОПЕРАЦИИ:
            </div>
            <div style={{
              color: '#ff2222',
              fontSize: '36px',
              fontWeight: 900,
              letterSpacing: '8px',
              textShadow: '0 0 20px rgba(255,0,0,0.8)',
              marginTop: '4px'
            }}>
              ГРОМ
            </div>
          </div>
        </div>
      </div>

      {/* Bottom Action Strip */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderTop: '1px solid rgba(255,255,255,0.1)',
        paddingTop: '8px',
        zIndex: 10
      }}>
        <button
          onClick={() => navigate('/strategic-map')}
          className="ra4-btn-ussr clip-bevel-sm"
          style={{ padding: '8px 24px', fontSize: '14px' }}
        >
          ‹ НАЗАД
        </button>

        <button
          onClick={handleLaunch}
          className="clip-bevel-sm"
          style={{
            background: 'linear-gradient(180deg, #ff2222 0%, #7a0b0b 100%)',
            border: '1px solid #ff4444',
            color: '#ffffff',
            padding: '12px 60px',
            fontSize: '18px',
            fontWeight: 800,
            letterSpacing: '3px',
            cursor: 'pointer',
            boxShadow: '0 0 25px rgba(255,0,0,0.8)'
          }}
        >
          ★ ПРОДОЛЖИТЬ
        </button>

        <div style={{ display: 'flex', gap: '10px', alignItems: 'center' }}>
          <button onClick={() => navigate('/video-comms')} className="ra4-btn-ussr clip-bevel-sm" style={{ padding: '6px 16px', fontSize: '12px' }}>
            ⚙ НАСТРОЙКИ
          </button>
          <span style={{ color: '#ff4444', fontSize: '12px', letterSpacing: '1px' }}>
            БОЕВАЯ ГОТОВНОСТЬ: <strong>ВЫСОКАЯ</strong>
          </span>
        </div>
      </div>
    </div>
  );
};
