import React from 'react';
import { useGameState } from '../context/GameStateContext';
import { Outlet } from 'react-router-dom';

export const Layout: React.FC = () => {
  const { currentFaction } = useGameState();

  return (
    <div className={`theme-${currentFaction}`} style={{ width: '100%', height: '100%' }}>
      {/* Background layer with a subtle grid or texture could go here */}
      <Outlet />
    </div>
  );
};
