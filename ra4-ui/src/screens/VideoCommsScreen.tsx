import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { BrandLogo } from '../components/Brand';
import { FACTIONS } from '../data/factions';

export const VideoCommsScreen: React.FC = () => {
  const navigate = useNavigate();
  const eurasian = FACTIONS.eurasian;
  const atlantic = FACTIONS.atlantic;
  const [isMicMuted, setIsMicMuted] = useState(false);
  const [isVideoDisabled, setIsVideoDisabled] = useState(false);
  const [seconds, setSeconds] = useState(167);
  const [dialogueIndex, setDialogueIndex] = useState(0);

  const dialogues = [
    { speaker: 'ВОЛКОВА', text: 'Рид. У нас десять минут до горного коридора. Решение нужно сейчас.', color: eurasian.color },
    { speaker: 'РИД', text: 'Волкова, ваши колонны пересекли демилитаризованную зону.', color: atlantic.color },
    { speaker: 'ВОЛКОВА', text: 'Мы не отступим. Эта земля — под нами.', color: eurasian.color },
    { speaker: 'РИД', text: 'Тогда пусть история рассудит нас. Атлантический альянс объявляет полную готовность.', color: atlantic.color }
  ];

  useEffect(() => {
    const timer = setInterval(() => {
      setSeconds(s => s + 1);
    }, 1000);
    return () => clearInterval(timer);
  }, []);

  const formatTime = (totalSec: number) => {
    const m = Math.floor(totalSec / 60).toString().padStart(2, '0');
    const s = (totalSec % 60).toString().padStart(2, '0');
    return `00:${m}:${s}`;
  };

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/remaster/09_secure_channel_eurasian_atlantic.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '16px 30px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Jura', sans-serif"
      }}
    >
      {/* ===== Top Header ===== */}
      <div style={{ display: 'grid', gridTemplateColumns: '1fr auto 1fr', alignItems: 'start', zIndex: 10 }}>
        <div style={{
          display: 'inline-flex',
          alignItems: 'center',
          gap: '8px',
          padding: '6px 12px',
          background: 'rgba(8,7,14,0.85)',
          border: '1px solid rgba(255,255,255,0.2)',
          borderRadius: '3px',
          fontSize: '11px',
          color: '#aab0ba',
          letterSpacing: '2px',
          justifySelf: 'start'
        }}>
          🔒 ШИФРОВАНИЕ AES-4
        </div>

        <div style={{ textAlign: 'center' }}>
          <BrandLogo scale={0.5} subtitle="ЗАЩИЩЁННЫЙ КАНАЛ СВЯЗИ" />
        </div>

        <div style={{ display: 'flex', justifyContent: 'flex-end' }}>
          <div style={{
            display: 'inline-flex',
            alignItems: 'center',
            gap: '8px',
            padding: '6px 12px',
            background: 'rgba(8,7,14,0.85)',
            border: '1px solid rgba(255,255,255,0.2)',
            borderRadius: '3px',
            fontSize: '11px',
            color: '#57e89a',
            letterSpacing: '2px'
          }}>
            📶 СПУТНИК СТАБИЛЕН
          </div>
        </div>
      </div>

      {/* Session banner */}
      <div style={{
        display: 'flex',
        justifyContent: 'center',
        alignItems: 'center',
        gap: '18px',
        fontSize: '11px',
        letterSpacing: '3px',
        color: '#9aa2ae',
        zIndex: 10,
        margin: '-6px 0'
      }}>
        <span>ПРЯМОЕ СОЕДИНЕНИЕ • БЕЗ ЗАПИСИ</span>
        <span style={{ color: '#e8ebf0', fontFamily: "'Orbitron', sans-serif" }}>⏱ {formatTime(seconds)}</span>
      </div>

      {/* ===== Split Call ===== */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '1fr auto 1fr',
        flex: 1,
        minHeight: 0,
        alignItems: 'stretch',
        gap: 0,
        position: 'relative',
        zIndex: 5
      }}>
        {/* Left: Волкова / Евразийский пакт */}
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          padding: '12px 14px',
          borderRight: '1px solid rgba(176,108,255,0.55)',
          background: 'linear-gradient(90deg, rgba(20,10,40,0.45), rgba(10,6,20,0.15))'
        }}>
          <div style={{
            alignSelf: 'flex-start',
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
            padding: '7px 14px',
            background: 'rgba(8,7,14,0.88)',
            border: `1px solid ${eurasian.color}88`,
            borderRadius: '4px'
          }}>
            <span style={{ fontSize: '22px', color: eurasian.color }}>❖</span>
            <div>
              <div style={{ fontFamily: "'Oswald', sans-serif", color: '#ffffff', fontSize: '15px', fontWeight: 800 }}>ИРИНА ВОЛКОВА</div>
              <div style={{ color: '#9aa2b0', fontSize: '10px', letterSpacing: '1px' }}>ЕВРАЗИЙСКИЙ ПАКТ</div>
            </div>
          </div>

          <div style={{
            width: '210px',
            padding: '9px 13px',
            background: 'rgba(6,5,12,0.9)',
            border: '1px solid rgba(255,255,255,0.14)',
            borderRadius: '4px',
            fontSize: '10.5px',
            color: '#98a0aa'
          }}>
            <div style={{ color: eurasian.color, fontWeight: 700, marginBottom: '3px', letterSpacing: '1px' }}>КАНАЛ ЗАЩИЩЁН</div>
            <div>Задержка: <strong style={{ color: '#57e89a' }}>42 мс</strong></div>
            <div>Потери пакетов: <strong style={{ color: '#57e89a' }}>низкие</strong></div>
          </div>
        </div>

        {/* Center divider glow */}
        <div style={{
          width: '2px',
          background: 'linear-gradient(180deg, transparent, #ff3c28 35%, #ff3c28 65%, transparent)',
          boxShadow: '0 0 18px rgba(255,60,40,0.85)'
        }} />

        {/* Right: Рид / Атлантический альянс */}
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          alignItems: 'flex-end',
          padding: '12px 14px',
          background: 'linear-gradient(270deg, rgba(8,18,44,0.45), rgba(5,8,20,0.15))'
        }}>
          <div style={{
            display: 'flex',
            alignItems: 'center',
            gap: '10px',
            padding: '7px 14px',
            background: 'rgba(6,8,16,0.88)',
            border: `1px solid ${atlantic.color}88`,
            borderRadius: '4px'
          }}>
            <div style={{ textAlign: 'right' }}>
              <div style={{ fontFamily: "'Oswald', sans-serif", color: '#ffffff', fontSize: '15px', fontWeight: 800 }}>МАРКУС РИД</div>
              <div style={{ color: '#8fa4b8', fontSize: '10px', letterSpacing: '1px' }}>АТЛАНТИЧЕСКИЙ АЛЬЯНС</div>
            </div>
            <span style={{ fontSize: '22px', color: atlantic.color }}>⬢</span>
          </div>

          <div style={{
            width: '210px',
            padding: '9px 13px',
            background: 'rgba(4,7,14,0.9)',
            border: '1px solid rgba(255,255,255,0.14)',
            borderRadius: '4px',
            fontSize: '10.5px',
            color: '#8a94a8'
          }}>
            <div style={{ color: atlantic.color, fontWeight: 700, marginBottom: '3px', letterSpacing: '1px' }}>КАНАЛ ЗАЩИЩЁН</div>
            <div>Задержка: <strong style={{ color: '#57b8ff' }}>41 мс</strong></div>
            <div>Потери пакетов: <strong style={{ color: '#57b8ff' }}>низкие</strong></div>
          </div>
        </div>
      </div>

      {/* ===== Subtitle Bar ===== */}
      <div
        onClick={() => setDialogueIndex(i => (i + 1) % dialogues.length)}
        style={{
          zIndex: 10,
          maxWidth: '900px',
          margin: '10px auto 0',
          background: 'rgba(4,4,8,0.88)',
          border: '1px solid rgba(255,255,255,0.16)',
          borderRadius: '4px',
          padding: '11px 22px',
          textAlign: 'center',
          cursor: 'pointer',
          boxShadow: '0 6px 24px rgba(0,0,0,0.85)'
        }}
      >
        <span style={{ color: dialogues[dialogueIndex].color, fontFamily: "'Oswald', sans-serif", fontWeight: 800, fontSize: '14px', letterSpacing: '1px' }}>
          «{dialogues[dialogueIndex].speaker}:&nbsp;
        </span>
        <span style={{ color: '#f2f4f8', fontSize: '14.5px' }}>{dialogues[dialogueIndex].text}</span>
      </div>

      {/* ===== Bottom Controls ===== */}
      <div style={{
        display: 'flex',
        justifyContent: 'center',
        alignItems: 'center',
        gap: '14px',
        padding: '14px 0 6px 0',
        zIndex: 10
      }}>
        <button
          onClick={() => setIsMicMuted(m => !m)}
          className="clip-bevel-sm"
          style={{
            height: '46px',
            padding: '0 18px',
            background: 'rgba(8,7,14,0.88)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: isMicMuted ? '#777' : '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '12px',
            fontWeight: 600,
            letterSpacing: '1.5px',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            gap: '9px'
          }}
        >
          <span style={{ fontSize: '16px' }}>{isMicMuted ? '🔇' : '🎙'}</span> МИКРОФОН
        </button>

        <button
          onClick={() => setIsVideoDisabled(v => !v)}
          className="clip-bevel-sm"
          style={{
            height: '46px',
            padding: '0 18px',
            background: 'rgba(8,7,14,0.88)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: isVideoDisabled ? '#777' : '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '12px',
            fontWeight: 600,
            letterSpacing: '1.5px',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            gap: '9px'
          }}
        >
          <span style={{ fontSize: '16px' }}>{isVideoDisabled ? '📵' : '📷'}</span> КАМЕРА
        </button>

        <button
          onClick={() => navigate('/briefing')}
          className="clip-bevel-sm"
          style={{
            height: '52px',
            padding: '0 34px',
            background: 'linear-gradient(180deg, #ff3c28 0%, #8a1408 100%)',
            border: '1px solid #ff5c47',
            borderRadius: '4px',
            color: '#ffffff',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '14px',
            fontWeight: 800,
            letterSpacing: '2px',
            cursor: 'pointer',
            boxShadow: '0 0 24px rgba(255,60,40,0.8)',
            display: 'flex',
            alignItems: 'center',
            gap: '10px'
          }}
        >
          ⮎&nbsp;&nbsp;ЗАВЕРШИТЬ СЕАНС
        </button>

        <button
          onClick={() => navigate('/strategic-map')}
          className="clip-bevel-sm"
          style={{
            height: '46px',
            padding: '0 18px',
            background: 'rgba(8,7,14,0.88)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '12px',
            fontWeight: 600,
            letterSpacing: '1.5px',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            gap: '9px'
          }}
        >
          🗺 КАРТА ОПЕРАЦИИ
        </button>

        <button
          onClick={() => navigate('/briefing')}
          className="clip-bevel-sm"
          style={{
            height: '46px',
            padding: '0 18px',
            background: 'rgba(8,7,14,0.88)',
            border: '1px solid rgba(255,255,255,0.25)',
            borderRadius: '4px',
            color: '#e8ebf0',
            fontFamily: "'Oswald', sans-serif",
            fontSize: '12px',
            fontWeight: 600,
            letterSpacing: '1.5px',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            gap: '9px'
          }}
        >
          ⛶ ПОЛНЫЙ ЭКРАН
        </button>
      </div>
    </div>
  );
};
