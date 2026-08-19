import React, { useEffect } from 'react';
import { useNavigate } from 'react-router-dom';

export const StartScreen: React.FC = () => {
  const navigate = useNavigate();

  useEffect(() => {
    const handleKeyDown = () => {
      navigate('/menu');
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [navigate]);

  return (
    <div
      onClick={() => navigate('/menu')}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        cursor: 'pointer',
        background: `url('/screenshots/1.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'space-between',
        padding: '60px 20px',
        overflow: 'hidden'
      }}
    >
      {/* Cinematic Vignette */}
      <div style={{
        position: 'absolute',
        inset: 0,
        background: 'radial-gradient(ellipse at center, rgba(0,0,0,0) 40%, rgba(0,0,0,0.7) 100%)',
        pointerEvents: 'none'
      }} />

      {/* Top spacer */}
      <div style={{ width: '100%', height: '80px' }} />

      {/* Main Title Center Glow and Soviet Star */}
      <div style={{
        zIndex: 2,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        textAlign: 'center',
        marginTop: '-40px'
      }}>
        <div style={{
          fontSize: '18px',
          letterSpacing: '8px',
          color: '#e0e0e0',
          fontFamily: "'Oswald', sans-serif",
          textShadow: '0 0 10px rgba(255,255,255,0.4)',
          marginBottom: '8px',
          fontWeight: 400
        }}>
          COMMAND & CONQUER™
        </div>

        <h1 style={{
          fontSize: '5.5rem',
          margin: 0,
          fontFamily: "'Oswald', sans-serif",
          fontWeight: 800,
          letterSpacing: '6px',
          color: '#ff2222',
          textShadow: '0 0 30px rgba(255,20,20,0.8), 0 0 60px rgba(255,0,0,0.5), 0 4px 10px rgba(0,0,0,0.9)',
          lineHeight: 1.05
        }}>
          RED ALERT 4
        </h1>
      </div>

      {/* Bottom Pulsing Prompt */}
      <div style={{
        zIndex: 2,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        gap: '12px'
      }}>
        <div style={{
          fontFamily: "'Oswald', sans-serif",
          fontSize: '1.4rem',
          letterSpacing: '5px',
          color: '#ffffff',
          textShadow: '0 0 12px rgba(255,255,255,0.8), 0 0 20px rgba(255,50,50,0.6)',
          animation: 'alert-flash 1.8s ease-in-out infinite'
        }}>
          НАЖМИТЕ ЛЮБУЮ КЛАВИШУ
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '15px' }}>
          <div style={{ width: '120px', height: '1px', background: 'linear-gradient(90deg, transparent, rgba(255,50,50,0.8))' }} />
          <span style={{ color: '#ff2222', fontSize: '18px', filter: 'drop-shadow(0 0 6px rgba(255,0,0,0.8))' }}>★</span>
          <div style={{ width: '120px', height: '1px', background: 'linear-gradient(90deg, rgba(255,50,50,0.8), transparent)' }} />
        </div>
      </div>
    </div>
  );
};
