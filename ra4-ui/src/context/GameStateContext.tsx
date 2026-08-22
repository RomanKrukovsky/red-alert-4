import React, { createContext, useContext, useState } from 'react';

export type Faction = 'eurasian' | 'atlantic' | 'eastern' | 'pacific' | 'independent';

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
  currentFaction: 'eurasian',
  setFaction: () => {},
  resources: {
    credits: 23450,
    power: 17820,
    intel: 9680,
  }
};

const GameStateContext = createContext<GameState>(defaultState);

export const GameProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [currentFaction, setFaction] = useState<Faction>('eurasian');
  const [resources] = useState(defaultState.resources);

  return (
    <GameStateContext.Provider value={{ currentFaction, setFaction, resources }}>
      {children}
    </GameStateContext.Provider>
  );
};

// eslint-disable-next-line react-refresh/only-export-components, react/only-export-components
export const useGameState = () => useContext(GameStateContext);
