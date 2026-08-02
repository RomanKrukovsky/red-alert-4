import React, { useState } from 'react';

export interface StrategicMapScreenProps {
  onSelectMission: (missionId: string) => void;
  onBack: () => void;
}

export const StrategicMapScreen: React.FC<StrategicMapScreenProps> = ({ onSelectMission, onBack }) => {
  const [selectedNode, setSelectedNode] = useState('M1');

  const nodes = [
    { id: 'M1', name: 'Операция 1: Красный Рубеж', status: 'ACTIVE', pos: { x: 25, y: 35 } },
    { id: 'M2', name: 'Операция 2: Заполярный Порт', status: 'LOCKED', pos: { x: 45, y: 25 } },
    { id: 'M3', name: 'Операция 3: Волга-Один', status: 'LOCKED', pos: { x: 65, y: 45 } },
    { id: 'M4', name: 'Операция 4: Берлинский Заслон', status: 'LOCKED', pos: { x: 80, y: 65 } }
  ];

  const current = nodes.find(n => n.id === selectedNode) ?? nodes[0];

  return (
    <div style={containerStyle}>
      <div style={headerStyle}>
        <button style={backBtnStyle} onClick={onBack}>◄ НАЗАД В КАМПАНИЮ</button>
        <h2 style={{ color: '#ff4d4d', margin: 0, fontFamily: 'Orbitron, sans-serif' }}>СТРАТЕГИЧЕСКАЯ КАРТА ТЕАТРА ДЕЙСТВИЙ</h2>
        <div style={{ width: '120px' }} />
      </div>

      <div style={bodyStyle}>
        {/* Central Map Canvas Representation (SC-08) */}
        <div style={mapAreaStyle}>
          {nodes.map(node => (
            <div
              key={node.id}
              onClick={() => setSelectedNode(node.id)}
              style={{
                position: 'absolute',
                left: `${node.pos.x}%`,
                top: `${node.pos.y}%`,
                width: '32px',
                height: '32px',
                borderRadius: '50%',
                backgroundColor: node.status === 'ACTIVE' ? '#ff4d4d' : '#444',
                border: selectedNode === node.id ? '3px solid #00ffc8' : '2px solid #fff',
                boxShadow: selectedNode === node.id ? '0 0 20px #00ffc8' : 'none',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: '#fff',
                fontWeight: 'bold',
                fontSize: '12px',
                cursor: 'pointer'
              }}
            >
              {node.id}
            </div>
          ))}
        </div>

        {/* Right Info Panel (SC-08) */}
        <div style={infoPanelStyle}>
          <h3 style={{ color: '#ff4d4d', marginTop: 0, fontFamily: 'Orbitron, sans-serif' }}>{current.name}</h3>
          <div style={{ fontSize: '13px', color: '#aaa', marginBottom: '20px' }}>
            СТАТУС: <span style={{ color: current.status === 'ACTIVE' ? '#00ffc8' : '#888', fontWeight: 'bold' }}>{current.status}</span>
          </div>
          <p style={{ color: '#ccc', fontSize: '13px', lineHeight: 1.5 }}>
            Стратегический узел региона. Выполните поставленные задачи для разблокирования следующего сектора.
          </p>

          <button
            disabled={current.status !== 'ACTIVE'}
            style={{
              ...launchBtnStyle,
              backgroundColor: current.status === 'ACTIVE' ? '#ff4d4d' : '#444',
              cursor: current.status === 'ACTIVE' ? 'pointer' : 'not-allowed'
            }}
            onClick={() => onSelectMission(current.id)}
          >
            ► К БРИФИНГУ МИССИИ
          </button>
        </div>
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
  zIndex: 3750,
  fontFamily: 'Inter, system-ui, sans-serif'
};

const headerStyle: React.CSSProperties = {
  display: 'flex',
  justifyContent: 'space-between',
  alignItems: 'center',
  marginBottom: '20px'
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

const bodyStyle: React.CSSProperties = {
  flex: 1,
  display: 'grid',
  gridTemplateColumns: '1fr 340px',
  gap: '24px'
};

const mapAreaStyle: React.CSSProperties = {
  position: 'relative',
  backgroundColor: 'rgba(12, 18, 28, 0.9)',
  border: '1px solid rgba(255, 255, 255, 0.15)',
  borderRadius: '8px',
  backgroundImage: 'radial-gradient(circle at 50% 50%, rgba(0, 255, 200, 0.05) 0%, transparent 80%)'
};

const infoPanelStyle: React.CSSProperties = {
  backgroundColor: 'rgba(12, 18, 28, 0.9)',
  border: '1px solid rgba(255, 255, 255, 0.15)',
  borderRadius: '8px',
  padding: '24px',
  display: 'flex',
  flexDirection: 'column',
  justifyContent: 'space-between'
};

const launchBtnStyle: React.CSSProperties = {
  color: '#fff',
  border: 'none',
  borderRadius: '4px',
  padding: '14px 24px',
  fontSize: '15px',
  fontWeight: 'bold',
  fontFamily: 'Orbitron, sans-serif'
};
