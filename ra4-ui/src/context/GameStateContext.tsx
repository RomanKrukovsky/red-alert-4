import React, { createContext, useContext, useState } from 'react';

export type Faction = 'ussr' | 'allies' | 'ec' | 'chrono';

interface GameState {
  currentFaction: Faction;
  setFaction: (faction: Faction) => void;
  resources: {
    credits: number;
    power: number;
    intel: number;
  };
}

const defaultState: GameState = {
  currentFaction: 'ussr',
  setFaction: () => {},
  resources: {
    credits: 15000,
    power: 100,
    intel: 50,
  }
};

const GameStateContext = createContext<GameState>(defaultState);

export const GameProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [currentFaction, setFaction] = useState<Faction>('ussr');
  const [resources] = useState(defaultState.resources);

  return (
    <GameStateContext.Provider value={{ currentFaction, setFaction, resources }}>
      {children}
    </GameStateContext.Provider>
  );
};

export const useGameState = () => useContext(GameStateContext);
