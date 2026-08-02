import React from 'react';
import { useUIStore } from '../store.js';

export interface FrameProps extends React.HTMLAttributes<HTMLDivElement> {
  variant?: 'panel' | 'beveled' | 'modal' | 'minimap';
  withGlow?: boolean;
}

export const Frame: React.FC<FrameProps> = ({
  children,
  variant = 'panel',
  withGlow = false,
  className = '',
  ...props
}) => {
  const { theme } = useUIStore();

  const baseClasses = `ra4-frame ra4-frame-${variant} ${withGlow ? 'ra4-frame-glow' : ''} ${className}`;

  return (
    <div className={baseClasses} {...props}>
      <div className="ra4-frame-border-top" />
      <div className="ra4-frame-border-right" />
      <div className="ra4-frame-border-bottom" />
      <div className="ra4-frame-border-left" />
      <div className="ra4-frame-content">
        {children}
      </div>
    </div>
  );
};
