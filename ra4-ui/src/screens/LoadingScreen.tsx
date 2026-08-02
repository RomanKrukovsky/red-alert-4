import React from 'react';

export interface LoadingScreenProps {
  progress: number;
  mapName?: string;
}

export const LoadingScreen: React.FC<LoadingScreenProps> = ({ progress, mapName = 'Красный Рубеж' }) => {
  return (
    <div className="ra4-loading-screen theme-soviet">
      <div className="ra4-loading-content">
        
        <div style={{ marginBottom: '30px', fontSize: '64px', color: '#ff2400', filter: 'drop-shadow(0 0 20px #ff0000)' }}>
          ★
        </div>

        <div className="ra4-loading-title">
          ПОДГОТОВКА К БОЮ: {mapName.toUpperCase()}
        </div>
        
        <div className="ra4-loading-subtitle">
          ИНИЦИАЛИЗАЦИЯ ДВИЖКА И ЗАГРУЗКА РЕСУРСОВ...
        </div>

        <div className="ra4-loading-bar-container">
          <div className="ra4-loading-bar-fill" style={{ width: `${progress}%` }} />
        </div>
        
        <div className="ra4-loading-percentage">
          {Math.floor(progress)}%
        </div>

        <div className="ra4-loading-hint">
          <div className="ra4-loading-hint-title">СЕКРЕТНЫЕ МАТЕРИАЛЫ</div>
          <div>Атакуйте экстракторы ресурсов противника. Лишение врага кредитов — самый верный путь к победе в затяжной войне.</div>
        </div>
        
      </div>
    </div>
  );
};
