import React, { useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { Button } from '../components/Button';
import { Panel } from '../components/Panel';
import { useGameState } from '../context/GameStateContext';

export const MainMenu: React.FC = () => {
  const navigate = useNavigate();
  const { setFaction } = useGameState();

  // Ensure default theme is USSR for the main menu
  useEffect(() => {
    setFaction('ussr');
  }, [setFaction]);

  return (
    <div 
      style={{ 
        width: '100%', 
        height: '100%', 
        display: 'flex', 
        alignItems: 'center', 
        padding: '80px',
        background: `url('/screenshots/2.png') no-repeat center center`,
        backgroundSize: 'cover',
        position: 'relative'
      }}
    >
      {/* Dark overlay to make the custom UI pop and hide baked-in elements slightly */}
      <div style={{ position: 'absolute', top: 0, left: 0, right: 0, bottom: 0, background: 'linear-gradient(90deg, rgba(10,5,5,0.95) 0%, rgba(10,5,5,0.4) 40%, rgba(0,0,0,0.1) 100%)', zIndex: 1 }} />
      
      <div style={{ position: 'relative', zIndex: 2, display: 'flex', flexDirection: 'column', height: '100%', justifyContent: 'center' }}>
        
        {/* Title Logo Area */}
        <div style={{ marginBottom: '60px', paddingLeft: '20px' }}>
          <h2 style={{ color: '#ffb700', fontSize: '1.2rem', letterSpacing: '4px', margin: 0, fontFamily: 'var(--font-primary)' }}>COMMAND & CONQUER™</h2>
          <h1 className="glow-text" style={{ color: 'var(--theme-primary)', fontSize: '5rem', margin: 0, lineHeight: 1, fontFamily: 'var(--font-secondary)', fontWeight: 700 }}>RED ALERT 4</h1>
        </div>

        {/* Navigation Menu */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '12px', width: '350px' }}>
          <Button variant="primary" size="lg" onClick={() => navigate('/campaign-select')} style={{ justifyContent: 'flex-start', paddingLeft: '30px' }}>
            <span style={{ display: 'inline-block', width: '30px' }}>★</span> КАМПАНИЯ
          </Button>
          <Button variant="secondary" size="lg" style={{ justifyContent: 'flex-start', paddingLeft: '30px' }}>
            <span style={{ display: 'inline-block', width: '30px' }}>🌐</span> СЕТЕВАЯ ИГРА
          </Button>
          <Button variant="secondary" size="lg" onClick={() => navigate('/skirmish')} style={{ justifyContent: 'flex-start', paddingLeft: '30px' }}>
            <span style={{ display: 'inline-block', width: '30px' }}>⚔️</span> СХВАТКА
          </Button>
          <Button variant="secondary" size="lg" style={{ justifyContent: 'flex-start', paddingLeft: '30px' }}>
            <span style={{ display: 'inline-block', width: '30px' }}>🛠</span> РЕДАКТОР
          </Button>
          <Button variant="secondary" size="lg" style={{ justifyContent: 'flex-start', paddingLeft: '30px' }}>
            <span style={{ display: 'inline-block', width: '30px' }}>📖</span> ЭНЦИКЛОПЕДИЯ
          </Button>
          <Button variant="secondary" size="lg" style={{ justifyContent: 'flex-start', paddingLeft: '30px' }}>
            <span style={{ display: 'inline-block', width: '30px' }}>⚙️</span> НАСТРОЙКИ
          </Button>
          <Button variant="secondary" size="lg" style={{ justifyContent: 'flex-start', paddingLeft: '30px' }}>
            <span style={{ display: 'inline-block', width: '30px' }}>🚪</span> ВЫХОД
          </Button>
        </div>

      </div>
      
      {/* Footer Info Panel */}
      <div style={{ position: 'absolute', bottom: '40px', left: '80px', right: '80px', zIndex: 2, display: 'flex', gap: '20px' }}>
        <Panel variant="bordered" style={{ flex: 1, display: 'flex', alignItems: 'center', gap: '20px' }}>
           <div>
             <div style={{ color: 'var(--theme-primary)', fontWeight: 'bold' }}>КОМАНДИР</div>
             <div style={{ color: 'var(--theme-text-muted)', fontSize: '0.8rem' }}>УРОВЕНЬ 25</div>
           </div>
        </Panel>
        <Panel variant="bordered" style={{ flex: 2 }}>
          <div style={{ color: 'var(--theme-primary)', fontWeight: 'bold', marginBottom: '5px' }}>НОВОСТИ</div>
          <div style={{ color: 'var(--theme-text-muted)', fontSize: '0.9rem' }}>Добро пожаловать, командир. Красная угроза возвращается.</div>
        </Panel>
      </div>

    </div>
  );
};
