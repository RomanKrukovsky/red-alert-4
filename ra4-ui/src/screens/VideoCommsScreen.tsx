import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';

export const VideoCommsScreen: React.FC = () => {
  const navigate = useNavigate();
  const [isMicMuted, setIsMicMuted] = useState(false);
  const [isVideoDisabled, setIsVideoDisabled] = useState(false);
  const [seconds, setSeconds] = useState(167); // 00:02:47
  const [dialogueIndex, setDialogueIndex] = useState(0);

  const dialogues = [
    { speaker: 'СОКОЛОВ', text: 'Госпожа президент... у вас есть один шанс избежать войны.', color: '#ff2222' },
    { speaker: 'УОРД', text: 'Маршал, ваши войска пересекли демилитаризованную зону. Отзовите бронеколонны немедленно.', color: '#0088ff' },
    { speaker: 'СОКОЛОВ', text: 'Эта земля принадлежит трудовому народу. Мы не отступим.', color: '#ff2222' },
    { speaker: 'УОРД', text: 'Тогда пусть история рассудит нас. Альянс объявляет полную боевую готовность.', color: '#0088ff' }
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

  const handleNextDialogue = () => {
    setDialogueIndex((prev) => (prev + 1) % dialogues.length);
  };

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/screenshots/10.png') no-repeat center center`,
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
      {/* Top Header Strip */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderBottom: '1px solid rgba(255,255,255,0.15)',
        paddingBottom: '10px',
        zIndex: 10
      }}>
        {/* Left Profile */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
          <div style={{
            width: '28px',
            height: '28px',
            borderRadius: '50%',
            background: '#3a0808',
            border: '1px solid #ff3333',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            color: '#ff2222',
            fontSize: '14px'
          }}>
            ★
          </div>
          <div>
            <div style={{ color: '#fff', fontSize: '12px', fontWeight: 700 }}>ТОВАРИЩ КОМАНДИР</div>
            <div style={{ color: '#ff4444', fontSize: '10px' }}>УРОВЕНЬ 45 ★</div>
          </div>
        </div>

        {/* Center Title & Hotline Security Banner */}
        <div style={{ textAlign: 'center' }}>
          <div style={{ color: '#ff2222', fontSize: '20px', fontWeight: 800, letterSpacing: '3px', lineHeight: 1 }}>
            COMMAND & CONQUER RED ALERT 4
          </div>
          <div style={{ color: '#ffdd00', fontSize: '12px', letterSpacing: '2px', marginTop: '4px' }}>
            🔒 ЗАЩИЩЁННАЯ ЛИНИЯ СВЯЗИ (ШИФРОВАНИЕ: КРАСНАЯ ЗВЕЗДА • ПРОТОКОЛ: РА-4)
          </div>
        </div>

        {/* Right Currencies */}
        <div style={{ display: 'flex', gap: '14px', color: '#ccc', fontSize: '12px' }}>
          <span style={{ color: '#ffdd00' }}>💰 23 450</span>
          <span style={{ color: '#00ffcc' }}>⚡ 17 820</span>
          <span style={{ color: '#00ccff' }}>⚛ 9 680</span>
        </div>
      </div>

      {/* Main Split Video Calling Space */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '1fr 1fr',
        gap: '24px',
        flex: 1,
        margin: '16px 0',
        zIndex: 5,
        alignItems: 'stretch'
      }}>
        {/* Left Side: Marshal Viktor Sokolov (USSR) */}
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          padding: '12px',
          border: '1px solid rgba(255, 34, 34, 0.4)',
          borderRadius: '6px',
          background: 'linear-gradient(180deg, rgba(30, 8, 8, 0.5) 0%, rgba(10, 3, 3, 0.7) 100%)'
        }}>
          {/* Header */}
          <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
            <span style={{ color: '#ff2222', fontSize: '22px' }}>★</span>
            <div>
              <div style={{ color: '#ff2222', fontSize: '16px', fontWeight: 800 }}>МАРШАЛ ВИКТОР СОКОЛОВ</div>
              <div style={{ color: '#aaa', fontSize: '11px' }}>ВЕРХОВНОЕ КОМАНДОВАНИЕ СССР</div>
            </div>
          </div>

          {/* Connection Stats Left */}
          <div style={{
            background: 'rgba(0,0,0,0.6)',
            padding: '8px 12px',
            borderRadius: '4px',
            border: '1px solid rgba(255,50,50,0.2)',
            fontSize: '11px',
            color: '#aaa',
            width: '200px',
            fontFamily: "'Inter', sans-serif"
          }}>
            <div style={{ color: '#ff4444', fontWeight: 700, marginBottom: '2px' }}>КАНАЛ ЗАЩИЩЁН</div>
            <div>Статус: <strong style={{ color: '#00ff66' }}>ОТЛИЧНО</strong></div>
            <div>Задержка: <strong>17 мс</strong></div>
            <div>Потеря пакетов: <strong>0.0%</strong></div>
            <div style={{ marginTop: '4px', color: '#fff', fontSize: '10px' }}>
              МОСКВА • ЦК КПСС СЕКРЕТНО
            </div>
          </div>
        </div>

        {/* Right Side: President Eleanor Ward (USA/Alliance) */}
        <div style={{
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          alignItems: 'flex-end',
          padding: '12px',
          border: '1px solid rgba(0, 136, 255, 0.4)',
          borderRadius: '6px',
          background: 'linear-gradient(180deg, rgba(8, 20, 35, 0.5) 0%, rgba(3, 8, 15, 0.7) 100%)'
        }}>
          {/* Header */}
          <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
            <div style={{ textAlign: 'right' }}>
              <div style={{ color: '#0088ff', fontSize: '16px', fontWeight: 800 }}>ПРЕЗИДЕНТ ЭЛЕАНОР УОРД</div>
              <div style={{ color: '#aaa', fontSize: '11px' }}>СОЕДИНЁННЫЕ ШТАТЫ АМЕРИКИ</div>
            </div>
            <span style={{ color: '#0088ff', fontSize: '22px' }}>🦅</span>
          </div>

          {/* Connection Stats Right */}
          <div style={{
            background: 'rgba(0,0,0,0.6)',
            padding: '8px 12px',
            borderRadius: '4px',
            border: '1px solid rgba(0,136,255,0.2)',
            fontSize: '11px',
            color: '#aaa',
            width: '200px',
            textAlign: 'left',
            fontFamily: "'Inter', sans-serif"
          }}>
            <div style={{ color: '#0088ff', fontWeight: 700, marginBottom: '2px' }}>КАНАЛ ЗАЩИЩЁН</div>
            <div>Статус: <strong style={{ color: '#00ff66' }}>ОТЛИЧНО</strong></div>
            <div>Задержка: <strong>18 мс</strong></div>
            <div>Потеря пакетов: <strong>0.0%</strong></div>
            <div style={{ marginTop: '4px', color: '#fff', fontSize: '10px' }}>
              ВАШИНГТОН • БЕЛЫЙ ДОМ СЕКРЕТНО
            </div>
          </div>
        </div>
      </div>

      {/* Session Metadata & Subtitles Bar */}
      <div style={{
        display: 'flex',
        flexDirection: 'column',
        gap: '6px',
        zIndex: 10,
        marginBottom: '8px'
      }}>
        {/* Session Code */}
        <div style={{
          display: 'flex',
          justifyContent: 'space-between',
          color: '#888',
          fontSize: '11px',
          letterSpacing: '2px'
        }}>
          <span>КОД СЕССИИ: RA4-CC-1987-7A</span>
          <span>ПРЯМОЕ СОЕДИНЕНИЕ • БЕЗ ЗАПИСИ</span>
          <span>ДЛИТЕЛЬНОСТЬ: {formatTime(seconds)}</span>
        </div>

        {/* Subtitles Area (Interactive Dialogue) */}
        <div
          onClick={handleNextDialogue}
          style={{
            background: 'rgba(0,0,0,0.85)',
            border: '1px solid rgba(255,255,255,0.15)',
            borderRadius: '4px',
            padding: '12px 20px',
            textAlign: 'center',
            cursor: 'pointer',
            boxShadow: '0 4px 20px rgba(0,0,0,0.8)'
          }}
        >
          <span style={{ color: dialogues[dialogueIndex].color, fontWeight: 800, fontSize: '16px', letterSpacing: '1px' }}>
            {dialogues[dialogueIndex].speaker}:&nbsp;
          </span>
          <span style={{ color: '#ffffff', fontSize: '16px', fontFamily: "'Inter', sans-serif" }}>
            {dialogues[dialogueIndex].text}
          </span>
          <span style={{ color: '#ffdd00', fontSize: '12px', marginLeft: '12px' }}>
            (Клик для продолжения реплики)
          </span>
        </div>
      </div>

      {/* Bottom Video Calling Controls */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderTop: '1px solid rgba(255,255,255,0.1)',
        paddingTop: '8px',
        zIndex: 10
      }}>
        {/* Left Security State */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px', color: '#ff4444', fontSize: '13px' }}>
          <span>★ ЗАЩИТА СЕТИ:</span>
          <strong>УРОВЕНЬ КРАСНЫЙ</strong>
        </div>

        {/* Center Calling Buttons */}
        <div style={{ display: 'flex', gap: '12px' }}>
          <button
            onClick={() => setIsMicMuted(!isMicMuted)}
            className="ra4-btn-ussr clip-bevel-sm"
            style={{ padding: '8px 16px', fontSize: '13px', background: isMicMuted ? '#660000' : undefined }}
          >
            {isMicMuted ? '🔇 ВКЛ. МИКРОФОН' : '🎤 ОТКЛ. МИКРОФОН'}
          </button>

          <button
            onClick={() => setIsVideoDisabled(!isVideoDisabled)}
            className="ra4-btn-ussr clip-bevel-sm"
            style={{ padding: '8px 16px', fontSize: '13px', background: isVideoDisabled ? '#660000' : undefined }}
          >
            {isVideoDisabled ? '📹 ВКЛ. ВИДЕО' : '📷 ОТКЛ. ВИДЕО'}
          </button>

          <button
            onClick={() => navigate('/briefing')}
            className="clip-bevel-sm"
            style={{
              background: 'linear-gradient(180deg, #dd1111 0%, #880000 100%)',
              border: '1px solid #ff4444',
              color: '#ffffff',
              padding: '8px 24px',
              fontSize: '14px',
              fontWeight: 800,
              cursor: 'pointer',
              boxShadow: '0 0 15px rgba(255,0,0,0.7)'
            }}
          >
            ⮌ ЗАВЕРШИТЬ СЕАНС
          </button>

          <button
            onClick={() => navigate('/hud?mode=ussr-tank-assault')}
            className="ra4-btn-ussr clip-bevel-sm"
            style={{ padding: '8px 16px', fontSize: '13px' }}
          >
            📊 ПОДЕЛИТЬСЯ ДАННЫМИ
          </button>
        </div>

        {/* Right Signal Status */}
        <div style={{ color: '#00ff66', fontSize: '12px' }}>
          СИГНАЛ СТАБИЛЕН • СПУТНИК К-07 📡
        </div>
      </div>
    </div>
  );
};
