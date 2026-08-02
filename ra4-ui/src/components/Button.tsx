import React from 'react';
import { useUIStore } from '../store.js';

export interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: 'primary' | 'secondary' | 'metallic' | 'holographic' | 'ghost';
  size?: 'sm' | 'md' | 'lg';
  active?: boolean;
}

export const Button: React.FC<ButtonProps> = ({ 
  children, 
  variant = 'primary', 
  size = 'md', 
  active = false,
  className = '', 
  ...props 
}) => {
  const { theme } = useUIStore();
  
  // Base classes that apply layout, typography and effects via tokens
  const baseClasses = `ra4-button ra4-button-${variant} ra4-button-${size} ${active ? 'active' : ''} ${className}`;
  
  return (
    <button className={baseClasses} {...props}>
      <span className="ra4-button-inner">
        {children}
      </span>
      {variant === 'primary' && <div className="ra4-button-glow" />}
    </button>
  );
};
