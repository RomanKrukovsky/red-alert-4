import React from 'react';
import { useNavigate } from 'react-router-dom';

export const StartScreen: React.FC = () => {
  const navigate = useNavigate();
  return (
    <div style={{ width: '100%', height: '100%', display: 'flex', alignItems: 'center', justifyContent: 'center', cursor: 'pointer' }} onClick={() => navigate('/menu')}>
      <h1 className="glow-text" style={{ fontSize: '4rem', color: 'red' }}>RED ALERT 4</h1>
      <p style={{ position: 'absolute', bottom: '10%', opacity: 0.5 }}>CLICK TO START</p>
    </div>
  );
};
