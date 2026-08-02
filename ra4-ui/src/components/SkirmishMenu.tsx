import React from 'react';
import { useUIStore } from '../store.js';
import { MatchState } from '@ra4/shared-types';

export interface SkirmishMenuProps {
  onStartMatch: () => void;
  onRestartMatch: () => void;
  onOpenGallery?: () => void;
}

export const SkirmishMenu: React.FC<SkirmishMenuProps> = ({ onStartMatch, onRestartMatch, onOpenGallery }) => {
  const snapshot = useUIStore((s) => s.snapshot);
  const activePlayerIndex = useUIStore((s) => s.activePlayerIndex);

  if (!snapshot) {
    return (
      <div style={overlayStyle}>
        <div style={panelStyle}>
          <h1 style={{ color: '#00ffc8', fontSize: '32px', marginBottom: '10px' }}>RED ALERT 4: BROWSER RTS</h1>
          <p style={{ color: '#aaa', marginBottom: '30px' }}>Режим «Схватка» — СССР vs Альянс (ИИ)</p>
          <div style={{ display: 'flex', gap: '15px', justifyContent: 'center' }}>
            <button style={btnStyle} onClick={onStartMatch}>
              ► НАЧАТЬ МАТЧ
            </button>
            {onOpenGallery && (
              <button style={{ ...btnStyle, backgroundColor: '#3b82f6', color: '#fff' }} onClick={onOpenGallery}>
                🔍 3D ASSET GALLERY
              </button>
            )}
          </div>
        </div>
      </div>
    );
  }

  // Check Match Finish (Victory / Defeat)
  const player = snapshot.players[activePlayerIndex];
  const enemyPlayer = snapshot.players.find((_, idx) => idx !== activePlayerIndex);

  if (player && enemyPlayer) {
    if (!enemyPlayer.hasHQ && snapshot.entities.filter(e => e.playerIndex !== activePlayerIndex && e.isBuilding).length === 0) {
      return (
        <div style={overlayStyle}>
          <div style={{ ...panelStyle, border: '2px solid #00ffc8' }}>
            <h1 style={{ color: '#00ffc8', fontSize: '36px', marginBottom: '10px' }}>🏆 ПОБЕДА!</h1>
            <p style={{ color: '#fff', marginBottom: '20px' }}>База ИИ-противника полностью уничтожена.</p>
            <button style={btnStyle} onClick={onRestartMatch}>
              ↻ СЫГРАТЬ ЕЩЁ РАЗ
            </button>
          </div>
        </div>
      );
    } else if (!player.hasHQ && snapshot.entities.filter(e => e.playerIndex === activePlayerIndex && e.isBuilding).length === 0) {
      return (
        <div style={overlayStyle}>
          <div style={{ ...panelStyle, border: '2px solid #ff4444' }}>
            <h1 style={{ color: '#ff4444', fontSize: '36px', marginBottom: '10px' }}>💀 ПОРАЖЕНИЕ</h1>
            <p style={{ color: '#fff', marginBottom: '20px' }}>Ваша штаб-квартира и производственные мощности уничтожены.</p>
            <button style={btnStyle} onClick={onRestartMatch}>
              ↻ НАЧАТЬ ЗАНОВО
            </button>
          </div>
        </div>
      );
    }
  }

  return null;
};

const overlayStyle: React.CSSProperties = {
  position: 'absolute',
  top: 0,
  left: 0,
  width: '100%',
  height: '100%',
  backgroundColor: 'rgba(5, 8, 15, 0.85)',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  zIndex: 2000
};

const panelStyle: React.CSSProperties = {
  backgroundColor: '#0c101a',
  border: '1px solid #1e293b',
  borderRadius: '8px',
  padding: '40px 60px',
  textAlign: 'center',
  boxShadow: '0 0 30px rgba(0, 255, 200, 0.2)'
};

const btnStyle: React.CSSProperties = {
  backgroundColor: '#00ffc8',
  color: '#05080f',
  border: 'none',
  borderRadius: '4px',
  padding: '14px 28px',
  fontSize: '18px',
  fontWeight: 'bold',
  cursor: 'pointer',
  transition: 'transform 0.1s ease'
};
