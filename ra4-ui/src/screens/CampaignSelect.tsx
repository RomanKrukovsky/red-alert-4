import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { Button } from '../components/Button';
import { Panel } from '../components/Panel';
import { useGameState, type Faction } from '../context/GameStateContext';
import { CroppedImage } from '../components/CroppedImage';

const factionData: Record<Faction, { name: string; desc: string; color: string; cropX: number; bgSrc: string }> = {
  ussr: {
    name: 'СССР',
    desc: 'СЛАВА РОДИНЕ. БУДУЩЕЕ ЗА НАМИ.\n\nВозглавьте возрождённый Советский Союз в борьбе за мировое господство. Мощь, дисциплина и стальная воля помогут сокрушить врагов революции.',
    color: '#ff1a1a',
    cropX: 250, // Approx X position in 3.png
    bgSrc: '/screenshots/4.png' // Use campaign map as general bg
  },
  allies: {
    name: 'АЛЬЯНС',
    desc: 'СВОБОДА И ПРОЦВЕТАНИЕ.\n\nАльянс стоит на страже мира. Объедините нации, технологии и волю, чтобы обеспечить стабильность в неопределенном мире.',
    color: '#0088ff',
    cropX: 630,
    bgSrc: '/screenshots/5.png'
  },
  ec: {
    name: 'ВОСТОЧНАЯ КОАЛИЦИЯ',
    desc: 'МУДРОСТЬ ДРАКОНА. СИЛА ТИГРА.\n\nДревние традиции и передовые технологии слились воедино. Империя восстает, чтобы занять свое законное место.',
    color: '#00ff66',
    cropX: 1010,
    bgSrc: '/screenshots/6.png'
  },
  chrono: {
    name: 'ХРОНОЛЕГИОН',
    desc: 'ВЛАСТЬ НАД ВРЕМЕНЕМ. ГОСПОДСТВО НАД ВСЕЛЕННОЙ.\n\nОни пришли не из этого времени. Судьба податлива тем, кто владеет временем.',
    color: '#aa00ff',
    cropX: 1390,
    bgSrc: '/screenshots/7.png'
  }
};

export const CampaignSelect: React.FC = () => {
  const navigate = useNavigate();
  const { setFaction } = useGameState();
  const [hovered, setHovered] = useState<Faction | null>(null);

  const activeFaction = hovered || 'ussr';
  const data = factionData[activeFaction];

  const handleSelect = (faction: Faction) => {
    setFaction(faction);
    navigate(`/campaign/${faction}`);
  };

  return (
    <div 
      className={`theme-${activeFaction}`}
      style={{ 
        width: '100%', 
        height: '100%', 
        display: 'flex', 
        flexDirection: 'column',
        alignItems: 'center', 
        padding: '40px',
        background: `url('${data.bgSrc}') no-repeat center center`,
        backgroundSize: 'cover',
        transition: 'background 0.5s ease',
        position: 'relative'
      }}
    >
      <div style={{ position: 'absolute', inset: 0, background: 'rgba(0,0,0,0.7)', zIndex: 1 }} />

      <div style={{ position: 'relative', zIndex: 2, width: '100%', maxWidth: '1600px', display: 'flex', flexDirection: 'column', height: '100%' }}>
        
        {/* Header */}
        <div style={{ textAlign: 'center', marginBottom: '40px' }}>
          <h1 className="glow-text" style={{ fontSize: '3.5rem', margin: 0, fontFamily: 'var(--font-secondary)' }}>ВЫБОР КАМПАНИИ</h1>
        </div>

        {/* Content Split */}
        <div style={{ display: 'flex', gap: '40px', flex: 1 }}>
          
          {/* Faction Cards */}
          <div style={{ display: 'flex', gap: '20px', flex: 2 }}>
            {(Object.keys(factionData) as Faction[]).map(f => (
              <div 
                key={f}
                onMouseEnter={() => setHovered(f)}
                onClick={() => handleSelect(f)}
                className={f === activeFaction ? 'glow-box clip-angled-all' : 'clip-angled-all'}
                style={{
                  flex: 1,
                  cursor: 'pointer',
                  border: `2px solid ${f === activeFaction ? factionData[f].color : 'var(--theme-border)'}`,
                  transition: 'all 0.3s ease',
                  transform: f === activeFaction ? 'scale(1.02)' : 'scale(1)',
                  position: 'relative',
                  overflow: 'hidden',
                  background: 'var(--theme-bg-panel)'
                }}
              >
                {/* We use CroppedImage to extract the card art from 3.png */}
                <CroppedImage 
                  src="/screenshots/3.png" 
                  x={factionData[f].cropX} 
                  y={220} 
                  width="100%" 
                  height="100%" 
                  style={{ opacity: f === activeFaction ? 1 : 0.5, transition: 'opacity 0.3s' }}
                />
                
                {/* Title Overlay */}
                <div style={{ 
                  position: 'absolute', 
                  bottom: 0, 
                  left: 0, 
                  right: 0, 
                  padding: '20px', 
                  background: 'linear-gradient(0deg, rgba(0,0,0,0.9) 0%, transparent 100%)',
                  textAlign: 'center'
                }}>
                  <h2 style={{ color: factionData[f].color, margin: 0, fontSize: '2rem' }}>{factionData[f].name}</h2>
                </div>
              </div>
            ))}
          </div>

          {/* Info Sidebar */}
          <Panel variant="solid" angled style={{ flex: 1, display: 'flex', flexDirection: 'column' }}>
            <h3 style={{ color: 'var(--theme-primary)', borderBottom: '1px solid var(--theme-border)', paddingBottom: '10px', marginBottom: '20px' }}>О ВЫБРАННОЙ КАМПАНИИ</h3>
            
            <h2 className="glow-text" style={{ fontSize: '2.5rem', marginBottom: '10px' }}>{data.name}</h2>
            
            <div style={{ color: 'var(--theme-text-muted)', lineHeight: '1.6', flex: 1, whiteSpace: 'pre-wrap' }}>
              {data.desc}
            </div>

            <div style={{ marginTop: 'auto' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '10px' }}>
                <span>СЛОЖНОСТЬ</span>
                <span className="text-primary">ВЕТЕРАН</span>
              </div>
              <Button size="lg" style={{ width: '100%' }} onClick={() => handleSelect(activeFaction)}>
                ПРОДОЛЖИТЬ КАМПАНИЮ
              </Button>
            </div>
          </Panel>
        </div>

        {/* Footer */}
        <div style={{ marginTop: '30px' }}>
          <Button variant="secondary" onClick={() => navigate('/menu')}>&lt; НАЗАД</Button>
        </div>
      </div>
    </div>
  );
};
