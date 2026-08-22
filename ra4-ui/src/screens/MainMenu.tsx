import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';
import { FRONT_LEGEND } from '../data/factions';

const MENU_ITEMS = [
  { id: 'campaign', label: 'КАМПАНИЯ', icon: '❖', path: '/campaign-select' },
  { id: 'multiplayer', label: 'СЕТЕВАЯ ИГРА', icon: '🌐', path: '/skirmish' },
  { id: 'skirmish', label: 'СХВАТКА', icon: '⚔', path: '/skirmish?mode=skirmish' },
  { id: 'editor', label: 'РЕДАКТОР', icon: '🛠', path: '/strategic-map' },
  { id: 'encyclopedia', label: 'ЭНЦИКЛОПЕДИЯ', icon: '📖', path: '/hud?mode=eurasian-ground' },
  { id: 'mods', label: 'МОДИФИКАЦИИ', icon: '⚙', path: '/briefing' },
  { id: 'settings', label: 'НАСТРОЙКИ', icon: '⚙', path: '/video-comms' },
  { id: 'exit', label: 'ВЫХОД', icon: '⮌', path: '/' }
];

export const MainMenu: React.FC = () => {
  const navigate = useNavigate();
  const [activeMenuIndex, setActiveMenuIndex] = useState(0);
  const [operationPhase] = useState(2);

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
        justifyContent: 'space-between',
        padding: '26px 34px 18px 34px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* Top Center Logo */}
      <div style={{
        position: 'absolute',
        top: '22px',
        left: '50%',
        transform: 'translateX(-50%)',
        zIndex: 10,
        pointerEvents: 'none',
        textAlign: 'center'
      }}>
        <div style={{ fontSize: '30px', color: '#dfe5ec', filter: 'drop-shadow(0 0 10px rgba(140,170,255,0.5))', lineHeight: 1 }}>⫸⫷</div>
        <div style={{ marginTop: '4px' }}>
          <BrandLogo scale={0.82} subtitle="ГЛОБАЛЬНЫЙ КОМАНДНЫЙ ЦЕНТР" />
        </div>
      </div>

      {/* Left Vertical Menu Stack */}
      <div style={{
        display: 'flex',
        flexDirection: 'column',
        gap: '9px',
        width: '330px',
        zIndex: 5,
        marginTop: '90px'
      }}>
        {MENU_ITEMS.map((item, idx) => {
          const isSelected = activeMenuIndex === idx;
          return (
            <button
              key={item.id}
              onMouseEnter={() => setActiveMenuIndex(idx)}
              onClick={() => navigate(item.path)}
              className="clip-bevel-sm"
              style={{
                height: '52px',
                display: 'flex',
                alignItems: 'center',
                gap: '16px',
                padding: '0 20px',
                cursor: 'pointer',
                textAlign: 'left',
                fontFamily: "'Oswald', sans-serif",
                fontSize: '17px',
                letterSpacing: '3px',
                fontWeight: 600,
                color: isSelected ? '#ffffff' : '#b8c0cc',
                background: isSelected
                  ? 'linear-gradient(90deg, rgba(88,48,150,0.95) 0%, rgba(40,22,70,0.92) 55%, rgba(14,10,24,0.85) 100%)'
                  : 'linear-gradient(90deg, rgba(12,14,22,0.9) 0%, rgba(8,9,14,0.82) 100%)',
                border: isSelected
                  ? '1px solid rgba(176,108,255,0.9)'
                  : '1px solid rgba(120,135,160,0.28)',
                borderLeft: isSelected ? '4px solid #b06cff' : '4px solid rgba(60,68,84,0.6)',
                boxShadow: isSelected
                  ? '0 0 22px rgba(176,108,255,0.45), inset 0 0 18px rgba(176,108,255,0.15)'
                  : '0 2px 8px rgba(0,0,0,0.7)',
                clipPath: 'polygon(0 8px, 14px 0, 100% 0, 100% calc(100% - 8px), calc(100% - 14px) 100%, 0 100%)',
                transition: 'all 0.15s ease'
              }}
            >
              <span style={{ fontSize: '19px', width: '24px', textAlign: 'center', filter: isSelected ? 'drop-shadow(0 0 6px rgba(200,160,255,0.9))' : 'none' }}>
                {item.icon}
              </span>
              <span>{item.label}</span>
            </button>
          );
        })}
      </div>

      {/* Bottom Dashboard Bar */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '300px 1.1fr 1fr 260px',
        gap: '12px',
        alignItems: 'stretch',
        zIndex: 10
      }}>
        {/* Commander Profile Card */}
        <div className="ra4-panel" style={{
          padding: '12px 16px',
          display: 'flex',
          gap: '14px'
        }}>
          <div style={{
            width: '64px',
            height: '72px',
            borderRadius: '4px',
            border: '1px solid rgba(140,165,210,0.5)',
            background: 'linear-gradient(180deg, rgba(40,52,80,0.9), rgba(14,18,30,0.95))',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            fontSize: '30px',
            color: '#9fb6dd',
            flexShrink: 0
          }}>
            ⚔
          </div>
          <div style={{ flex: 1, display: 'flex', flexDirection: 'column' }}>
            <div style={{ color: '#eef2f8', fontSize: '15px', fontWeight: 700, letterSpacing: '1px' }}>
              КОМАНДИР • УРОВЕНЬ 24
            </div>
            <div style={{
              marginTop: '8px',
              width: '100%',
              height: '7px',
              background: 'rgba(0,0,0,0.75)',
              borderRadius: '3px',
              overflow: 'hidden',
              border: '1px solid rgba(255,255,255,0.14)'
            }}>
              <div style={{ width: '72%', height: '100%', background: 'linear-gradient(90deg, #4f7dd9, #7fb2ff)' }} />
            </div>
            <div style={{ color: '#93a0b2', fontSize: '11px', marginTop: '4px' }}>
              34 750 / 48 000 ОП
            </div>
            {/* Faction badges row */}
            <div style={{ display: 'flex', gap: '8px', marginTop: 'auto' }}>
              {FRONT_LEGEND.map(f => (
                <div key={f.name} style={{
                  width: '30px',
                  height: '30px',
                  borderRadius: '4px',
                  border: `1px solid ${f.color}66`,
                  background: `radial-gradient(circle at 50% 35%, ${f.color}33, rgba(6,8,14,0.95))`,
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  color: f.color,
                  fontSize: '14px'
                }}>
                  ❖
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Front Summary Card */}
        <div className="ra4-panel" style={{ padding: '12px 16px', display: 'flex', gap: '16px' }}>
          <div style={{ flex: 1 }}>
            <div style={{ color: '#dfe5ee', fontSize: '15px', fontWeight: 800, letterSpacing: '2px', marginBottom: '8px' }}>
              СВОДКА ФРОНТОВ
            </div>
            {FRONT_LEGEND.map(f => (
              <div key={f.name} style={{ display: 'flex', alignItems: 'center', gap: '8px', marginBottom: '4px' }}>
                <span style={{ width: '10px', height: '10px', background: f.color, borderRadius: '2px', boxShadow: `0 0 6px ${f.color}` }} />
                <span style={{ color: '#aab4c2', fontSize: '11px', letterSpacing: '1px' }}>{f.name}</span>
              </div>
            ))}
          </div>
          <div style={{
            width: '130px',
            alignSelf: 'stretch',
            borderRadius: '4px',
            border: '1px solid rgba(255,255,255,0.14)',
            background:
              'linear-gradient(160deg, rgba(176,108,255,0.35) 0%, rgba(63,141,255,0.3) 38%, rgba(47,217,138,0.3) 66%, rgba(232,161,61,0.32) 100%)',
            position: 'relative',
            overflow: 'hidden'
          }}>
            <svg viewBox="0 0 100 100" preserveAspectRatio="none" style={{ position: 'absolute', inset: 0, width: '100%', height: '100%' }}>
              <path d="M0,55 L25,42 L45,58 L70,38 L100,52 L100,100 L0,100 Z" fill="rgba(10,14,24,0.55)" />
              <path d="M0,62 L30,50 L52,66 L78,46 L100,58" stroke="rgba(255,255,255,0.35)" strokeWidth="1" fill="none" />
            </svg>
          </div>
        </div>

        {/* Current Operation Card */}
        <div className="ra4-panel" style={{ padding: '12px 16px', position: 'relative', overflow: 'hidden' }}>
          <div style={{
            position: 'absolute',
            top: '8px',
            right: '10px',
            fontSize: '22px',
            color: '#9fb6dd'
          }}>✪</div>
          <div style={{ color: '#dfe5ee', fontSize: '15px', fontWeight: 800, letterSpacing: '2px', marginBottom: '6px' }}>
            ТЕКУЩАЯ ОПЕРАЦИЯ
          </div>
          {/* Operation thumbnail */}
          <div style={{
            height: '56px',
            borderRadius: '3px',
            border: '1px solid rgba(255,255,255,0.16)',
            background: 'linear-gradient(180deg, rgba(30,44,70,0.9), rgba(8,10,18,0.95))',
            display: 'flex',
            alignItems: 'flex-end',
            padding: '4px 8px',
            marginBottom: '8px'
          }}>
            <span style={{ color: '#7d8ea8', fontSize: '9px', letterSpacing: '1px' }}>ОПЕРАТИВНАЯ ЗОНА • АТЛАНТИКА</span>
          </div>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'baseline' }}>
            <strong style={{ color: '#ffffff', fontSize: '14px', letterSpacing: '1px' }}>БАРЬЕР «ТИФОН»</strong>
            <span style={{ color: '#8fa0b6', fontSize: '11px' }}>ФАЗА 2/4</span>
          </div>
          <div style={{ color: '#93a0b2', fontSize: '11px', margin: '3px 0 6px' }}>
            Удержать острова и защитить логистические маршруты.
          </div>
          <div style={{ display: 'flex', gap: '4px' }}>
            {[1, 2, 3, 4, 5, 6, 7].map(i => (
              <div key={i} style={{
                flex: 1,
                height: '6px',
                borderRadius: '2px',
                background: i <= operationPhase * 2 ? 'linear-gradient(90deg, #4f7dd9, #7fb2ff)' : 'rgba(255,255,255,0.12)'
              }} />
            ))}
          </div>
        </div>

        {/* Network Status Card */}
        <div className="ra4-panel" style={{ padding: '12px 16px', display: 'flex', flexDirection: 'column' }}>
          <div style={{ color: '#dfe5ee', fontSize: '15px', fontWeight: 800, letterSpacing: '2px', marginBottom: '6px' }}>
            СОСТОЯНИЕ СЕТИ
          </div>
          <div style={{
            flex: 1,
            borderRadius: '3px',
            border: '1px solid rgba(255,255,255,0.14)',
            background:
              'radial-gradient(circle at 20% 30%, rgba(47,217,138,0.25), transparent 40%), radial-gradient(circle at 70% 60%, rgba(63,141,255,0.3), transparent 45%), rgba(6,10,18,0.9)',
            position: 'relative',
            overflow: 'hidden'
          }}>
            <svg viewBox="0 0 100 60" style={{ position: 'absolute', inset: 0, width: '100%', height: '100%' }}>
              {[[18, 20], [32, 36], [48, 16], [60, 42], [76, 26], [86, 48]].map(([x, y], i) => (
                <circle key={i} cx={x} cy={y} r="1.8" fill="#57e89a">
                  <animate attributeName="opacity" values="1;0.3;1" dur={`${1.6 + i * 0.3}s`} repeatCount="indefinite" />
                </circle>
              ))}
              <path d="M18,20 L32,36 L48,16 L60,42 L76,26 L86,48" stroke="rgba(87,232,154,0.4)" strokeWidth="0.7" fill="none" />
            </svg>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '8px', marginTop: '8px' }}>
            <span style={{ color: '#57e89a', fontSize: '13px' }}>📶</span>
            <span style={{ color: '#57e89a', fontSize: '12px', fontWeight: 700, letterSpacing: '1px' }}>СЕТЬ: ПОДКЛЮЧЕНО</span>
          </div>
        </div>
      </div>
    </div>
  );
};
