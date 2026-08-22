import React, { useState } from 'react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';

interface PlayerSlot {
  slot: number;
  name: string;
  faction: string;
  factionColor: string;
  team: number;
  category: string;
  country: string;
  status: string;
  ready: boolean;
}

const INITIAL_PLAYERS: PlayerSlot[] = [
  { slot: 1, name: 'SteelWolf_88', faction: 'ЕВРАЗИЙСКИЙ ПАКТ', factionColor: '#b06cff', team: 1, category: 'БРОНЕТЕХНИКА', country: 'РОССИЯ', status: 'СЕТЕВАЯ ИГРА', ready: true },
  { slot: 2, name: 'Alec_Command', faction: 'АТЛАНТИЧЕСКИЙ АЛЬЯНС', factionColor: '#3f8dff', team: 1, category: 'АВИАЦИЯ', country: 'США', status: 'СЕТЕВАЯ ИГРА', ready: true },
  { slot: 3, name: 'JadeTiger', faction: 'ВОСТОЧНАЯ КОАЛИЦИЯ', factionColor: '#2fd98a', team: 2, category: 'ДРОНЫ', country: 'КИТАЙ', status: 'СЕТЕВАЯ ИГРА', ready: true },
  { slot: 4, name: 'StormRider', faction: 'ТИХООКЕАНСКИЙ ПАКТ', factionColor: '#2fd4c8', team: 2, category: 'ФЛОТ', country: 'ЯПОНИЯ', status: 'ОДИНОЧНАЯ ИГРА', ready: true },
  { slot: 5, name: 'DesertFalcon', faction: 'НЕЗАВИСИМЫЕ ДЕРЖАВЫ', factionColor: '#e8a13d', team: 3, category: 'ПЕХОТА', country: 'ИРАН', status: 'БОТ • ЭКСПЕРТ', ready: true },
  { slot: 6, name: 'Ironclaw', faction: 'ЕВРАЗИЙСКИЙ ПАКТ', factionColor: '#b06cff', team: 3, category: 'АРТИЛЛЕРИЯ', country: 'ГЕРМАНИЯ', status: 'СЕТЕВАЯ ИГРА', ready: true },
  { slot: 7, name: 'NightOwl', faction: 'АТЛАНТИЧЕСКИЙ АЛЬЯНС', factionColor: '#3f8dff', team: 4, category: 'РЭБ', country: 'ВЕЛИКОБРИТАНИЯ', status: 'СЕТЕВАЯ ИГРА', ready: true },
  { slot: 8, name: 'VostokCross', faction: 'ВОСТОЧНАЯ КОАЛИЦИЯ', factionColor: '#2fd98a', team: 4, category: 'ЛОГИСТИКА', country: 'ИНДИЯ', status: 'НАБЛЮДАТЕЛЬ', ready: false }
];

export const SkirmishScreen: React.FC = () => {
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const isSkirmish = searchParams.get('mode') === 'skirmish';

  const [players] = useState<PlayerSlot[]>(INITIAL_PLAYERS);
  const [gameMode, setGameMode] = useState(isSkirmish ? 'СХВАТКА' : 'БИТВА НА УНИЧТОЖЕНИЕ');
  const [chatMessages, setChatMessages] = useState<string[]>([
    'СЕРВЕР: Игрок [JadeTiger] присоединился к лобби.',
    'SteelWolf_88: Беру броню на центр.',
    'Alec_Command: Авиация моя. Прикройте восток.'
  ]);
  const [chatInput, setChatInput] = useState('');

  const handleSendChat = (e: React.FormEvent) => {
    e.preventDefault();
    if (!chatInput.trim()) return;
    setChatMessages([...chatMessages, `Вы: ${chatInput}`]);
    setChatInput('');
  };

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/remaster/11_multiplayer_lobby.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        padding: '14px 30px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* Scrim */}
      <div style={{ position: 'absolute', inset: 0, background: 'linear-gradient(180deg, rgba(4,4,8,0.55), rgba(4,4,8,0.75))', zIndex: 1 }} />

      {/* ===== Header ===== */}
      <div style={{ display: 'grid', gridTemplateColumns: '220px 1fr 220px', alignItems: 'start', zIndex: 10 }}>
        <div style={{
          justifySelf: 'start',
          fontFamily: "'Orbitron', sans-serif",
          fontSize: '15px',
          fontWeight: 800,
          letterSpacing: '2px',
          color: '#57e89a',
          textShadow: '0 0 12px rgba(87,232,154,0.6)'
        }}>
          8/8 ИГРОКОВ ГОТОВЫ
        </div>

        <div style={{ textAlign: 'center' }}>
          <BrandLogo scale={0.52} subtitle="ДОБРО ПОЖАЛОВАТЬ В СЕТЕВОЙ БОЙ" />
        </div>
        <div />
      </div>

      {/* ===== Main 3-column grid ===== */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '250px 1fr 330px',
        gap: '16px',
        flex: 1,
        margin: '16px 0 10px 0',
        zIndex: 5,
        minHeight: 0
      }}>
        {/* Left Match Parameters */}
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          gap: '10px'
        }}>
          <div style={{
            flex: 1,
            padding: '14px 16px',
            background: 'linear-gradient(180deg, rgba(10,8,16,0.93), rgba(6,5,10,0.97))',
            border: '1px solid rgba(255,255,255,0.18)',
            borderRadius: '6px'
          }}>
            <div style={{ fontFamily: "'Oswald', sans-serif", color: '#dfe5ee', fontSize: '14px', fontWeight: 700, letterSpacing: '2px', marginBottom: '12px' }}>
              ПАРАМЕТРЫ МАТЧА
            </div>

            <div style={{ color: '#8b93a2', fontSize: '10px', letterSpacing: '1px', marginBottom: '5px' }}>РЕЖИМ ИГРЫ</div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '5px', marginBottom: '12px' }}>
              {['БИТВА НА УНИЧТОЖЕНИЕ', 'СХВАТКА', 'ВЫЖИВАНИЕ ВСЕХ'].map(m => (
                <button
                  key={m}
                  onClick={() => setGameMode(m)}
                  style={{
                    textAlign: 'left',
                    padding: '6px 10px',
                    background: gameMode === m ? 'rgba(176,108,255,0.22)' : 'rgba(255,255,255,0.04)',
                    border: `1px solid ${gameMode === m ? '#b06cff' : 'rgba(255,255,255,0.12)'}`,
                    borderRadius: '3px',
                    color: gameMode === m ? '#ffffff' : '#9aa2ae',
                    fontFamily: "'Oswald', sans-serif",
                    fontSize: '11px',
                    letterSpacing: '1px',
                    cursor: 'pointer'
                  }}
                >
                  ◉ {m}
                </button>
              ))}
            </div>

            {[
              { label: 'СКОРОСТЬ', value: 'СТАНДАРТ' },
              { label: 'СТАРТОВЫЕ ЮНИТЫ', value: 'ВКЛ' },
              { label: 'СТРАТЕГИЧЕСКИЕ ТОЧКИ', value: '6' },
              { label: 'ВРЕМЯ СУТОК', value: '06:00 УТРА' }
            ].map(p => (
              <div key={p.label} style={{
                display: 'flex',
                justifyContent: 'space-between',
                alignItems: 'center',
                padding: '6px 0',
                borderTop: '1px solid rgba(255,255,255,0.08)',
                fontSize: '11px',
                color: '#9aa2ae'
              }}>
                <span>{p.label}</span>
                <strong style={{ color: '#fff' }}>{p.value}</strong>
              </div>
            ))}
          </div>

          <button
            onClick={() => navigate('/menu')}
            className="clip-bevel-sm"
            style={{
              height: '40px',
              background: 'rgba(8,7,14,0.88)',
              border: '1px solid rgba(255,255,255,0.25)',
              borderRadius: '4px',
              color: '#e8ebf0',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '13px',
              fontWeight: 600,
              letterSpacing: '2px',
              cursor: 'pointer'
            }}
          >
            ✉&nbsp;&nbsp;ПРИГЛАСИТЬ ДРУГА
          </button>

          <button
            onClick={() => navigate('/menu')}
            className="clip-bevel-sm"
            style={{
              height: '40px',
              background: 'rgba(60,12,12,0.85)',
              border: '1px solid rgba(255,80,60,0.5)',
              borderRadius: '4px',
              color: '#ffb0a4',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '13px',
              fontWeight: 600,
              letterSpacing: '2px',
              cursor: 'pointer'
            }}
          >
            ⮎&nbsp;&nbsp;ПОКИНУТЬ ЛОББИ
          </button>
        </div>

        {/* Center Players Table + Chat */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '12px', minHeight: 0 }}>
          <div style={{
            flex: 1,
            overflow: 'auto',
            background: 'linear-gradient(180deg, rgba(10,8,16,0.93), rgba(6,5,10,0.97))',
            border: '1px solid rgba(255,255,255,0.18)',
            borderRadius: '6px',
            padding: '12px 14px'
          }}>
            {/* Table header */}
            <div style={{
              display: 'grid',
              gridTemplateColumns: '36px 1.3fr 150px 130px 110px 90px',
              gap: '8px',
              paddingBottom: '8px',
              borderBottom: '1px solid rgba(255,255,255,0.14)',
              color: '#8b93a2',
              fontSize: '10px',
              letterSpacing: '1.5px'
            }}>
              <span>#</span><span>ИГРОК</span><span>БОЕВАЯ КАТЕГОРИЯ</span><span>СТРАНА</span><span>СТАТУС</span><span style={{ textAlign: 'right' }}>ГОТОВНОСТЬ</span>
            </div>

            {players.map(p => (
              <div key={p.slot} style={{
                display: 'grid',
                gridTemplateColumns: '36px 1.3fr 150px 130px 110px 90px',
                gap: '8px',
                alignItems: 'center',
                padding: '7px 0',
                borderBottom: '1px solid rgba(255,255,255,0.06)'
              }}>
                <span style={{
                  width: '24px', height: '24px',
                  display: 'inline-flex', alignItems: 'center', justifyContent: 'center',
                  clipPath: 'polygon(50% 0, 93% 25%, 93% 75%, 50% 100%, 7% 75%, 7% 25%)',
                  background: `${p.factionColor}33`,
                  border: `1px solid ${p.factionColor}`,
                  color: p.factionColor,
                  fontSize: '10px',
                  fontWeight: 700
                }}>
                  {p.slot}
                </span>
                <span style={{
                  display: 'flex',
                  alignItems: 'center',
                  gap: '8px',
                  color: '#ffffff',
                  fontSize: '12.5px',
                  fontWeight: 700,
                  whiteSpace: 'nowrap',
                  overflow: 'hidden',
                  textOverflow: 'ellipsis'
                }}>
                  <span style={{ width: '8px', height: '8px', borderRadius: '2px', background: p.factionColor }} />
                  {p.name}
                </span>
                <span style={{ color: '#c3c8d2', fontSize: '11px', whiteSpace: 'nowrap' }}>{p.category}</span>
                <span style={{ color: p.factionColor, fontSize: '10.5px', letterSpacing: '0.5px', whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{p.country}</span>
                <span style={{
                  fontSize: '9px',
                  letterSpacing: '0.5px',
                  color: p.status === 'БОТ • ЭКСПЕРТ' ? '#e0c25c' : (p.status === 'НАБЛЮДАТЕЛЬ' ? '#8b93a2' : '#cdd3dd'),
                  whiteSpace: 'nowrap'
                }}>
                  {p.status}
                </span>
                <span style={{
                  textAlign: 'right',
                  color: p.ready ? '#57e89a' : '#777',
                  fontWeight: 800,
                  fontSize: '11px'
                }}>
                  {p.ready ? 'ГОТОВ ✓' : '—'}
                </span>
              </div>
            ))}
          </div>

          {/* Chat */}
          <div style={{
            height: '150px',
            display: 'flex',
            flexDirection: 'column',
            background: 'rgba(6,5,10,0.94)',
            border: '1px solid rgba(255,255,255,0.18)',
            borderRadius: '6px',
            padding: '10px 12px'
          }}>
            <div style={{ color: '#8b93a2', fontSize: '10px', letterSpacing: '2px', marginBottom: '6px' }}>ЧАТ ЛОББИ</div>
            <div style={{ flex: 1, overflowY: 'auto', display: 'flex', flexDirection: 'column', gap: '3px' }}>
              {chatMessages.map((msg, i) => (
                <div key={i} style={{ fontSize: '11px', color: msg.startsWith('СЕРВЕР') ? '#e0c25c' : (msg.startsWith('Вы:') ? '#b06cff' : '#cdd3dd') }}>
                  {msg}
                </div>
              ))}
            </div>
            <form onSubmit={handleSendChat} style={{ display: 'flex', gap: '6px', marginTop: '6px' }}>
              <input
                type="text"
                value={chatInput}
                onChange={e => setChatInput(e.target.value)}
                placeholder="Сообщение..."
                style={{
                  flex: 1,
                  background: 'rgba(0,0,0,0.7)',
                  border: '1px solid rgba(255,255,255,0.18)',
                  borderRadius: '3px',
                  color: '#fff',
                  padding: '6px 10px',
                  fontSize: '12px',
                  outline: 'none'
                }}
              />
              <button type="submit" style={{
                background: '#b06cff',
                border: 'none',
                borderRadius: '3px',
                color: '#0b0712',
                fontWeight: 800,
                fontSize: '11px',
                padding: '0 14px',
                cursor: 'pointer'
              }}>ОТПР</button>
            </form>
          </div>
        </div>

        {/* Right Map Panel */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '12px' }}>
          <div style={{
            padding: '12px 14px',
            background: 'linear-gradient(180deg, rgba(10,8,16,0.93), rgba(6,5,10,0.97))',
            border: '1px solid rgba(255,255,255,0.18)',
            borderRadius: '6px'
          }}>
            <div style={{ fontFamily: "'Oswald', sans-serif", color: '#dfe5ee', fontSize: '13px', fontWeight: 700, letterSpacing: '2px', marginBottom: '8px' }}>
              КАРТА
            </div>
            <div style={{
              height: '150px',
              borderRadius: '3px',
              border: '1px solid rgba(255,255,255,0.16)',
              background:
                'radial-gradient(circle at 30% 40%, rgba(47,212,200,0.3), transparent 45%), radial-gradient(circle at 65% 65%, rgba(63,141,255,0.28), transparent 45%), linear-gradient(160deg, #0c2233, #071320)',
              position: 'relative'
            }}>
              <span style={{
                position: 'absolute',
                bottom: '6px',
                left: '8px',
                color: '#9fd8d2',
                fontSize: '10px',
                letterSpacing: '1px'
              }}>
                ▲ АРХИПЕЛАГ «ТИФОН»
              </span>
              {[1, 2, 3, 4, 5, 6].map(i => (
                <span key={i} style={{
                  position: 'absolute',
                  left: `${15 + (i % 3) * 28}%`,
                  top: `${20 + Math.floor(i / 3.1) * 34}%`,
                  width: '16px',
                  height: '16px',
                  borderRadius: '50%',
                  border: '1px dashed rgba(159,216,210,0.7)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  color: '#9fd8d2',
                  fontSize: '9px'
                }}>
                  {i}
                </span>
              ))}
            </div>

            <div style={{ marginTop: '10px' }}>
              <div style={{ color: '#8b93a2', fontSize: '10px', letterSpacing: '1px', marginBottom: '5px' }}>ПАРАМЕТРЫ КАРТЫ</div>
              {[
                { label: 'ИГРОКИ', value: '8' },
                { label: 'ПОГОДА', value: 'ДИНАМИЧЕСКАЯ' },
                { label: 'РЕСУРСЫ', value: 'СТАНДАРТ' },
                { label: 'НАБЛЮДАТЕЛИ', value: '2' }
              ].map(r => (
                <div key={r.label} style={{
                  display: 'flex',
                  justifyContent: 'space-between',
                  padding: '5px 0',
                  borderTop: '1px solid rgba(255,255,255,0.07)',
                  fontSize: '11px',
                  color: '#9aa2ae'
                }}>
                  <span>{r.label}</span>
                  <strong style={{ color: '#fff' }}>{r.value}</strong>
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>

      {/* ===== Bottom Bar ===== */}
      <div style={{ display: 'grid', gridTemplateColumns: '170px 1fr auto', gap: '14px', alignItems: 'center', paddingBottom: '4px', zIndex: 10 }}>
        <button
          onClick={() => navigate('/menu')}
          className="clip-bevel-sm"
          style={{
            height: '44px',
            background: 'rgba(8,7,14,0.88)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '14px',
            fontWeight: 600,
            letterSpacing: '2px',
            cursor: 'pointer'
          }}
        >
          ‹&nbsp;&nbsp;В МЕНЮ
        </button>
        <div />

        <div style={{ display: 'flex', gap: '12px' }}>
          <button
            onClick={() => navigate('/loading')}
            className="clip-bevel-sm"
            style={{
              height: '46px',
              padding: '0 20px',
              background: 'rgba(8,7,14,0.88)',
              border: '1px solid rgba(255,255,255,0.25)',
              borderRadius: '4px',
              color: '#e8ebf0',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '13px',
              fontWeight: 600,
              letterSpacing: '2px',
              cursor: 'pointer'
            }}
          >
            👁&nbsp;&nbsp;НАБЛЮДАТЬ
          </button>

          <button
            onClick={() => navigate('/loading')}
            className="clip-bevel-sm"
            style={{
              height: '56px',
              padding: '0 70px',
              background: 'linear-gradient(180deg, #b06cff 0%, #5a2fa0 100%)',
              border: '1px solid #d8b4ff',
              borderRadius: '4px',
              color: '#0b0712',
              fontFamily: "'Oswald', sans-serif",
              fontSize: '19px',
              fontWeight: 800,
              letterSpacing: '3px',
              cursor: 'pointer',
              boxShadow: '0 0 28px rgba(176,108,255,0.85)',
              clipPath: 'polygon(0 10px, 12px 0, 100% 0, 100% calc(100% - 10px), calc(100% - 12px) 100%, 0 100%)'
            }}
          >
            ⚔&nbsp;&nbsp;НАЧАТЬ БИТВУ
          </button>
        </div>
      </div>
    </div>
  );
};
