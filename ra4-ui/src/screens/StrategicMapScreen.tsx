import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';
import { FACTIONS } from '../data/factions';

interface MapNode {
  id: string;
  num: string;
  name: string;
  x: number;
  y: number;
  icon: string;
  status: 'completed' | 'active' | 'locked';
}

const NODES: MapNode[] = [
  { id: 'n1', num: '01', name: 'ПОЛЯРНЫЙ ЭХО', x: 19, y: 33, icon: '🛡', status: 'completed' },
  { id: 'n2', num: '02', name: 'БАЗА «КАРГАЛЫ»', x: 11, y: 52, icon: '🛡', status: 'completed' },
  { id: 'n3', num: '03', name: 'УЗЕЛ «СЕВЕР»', x: 32, y: 26, icon: '👁', status: 'completed' },
  { id: 'n4', num: '04', name: 'ЖЕЛЕЗНЫЙ ЗУБ', x: 35, y: 43, icon: '⚔', status: 'completed' },
  { id: 'n5', num: '05', name: 'РЕЛЕЙНАЯ СТАНЦИЯ', x: 57, y: 31, icon: '📦', status: 'completed' },
  { id: 'n6', num: '06', name: 'ГЛУБОКИЙ СИГНАЛ', x: 51, y: 52, icon: '👁', status: 'completed' },
  { id: 'n7', num: '07', name: 'ТИХИЙ РЕЛЕЙ', x: 38, y: 71, icon: '⚔', status: 'active' },
  { id: 'n8', num: '08', name: 'ЧИСТЫЙ КЛЮЧ', x: 59, y: 84, icon: '🛡', status: 'locked' }
];

const ROUTES: [number, number][][] = [
  [[19, 33], [32, 26]],
  [[19, 33], [35, 43]],
  [[11, 52], [35, 43]],
  [[32, 26], [57, 31]],
  [[35, 43], [38, 71]],
  [[57, 31], [51, 52]],
  [[51, 52], [59, 84]],
  [[38, 71], [59, 84]]
];

export const StrategicMapScreen: React.FC = () => {
  const navigate = useNavigate();
  const fac = FACTIONS.eurasian;
  const [selectedNodeId, setSelectedNodeId] = useState('n7');
  const [objectives, setObjectives] = useState([false, false]);

  const toggleObjective = (i: number) => {
    const next = [...objectives];
    next[i] = !next[i];
    setObjectives(next);
  };

  return (
    <div
      className={fac.themeClass}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/remaster/07_campaign_map_eurasian.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* ===== Top Command Strip ===== */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '120px 1fr 620px',
        alignItems: 'start',
        padding: '10px 24px',
        zIndex: 10,
        gap: '12px'
      }}>
        {/* Hex Emblem */}
        <div
          onClick={() => navigate(`/campaign/${fac.id}`)}
          style={{
            width: '78px',
            height: '88px',
            clipPath: 'polygon(50% 0, 100% 25%, 100% 75%, 50% 100%, 0 75%, 0 25%)',
            background: `linear-gradient(180deg, ${fac.color}44, rgba(8,6,14,0.95))`,
            border: `1px solid ${fac.color}`,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            fontSize: '34px',
            color: fac.color,
            cursor: 'pointer'
          }}
        >
          {fac.crest}
        </div>

        {/* Center Title */}
        <div style={{ textAlign: 'center' }}>
          <BrandLogo scale={0.56} />
          <div style={{
            fontFamily: "'Oswald', sans-serif",
            color: fac.color,
            fontSize: '15px',
            fontWeight: 700,
            letterSpacing: '4px',
            marginTop: '4px'
          }}>
            КАМПАНИЯ: ЕВРАЗИЙСКИЙ ПАКТ
          </div>
          <div style={{ color: '#c9cfda', fontSize: '12px', letterSpacing: '5px', marginTop: '3px' }}>
            ГЛАВА 4: БЕЛЫЙ ШУМ
          </div>
        </div>

        {/* Commander + Resources */}
        <div style={{ display: 'flex', justifyContent: 'flex-end', alignItems: 'flex-start', gap: '14px' }}>
          <div style={{
            display: 'flex',
            gap: '12px',
            padding: '8px 14px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.18)',
            borderRadius: '4px',
            minWidth: '230px'
          }}>
            <div style={{
              width: '46px',
              height: '54px',
              borderRadius: '3px',
              border: `1px solid ${fac.color}66`,
              background: 'linear-gradient(180deg, #2a2038, #0d0a16)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              fontSize: '22px',
              color: fac.color
            }}>
              ⚔
            </div>
            <div style={{ flex: 1 }}>
              <div style={{ color: '#fff', fontSize: '13px', fontWeight: 700 }}>КОМАНДИР</div>
              <div style={{ color: '#9aa2b0', fontSize: '11px' }}>РАНГ: ПОЛКОВНИК • УРОВЕНЬ 45</div>
              <div style={{ marginTop: '5px', height: '4px', background: 'rgba(0,0,0,0.8)', borderRadius: '2px', overflow: 'hidden' }}>
                <div style={{ width: '68%', height: '100%', background: fac.color }} />
              </div>
            </div>
          </div>

          <div style={{
            display: 'flex',
            gap: '18px',
            padding: '8px 16px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.18)',
            borderRadius: '4px'
          }}>
            <div>
              <div style={{ color: '#e8ecf2', fontSize: '14px', fontWeight: 800 }}>125 840</div>
              <div style={{ color: '#8b93a2', fontSize: '9px', letterSpacing: '1px' }}>ОПЕРАЦИОННЫЕ СРЕДСТВА</div>
            </div>
            <div>
              <div style={{ color: '#e8ecf2', fontSize: '14px', fontWeight: 800 }}>8 450</div>
              <div style={{ color: '#8b93a2', fontSize: '9px', letterSpacing: '1px' }}>РЕСУРСЫ РЭБ</div>
            </div>
            <div>
              <div style={{ color: '#e8ecf2', fontSize: '14px', fontWeight: 800 }}>1 250</div>
              <div style={{ color: '#8b93a2', fontSize: '9px', letterSpacing: '1px' }}>ДАННЫЕ</div>
            </div>
            <div style={{ display: 'flex', gap: '8px', alignItems: 'center' }}>
              {['✉', '👥', '⚙'].map((ic, i) => (
                <button key={i} onClick={() => navigate('/video-comms')} style={{
                  width: '30px', height: '30px',
                  background: 'rgba(20,18,30,0.9)', border: '1px solid rgba(255,255,255,0.2)',
                  borderRadius: '3px', color: '#cfd4dd', cursor: 'pointer'
                }}>{ic}</button>
              ))}
            </div>
          </div>
        </div>
      </div>

      {/* ===== Middle: Map + Panels ===== */}
      <div style={{ flex: 1, position: 'relative', zIndex: 5 }}>
        {/* Left Operations Menu */}
        <div style={{
          position: 'absolute',
          top: '30px',
          left: '24px',
          display: 'flex',
          flexDirection: 'column',
          gap: '6px',
          padding: '12px',
          background: 'rgba(8,7,14,0.82)',
          border: '1px solid rgba(255,255,255,0.16)',
          borderRadius: '6px',
          minWidth: '170px'
        }}>
          {[
            { icon: '🛡', label: 'ОБОРОНА' },
            { icon: '👁', label: 'РАЗВЕДКА' },
            { icon: '⚔', label: 'УДАР' },
            { icon: '📦', label: 'ЛОГИСТИКА' },
            { icon: '✪', label: 'ОСОБАЯ ОПЕРАЦИЯ' }
          ].map(item => (
            <button
              key={item.label}
              onClick={() => navigate('/briefing')}
              style={{
                display: 'flex',
                alignItems: 'center',
                gap: '10px',
                padding: '7px 10px',
                background: 'transparent',
                border: 'none',
                borderRadius: '4px',
                color: '#cdd3dd',
                fontFamily: "'Oswald', sans-serif",
                fontSize: '13px',
                letterSpacing: '1.5px',
                textAlign: 'left',
                cursor: 'pointer'
              }}
            >
              <span style={{ fontSize: '15px', filter: 'drop-shadow(0 0 5px rgba(176,108,255,0.7))' }}>{item.icon}</span>
              {item.label}
            </button>
          ))}
        </div>

        {/* Interactive Route Lines */}
        <svg style={{ position: 'absolute', inset: 0, width: '100%', height: '100%', pointerEvents: 'none' }}>
          {ROUTES.map(([a, b], i) => (
            <line
              key={i}
              x1={`${a[0]}%`} y1={`${a[1]}%`}
              x2={`${b[0]}%`} y2={`${b[1]}%`}
              stroke="#b06cff"
              strokeWidth="2.5"
              strokeDasharray="8 6"
              opacity="0.55"
            />
          ))}
        </svg>

        {/* Mission Nodes */}
        {NODES.map(node => {
          const isActive = node.id === selectedNodeId;
          const size = node.status === 'active' ? 64 : 44;
          return (
            <div
              key={node.id}
              onClick={() => setSelectedNodeId(node.id)}
              style={{
                position: 'absolute',
                left: `${node.x}%`,
                top: `${node.y}%`,
                transform: 'translate(-50%, -50%)',
                cursor: 'pointer',
                display: 'flex',
                alignItems: 'center',
                gap: '10px',
                zIndex: node.status === 'active' ? 12 : 6
              }}
            >
              <div style={{
                width: `${size}px`,
                height: `${size}px`,
                clipPath: 'polygon(50% 0, 93% 25%, 93% 75%, 50% 100%, 7% 75%, 7% 25%)',
                background: node.status === 'active'
                  ? 'radial-gradient(circle, #d8b4ff 0%, #8b4fe0 60%, #3a1f66 100%)'
                  : 'linear-gradient(180deg, rgba(30,22,48,0.95), rgba(10,7,18,0.95))',
                border: `1px solid ${isActive || node.status === 'active' ? '#e2ccff' : 'rgba(176,108,255,0.5)'}`,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontSize: node.status === 'active' ? '24px' : '17px',
                color: node.status === 'locked' ? '#777' : '#fff',
                boxShadow: node.status === 'active' ? '0 0 30px rgba(176,108,255,0.95)' : 'none',
                animation: isActive ? 'alert-flash 1.8s infinite' : undefined,
                flexShrink: 0
              }}>
                {node.icon}
              </div>
              <div style={{
                background: 'rgba(6,5,10,0.88)',
                border: '1px solid rgba(176,108,255,0.4)',
                borderRadius: '3px',
                padding: '3px 9px',
                whiteSpace: 'nowrap'
              }}>
                <span style={{ color: '#fff', fontSize: '12px', fontWeight: 700, letterSpacing: '1px' }}>
                  {node.num}. {node.name}
                </span>
                <div style={{ color: fac.color, fontSize: '10px', letterSpacing: '3px' }}>
                  ◆ ◆ ◆
                </div>
              </div>
            </div>
          );
        })}

        {/* Bottom-left Chapter Progress Panel */}
        <div style={{
          position: 'absolute',
          bottom: '24px',
          left: '24px',
          width: '330px',
          padding: '16px 18px',
          background: 'linear-gradient(180deg, rgba(10,8,16,0.94), rgba(6,5,10,0.97))',
          border: `1px solid ${fac.color}66`,
          borderRadius: '6px'
        }}>
          <div style={{ fontFamily: "'Oswald', sans-serif", color: fac.color, fontSize: '14px', fontWeight: 700, letterSpacing: '2px', marginBottom: '10px' }}>
            ПРОГРЕСС ГЛАВЫ
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '12px', color: '#aeb4c0', marginBottom: '8px' }}>
            <span>Выполнено миссий</span>
            <strong style={{ color: '#fff' }}>6 / 12</strong>
          </div>
          <div style={{ display: 'flex', gap: '5px', marginBottom: '14px' }}>
            {Array.from({ length: 12 }).map((_, i) => (
              <div key={i} style={{
                flex: 1,
                height: '8px',
                borderRadius: '2px',
                background: i < 6 ? fac.color : 'rgba(255,255,255,0.12)'
              }} />
            ))}
          </div>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '8px' }}>
            {[
              { label: '4 МИССИИ', done: true, icon: '🎖' },
              { label: '8 МИССИЙ', done: true, icon: '🏅' },
              { label: '12 МИССИЙ', done: false, icon: '🚀' }
            ].map(m => (
              <div key={m.label} style={{ position: 'relative', textAlign: 'center' }}>
                <div style={{ color: '#98a0ac', fontSize: '10px', letterSpacing: '1px', marginBottom: '4px' }}>{m.label}</div>
                <div style={{
                  height: '40px',
                  borderRadius: '3px',
                  border: '1px solid rgba(255,255,255,0.14)',
                  background: m.done ? `${fac.color}22` : 'rgba(255,255,255,0.05)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  fontSize: '18px',
                  filter: m.done ? 'grayscale(0)' : 'grayscale(1) opacity(0.5)'
                }}>
                  {m.icon}
                  {m.done && (
                    <span style={{
                      position: 'absolute',
                      bottom: '-6px',
                      right: '-4px',
                      width: '18px',
                      height: '18px',
                      borderRadius: '50%',
                      background: fac.color,
                      color: '#0b0712',
                      fontSize: '11px',
                      fontWeight: 900,
                      display: 'flex',
                      alignItems: 'center',
                      justifyContent: 'center'
                    }}>✓</span>
                  )}
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Right Mission Detail Card */}
        <div style={{
          position: 'absolute',
          top: '10px',
          right: '24px',
          width: '400px',
          padding: '18px 20px',
          background: 'linear-gradient(180deg, rgba(10,8,16,0.94), rgba(6,5,10,0.97))',
          border: `1px solid ${fac.color}88`,
          borderRadius: '6px',
          boxShadow: `inset 0 0 30px ${fac.color}10`
        }}>
          <div style={{ fontFamily: "'Oswald', sans-serif", color: '#ffffff', fontSize: '19px', fontWeight: 800, letterSpacing: '1px', marginBottom: '12px' }}>
            07. ОПЕРАЦИЯ «ТИХИЙ РЕЛЕЙ»
          </div>

          {/* Thumbnail */}
          <div style={{
            height: '130px',
            borderRadius: '4px',
            border: '1px solid rgba(255,255,255,0.16)',
            background: 'linear-gradient(160deg, #241a38 0%, #120c20 55%, #090613 100%)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            marginBottom: '14px',
            position: 'relative'
          }}>
            <span style={{ fontSize: '40px', filter: 'drop-shadow(0 0 12px rgba(176,108,255,0.8))' }}>⚔</span>
            <span style={{ position: 'absolute', bottom: '6px', right: '8px', color: '#8b93a2', fontSize: '9px', letterSpacing: '1px' }}>
              РАЙОН ОПЕРАЦИИ • ГОРНЫЙ КОРИДОР
            </span>
          </div>

          <div style={{ color: fac.color, fontSize: '12px', fontWeight: 700, letterSpacing: '2px', marginBottom: '4px' }}>ЦЕЛЬ МИССИИ</div>
          <p style={{ color: '#c3c8d2', fontSize: '12px', lineHeight: 1.5, margin: '0 0 12px 0' }}>
            Нарушить сеть наведения противника и провести броне группу через горный коридор.
          </p>

          <div style={{ color: fac.color, fontSize: '12px', fontWeight: 700, letterSpacing: '2px', marginBottom: '6px' }}>ЗАДАЧИ</div>
          {['Подавить 3 узла связи', 'Сохранить мобильный комплекс РЭБ'].map((t, i) => (
            <div key={i} onClick={() => toggleObjective(i)} style={{
              display: 'flex',
              alignItems: 'center',
              gap: '9px',
              fontSize: '12px',
              color: objectives[i] ? '#fff' : '#aab0bc',
              cursor: 'pointer',
              marginBottom: '5px'
            }}>
              <span style={{
                width: '13px', height: '13px',
                border: `1px solid ${fac.color}`,
                borderRadius: '2px',
                display: 'inline-flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontSize: '9px',
                color: fac.color,
                background: objectives[i] ? `${fac.color}33` : 'transparent'
              }}>
                {objectives[i] ? '✓' : ''}
              </span>
              {t}
            </div>
          ))}

          <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', margin: '12px 0 10px 0' }} />

          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <div style={{ color: fac.color, fontSize: '12px', fontWeight: 700, letterSpacing: '2px' }}>НАГРАДЫ</div>
            <div style={{ display: 'flex', gap: '18px' }}>
              <span style={{ color: '#ffd76a', fontSize: '12px' }}>★ ОПЫТ <strong>1 500</strong></span>
              <span style={{ color: '#8fd4ff', fontSize: '12px' }}>▦ ДАННЫЕ <strong>800</strong></span>
            </div>
          </div>

          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: '10px', padding: '7px 10px', background: 'rgba(255,255,255,0.05)', borderRadius: '3px' }}>
            <span style={{ color: '#98a0ac', fontSize: '11px', letterSpacing: '1px' }}>СЛОЖНОСТЬ</span>
            <span style={{ color: '#fff', fontSize: '12px', fontWeight: 700 }}>ВЕТЕРАН ❖ ▾</span>
          </div>

          <button
            onClick={() => navigate('/briefing')}
            style={{
              width: '100%',
              marginTop: '14px',
              height: '50px',
              background: `linear-gradient(180deg, ${fac.color}, ${fac.dimColor})`,
              border: `1px solid ${fac.color}`,
              borderRadius: '4px',
              color: '#0b0712',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '17px',
              fontWeight: 800,
              letterSpacing: '2px',
              cursor: 'pointer',
              boxShadow: `0 0 22px ${fac.color}88`,
              clipPath: 'polygon(0 8px, 10px 0, 100% 0, 100% calc(100% - 8px), calc(100% - 10px) 100%, 0 100%)'
            }}
          >
            ❖&nbsp;&nbsp;НАЧАТЬ МИССИЮ
          </button>

          <button
            onClick={() => navigate('/briefing')}
            style={{
              width: '100%',
              marginTop: '8px',
              height: '38px',
              background: 'rgba(255,255,255,0.06)',
              border: '1px solid rgba(255,255,255,0.2)',
              borderRadius: '4px',
              color: '#cdd3dd',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '13px',
              letterSpacing: '2px',
              cursor: 'pointer'
            }}
          >
            ИНФОРМАЦИЯ О МИССИИ
          </button>
        </div>
      </div>

      {/* ===== Bottom Bar ===== */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '150px 1fr auto auto',
        gap: '14px',
        alignItems: 'center',
        padding: '10px 24px 14px 24px',
        zIndex: 10
      }}>
        <button
          onClick={() => navigate(`/campaign/${fac.id}`)}
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

        <div />{/* spacer under map emblem */}

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
            letterSpacing: '2px',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            gap: '10px'
          }}
        >
          🌐 МИРОВАЯ ОБСТАНОВКА
        </button>

        <button
          onClick={() => navigate('/briefing')}
          style={{
            height: '42px',
            padding: '0 22px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '13px',
            letterSpacing: '2px',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            gap: '10px'
          }}
        >
          🗀 АРХИВ БРИФИНГОВ
        </button>
      </div>
    </div>
  );
};
