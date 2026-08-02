import React from 'react';
import { useUIStore } from '../store.js';
import { OFFICIAL_FACTIONS } from '@ra4/content-runtime';
import { Button } from './Button.js';

export const HUDHeader: React.FC = () => {
  const snapshot = useUIStore((s) => s.snapshot);
  const playerIdx = useUIStore((s) => s.activePlayerIndex);
  const toggleMenu = useUIStore((s) => s.toggleMenu);

  const playerState = snapshot?.players[playerIdx] ?? {
    credits: 10000,
    powerProduced: 100,
    powerConsumed: 0,
    powerLow: false,
    commandCapUsed: 0,
    commandCapMax: 50,
    factionResource: 0
  };

  const factionSpec = OFFICIAL_FACTIONS[playerIdx % 4];

  return (
    <div className="ra4-frame" style={{
      width: '100%',
      height: '60px',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'space-between',
      padding: '0 30px',
      borderTop: 'none',
      borderLeft: 'none',
      borderRight: 'none',
      clipPath: 'polygon(0 0, 100% 0, 100% 100%, 20px 100%, 0 calc(100% - 20px))' // angled bottom-left
    }}>
      {/* Left: Faction Title & Resource */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
        <div style={{ fontSize: '20px', fontWeight: 900, letterSpacing: '2px', color: 'var(--faction-secondary)' }}>
          {factionSpec.name.toUpperCase()}
        </div>
      </div>

      {/* Center: Core Resources */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '40px' }}>
        {/* Credits */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
          <span style={{ fontSize: '24px' }}>🪙</span>
          <span className="ra4-tabular-num" style={{ fontSize: '24px', fontWeight: 900, color: '#ffd700', textShadow: '0 0 10px rgba(255, 215, 0, 0.5)' }}>
            {playerState.credits.toLocaleString()}
          </span>
        </div>

        {/* Power Grid */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
          <span style={{ fontSize: '24px', color: playerState.powerLow ? '#ff0000' : '#00ff00' }}>⚡</span>
          <div style={{ display: 'flex', flexDirection: 'column' }}>
            <span className="ra4-tabular-num" style={{ fontSize: '18px', fontWeight: 900, color: playerState.powerLow ? '#ff4d4d' : '#4dff88' }}>
              {playerState.powerConsumed} / {playerState.powerProduced}
            </span>
          </div>
        </div>
      </div>

      {/* Right: Timer & Menu */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
        <Button variant="primary" onClick={toggleMenu} style={{ padding: '10px 30px', fontSize: '14px' }}>
          МЕНЮ (ESC)
        </Button>
      </div>
    </div>
  );
};
