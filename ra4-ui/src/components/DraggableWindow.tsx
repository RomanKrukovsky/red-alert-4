import React, { useEffect, useRef, useState } from 'react';
import { useWindowStore } from '../windowStore.js';
import { useUIStore } from '../store.js';

interface DraggableWindowProps {
  id: string;
  title: string;
  initialX?: number;
  initialY?: number;
  width?: number;
  height?: number;
  children: React.ReactNode;
}

export const DraggableWindow: React.FC<DraggableWindowProps> = ({ id, title, initialX = 100, initialY = 100, width, height, children }) => {
  const registerWindow = useWindowStore((s) => s.registerWindow);
  const updatePosition = useWindowStore((s) => s.updatePosition);
  const bringToFront = useWindowStore((s) => s.bringToFront);
  const setMinimized = useWindowStore((s) => s.setMinimized);
  const setClosed = useWindowStore((s) => s.setClosed);
  
  const windowState = useWindowStore((s) => s.windows[id]);
  const theme = useUIStore((s) => s.theme);
  
  const headerRef = useRef<HTMLDivElement>(null);
  const [isDragging, setIsDragging] = useState(false);
  const [dragOffset, setDragOffset] = useState({ x: 0, y: 0 });

  useEffect(() => {
    registerWindow(id, { id, title, x: initialX, y: initialY, width, height, minimized: false, closed: false });
  }, [id, title, initialX, initialY, width, height, registerWindow]);

  useEffect(() => {
    const handleMouseMove = (e: MouseEvent) => {
      if (isDragging && windowState) {
        updatePosition(id, e.clientX - dragOffset.x, e.clientY - dragOffset.y);
      }
    };
    
    const handleMouseUp = () => {
      setIsDragging(false);
    };

    if (isDragging) {
      document.addEventListener('mousemove', handleMouseMove);
      document.addEventListener('mouseup', handleMouseUp);
    }
    return () => {
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };
  }, [isDragging, dragOffset, id, windowState, updatePosition]);

  if (!windowState || windowState.closed) return null;

  return (
    <div 
      className={`ra4-window ${theme}`} 
      onPointerDown={() => bringToFront(id)}
      style={{
        position: 'absolute',
        left: windowState.x,
        top: windowState.y,
        zIndex: windowState.zIndex,
        width: windowState.width,
        height: windowState.height,
        display: windowState.minimized ? 'none' : 'flex',
        flexDirection: 'column',
        pointerEvents: 'auto',
      }}
    >
      <div 
        ref={headerRef}
        className="ra4-window-header"
        onPointerDown={(e) => {
          setIsDragging(true);
          setDragOffset({
            x: e.clientX - windowState.x,
            y: e.clientY - windowState.y
          });
          bringToFront(id);
          e.stopPropagation();
        }}
      >
        <span className="ra4-window-title">{title}</span>
        <div className="ra4-window-controls">
          <button className="ra4-window-btn ra4-window-minimize" onClick={() => setMinimized(id, true)}>_</button>
          <button className="ra4-window-btn ra4-window-close" onClick={() => setClosed(id, true)}>X</button>
        </div>
      </div>
      <div className="ra4-window-content">
        {children}
      </div>
    </div>
  );
};
