import React from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import { Button } from '../components/Button';
import { Panel } from '../components/Panel';
import { ProgressBar } from '../components/ProgressBar';
import type { Faction } from '../context/GameStateContext';

const factionBriefingData: Record<Faction, { title: string; subtitle: string; desc: string; bgSrc: string }> = {
  ussr: {
    title: 'СССР',
    subtitle: 'СЛАВА СОВЕТСКОМУ СОЮЗУ!',
    desc: 'Враг у наших границ. Империалисты и предатели стремятся задушить нашу Родину в огне и лжи. Только дисциплина, сталь и вера в дело Ленина приведут нас к окончательной победе.\n\nТоварищ, судьба мира в твоих приказах!',
    bgSrc: '/screenshots/4.png'
  },
  allies: {
    title: 'АЛЬЯНСА',
    subtitle: 'ВЕРНОСТЬ. ЕДИНСТВО. ПОБЕДА.',
    desc: 'Альянс стоит на страже свободы и процветания. Перед лицом новой угрозы мы объединим нации, технологии и волю, чтобы обеспечить мир и стабильность в неопределенном мире.',
    bgSrc: '/screenshots/5.png'
  },
  ec: {
    title: 'ВОСТОЧНАЯ КОАЛИЦИЯ',
    subtitle: 'МУДРОСТЬ И СИЛА.',
    desc: 'Дракон пробудился. Настало время вернуть наши исконные территории и доказать превосходство нашего пути.',
    bgSrc: '/screenshots/6.png'
  },
  chrono: {
    title: 'ХРОНОЛЕГИОН',
    subtitle: 'ВЛАСТЬ НАД ВРЕМЕНЕМ. ГОСПОДСТВО НАД ВСЕЛЕННОЙ.',
    desc: 'Они пришли не из этого времени. Хронолегион существует вне линейности, наблюдая за историей и вмешиваясь в неё. Их цель — не завоевание, а исправление. Те, кто стоит на их пути, будут стёрты из всех времён.',
    bgSrc: '/screenshots/7.png'
  }
};

export const FactionBriefing: React.FC = () => {
  const { faction } = useParams<{ faction: Faction }>();
  const navigate = useNavigate();
  
  const currentFaction = faction || 'ussr';
  const data = factionBriefingData[currentFaction];

  return (
    <div 
      className={`theme-${currentFaction}`}
      style={{ 
        width: '100%', 
        height: '100%', 
        position: 'relative',
        background: `url('${data.bgSrc}') no-repeat center center`,
        backgroundSize: 'cover'
      }}
    >
      {/* Gradients to hide baked-in UI on the sides */}
      <div style={{ position: 'absolute', top: 0, bottom: 0, left: 0, width: '350px', background: 'linear-gradient(90deg, var(--theme-bg-dark) 40%, transparent 100%)', zIndex: 1 }} />
      <div style={{ position: 'absolute', top: 0, bottom: 0, right: 0, width: '500px', background: 'linear-gradient(-90deg, var(--theme-bg-dark) 40%, transparent 100%)', zIndex: 1 }} />
      <div style={{ position: 'absolute', bottom: 0, left: 0, right: 0, height: '150px', background: 'linear-gradient(0deg, var(--theme-bg-dark) 20%, transparent 100%)', zIndex: 1 }} />

      <div style={{ position: 'relative', zIndex: 2, display: 'flex', width: '100%', height: '100%', padding: '40px' }}>
        
        {/* Left Sidebar (Faction Selection similar to screenshot) */}
        <div style={{ width: '250px', display: 'flex', flexDirection: 'column', gap: '10px', paddingTop: '80px' }}>
          <h3 className="text-muted" style={{ marginBottom: '10px' }}>— КАМПАНИЯ</h3>
          
          {(['ussr', 'allies', 'ec', 'chrono'] as Faction[]).map(f => (
            <Panel 
              key={f}
              variant={f === currentFaction ? 'bordered' : 'solid'}
              angled
              glow={f === currentFaction}
              onClick={() => navigate(`/campaign/${f}`)}
              style={{ cursor: 'pointer', padding: '15px', display: 'flex', alignItems: 'center', transition: 'all 0.2s' }}
            >
              <span style={{ fontWeight: 'bold' }}>{factionBriefingData[f].title}</span>
            </Panel>
          ))}
          
          <Panel variant="solid" angled style={{ marginTop: '10px', opacity: 0.5 }}>
            СЕКРЕТНЫЙ ПРОТОКОЛ<br/><small>СКОРО</small>
          </Panel>
        </div>

        {/* Center Space for Art */}
        <div style={{ flex: 1 }} />

        {/* Right Info Panel */}
        <div style={{ width: '450px', display: 'flex', flexDirection: 'column', justifyContent: 'center', gap: '30px', paddingRight: '20px' }}>
          
          <div>
            <h3 style={{ color: 'var(--theme-text-muted)', margin: 0, fontSize: '1.2rem' }}>КАМПАНИЯ</h3>
            <h1 className="glow-text" style={{ fontSize: '4rem', margin: '-10px 0 10px 0', fontFamily: 'var(--font-secondary)' }}>
              {data.title}
            </h1>
            <h4 style={{ color: 'var(--theme-primary)' }}>{data.subtitle}</h4>
          </div>

          <div style={{ color: 'var(--theme-text-muted)', lineHeight: '1.6', fontSize: '1.1rem' }}>
            {data.desc}
          </div>

          <Panel variant="solid" angled>
            <ProgressBar progress={58} label="ПРОГРЕСС КАМПАНИИ" showValue />
            <div style={{ display: 'flex', justifyContent: 'space-between', marginTop: '15px' }}>
              <span className="text-muted">МИССИЙ ЗАВЕРШЕНО</span>
              <span>14 / 24</span>
            </div>
          </Panel>

          <Panel variant="solid" angled style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
             <span className="text-muted">УРОВЕНЬ СЛОЖНОСТИ</span>
             <span className="text-primary glow-text" style={{ fontWeight: 'bold' }}>ВЕТЕРАН</span>
          </Panel>
          
          <div style={{ display: 'flex', gap: '15px', marginTop: '20px' }}>
            <Button variant="secondary" style={{ flex: 1 }}>НОВАЯ ИГРА</Button>
            <Button variant="primary" size="lg" style={{ flex: 2 }} onClick={() => navigate(`/ingame/${currentFaction}`)}>ПРОДОЛЖИТЬ</Button>
          </div>

        </div>
      </div>
      
      {/* Footer Nav */}
      <div style={{ position: 'absolute', bottom: '30px', left: '40px', zIndex: 2 }}>
        <Button variant="secondary" onClick={() => navigate('/campaign-select')}>&lt; НАЗАД</Button>
      </div>

    </div>
  );
};
