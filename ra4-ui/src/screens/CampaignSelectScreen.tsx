import React, { useState } from 'react';

export interface CampaignSelectScreenProps {
  onSelectFaction: (factionId: string) => void;
  onBack: () => void;
}

export const CampaignSelectScreen: React.FC<CampaignSelectScreenProps> = ({ onSelectFaction, onBack }) => {
  const [selectedFaction, setSelectedFaction] = useState('USSR');

  const factions = [
    { id: 'USSR', name: 'СССР', subtitle: 'Советский Союз', progress: '58% Завершено', missions: '14/24', color: '#ff2a4b' },
    { id: 'ALLIES', name: 'АЛЬЯНС', subtitle: 'Западные Союзники', progress: '100% Завершено', missions: '18/18', color: '#2a8bf2' },
    { id: 'COALITION', name: 'ВОСТОЧНАЯ КОАЛИЦИЯ', subtitle: 'Паназиатский Пакт', progress: '0% Завершено', missions: '0/16', color: '#26b259' },
    { id: 'CHRONO', name: 'ХРОНОЛЕГИОН', subtitle: 'Стражи Времени', progress: '0% Завершено', missions: '0/12', color: '#9933da' }
  ];

  const current = factions.find(f => f.id === selectedFaction) ?? factions[0];

  return (
    <div style={containerStyle}>
      <div style={headerStyle}>
        <button style={backBtnStyle} onClick={onBack}>◄ НАЗАД В МЕНЮ</button>
        <h2 style={{ color: '#00ffc8', margin: 0, fontFamily: 'Orbitron, sans-serif' }}>ВЫБОР ТЕАТРА ВОЕННЫХ ДЕЙСТВИЙ</h2>
        <div style={{ width: '120px' }} />
      </div>

      <div style={gridStyle}>
        {factions.map(f => (
          <div
            key={f.id}
            onClick={() => setSelectedFaction(f.id)}
            style={{
              ...cardStyle,
              borderColor: selectedFaction === f.id ? f.color : 'rgba(255, 255, 255, 0.15)',
              boxShadow: selectedFaction === f.id ? `0 0 25px ${f.color}` : 'none'
            }}
          >
            <div style={{ fontSize: '12px', color: f.color, fontWeight: 'bold' }}>{f.subtitle}</div>
            <h3 style={{ fontSize: '22px', margin: '6px 0', color: '#fff', fontFamily: 'Orbitron, sans-serif' }}>{f.name}</h3>
            <div style={{ fontSize: '13px', color: '#aaa', marginTop: '10px' }}>Прогресс: {f.progress}</div>
            <div style={{ fontSize: '12px', color: '#888' }}>Миссии: {f.missions}</div>
          </div>
        ))}
      </div>

      <div style={detailPanelStyle}>
        <h3 style={{ color: current.color, marginTop: 0, fontFamily: 'Orbitron, sans-serif' }}>{current.name} — ДЕТАЛИ КАМПАНИИ</h3>
        <p style={{ color: '#ccc', fontSize: '14px', lineHeight: 1.6 }}>
          Возглавьте армию {current.subtitle} в глобальном противостоянии. Пройдите {current.missions} тактических операций с уникальными типами войск и супероружием.
        </p>
        <button style={{ ...launchBtnStyle, backgroundColor: current.color }} onClick={() => onSelectFaction(current.id)}>
          ► НАЧАТЬ КАМПАНИЮ {current.name}
        </button>
      </div>
    </div>
  );
};

const containerStyle: React.CSSProperties = {
  position: 'absolute',
  top: 0,
  left: 0,
  width: '100%',
  height: '100%',
  backgroundColor: '#070b12',
  display: 'flex',
  flexDirection: 'column',
  padding: '30px 40px',
  boxSizing: 'border-box',
  zIndex: 3600,
  fontFamily: 'Inter, system-ui, sans-serif'
};

const headerStyle: React.CSSProperties = {
  display: 'flex',
  justifyContent: 'space-between',
  alignItems: 'center',
  marginBottom: '30px'
};

const backBtnStyle: React.CSSProperties = {
  background: 'transparent',
  border: '1px solid rgba(255, 255, 255, 0.2)',
  color: '#ccc',
  padding: '10px 18px',
  borderRadius: '4px',
  cursor: 'pointer',
  fontWeight: 'bold'
};

const gridStyle: React.CSSProperties = {
  display: 'grid',
  gridTemplateColumns: 'repeat(4, 1fr)',
  gap: '20px',
  marginBottom: '30px'
};

const cardStyle: React.CSSProperties = {
  backgroundColor: 'rgba(12, 18, 28, 0.9)',
  border: '1px solid rgba(255, 255, 255, 0.15)',
  borderRadius: '8px',
  padding: '24px',
  cursor: 'pointer',
  transition: 'all 0.2s ease'
};

const detailPanelStyle: React.CSSProperties = {
  backgroundColor: 'rgba(12, 18, 28, 0.85)',
  border: '1px solid rgba(255, 255, 255, 0.15)',
  borderRadius: '8px',
  padding: '30px',
  flex: 1,
  display: 'flex',
  flexDirection: 'column',
  justifyContent: 'space-between'
};

const launchBtnStyle: React.CSSProperties = {
  alignSelf: 'flex-start',
  color: '#fff',
  border: 'none',
  borderRadius: '4px',
  padding: '14px 28px',
  fontSize: '16px',
  fontWeight: 'bold',
  cursor: 'pointer',
  fontFamily: 'Orbitron, sans-serif'
};
