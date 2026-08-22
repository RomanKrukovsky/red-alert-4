import React, { useState } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';

interface ScreenOption {
  id: string;
  name: string;
  path: string;
  screenshot: string;
  category: 'Меню' | 'Кампания' | 'Брифинг' | 'Бой' | 'Сетевая';
}

const SCREENS: ScreenOption[] = [
  { id: 'title', name: '01. Заставка (SCARLET HORIZON)', path: '/', screenshot: '01', category: 'Меню' },
  { id: 'main-menu', name: '02. Главное меню (Командный центр)', path: '/menu', screenshot: '02', category: 'Меню' },
  { id: 'campaign-select', name: '03-06. Выбор кампании (5 блоков)', path: '/campaign-select', screenshot: '03-06', category: 'Кампания' },
  { id: 'campaign-eurasian', name: '03. Кампания Евразийского пакта (Россия)', path: '/campaign/eurasian', screenshot: '03', category: 'Кампания' },
  { id: 'campaign-atlantic', name: '04. Кампания Атлантического альянса (США)', path: '/campaign/atlantic', screenshot: '04', category: 'Кампания' },
  { id: 'campaign-eastern', name: '05. Кампания Восточной коалиции (Китай)', path: '/campaign/eastern', screenshot: '05', category: 'Кампания' },
  { id: 'campaign-pacific', name: '06. Кампания Тихоокеанского пакта (Япония)', path: '/campaign/pacific', screenshot: '06', category: 'Кампания' },
  { id: 'strategic-map', name: '07. Карта кампании (Тихий релей)', path: '/strategic-map', screenshot: '07', category: 'Кампания' },
  { id: 'briefing', name: '08. Брифинг операции', path: '/briefing', screenshot: '08', category: 'Брифинг' },
  { id: 'video-comms', name: '09. Защищённый канал связи', path: '/video-comms', screenshot: '09', category: 'Брифинг' },
  { id: 'loading', name: '10. Загрузка миссии', path: '/loading', screenshot: '10', category: 'Брифинг' },
  { id: 'skirmish-lobby', name: '11. Сетевое лобби (8 игроков)', path: '/skirmish', screenshot: '11', category: 'Сетевая' },
  { id: 'hud-eurasian-ground', name: '12. HUD Пакта (Прорыв обороны)', path: '/hud?mode=eurasian-ground', screenshot: '12', category: 'Бой' },
  { id: 'hud-atlantic-naval', name: '13. HUD Альянса (Морская операция)', path: '/hud?mode=atlantic-naval', screenshot: '13', category: 'Бой' },
  { id: 'hud-eastern-base', name: '14. HUD Коалиции (Умный автозавод)', path: '/hud?mode=eastern-base', screenshot: '14', category: 'Бой' },
  { id: 'hud-pacific-air', name: '15. HUD Пакта (Воздушный бой)', path: '/hud?mode=pacific-air', screenshot: '15', category: 'Бой' },
  { id: 'hud-independent-iran', name: '16. HUD Держав (Иран • Сатурационный удар)', path: '/hud?mode=independent-iran', screenshot: '16', category: 'Бой' },
  { id: 'hud-eurasian-base', name: '17. HUD Пакта (База «Белый шум»)', path: '/hud?mode=eurasian-base', screenshot: '17', category: 'Бой' },
  { id: 'hud-pacific-base', name: '18. HUD Пакта (База «Айланд»)', path: '/hud?mode=pacific-base', screenshot: '18', category: 'Бой' },
  { id: 'campaign-independent', name: '19. Кампания Независимых держав (Иран)', path: '/campaign/independent', screenshot: '19', category: 'Кампания' }
];

export const ScreenNavigator: React.FC = () => {
  const navigate = useNavigate();
  const location = useLocation();
  const [isOpen, setIsOpen] = useState(false);

  const currentPath = location.pathname + location.search;
  const currentScreen =
    SCREENS.find(s => s.path === currentPath) ||
    SCREENS.find(s => location.pathname === s.path.split('?')[0]) ||
    SCREENS[0];

  return (
    <div style={{
      position: 'fixed',
      top: 0,
      left: '50%',
      transform: 'translateX(-50%)',
      zIndex: 9999,
      fontFamily: "'Jura', sans-serif"
    }}>
      {/* Toggle Button */}
      <div
        onClick={() => setIsOpen(!isOpen)}
        style={{
          background: 'linear-gradient(180deg, rgba(14,10,22,0.92) 0%, rgba(6,5,10,0.96) 100%)',
          border: '1px solid rgba(176,108,255,0.6)',
          borderTop: 'none',
          padding: '4px 18px',
          borderRadius: '0 0 8px 8px',
          color: '#d8b4ff',
          fontSize: '12px',
          letterSpacing: '1.5px',
          cursor: 'pointer',
          boxShadow: '0 4px 14px rgba(0,0,0,0.85), 0 0 10px rgba(176,108,255,0.35)',
          display: 'flex',
          alignItems: 'center',
          gap: '10px'
        }}
      >
        <span style={{ color: '#b06cff' }}>❖ SH SCREEN NAVIGATOR</span>
        <span style={{ color: '#555c68' }}>|</span>
        <span style={{ color: '#ffffff' }}>{currentScreen.name}</span>
        <span style={{ fontSize: '10px', color: '#d8b4ff' }}>{isOpen ? '▲ СВЕРНУТЬ' : '▼ ВЫБРАТЬ ЭКРАН (01-19)'}</span>
      </div>

      {/* Screen Selection Dropdown Modal */}
      {isOpen && (
        <div style={{
          position: 'absolute',
          top: '100%',
          left: '50%',
          transform: 'translateX(-50%)',
          width: '960px',
          maxHeight: '80vh',
          overflowY: 'auto',
          background: 'rgba(8, 6, 12, 0.98)',
          border: '1px solid #b06cff',
          boxShadow: '0 12px 40px rgba(0,0,0,0.95), 0 0 25px rgba(176,108,255,0.35)',
          borderRadius: '8px',
          padding: '16px',
          backdropFilter: 'blur(12px)',
          marginTop: '6px'
        }}>
          <div style={{
            display: 'flex',
            justifyContent: 'space-between',
            alignItems: 'center',
            marginBottom: '12px',
            borderBottom: '1px solid rgba(176,108,255,0.3)',
            paddingBottom: '8px'
          }}>
            <div style={{ color: '#d8b4ff', fontSize: '15px', fontWeight: 700, letterSpacing: '2px' }}>
              РЕЕСТР ЭКРАНОВ SCARLET HORIZON (REMASTER)
            </div>
            <button
              onClick={() => setIsOpen(false)}
              style={{
                background: 'transparent',
                border: '1px solid #666',
                color: '#aaa',
                cursor: 'pointer',
                padding: '2px 8px',
                borderRadius: '4px'
              }}
            >
              ✕ ЗАКРЫТЬ
            </button>
          </div>

          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '10px' }}>
            {SCREENS.map(screen => {
              const isActive = currentPath === screen.path;
              return (
                <div
                  key={screen.id}
                  onClick={() => {
                    navigate(screen.path);
                    setIsOpen(false);
                  }}
                  style={{
                    background: isActive
                      ? 'linear-gradient(180deg, rgba(106,63,160,0.9) 0%, rgba(30,18,50,0.95) 100%)'
                      : 'linear-gradient(180deg, rgba(25, 20, 30, 0.8) 0%, rgba(12, 10, 16, 0.9) 100%)',
                    border: `1px solid ${isActive ? '#b06cff' : 'rgba(255,255,255,0.15)'}`,
                    borderRadius: '4px',
                    padding: '8px 10px',
                    cursor: 'pointer',
                    display: 'flex',
                    flexDirection: 'column',
                    gap: '4px',
                    boxShadow: isActive ? '0 0 12px rgba(176,108,255,0.55)' : 'none',
                    transition: 'all 0.15s ease'
                  }}
                >
                  <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                    <span style={{
                      fontSize: '10px',
                      color:
                        screen.category === 'Бой' ? '#ff8a75' :
                        screen.category === 'Кампания' ? '#7fb2ff' : '#ffd76a',
                      textTransform: 'uppercase'
                    }}>
                      [{screen.category}]
                    </span>
                    <span style={{ fontSize: '10px', color: '#888' }}>
                      РЕФЕРЕНС №{screen.screenshot}
                    </span>
                  </div>
                  <div style={{
                    color: isActive ? '#ffffff' : '#e0e0e6',
                    fontSize: '13px',
                    fontWeight: 600,
                    whiteSpace: 'nowrap',
                    overflow: 'hidden',
                    textOverflow: 'ellipsis'
                  }}>
                    {screen.name}
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      )}
    </div>
  );
};
