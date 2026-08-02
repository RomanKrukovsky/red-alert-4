import React from 'react';
import classNames from 'classnames';
import './Panel.css';

interface PanelProps extends React.HTMLAttributes<HTMLDivElement> {
  variant?: 'solid' | 'transparent' | 'bordered';
  angled?: boolean;
  glow?: boolean;
}

export const Panel: React.FC<PanelProps> = ({
  children,
  variant = 'solid',
  angled = true,
  glow = false,
  className,
  ...props
}) => {
  const classes = classNames(
    'ra4-panel',
    `ra4-panel--${variant}`,
    {
      'clip-angled-all': angled,
      'glow-box': glow,
    },
    className
  );

  return (
    <div className={classes} {...props}>
      {children}
    </div>
  );
};
