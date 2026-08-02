import React from 'react';

export interface MissionBriefingScreenProps {
  onContinue: () => void;
  onBack: () => void;
}

export const MissionBriefingScreen: React.FC<MissionBriefingScreenProps> = ({ onContinue, onBack }) => {
  return (
    <div style={containerStyle}>
      <div style={headerStyle}>
        <button style={backBtnStyle} onClick={onBack}>◄ НАЗАДК КАРТЕ</button>
        <h2 style={{ color: '#ff4d4d', margin: 0, fontFamily: 'Orbitron, sans-serif' }}>ТАКТИЧЕСКИЙ БРИФИНГ И ВИДЕОСВЯЗЬ</h2>
        <div style={{ width: '120px' }} />
      </div>

      <div style={bodyStyle}>
        {/* Comms Video Feed (SC-10) */}
        <div style={videoBoxStyle}>
          <div style={videoHeaderStyle}>🔴 ПРЯМОЙ ЭФИР — КАНАЛ СВЯЗИ ШТАБА</div>
          <div style={screenPlaceholderStyle}>
            <div style={{ fontSize: '48px' }}>🎖️</div>
            <div style={{ fontSize: '16px', fontWeight: 'bold', color: '#fff', marginTop: '10px' }}>ГЕНЕРАЛ-МАЙОР ВОЛКОВ</div>
            <div style={{ fontSize: '12px', color: '#ff4d4d' }}>ВЧ-83921 КОМАНДНЫЙ ПУНКТ</div>
          </div>
          <div style={subtitlesStyle}>
            «Товарищ командир, противник развернул авангард бронетехники у рудных полей. Ваша задача — возвести Тяжёлый завод и нанести встречный удар!»
          </div>
        </div>

        {/* Objectives & Map Intel (SC-09) */}
        <div style={intelBoxStyle}>
          <div style={panelTitleStyle}>ЦЕЛИ ОПЕРАЦИИ «КРАСНЫЙ РУБЕЖ»</div>
          <div style={objItemStyle}>
            <span style={{ color: '#ff4d4d', fontWeight: 'bold' }}>[ОСНОВНАЯ]</span> Построить Рудный комбинат и Тяжёлый завод.
          </div>
          <div style={objItemStyle}>
            <span style={{ color: '#ff4d4d', fontWeight: 'bold' }}>[ОСНОВНАЯ]</span> Уничтожить главную базу ИИ-соперника.
          </div>
          <div style={objItemStyle}>
            <span style={{ color: '#ffd700', fontWeight: 'bold' }}>[ДОПОЛНИТЕЛЬНАЯ]</span> Сохранить начальный комбайн ГРМ-8 «Богатырь».
          </div>
        </div>
      </div>

      <div style={footerStyle}>
        <button style={continueBtnStyle} onClick={onContinue}>
          ► НАЧАТЬ ВЫПОЛНЕНИЕ МИССИИ
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
  zIndex: 3800,
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
  gridTemplateColumns: '1fr 400px',
  gap: '24px'
};

const videoBoxStyle: React.CSSProperties = {
  backgroundColor: 'rgba(12, 18, 28, 0.9)',
  border: '1px solid rgba(255, 42, 75, 0.4)',
  borderRadius: '8px',
  padding: '20px',
  display: 'flex',
  flexDirection: 'column'
};

const videoHeaderStyle: React.CSSProperties = {
  fontSize: '12px',
  fontWeight: 'bold',
  color: '#ff4d4d',
  marginBottom: '15px'
};

const screenPlaceholderStyle: React.CSSProperties = {
  flex: 1,
  backgroundColor: 'rgba(0, 0, 0, 0.6)',
  border: '1px solid rgba(255, 255, 255, 0.1)',
  borderRadius: '6px',
  display: 'flex',
  flexDirection: 'column',
  alignItems: 'center',
  justifyContent: 'center'
};

const subtitlesStyle: React.CSSProperties = {
  marginTop: '15px',
  padding: '14px',
  backgroundColor: 'rgba(0, 0, 0, 0.4)',
  borderLeft: '4px solid #ff4d4d',
  fontSize: '13px',
  color: '#fff',
  fontStyle: 'italic'
};

const intelBoxStyle: React.CSSProperties = {
  backgroundColor: 'rgba(12, 18, 28, 0.9)',
  border: '1px solid rgba(255, 255, 255, 0.15)',
  borderRadius: '8px',
  padding: '20px',
  display: 'flex',
  flexDirection: 'column',
  gap: '12px'
};

const panelTitleStyle: React.CSSProperties = {
  fontSize: '12px',
  fontWeight: 'bold',
  letterSpacing: '2px',
  color: '#888',
  marginBottom: '10px'
};

const objItemStyle: React.CSSProperties = {
  padding: '12px',
  backgroundColor: 'rgba(255, 255, 255, 0.03)',
  border: '1px solid rgba(255, 255, 255, 0.1)',
  borderRadius: '4px',
  fontSize: '13px',
  color: '#fff'
};

const footerStyle: React.CSSProperties = {
  marginTop: '20px',
  display: 'flex',
  justifyContent: 'flex-end'
};

const continueBtnStyle: React.CSSProperties = {
  backgroundColor: '#ff4d4d',
  color: '#fff',
  border: 'none',
  borderRadius: '4px',
  padding: '16px 36px',
  fontSize: '18px',
  fontWeight: 'bold',
  cursor: 'pointer',
  fontFamily: 'Orbitron, sans-serif'
};
