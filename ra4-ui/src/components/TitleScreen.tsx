import React, { useEffect } from 'react';

export interface TitleScreenProps {
  onEnter: () => void;
}

export const TitleScreen: React.FC<TitleScreenProps> = ({ onEnter }) => {
  useEffect(() => {
    const handleKeyDown = () => onEnter();
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [onEnter]);

  return (
    <div className="ra4-title-screen theme-soviet" onClick={onEnter}>
      <div className="ra4-title-particles" />
      <div className="ra4-title-content">
        <div className="ra4-title-star" />
        <div className="ra4-title-cc">COMMAND & CONQUER™</div>
        <h1 className="ra4-title-main">RED ALERT 4</h1>
      </div>
      <div className="ra4-title-prompt">НАЖМИТЕ ЛЮБУЮ КЛАВИШУ</div>
    </div>
  );
};
