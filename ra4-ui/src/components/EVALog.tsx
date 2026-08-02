import React, { useEffect, useRef } from 'react';
import { useUIStore } from '../store.js';

export const EVALog: React.FC = () => {
  const evaLogs = useUIStore((state) => state.evaLogs);
  const containerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (containerRef.current) {
      containerRef.current.scrollTop = containerRef.current.scrollHeight;
    }
  }, [evaLogs]);

  const getColor = (type?: 'INFO' | 'WARN' | 'DANGER') => {
    switch (type) {
      case 'DANGER':
        return '#ff3344';
      case 'WARN':
        return '#ffcc00';
      case 'INFO':
      default:
        return '#00ffc8';
    }
  };

  return (
    <div
      ref={containerRef}
      className="ra4-eva-log"
      style={{
        width: '380px',
        maxHeight: '130px',
        overflowY: 'auto',
        display: 'flex',
        flexDirection: 'column',
        gap: '4px',
        fontSize: '12px',
        fontFamily: 'var(--font-family-mono, monospace)',
        letterSpacing: 'var(--letter-spacing-wide, 0.05em)',
        pointerEvents: 'none',
        textShadow: '0 1px 3px rgba(0, 0, 0, 0.9)',
        padding: '6px 10px',
        background: 'rgba(5, 15, 25, 0.65)',
        borderLeft: '2px solid #00ffc8',
        borderRadius: '0 4px 4px 0',
        boxShadow: '0 0 10px rgba(0, 255, 200, 0.15)',
        backdropFilter: 'blur(4px)',
      }}
    >
      {evaLogs.map((log) => (
        <div key={log.id} style={{ color: getColor(log.type), display: 'flex', gap: '8px' }}>
          <span style={{ opacity: 0.6, fontSize: '10px' }}>[{log.timestamp}]</span>
          <span>{log.message}</span>
        </div>
      ))}
    </div>
  );
};
