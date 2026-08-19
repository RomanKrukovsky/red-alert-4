import React from 'react';
import { useGameState } from '../context/GameStateContext';
import { Outlet } from 'react-router-dom';
import { ScreenNavigator } from './ScreenNavigator';

export const Layout: React.FC = () => {
  const { currentFaction } = useGameState();

  return (
    <div className={`theme-${currentFaction}`} style={{ width: '100vw', height: '100vh', position: 'relative', overflow: 'hidden' }}>
      {/* Dev Screen Navigator for fast jumps to all 24 reference screens */}
      <ScreenNavigator />

      {/* Screen Outlet */}
      <Outlet />
    </div>
  );
};
