import React, { useState } from 'react';
import { Button } from './Button.js';

export interface MainMenuScreenProps {
  onSelectOption: (option: string) => void;
}

export const MainMenuScreen: React.FC<MainMenuScreenProps> = ({ onSelectOption }) => {
  const [activeItem, setActiveItem] = useState('СХВАТКА');

  const navItems = [
    { id: 'CAMPAIGN', label: 'КАМПАНИЯ', icon: '★' },
    { id: 'MULTIPLAYER', label: 'СЕТЕВАЯ ИГРА', icon: '🌐' },
    { id: 'SKIRMISH', label: 'СХВАТКА', icon: '⚔️' },
    { id: 'EDITOR', label: 'РЕДАКТОР', icon: '🛠️' },
    { id: 'ENCYCLOPEDIA', label: 'ЭНЦИКЛОПЕДИЯ', icon: '📖' },
    { id: 'MODS', label: 'МОДИФИКАЦИИ', icon: '⚙️' },
    { id: 'SETTINGS', label: 'НАСТРОЙКИ', icon: '⚙️' },
    { id: 'EXIT', label: 'ВЫХОД', icon: '🚪' }
  ];

  const handleNavClick = (id: string, label: string) => {
    setActiveItem(label);
    onSelectOption(id);
  };

  return (
    <div className="ra4-main-menu theme-soviet">
      <div className="ra4-menu-top">
        <div className="ra4-menu-title">
          <div className="ra4-menu-cc-text">COMMAND & CONQUER™</div>
          <div className="ra4-menu-ra-text">RED ALERT 4</div>
        </div>
      </div>

      <div className="ra4-menu-content">
        <div className="ra4-menu-nav-panel">
          {navItems.map(item => (
            <Button
              key={item.id}
              variant="primary"
              className={`ra4-menu-nav-btn ${activeItem === item.label ? 'active' : ''}`}
              onClick={() => handleNavClick(item.id, item.label)}
            >
              <span style={{ opacity: 0.5, fontSize: '16px' }}>{item.icon}</span>
              {item.label}
            </Button>
          ))}
        </div>
        
        <div className="ra4-menu-center">
           <div className="ra4-menu-emblem" />
        </div>
      </div>

      <div className="ra4-menu-bottom">
        <div className="ra4-menu-bottom-panel ra4-menu-commander">
           <div className="ra4-menu-commander-avatar">★</div>
           <div className="ra4-menu-commander-info">
              <h3>КОМАНДИР</h3>
              <div style={{ color: '#fff', fontSize: '14px', marginBottom: '8px' }}>УРОВЕНЬ 25</div>
              <div style={{ width: '100%', height: '4px', background: '#330000' }}>
                 <div style={{ width: '60%', height: '100%', background: '#ff2400' }} />
              </div>
              <div style={{ fontSize: '10px', marginTop: '4px', textAlign: 'right' }} className="ra4-tabular-num">45 780 / 75 000</div>
           </div>
        </div>

        <div className="ra4-menu-bottom-panel ra4-menu-news">
          <h4>НОВОСТИ</h4>
          <div style={{ fontSize: '14px', color: '#ccc', lineHeight: '1.5' }}>
            Добро пожаловать, командир.<br/>Красная угроза возвращается.
          </div>
        </div>
        
        <div className="ra4-menu-bottom-panel ra4-menu-news">
          <h4>СВОДКА ОПЕРАЦИЙ</h4>
          <div style={{ fontSize: '14px', color: '#ccc', lineHeight: '1.5' }}>
            Глобальная обстановка нестабильна.<br/>Будьте готовы к любому сценарию.
          </div>
        </div>
      </div>

      <div className="ra4-menu-footer-strip">
        <div>СЕТЬ: <span style={{ color: '#00ff00' }}>ПОДКЛЮЧЕНО</span></div>
        <div>СЕРВИСЫ: <span style={{ color: '#00ff00' }}>ДОСТУПНЫ</span></div>
        <div>ВЕРСИЯ 1.0.0.0</div>
        <div>© 2024 ELECTRONIC ARTS INC.</div>
      </div>
    </div>
  );
};
