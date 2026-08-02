import React from 'react';

interface ProgressBarProps {
  progress: number; // 0 to 100
  color?: string; // override theme primary
  label?: string;
  showValue?: boolean;
}

export const ProgressBar: React.FC<ProgressBarProps> = ({
  progress,
  color,
  label,
  showValue = false
}) => {
  const barColor = color || 'var(--theme-primary)';
  const clampedProgress = Math.min(100, Math.max(0, progress));

  return (
    <div className="ra4-progress-container" style={{ width: '100%' }}>
      {(label || showValue) && (
        <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '4px', fontSize: '0.8rem', fontFamily: 'var(--font-secondary)', textTransform: 'uppercase' }}>
          {label && <span>{label}</span>}
          {showValue && <span style={{ color: barColor }}>{Math.round(clampedProgress)}%</span>}
        </div>
      )}
      <div 
        className="ra4-progress-track clip-angled-tl-br" 
        style={{ 
          background: 'rgba(0,0,0,0.5)', 
          height: '12px', 
          width: '100%', 
          border: '1px solid var(--theme-border)',
          position: 'relative'
        }}
      >
        <div 
          className="ra4-progress-fill clip-angled-tl-br" 
          style={{ 
            width: `${clampedProgress}%`, 
            height: '100%', 
            background: barColor,
            boxShadow: `0 0 10px ${barColor}`,
            transition: 'width 0.3s ease'
          }}
        />
      </div>
    </div>
  );
};
