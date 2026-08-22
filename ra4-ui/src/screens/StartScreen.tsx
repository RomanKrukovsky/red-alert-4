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
        background: `url('/remaster/01_title_screen.png') no-repeat center center`,
        backgroundSize: 'cover',
        overflow: 'hidden'
      }}
    >
      {/* Cinematic vignette */}
      <div style={{
        position: 'absolute',
        inset: 0,
        background: 'radial-gradient(ellipse at center, rgba(0,0,0,0) 45%, rgba(0,0,0,0.55) 100%)',
        pointerEvents: 'none'
      }} />

      {/* Bottom prompt */}
      <div style={{
        position: 'absolute',
        bottom: '7.5%',
        left: 0,
        right: 0,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        gap: '10px'
      }}>
        <div style={{
          fontFamily: "'Jura', sans-serif",
          fontSize: '1.35rem',
          fontWeight: 600,
          letterSpacing: '10px',
          color: '#eef2f6',
          textShadow: '0 2px 12px rgba(0,0,0,0.95), 0 0 24px rgba(120,160,255,0.35)',
          animation: 'alert-flash 2.4s ease-in-out infinite'
        }}>
          НАЖМИТЕ ЛЮБУЮ КЛАВИШУ
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
          <div style={{ width: '150px', height: '1px', background: 'linear-gradient(90deg, transparent, rgba(255,60,40,0.9))' }} />
          <span style={{ width: '5px', height: '5px', borderRadius: '50%', background: '#ff3c28', boxShadow: '0 0 8px rgba(255,60,40,0.9)' }} />
          <div style={{ width: '150px', height: '1px', background: 'linear-gradient(90deg, rgba(255,60,40,0.9), transparent)' }} />
        </div>
      </div>

      {/* Pre-release tag */}
      <div style={{
        position: 'absolute',
        bottom: '18px',
        right: '22px',
        fontFamily: "'Jura', sans-serif",
        fontSize: '11px',
        letterSpacing: '3px',
        color: '#9aa4b0',
        textShadow: '0 1px 4px rgba(0,0,0,0.9)'
      }}>
        ПРЕДВАРИТЕЛЬНАЯ ВЕРСИЯ
      </div>
    </div>
  );
};
