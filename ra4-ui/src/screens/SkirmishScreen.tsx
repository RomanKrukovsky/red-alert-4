import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';

interface PlayerSlot {
  id: number;
  name: string;
  faction: string;
  team: number;
  color: string;
  isAi: boolean;
  isReady: boolean;
  ping: number;
}

export const SkirmishScreen: React.FC = () => {
  const navigate = useNavigate();

  const [players, setPlayers] = useState<PlayerSlot[]>([
    { id: 1, name: 'Товарищ Командир (Вы)', faction: 'СССР', team: 1, color: '#ff2222', isAi: false, isReady: true, ping: 12 },
    { id: 2, name: 'General_Hawk', faction: 'АЛЬЯНС', team: 1, color: '#0088ff', isAi: false, isReady: true, ping: 24 },
    { id: 3, name: 'RedDragon', faction: 'ВОСТОЧНАЯ КОАЛИЦИЯ', team: 1, color: '#00ff66', isAi: false, isReady: true, ping: 45 },
    { id: 4, name: 'Chrono_Master', faction: 'ХРОНОЛЕГИОН', team: 1, color: '#aa00ff', isAi: false, isReady: true, ping: 18 },
    { id: 5, name: 'Бот (Эксперт)', faction: 'СССР', team: 2, color: '#ff4444', isAi: true, isReady: true, ping: 0 },
    { id: 6, name: 'Бот (Сложный)', faction: 'АЛЬЯНС', team: 2, color: '#33aaff', isAi: true, isReady: true, ping: 0 },
    { id: 7, name: 'Бот (Сложный)', faction: 'ВОСТОЧНАЯ КОАЛИЦИЯ', team: 2, color: '#33ff88', isAi: true, isReady: true, ping: 0 },
    { id: 8, name: 'Бот (Безумный)', faction: 'ХРОНОЛЕГИОН', team: 2, color: '#cc44ff', isAi: true, isReady: true, ping: 0 }
  ]);

  const [chatMessages, setChatMessages] = useState<string[]>([
    'СЕРВЕР: Игрок [General_Hawk] присоединился к лобби.',
    'General_Hawk: Всем привет! Я беру авиацию.',
    'RedDragon: Я держу центр и ресурсы.',
    'Chrono_Master: Заряжаю телепорты на вражескую базу.'
  ]);
  const [chatInput, setChatInput] = useState('');

  const handleSendChat = (e: React.FormEvent) => {
    e.preventDefault();
    if (!chatInput.trim()) return;
    setChatMessages([...chatMessages, `Товарищ Командир: ${chatInput}`]);
    setChatInput('');
  };

  const handleStartGame = () => {
    navigate('/loading');
  };

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/screenshots/17.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '16px 36px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Oswald', sans-serif"
      }}
    >
      {/* Top Strip */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderBottom: '1px solid rgba(255,50,50,0.3)',
        paddingBottom: '10px',
        zIndex: 10
      }}>
        <div>
          <div style={{ color: '#888', fontSize: '10px', letterSpacing: '3px' }}>COMMAND & CONQUER™</div>
          <div style={{ color: '#ff2222', fontSize: '22px', fontWeight: 800, letterSpacing: '2px', lineHeight: 1 }}>
            RED ALERT 4
          </div>
        </div>

        <div style={{ textAlign: 'center' }}>
          <div style={{ color: '#ffffff', fontSize: '18px', fontWeight: 800, letterSpacing: '2px' }}>
            СЕТЕВАЯ ИГРА: ЛОББИ 4x4
          </div>
          <div style={{ color: '#ffcc00', fontSize: '11px', letterSpacing: '1px' }}>
            КАРТА: АЛЯСКА - ХОЛОДНАЯ ВЕРШИНА ★ ПИНГ: 14 МС
          </div>
        </div>

        <div style={{ display: 'flex', gap: '14px', color: '#ccc', fontSize: '12px' }}>
          <span style={{ color: '#00ff66' }}>СЕРВЕР: ОНЛАЙН ●</span>
          <span>ИГРОКИ: 8 / 8</span>
        </div>
      </div>

      {/* Main Grid: 8 Player Slots + Map Preview + Chat */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '1fr 380px',
        gap: '20px',
        flex: 1,
        margin: '12px 0',
        zIndex: 5,
        alignItems: 'stretch'
      }}>
        {/* Left: 8 Player Slots */}
        <div className="ra4-panel clip-bevel-md" style={{
          padding: '16px',
          border: '1px solid rgba(255,50,50,0.4)',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          background: 'rgba(12, 6, 8, 0.94)'
        }}>
          <div>
            <div style={{ color: '#ff3333', fontSize: '12px', fontWeight: 700, letterSpacing: '2px', marginBottom: '10px' }}>
              СПИСОК УЧАСТНИКОВ (8 ИГРОКОВ)
            </div>

            <div style={{ display: 'flex', flexDirection: 'column', gap: '6px' }}>
              {players.map(p => (
                <div
                  key={p.id}
                  style={{
                    background: p.team === 1 ? 'rgba(30,12,12,0.8)' : 'rgba(12,18,30,0.8)',
                    border: `1px solid ${p.color}`,
                    borderRadius: '4px',
                    padding: '8px 12px',
                    display: 'flex',
                    justifyContent: 'space-between',
                    alignItems: 'center'
                  }}
                >
                  <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
                    <span style={{ color: p.color, fontWeight: 800 }}>#{p.id}</span>
                    <span style={{ color: '#ffffff', fontSize: '14px', fontWeight: 700 }}>{p.name}</span>
                    {p.isAi && <span style={{ background: '#333', fontSize: '9px', padding: '1px 4px', borderRadius: '2px' }}>ИИ</span>}
                  </div>

                  <div style={{ display: 'flex', alignItems: 'center', gap: '16px', fontSize: '12px' }}>
                    <span style={{ color: p.color }}>{p.faction}</span>
                    <span style={{ color: p.team === 1 ? '#ff4444' : '#0088ff', fontWeight: 700 }}>
                      КОМАНДА {p.team}
                    </span>
                    <span style={{ color: '#00ff66' }}>{p.ping > 0 ? `${p.ping} ms` : 'LOCAL'}</span>
                    <span style={{ color: '#00ff66', fontWeight: 800 }}>[ГОТОВ ✓]</span>
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Right: Map Card & Live Chat */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
          {/* Map Info Card */}
          <div className="ra4-panel clip-bevel-md" style={{
            padding: '14px',
            border: '1px solid rgba(255,255,255,0.2)',
            background: 'rgba(12, 6, 8, 0.94)'
          }}>
            <div style={{ color: '#ffdd00', fontSize: '14px', fontWeight: 800 }}>
              АЛЯСКА - ХОЛОДНАЯ ВЕРШИНА
            </div>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '11px', color: '#aaa', marginTop: '6px' }}>
              <span>РАЗМЕР: <strong>8 ИГРОКОВ (БОЛЬШАЯ)</strong></span>
              <span>РЕСУРСЫ: <strong>ВЫСОКИЕ</strong></span>
            </div>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '11px', color: '#aaa', marginTop: '2px' }}>
              <span>ТУМАН ВОЙНЫ: <strong>ВКЛЮЧЕН</strong></span>
              <span>СУПЕРОРУЖИЕ: <strong>ВКЛЮЧЕНО</strong></span>
            </div>
          </div>

          {/* Chat Feed */}
          <div className="ra4-panel clip-bevel-md" style={{
            flex: 1,
            padding: '12px',
            border: '1px solid rgba(255,255,255,0.2)',
            display: 'flex',
            flexDirection: 'column',
            justifyContent: 'space-between',
            background: 'rgba(12, 6, 8, 0.94)'
          }}>
            <div style={{ overflowY: 'auto', maxHeight: '160px', display: 'flex', flexDirection: 'column', gap: '4px' }}>
              {chatMessages.map((msg, i) => (
                <div key={i} style={{ fontSize: '11px', color: msg.startsWith('СЕРВЕР') ? '#ffdd00' : '#ddd', fontFamily: "'Inter', sans-serif" }}>
                  {msg}
                </div>
              ))}
            </div>

            <form onSubmit={handleSendChat} style={{ display: 'flex', gap: '6px', marginTop: '8px' }}>
              <input
                type="text"
                value={chatInput}
                onChange={e => setChatInput(e.target.value)}
                placeholder="Введите сообщение в чат..."
                style={{
                  flex: 1,
                  background: 'rgba(0,0,0,0.7)',
                  border: '1px solid rgba(255,255,255,0.2)',
                  color: '#fff',
                  padding: '6px 10px',
                  borderRadius: '3px',
                  fontSize: '12px'
                }}
              />
              <button
                type="submit"
                style={{
                  background: '#ff2222',
                  border: 'none',
                  color: '#fff',
                  padding: '6px 12px',
                  borderRadius: '3px',
                  cursor: 'pointer',
                  fontWeight: 700,
                  fontSize: '12px'
                }}
              >
                ОТПР.
              </button>
            </form>
          </div>
        </div>
      </div>

      {/* Bottom Bar */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderTop: '1px solid rgba(255,255,255,0.1)',
        paddingTop: '8px',
        zIndex: 10
      }}>
        <button
          onClick={() => navigate('/menu')}
          className="ra4-btn-ussr clip-bevel-sm"
          style={{ padding: '8px 24px', fontSize: '14px' }}
        >
          ‹ В ГЛАВНОЕ МЕНЮ
        </button>

        <button
          onClick={handleStartGame}
          className="clip-bevel-sm"
          style={{
            background: 'linear-gradient(180deg, #00aa44 0%, #005522 100%)',
            border: '1px solid #00ff66',
            color: '#ffffff',
            padding: '12px 60px',
            fontSize: '18px',
            fontWeight: 800,
            letterSpacing: '3px',
            cursor: 'pointer',
            boxShadow: '0 0 25px rgba(0,255,100,0.8)'
          }}
        >
          ★ В БОЙ!
        </button>

        <div style={{ display: 'flex', gap: '10px' }}>
          <button className="ra4-btn-ussr clip-bevel-sm" style={{ padding: '6px 14px', fontSize: '12px' }}>
            СМЕНИТЬ КАРТУ
          </button>
          <button className="ra4-btn-ussr clip-bevel-sm" style={{ padding: '6px 14px', fontSize: '12px' }}>
            НАСТРОЙКИ СЕТИ
          </button>
        </div>
      </div>
    </div>
  );
};
