import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';

export const MainMenu: React.FC = () => {
  const navigate = useNavigate();
  const [activeMenuIndex, setActiveMenuIndex] = useState(0);
  const [newsIndex, setNewsIndex] = useState(0);

  const menuItems = [
    { id: 'campaign', label: 'КАМПАНИЯ', icon: '★', path: '/campaign-select' },
    { id: 'multiplayer', label: 'СЕТЕВАЯ ИГРА', icon: '🌐', path: '/skirmish' },
    { id: 'skirmish', label: 'СХВАТКА', icon: '⚔', path: '/skirmish' },
    { id: 'editor', label: 'РЕДАКТОР', icon: '🛠', path: '/strategic-map' },
    { id: 'encyclopedia', label: 'ЭНЦИКЛОПЕДИЯ', icon: '📖', path: '/hud?mode=ussr-tank-assault' },
    { id: 'mods', label: 'МОДИФИКАЦИИ', icon: '⚙', path: '/briefing' },
    { id: 'settings', label: 'НАСТРОЙКИ', icon: '⚙', path: '/video-comms' },
    { id: 'exit', label: 'ВЫХОД', icon: '⮌', path: '/' }
  ];

  const newsItems = [
    { title: 'Добро пожаловать, командир.', subtitle: 'Красная угроза возвращается.' },
    { title: 'Новый театр военных действий:', subtitle: 'Арктический фронт открыт.' },
    { title: 'Хроноаномалия зафиксирована:', subtitle: 'Секретные испытания начались.' }
  ];

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/screenshots/2.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '30px 40px 20px 40px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Oswald', sans-serif"
      }}
    >
      {/* Top Center Logo and Insignia */}
      <div style={{
        position: 'absolute',
        top: '25px',
        left: '50%',
        transform: 'translateX(-50%)',
        textAlign: 'center',
        zIndex: 10,
        pointerEvents: 'none'
      }}>
        <div style={{
          color: '#e0e0e0',
          fontSize: '15px',
          letterSpacing: '5px',
          fontWeight: 400,
          textShadow: '0 0 8px rgba(255,255,255,0.4)'
        }}>
          COMMAND & CONQUER™
        </div>
        <div style={{
          color: '#ff2222',
          fontSize: '3.6rem',
          fontWeight: 800,
          letterSpacing: '4px',
          textShadow: '0 0 25px rgba(255,0,0,0.8), 0 2px 6px rgba(0,0,0,0.9)',
          lineHeight: 1
        }}>
          RED ALERT 4
        </div>
      </div>

      {/* Main Content Area: Left Menu Stack */}
      <div style={{
        display: 'flex',
        flex: 1,
        alignItems: 'center',
        zIndex: 5,
        marginTop: '60px'
      }}>
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          gap: '8px',
          width: '280px'
        }}>
          {menuItems.map((item, idx) => {
            const isSelected = activeMenuIndex === idx;
            return (
              <button
                key={item.id}
                onMouseEnter={() => setActiveMenuIndex(idx)}
                onClick={() => navigate(item.path)}
                className={isSelected ? 'ra4-btn-ussr active clip-bevel-sm' : 'ra4-btn-ussr clip-bevel-sm'}
                style={{
                  height: '48px',
                  display: 'flex',
                  alignItems: 'center',
                  padding: '0 18px',
                  gap: '14px',
                  fontSize: '17px',
                  textAlign: 'left',
                  borderLeft: isSelected ? '4px solid #ff2222' : '1px solid #6b1717',
                  background: isSelected
                    ? 'linear-gradient(90deg, rgba(160, 20, 20, 0.95) 0%, rgba(60, 10, 10, 0.9) 100%)'
                    : 'linear-gradient(90deg, rgba(25, 12, 12, 0.85) 0%, rgba(15, 8, 8, 0.85) 100%)'
                }}
              >
                <span style={{
                  color: isSelected ? '#ffdd00' : '#ff4444',
                  fontSize: '18px',
                  width: '20px',
                  display: 'inline-block',
                  textAlign: 'center'
                }}>
                  {item.icon}
                </span>
                <span style={{
                  color: isSelected ? '#ffffff' : '#e0d8d8',
                  letterSpacing: '2px',
                  fontWeight: 600
                }}>
                  {item.label}
                </span>
              </button>
            );
          })}
        </div>
      </div>

      {/* Bottom Status Panels & Dashboard Bar */}
      <div style={{
        display: 'flex',
        flexDirection: 'column',
        gap: '8px',
        zIndex: 10
      }}>
        <div style={{
          display: 'grid',
          gridTemplateColumns: '320px 1fr 1fr 180px',
          gap: '12px',
          alignItems: 'stretch'
        }}>
          {/* Commander Profile Card */}
          <div className="ra4-panel clip-bevel-sm" style={{
            padding: '10px 16px',
            display: 'flex',
            alignItems: 'center',
            gap: '14px'
          }}>
            <div style={{
              width: '46px',
              height: '46px',
              borderRadius: '4px',
              border: '1px solid #ff3333',
              background: 'rgba(50, 10, 10, 0.8)',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              fontSize: '24px',
              color: '#ff2222',
              boxShadow: '0 0 10px rgba(255,0,0,0.5)'
            }}>
              ★
            </div>
            <div style={{ flex: 1 }}>
              <div style={{ color: '#ff4444', fontSize: '15px', fontWeight: 700, letterSpacing: '1.5px' }}>
                КОМАНДИР
              </div>
              <div style={{ color: '#aaa', fontSize: '12px' }}>
                УРОВЕНЬ 25
              </div>
              {/* XP Bar */}
              <div style={{
                width: '100%',
                height: '5px',
                background: 'rgba(0,0,0,0.7)',
                borderRadius: '2px',
                marginTop: '4px',
                overflow: 'hidden',
                border: '1px solid rgba(255,50,50,0.3)'
              }}>
                <div style={{
                  width: '61%',
                  height: '100%',
                  background: 'linear-gradient(90deg, #ff2222, #ffcc00)'
                }} />
              </div>
              <div style={{ color: '#777', fontSize: '9px', marginTop: '2px', textAlign: 'right' }}>
                45 780 / 75 000
              </div>
            </div>
          </div>

          {/* News Feed Card */}
          <div className="ra4-panel clip-bevel-sm" style={{
            padding: '10px 16px',
            display: 'flex',
            flexDirection: 'column',
            justifyContent: 'space-between'
          }}>
            <div>
              <div style={{ color: '#ff3333', fontSize: '13px', fontWeight: 700, letterSpacing: '1.5px' }}>
                НОВОСТИ
              </div>
              <div style={{ color: '#ffffff', fontSize: '13px', marginTop: '2px' }}>
                {newsItems[newsIndex].title}
              </div>
              <div style={{ color: '#888', fontSize: '11px' }}>
                {newsItems[newsIndex].subtitle}
              </div>
            </div>
            <div style={{ display: 'flex', gap: '6px', marginTop: '4px' }}>
              {newsItems.map((_, i) => (
                <div
                  key={i}
                  onClick={() => setNewsIndex(i)}
                  style={{
                    width: i === newsIndex ? '16px' : '6px',
                    height: '5px',
                    borderRadius: '2px',
                    background: i === newsIndex ? '#ff2222' : 'rgba(255,255,255,0.2)',
                    cursor: 'pointer',
                    transition: 'all 0.2s'
                  }}
                />
              ))}
            </div>
          </div>

          {/* Operations Summary Card */}
          <div className="ra4-panel clip-bevel-sm" style={{
            padding: '10px 16px',
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center'
          }}>
            <div>
              <div style={{ color: '#ff3333', fontSize: '13px', fontWeight: 700, letterSpacing: '1.5px' }}>
                СВОДКА ОПЕРАЦИЙ
              </div>
              <div style={{ color: '#ddd', fontSize: '12px', maxWidth: '240px', marginTop: '2px' }}>
                Глобальная обстановка нестабильна. Будьте готовы к любому сценарию.
              </div>
            </div>
            <div style={{
              width: '90px',
              height: '42px',
              background: 'rgba(30, 10, 10, 0.6)',
              border: '1px solid rgba(255, 50, 50, 0.3)',
              borderRadius: '4px',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: '#ff4444',
              fontSize: '11px'
            }}>
              [КАРТА МИРА]
            </div>
          </div>

          {/* Faction Insignia Card */}
          <div className="ra4-panel clip-bevel-sm" style={{
            padding: '10px',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            background: 'radial-gradient(circle, rgba(80, 10, 10, 0.8) 0%, rgba(20, 5, 5, 0.95) 100%)'
          }}>
            <span style={{ fontSize: '32px', color: '#ff2222', filter: 'drop-shadow(0 0 10px rgba(255,0,0,0.8))' }}>
              ★
            </span>
          </div>
        </div>

        {/* Global Footer Meta Bar */}
        <div style={{
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center',
          padding: '6px 12px',
          color: '#777',
          fontSize: '11px',
          letterSpacing: '1.2px',
          borderTop: '1px solid rgba(255,255,255,0.1)'
        }}>
          <div style={{ display: 'flex', gap: '20px' }}>
            <span>СЕТЬ: <strong style={{ color: '#00ff66' }}>ПОДКЛЮЧЕНО</strong></span>
            <span>СЕРВИСЫ: <strong style={{ color: '#00ff66' }}>ДОСТУПНЫ</strong></span>
          </div>
          <div>
            ВЕРСИЯ 1.0.0.0 &nbsp; ★ &nbsp; © 2024 ELECTRONIC ARTS INC.
          </div>
        </div>
      </div>
    </div>
  );
};
