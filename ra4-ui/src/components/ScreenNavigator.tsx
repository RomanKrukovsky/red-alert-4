import React, { useState } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';

interface ScreenOption {
  id: string;
  name: string;
  path: string;
  screenshotNum: number;
  category: 'Меню' | 'Кампания' | 'Брифинг' | 'Бой' | 'Сетевая';
}

const SCREENS: ScreenOption[] = [
  { id: 'splash', name: '01. Заставка (Splash)', path: '/', screenshotNum: 1, category: 'Меню' },
  { id: 'main-menu', name: '02. Главное меню (War Room)', path: '/menu', screenshotNum: 2, category: 'Меню' },
  { id: 'campaign-select', name: '03. Выбор кампании (4 фракции)', path: '/campaign-select', screenshotNum: 3, category: 'Кампания' },
  { id: 'campaign-ussr', name: '04. Кампания СССР (Соколов)', path: '/campaign/ussr', screenshotNum: 4, category: 'Кампания' },
  { id: 'campaign-allies', name: '05. Кампания Альянса (Уорд)', path: '/campaign/allies', screenshotNum: 5, category: 'Кампания' },
  { id: 'campaign-ec', name: '06. Кампания Вост. Коалиции', path: '/campaign/ec', screenshotNum: 6, category: 'Кампания' },
  { id: 'campaign-chrono', name: '07. Кампания Хронолегиона', path: '/campaign/chrono', screenshotNum: 7, category: 'Кампания' },
  { id: 'strategic-map', name: '08. Тактическая карта (Евразия)', path: '/strategic-map', screenshotNum: 8, category: 'Кампания' },
  { id: 'briefing-ussr', name: '09. Брифинг «Красный рассвет»', path: '/briefing', screenshotNum: 9, category: 'Брифинг' },
  { id: 'video-comms', name: '10. Видеосвязь (Соколов vs Уорд)', path: '/video-comms', screenshotNum: 10, category: 'Брифинг' },
  { id: 'loading-kiev', name: '12/19. Загрузка миссии (Киев-86)', path: '/loading', screenshotNum: 12, category: 'Брифинг' },
  { id: 'hud-ussr-base', name: '13. HUD СССР (Строительство базы)', path: '/hud?mode=ussr-base', screenshotNum: 13, category: 'Бой' },
  { id: 'hud-allies-airfield', name: '14. HUD Альянса (Аэродром)', path: '/hud?mode=allies-airfield', screenshotNum: 14, category: 'Бой' },
  { id: 'hud-ec-base', name: '15. HUD Вост. Коалиции (База)', path: '/hud?mode=ec-base', screenshotNum: 15, category: 'Бой' },
  { id: 'hud-chrono-base', name: '16. HUD Хронолегиона (База)', path: '/hud?mode=chrono-base', screenshotNum: 16, category: 'Бой' },
  { id: 'skirmish-lobby', name: '17. Сетевое лобби (8 игроков)', path: '/skirmish', screenshotNum: 17, category: 'Сетевая' },
  { id: 'campaign-ec-detail', name: '18. Досье Вост. Коалиции (Гао)', path: '/campaign/ec?view=detail', screenshotNum: 18, category: 'Кампания' },
  { id: 'hud-ussr-tank-assault', name: '20. HUD СССР (Танковый штурм КВ-3)', path: '/hud?mode=ussr-tank-assault', screenshotNum: 20, category: 'Бой' },
  { id: 'hud-ussr-base-defense', name: '21. HUD СССР (Оборона / Тревога)', path: '/hud?mode=ussr-base-defense', screenshotNum: 21, category: 'Бой' },
  { id: 'hud-allies-naval', name: '22. HUD Альянса (Морской флот)', path: '/hud?mode=allies-naval', screenshotNum: 22, category: 'Бой' },
  { id: 'hud-allies-air-battle', name: '23. HUD Альянса (Воздушная битва)', path: '/hud?mode=allies-air-battle', screenshotNum: 23, category: 'Бой' },
  { id: 'hud-chrono-superweapon', name: '24. HUD Хроно (Супероружие)', path: '/hud?mode=chrono-superweapon', screenshotNum: 24, category: 'Бой' }
];

export const ScreenNavigator: React.FC = () => {
  const navigate = useNavigate();
  const location = useLocation();
  const [isOpen, setIsOpen] = useState(false);

  const currentPath = location.pathname + location.search;
  const currentScreen = SCREENS.find(s => s.path === currentPath) ||
    SCREENS.find(s => location.pathname === s.path.split('?')[0]) ||
    SCREENS[0];

  return (
    <div style={{
      position: 'fixed',
      top: 0,
      left: '50%',
      transform: 'translateX(-50%)',
      zIndex: 9999,
      fontFamily: "'Oswald', sans-serif"
    }}>
      {/* Toggle Button */}
      <div
        onClick={() => setIsOpen(!isOpen)}
        style={{
          background: 'linear-gradient(180deg, rgba(30,10,10,0.92) 0%, rgba(10,5,5,0.96) 100%)',
          border: '1px solid rgba(255, 60, 60, 0.6)',
          borderTop: 'none',
          padding: '4px 18px',
          borderRadius: '0 0 8px 8px',
          color: '#ffcc00',
          fontSize: '12px',
          letterSpacing: '1.5px',
          cursor: 'pointer',
          boxShadow: '0 4px 14px rgba(0,0,0,0.8), 0 0 10px rgba(255,50,50,0.4)',
          display: 'flex',
          alignItems: 'center',
          gap: '10px'
        }}
      >
        <span style={{ color: '#ff3333' }}>★ RA4 SCREEN NAVIGATOR</span>
        <span style={{ color: '#aaa' }}>|</span>
        <span style={{ color: '#ffffff' }}>{currentScreen.name}</span>
        <span style={{ fontSize: '10px', color: '#ffcc00' }}>{isOpen ? '▲ СВЕРНУТЬ' : '▼ ВЫБРАТЬ ЭКРАН (1-24)'}</span>
      </div>

      {/* Screen Selection Dropdown Modal */}
      {isOpen && (
        <div style={{
          position: 'absolute',
          top: '100%',
          left: '50%',
          transform: 'translateX(-50%)',
          width: '940px',
          maxHeight: '80vh',
          overflowY: 'auto',
          background: 'rgba(8, 6, 10, 0.98)',
          border: '1px solid #ff3333',
          boxShadow: '0 12px 40px rgba(0,0,0,0.95), 0 0 25px rgba(255,0,0,0.4)',
          borderRadius: '8px',
          padding: '16px',
          backdropFilter: 'blur(12px)',
          marginTop: '6px'
        }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '12px', borderBottom: '1px solid rgba(255,50,50,0.3)', paddingBottom: '8px' }}>
            <div style={{ color: '#ff3333', fontSize: '15px', fontWeight: 700, letterSpacing: '2px' }}>
              РЕЕСТР ВСЕХ 24 ЭКРАНОВ И БОЕВЫХ HUD RED ALERT 4
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

          <div style={{
            display: 'grid',
            gridTemplateColumns: 'repeat(3, 1fr)',
            gap: '10px'
          }}>
            {SCREENS.map(screen => {
              const isActive = (location.pathname + location.search) === screen.path;
              return (
                <div
                  key={screen.id}
                  onClick={() => {
                    navigate(screen.path);
                    setIsOpen(false);
                  }}
                  style={{
                    background: isActive
                      ? 'linear-gradient(180deg, rgba(140, 20, 20, 0.9) 0%, rgba(60, 5, 5, 0.95) 100%)'
                      : 'linear-gradient(180deg, rgba(25, 20, 25, 0.8) 0%, rgba(12, 10, 15, 0.9) 100%)',
                    border: `1px solid ${isActive ? '#ff4d4d' : 'rgba(255,255,255,0.15)'}`,
                    borderRadius: '4px',
                    padding: '8px 10px',
                    cursor: 'pointer',
                    display: 'flex',
                    flexDirection: 'column',
                    gap: '4px',
                    boxShadow: isActive ? '0 0 12px rgba(255,50,50,0.6)' : 'none',
                    transition: 'all 0.15s ease'
                  }}
                >
                  <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                    <span style={{
                      fontSize: '10px',
                      color: screen.category === 'Бой' ? '#ff8800' : (screen.category === 'Кампания' ? '#00ccff' : '#ffcc00'),
                      textTransform: 'uppercase'
                    }}>
                      [{screen.category}]
                    </span>
                    <span style={{ fontSize: '10px', color: '#888' }}>
                      СКРИНШОТ #{screen.screenshotNum}
                    </span>
                  </div>
                  <div style={{
                    color: isActive ? '#ffffff' : '#e0e0e0',
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
