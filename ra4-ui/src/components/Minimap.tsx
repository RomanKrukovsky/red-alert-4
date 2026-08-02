import React from 'react';
import { Frame } from './Frame.js';

export const Minimap: React.FC = () => {
  return (
    <Frame className="ra4-minimap-panel" style={{
      width: '300px',
      height: '300px',
      display: 'flex',
      flexDirection: 'column',
      padding: '10px',
      clipPath: 'polygon(0 0, 100% 0, 100% calc(100% - 30px), calc(100% - 30px) 100%, 0 100%)'
    }}>
      {/* Radar sweeping effect container */}
      <div style={{
        flex: 1,
        background: '#000',
        border: '1px solid var(--faction-secondary)',
        position: 'relative',
        overflow: 'hidden'
      }}>
        {/* Mock Map Image */}
        <div style={{
           width: '100%',
           height: '100%',
           backgroundImage: 'url("data:image/svg+xml;utf8,<svg xmlns=\\"http://www.w3.org/2000/svg\\" width=\\"10\\" height=\\"10\\"><rect width=\\"10\\" height=\\"10\\" fill=\\"%23110000\\"/><circle cx=\\"5\\" cy=\\"5\\" r=\\"1\\" fill=\\"%23330000\\"/></svg>")',
           backgroundSize: '20px 20px'
        }} />
        
        {/* Mock Units */}
        <div style={{ position: 'absolute', top: '40%', left: '40%', width: '4px', height: '4px', background: '#00ff00' }} />
        <div style={{ position: 'absolute', top: '45%', left: '38%', width: '4px', height: '4px', background: '#00ff00' }} />
        <div style={{ position: 'absolute', top: '70%', left: '80%', width: '4px', height: '4px', background: '#ff0000' }} />
        
        {/* Radar Sweep */}
        <div style={{
           position: 'absolute',
           top: '50%',
           left: '50%',
           width: '150%',
           height: '150%',
           background: 'conic-gradient(from 0deg, transparent 70%, rgba(255, 0, 0, 0.4) 100%)',
           transformOrigin: '0 0',
           animation: 'spin 4s linear infinite'
        }} />
      </div>
      
      {/* Map Controls */}
      <div style={{ display: 'flex', justifyContent: 'space-between', marginTop: '10px' }}>
         <button style={{ background: 'transparent', border: '1px solid var(--faction-dark)', color: 'var(--faction-text)', padding: '5px 10px', fontSize: '10px' }}>РЕЖИМ 1</button>
         <button style={{ background: 'transparent', border: '1px solid var(--faction-dark)', color: 'var(--faction-text)', padding: '5px 10px', fontSize: '10px' }}>РЕЖИМ 2</button>
      </div>
      
      <style>{`
        @keyframes spin { 100% { transform: rotate(360deg); } }
      `}</style>
    </Frame>
  );
};
