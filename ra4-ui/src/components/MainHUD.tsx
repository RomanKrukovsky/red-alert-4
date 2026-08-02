import React from 'react';
import { HUDHeader } from './HUDHeader.js';
import { Minimap } from './Minimap.js';
import { ProductionPanel } from './ProductionPanel.js';
import { CommandBar } from './CommandBar.js';
import { EVALog } from './EVALog.js';
import { useUIStore } from '../store.js';

interface MainHUDProps {
  onIssueCommand?: (cmd: any) => void;
}

export const MainHUD: React.FC<MainHUDProps> = ({ onIssueCommand }) => {
  const { theme, inputMode } = useUIStore();
  
  return (
    <div className={`ra4-hud-container theme-${theme}`}>
      {/* Top Bar */}
      <div className="ra4-hud-top-bar ra4-hud-interactive" style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <HUDHeader />
        
        {/* Input Mode Badge */}
        <div style={{
          backgroundColor: inputMode === 'DirectUnitControl' ? '#ff2a4b' : inputMode === 'Console' ? '#00ffc8' : inputMode === 'FreeCamera' ? '#e0f7fc' : 'rgba(0, 255, 200, 0.2)',
          color: inputMode === 'Console' || inputMode === 'FreeCamera' ? '#05101a' : '#ffffff',
          padding: '4px 12px',
          borderRadius: '4px',
          fontWeight: 'bold',
          fontSize: '0.85rem',
          letterSpacing: '1px',
          boxShadow: '0 0 10px rgba(0, 255, 200, 0.4)',
          border: '1px solid #00ffc8'
        }}>
          {inputMode === 'RTS' && 'РЕЖИМ: RTS [F - Прямое управление | Space+ПКМ - Свободная камера | ~ Консоль]'}
          {inputMode === 'DirectUnitControl' && 'РЕЖИМ: ПРЯМОЕ УПРАВЛЕНИЕ (WASD / LMB Стрельба) [F / Esc - Выход]'}
          {inputMode === 'FreeCamera' && 'РЕЖИМ: СВОБОДНАЯ КАМЕРА (Вращение кнопкой мыши)'}
          {inputMode === 'Console' && 'РЕЖИМ: АДМИН-КОНСОЛЬ (~ / Esc - Закрыть)'}
        </div>
      </div>

      {/* Middle Content */}
      <div className="ra4-hud-middle">
        {/* Left Side: EVA Messages */}
        <EVALog />

        {/* Right Side is empty here because Production Panel extends up from bottom */}
      </div>

      {/* Bottom Bar */}
      <div className="ra4-hud-bottom">
        <div className="ra4-hud-interactive">
           <Minimap />
        </div>
        <div className="ra4-hud-interactive" style={{ flex: 1, display: 'flex', justifyContent: 'center' }}>
           <CommandBar />
        </div>
        <div className="ra4-hud-interactive">
           <ProductionPanel onIssueCommand={onIssueCommand} />
        </div>
      </div>
    </div>
  );
};
