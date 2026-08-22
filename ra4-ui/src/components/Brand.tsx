import React from 'react';

interface BrandLogoProps {
  subtitle?: string;
  scale?: number;
  align?: 'center' | 'left';
}

/** SCARLET HORIZON steel-gradient wordmark with red horizon line */
export const BrandLogo: React.FC<BrandLogoProps> = ({ subtitle, scale = 1, align = 'center' }) => (
  <div style={{ textAlign: align, lineHeight: 1 }}>
    <div style={{
      fontFamily: "'Orbitron', sans-serif",
      fontWeight: 900,
      fontSize: `${2.6 * scale}rem`,
      letterSpacing: `${0.14 * scale}em`,
      background: 'linear-gradient(180deg, #f4f6f9 0%, #b9c2cf 38%, #6d7889 62%, #cfd6e0 100%)',
      WebkitBackgroundClip: 'text',
      backgroundClip: 'text',
      color: 'transparent',
      filter: 'drop-shadow(0 3px 6px rgba(0,0,0,0.85))'
    }}>
      SCARLET HORIZON
    </div>
    <div style={{
      height: '2px',
      margin: `${0.28 * scale}rem auto ${0.22 * scale}rem`,
      width: `${100 * scale}%`,
      maxWidth: `${420 * scale}px`,
      background: 'linear-gradient(90deg, transparent, rgba(255,60,40,0.15) 15%, #ff3c28 50%, rgba(255,60,40,0.15) 85%, transparent)',
      boxShadow: '0 0 12px rgba(255,60,40,0.65)'
    }} />
    {subtitle && (
      <div style={{
        fontFamily: "'Jura', sans-serif",
        fontSize: `${0.72 * scale}rem`,
        letterSpacing: `${0.42 * scale}em`,
        color: '#cdd5df',
        textShadow: '0 1px 4px rgba(0,0,0,0.9)'
      }}>
        {subtitle}
      </div>
    )}
  </div>
);
