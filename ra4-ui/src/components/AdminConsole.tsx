import React, { useState, useEffect, useRef } from 'react';
import { useUIStore } from '../store.js';

export interface AdminConsoleProps {
  onExecuteCommand: (cmd: string) => Promise<{ command: string; output: string; status: 'SUCCESS' | 'ERROR' | 'INFO' }>;
  onGetAutocomplete: (prefix: string) => string[];
  onClose: () => void;
}

export const AdminConsole: React.FC<AdminConsoleProps> = ({ onExecuteCommand, onGetAutocomplete, onClose }) => {
  const { consoleOpen, adminUser } = useUIStore();
  const [inputVal, setInputVal] = useState('');
  const [logHistory, setLogHistory] = useState<{ id: string; timestamp: string; command: string; output: string; status: string }[]>([
    { id: '0', timestamp: new Date().toLocaleTimeString(), command: 'auth check', output: 'Серверный токен администратора подтвержден [ROLE: ADMIN].', status: 'SUCCESS' }
  ]);
  const [historyIndex, setHistoryIndex] = useState(-1);
  const [commandHistory, setCommandHistory] = useState<string[]>([]);
  const [suggestions, setSuggestions] = useState<string[]>([]);
  const inputRef = useRef<HTMLInputElement>(null);
  const logEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (consoleOpen && inputRef.current) {
      inputRef.current.focus();
    }
  }, [consoleOpen]);

  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [logHistory]);

  if (!consoleOpen) return null;

  const handleKeyDown = async (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      if (!inputVal.trim()) return;
      const cmdToRun = inputVal;
      
      // Add a loading entry immediately
      const loadingId = Math.random().toString();
      setLogHistory(prev => [...prev, { id: loadingId, timestamp: new Date().toLocaleTimeString(), command: cmdToRun, output: 'Ожидание ответа...', status: 'INFO' }]);
      setCommandHistory(prev => [...prev, cmdToRun]);
      setHistoryIndex(-1);
      setInputVal('');
      setSuggestions([]);

      try {
        const res = await onExecuteCommand(cmdToRun);
        // Replace the loading entry with the actual result
        setLogHistory(prev => prev.map(log => log.id === loadingId ? {
          ...log,
          output: res.output,
          status: res.status
        } : log));
      } catch (err) {
        setLogHistory(prev => prev.map(log => log.id === loadingId ? {
          ...log,
          output: 'Ошибка выполнения команды',
          status: 'ERROR'
        } : log));
      }
    } else if (e.key === 'Tab') {
      e.preventDefault();
      const options = onGetAutocomplete(inputVal);
      if (options.length > 0) {
        setInputVal(options[0] + ' ');
        setSuggestions(options);
      }
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      if (commandHistory.length === 0) return;
      const nextIdx = historyIndex === -1 ? commandHistory.length - 1 : Math.max(0, historyIndex - 1);
      setHistoryIndex(nextIdx);
      setInputVal(commandHistory[nextIdx]);
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      if (historyIndex === -1) return;
      const nextIdx = historyIndex + 1;
      if (nextIdx >= commandHistory.length) {
        setHistoryIndex(-1);
        setInputVal('');
      } else {
        setHistoryIndex(nextIdx);
        setInputVal(commandHistory[nextIdx]);
      }
    } else if (e.key === 'Escape' || e.key === '`' || e.key === '~') {
      e.preventDefault();
      onClose();
    }
  };

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const val = e.target.value;
    setInputVal(val);
    if (val.trim()) {
      setSuggestions(onGetAutocomplete(val));
    } else {
      setSuggestions([]);
    }
  };

  return (
    <div style={{
      position: 'absolute',
      top: 0,
      left: 0,
      width: '100%',
      height: '45vh',
      backgroundColor: 'rgba(10, 15, 25, 0.92)',
      borderBottom: '2px solid #00ffc8',
      boxShadow: '0 10px 30px rgba(0, 255, 200, 0.3)',
      backdropFilter: 'blur(12px)',
      zIndex: 9999,
      display: 'flex',
      flexDirection: 'column',
      fontFamily: "'Chakra Petch', monospace",
      color: '#e0f7fc',
      padding: '12px 20px',
      boxSizing: 'border-box'
    }}>
      {/* Header Bar */}
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '8px', borderBottom: '1px solid rgba(0, 255, 200, 0.2)', paddingBottom: '6px' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px' }}>
          <span style={{ backgroundColor: '#00ffc8', color: '#05101a', padding: '2px 8px', fontWeight: 'bold', borderRadius: '2px', fontSize: '0.8rem' }}>
            [RA4 ADMIN CONSOLE v2.0]
          </span>
          <span style={{ fontSize: '0.85rem', color: '#80e5ff' }}>
            Пользователь: <strong style={{ color: '#00ffc8' }}>{adminUser?.nickname || 'Админ'}</strong> | Статус: <span style={{ color: '#00ffc8' }}>АВТОРИЗОВАН (SERVER-TOKEN VALIDATED)</span>
          </span>
        </div>
        <button
          onClick={onClose}
          style={{
            background: 'none',
            border: '1px solid #ff2a4b',
            color: '#ff2a4b',
            cursor: 'pointer',
            padding: '2px 10px',
            fontFamily: 'inherit',
            fontWeight: 'bold',
            fontSize: '0.8rem'
          }}
        >
          [ ЗАКРЫТЬ ~ ]
        </button>
      </div>

      {/* Terminal Log Area */}
      <div style={{
        flex: 1,
        overflowY: 'auto',
        backgroundColor: 'rgba(0, 0, 0, 0.4)',
        border: '1px solid rgba(0, 255, 200, 0.2)',
        padding: '10px',
        marginBottom: '8px',
        fontSize: '0.85rem',
        lineHeight: '1.4'
      }}>
        {logHistory.map((item) => (
          <div key={item.id} style={{ marginBottom: '6px' }}>
            <span style={{ color: '#5090a0', marginRight: '8px' }}>[{item.timestamp}]</span>
            <span style={{ color: '#00ffc8', fontWeight: 'bold' }}>&gt; {item.command}</span>
            <pre style={{ margin: '2px 0 0 16px', color: item.status === 'ERROR' ? '#ff2a4b' : item.status === 'INFO' ? '#80e5ff' : '#e0f7fc', whiteSpace: 'pre-wrap', fontFamily: 'inherit' }}>
              {item.output}
            </pre>
          </div>
        ))}
        <div ref={logEndRef} />
      </div>

      {/* Autocomplete Bar */}
      {suggestions.length > 0 && (
        <div style={{ display: 'flex', gap: '8px', marginBottom: '4px', fontSize: '0.8rem', color: '#80e5ff' }}>
          <span>Автодополнение [Tab]:</span>
          {suggestions.map((s, idx) => (
            <span key={idx} style={{ backgroundColor: 'rgba(0, 255, 200, 0.2)', padding: '0 6px', border: '1px solid #00ffc8', color: '#00ffc8' }}>
              {s}
            </span>
          ))}
        </div>
      )}

      {/* Input Field */}
      <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
        <span style={{ color: '#00ffc8', fontWeight: 'bold', fontSize: '1.1rem' }}>&gt;</span>
        <input
          ref={inputRef}
          type="text"
          value={inputVal}
          onChange={handleInputChange}
          onKeyDown={handleKeyDown}
          placeholder="Введите команду админа (help, spawn, give, god, fog, kill, teleport, win, lose, ai, fps)..."
          style={{
            flex: 1,
            backgroundColor: 'rgba(0, 255, 200, 0.05)',
            border: '1px solid #00ffc8',
            color: '#ffffff',
            padding: '8px 12px',
            fontFamily: 'inherit',
            fontSize: '0.9rem',
            outline: 'none'
          }}
        />
      </div>
    </div>
  );
};
