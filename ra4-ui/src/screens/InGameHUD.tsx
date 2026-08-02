import React, { useState } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { Button } from '../components/Button';
import { Panel } from '../components/Panel';
import { ProgressBar } from '../components/ProgressBar';
import { useGameState, type Faction } from '../context/GameStateContext';
import { CroppedImage } from '../components/CroppedImage';

// Mock data for build menu
const buildTabs = ['СТРОЕНИЯ', 'ПЕХОТА', 'ТЕХНИКА', 'АВИАЦИЯ'];
const buildItems = [
  { id: 1, name: 'ЭНЕРГОСТАНЦИЯ', cost: 800, time: '00:10' },
  { id: 2, name: 'БАРАКИ', cost: 600, time: '00:08' },
  { id: 3, name: 'ВОЕННЫЙ ЗАВОД', cost: 2000, time: '00:25' },
  { id: 4, name: 'РАДАР', cost: 1500, time: '00:15' },
  { id: 5, name: 'ПВО', cost: 900, time: '00:12' },
  { id: 6, name: 'СТЕНА', cost: 300, time: '00:02' },
];

export const InGameHUD: React.FC = () => {
  const { faction } = useParams<{ faction: Faction }>();
  const navigate = useNavigate();
  const { resources } = useGameState();
  const currentFaction = faction || 'ussr';

  const [activeTab, setActiveTab] = useState(buildTabs[0]);

  // Background map image based on faction
  const mapBg = {
    ussr: '/screenshots/13.png',
    allies: '/screenshots/14.png',
    ec: '/screenshots/15.png',
    chrono: '/screenshots/16.png'
  }[currentFaction];

  return (
    <div 
      className={`theme-${currentFaction}`}
      style={{ 
        width: '100%', 
        height: '100%', 
        position: 'relative',
        background: `url('${mapBg}') no-repeat center center`,
        backgroundSize: 'cover'
      }}
    >
      {/* Top Resource Bar */}
      <div style={{ position: 'absolute', top: '10px', right: '20px', display: 'flex', gap: '20px', background: 'var(--theme-bg-panel)', padding: '5px 20px', borderBottomLeftRadius: '10px', borderBottomRightRadius: '10px', border: '1px solid var(--theme-border)', borderTop: 'none', zIndex: 10 }}>
         <div style={{ color: '#ffcc00' }}>💰 {resources.credits}</div>
         <div style={{ color: '#00ccff' }}>⚡ {resources.power}</div>
         <div style={{ color: '#cc00ff' }}>🧠 {resources.intel}</div>
      </div>

      {/* Top Left Menu Button */}
      <div style={{ position: 'absolute', top: '10px', left: '20px', zIndex: 10 }}>
        <Button variant="secondary" size="sm" onClick={() => navigate('/menu')}>МЕНЮ</Button>
      </div>

      {/* Left Info Panel (e.g. Selected Building) */}
      <div style={{ position: 'absolute', bottom: '20px', left: '20px', zIndex: 10, display: 'flex', gap: '20px' }}>
        <Panel variant="solid" angled style={{ width: '350px' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: '15px' }}>
            <div>
              <h3 style={{ margin: 0, color: 'var(--theme-primary)' }}>ГЛАВНЫЙ ШТАБ</h3>
              <div style={{ fontSize: '0.8rem', color: 'var(--theme-text-muted)' }}>Здание управления</div>
            </div>
            <div style={{ fontSize: '1.5rem' }}>★</div>
          </div>
          
          <ProgressBar progress={100} color="#00ff00" label="5000 / 5000" />
          
          <p style={{ fontSize: '0.9rem', color: 'var(--theme-text-muted)', marginTop: '15px' }}>
            Ключевое здание базы. Разблокирует постройку всех основных зданий и юнитов.
          </p>
        </Panel>

        {/* Production Queue Preview */}
        <Panel variant="solid" angled style={{ width: '300px' }}>
          <h4 style={{ margin: '0 0 10px 0', borderBottom: '1px solid var(--theme-border)', paddingBottom: '5px' }}>ОЧЕРЕДЬ</h4>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '10px' }}>
             <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <span className="text-primary">1x ТАНК</span>
                <span className="text-muted">00:12</span>
             </div>
             <ProgressBar progress={60} />
          </div>
        </Panel>
      </div>

      {/* Bottom Command Bar */}
      <div style={{ position: 'absolute', bottom: '20px', left: '50%', transform: 'translateX(-50%)', zIndex: 10, display: 'flex', gap: '5px' }}>
        {['Атака', 'Оборона', 'Стоп', 'Охрана', 'Патруль'].map((cmd, i) => (
          <Button key={i} variant="secondary" size="sm" style={{ width: '60px', height: '40px', fontSize: '0.7rem' }}>
            {cmd}
          </Button>
        ))}
      </div>

      {/* Right Sidebar */}
      <div style={{ position: 'absolute', top: 0, right: 0, bottom: 0, width: '320px', background: 'var(--theme-bg-panel)', borderLeft: '2px solid var(--theme-border)', zIndex: 10, display: 'flex', flexDirection: 'column' }}>
        
        {/* Minimap Area */}
        <div style={{ height: '240px', padding: '10px' }}>
           <Panel variant="bordered" angled style={{ height: '100%', padding: 0, overflow: 'hidden', position: 'relative' }}>
              <div style={{ position: 'absolute', inset: 0, background: 'radial-gradient(circle, transparent 20%, rgba(0,0,0,0.8) 100%)', zIndex: 2, pointerEvents: 'none' }} />
              {/* Extracting minimap portion from current screen (rough guess for right-top area) */}
              <CroppedImage 
                src={mapBg} 
                x={1500} y={50} width="100%" height="100%" 
                style={{ zIndex: 1 }}
              />
           </Panel>
        </div>

        {/* Action Tabs under minimap */}
        <div style={{ display: 'flex', padding: '0 10px', gap: '5px', marginBottom: '10px' }}>
           <Button variant="secondary" size="sm" style={{ flex: 1 }}>⚙️</Button>
           <Button variant="secondary" size="sm" style={{ flex: 1 }}>💰</Button>
           <Button variant="secondary" size="sm" style={{ flex: 1 }}>🛡️</Button>
        </div>

        {/* Build Categories */}
        <div style={{ display: 'flex', padding: '0 10px', borderBottom: '1px solid var(--theme-border)' }}>
          {buildTabs.map(tab => (
            <div 
              key={tab} 
              onClick={() => setActiveTab(tab)}
              style={{ 
                flex: 1, 
                textAlign: 'center', 
                padding: '10px 5px', 
                fontSize: '0.8rem', 
                cursor: 'pointer',
                color: activeTab === tab ? 'var(--theme-primary)' : 'var(--theme-text-muted)',
                borderBottom: activeTab === tab ? '2px solid var(--theme-primary)' : '2px solid transparent',
                transition: 'all 0.2s'
              }}
            >
              {tab}
            </div>
          ))}
        </div>

        {/* Build Grid */}
        <div style={{ flex: 1, padding: '10px', overflowY: 'auto', display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '10px', alignContent: 'start' }}>
          {buildItems.map(item => (
            <div 
              key={item.id} 
              className="glow-box clip-angled-tl-br"
              style={{ 
                background: 'rgba(0,0,0,0.6)', 
                border: '1px solid var(--theme-border)', 
                height: '80px', 
                display: 'flex', 
                flexDirection: 'column', 
                justifyContent: 'flex-end', 
                padding: '5px',
                cursor: 'pointer',
                position: 'relative'
              }}
            >
              <div style={{ position: 'absolute', top: '5px', right: '5px', fontSize: '0.7rem', color: '#ffcc00' }}>{item.cost}</div>
              <div style={{ fontSize: '0.7rem', fontWeight: 'bold' }}>{item.name}</div>
            </div>
          ))}
        </div>

      </div>
    </div>
  );
};
