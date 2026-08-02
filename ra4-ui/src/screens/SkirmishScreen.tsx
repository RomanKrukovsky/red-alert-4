import React from 'react';
import { useNavigate } from 'react-router-dom';
import { Button } from '../components/Button';
import './SkirmishScreen.css';

export const SkirmishScreen: React.FC = () => {
  const navigate = useNavigate();

  const players = [
    { id: 1, name: 'SOKOLOV_1945', faction: 'СССР', color: '#ff0000', team: '1', ready: true },
    { id: 2, name: 'Allied_Command', faction: 'АЛЬЯНС', color: '#0055ff', team: '1', ready: true },
    { id: 3, name: 'Dragon_Warlord', faction: 'ВОСТОЧНАЯ КОАЛИЦИЯ', color: '#00aa00', team: '2', ready: true },
    { id: 4, name: 'ChronoLegionnaire', faction: 'ХРОНОЛЕГИОН', color: '#8800ff', team: '2', ready: true },
    { id: 5, name: 'RedOctober', faction: 'СССР', color: '#ff5500', team: '3', ready: true },
    { id: 6, name: 'SkyEagle', faction: 'АЛЬЯНС', color: '#00ccff', team: '3', ready: true },
    { id: 7, name: 'JadeTiger', faction: 'ВОСТОЧНАЯ КОАЛИЦИЯ', color: '#ffff00', team: '4', ready: true },
    { id: 8, name: 'TimeWalker', faction: 'ХРОНОЛЕГИОН', color: '#ff00ff', team: '4', ready: true },
  ];

  return (
    <div className="ra4-skirmish-screen theme-ussr">
      <div className="ra4-skirmish-header">
        <div style={{ position: 'absolute', left: 0, color: '#aaa', fontSize: '14px' }}>ЛОББИ СЕТЕВОГО МАТЧА</div>
        <div className="ra4-skirmish-title">COMMAND & CONQUER™ RED ALERT 4</div>
      </div>

      <div className="ra4-skirmish-content">
        
        {/* LEFT PANEL */}
        <div className="ra4-skirmish-panel ra4-skirmish-left">
          <div style={{ textAlign: 'center', marginBottom: '20px' }}>
            <div style={{ fontSize: '80px', color: 'var(--theme-primary)', filter: 'drop-shadow(0 0 10px red)' }}>★</div>
          </div>
          
          <div>
            <div className="ra4-skirmish-section-title">РЕЖИМ ИГРЫ</div>
            <div style={{ color: '#fff', fontSize: '16px', marginBottom: '15px' }}>СХВАТКА</div>
          </div>

          <div>
            <div className="ra4-skirmish-section-title">ПОБЕДНЫЕ УСЛОВИЯ</div>
            <div style={{ color: '#fff', fontSize: '14px', marginBottom: '15px' }}>УНИЧТОЖИТЬ ВСЕХ ПРОТИВНИКОВ</div>
          </div>

          <div>
            <div className="ra4-skirmish-section-title">НАСТРОЙКИ ЛОББИ</div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">ДРУЖЕСКИЙ ОГОНЬ</span>
              <span className="ra4-skirmish-setting-value">ВЫКЛ.</span>
            </div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">ОГРАНИЧЕНИЕ ВРЕМЕНИ</span>
              <span className="ra4-skirmish-setting-value">60 МИН.</span>
            </div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">ЛИМИТ ПО ОЧКАМ</span>
              <span className="ra4-skirmish-setting-value">НЕТ</span>
            </div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">НАБЛЮДАТЕЛИ</span>
              <span className="ra4-skirmish-setting-value">ВКЛ.</span>
            </div>
          </div>

          <div style={{ marginTop: 'auto', display: 'flex', flexDirection: 'column', gap: '10px' }}>
             <Button variant="primary" style={{ width: '100%' }}>ПРИГЛАСИТЬ ДРУГА</Button>
             <Button variant="secondary" style={{ width: '100%' }} onClick={() => navigate('/menu')}>ПОКИНУТЬ ЛОББИ</Button>
          </div>
        </div>

        {/* CENTER PANEL */}
        <div className="ra4-skirmish-panel ra4-skirmish-center">
          <div className="ra4-player-header">
            <div>#</div>
            <div>ИГРОК</div>
            <div>ФРАКЦИЯ</div>
            <div>ЦВЕТ</div>
            <div>КОМАНДА</div>
            <div>ГОТОВНОСТЬ</div>
          </div>
          
          <div className="ra4-player-grid">
            {players.map((p, idx) => (
              <div key={p.id} className="ra4-player-row">
                <div style={{ color: 'var(--theme-primary)', fontWeight: 'bold' }}>{idx + 1}</div>
                <div style={{ color: '#fff' }}>{p.name}</div>
                <div>{p.faction}</div>
                <div><div className="ra4-color-box" style={{ background: p.color }}></div></div>
                <div style={{ textAlign: 'center' }}>{p.team}</div>
                <div className="ra4-ready-icon">✓ ГОТОВ</div>
              </div>
            ))}
          </div>

          <div className="ra4-chat-box">
             <div style={{ color: '#ff5555' }}>SOKOLOV_1945: Всем удачи. За Родину!</div>
             <div style={{ color: '#66b3ff' }}>Allied_Command: For freedom!</div>
             <div style={{ color: '#55ff55' }}>Dragon_Warlord: Честь и традиции.</div>
          </div>
          
          <div className="ra4-chat-input-wrapper">
             <input type="text" className="ra4-chat-input" placeholder="НАПИСАТЬ СООБЩЕНИЕ..." />
             <Button variant="primary">ОТПРАВИТЬ</Button>
          </div>
        </div>

        {/* RIGHT PANEL */}
        <div className="ra4-skirmish-panel ra4-skirmish-right">
          <div>
            <div className="ra4-skirmish-section-title">КАРТА</div>
            <div style={{ fontSize: '16px', color: '#fff', marginBottom: '10px' }}>АЛЯСКА - ХОЛОДНАЯ ВЕРШИНА</div>
            <div className="ra4-map-preview clip-angled-tl-br" style={{ backgroundImage: `url('/screenshots/8.png')`, backgroundSize: 'cover' }}></div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">РАЗМЕР КАРТЫ</span>
              <span className="ra4-skirmish-setting-value">БОЛЬШАЯ (8 ИГРОКОВ)</span>
            </div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">ТИП ЛАНДШАФТА</span>
              <span className="ra4-skirmish-setting-value">ЗИМНИЙ</span>
            </div>
          </div>

          <div style={{ marginTop: '20px' }}>
            <div className="ra4-skirmish-section-title">НАСТРОЙКИ МАТЧА</div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">НАЧАЛЬНЫЕ РЕСУРСЫ</span>
              <span className="ra4-skirmish-setting-value">СРЕДНИЕ</span>
            </div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">СКОРОСТЬ ИГРЫ</span>
              <span className="ra4-skirmish-setting-value">НОРМАЛЬНО</span>
            </div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">ТЕХНОЛОГИИ</span>
              <span className="ra4-skirmish-setting-value">ВСЕ ВКЛ.</span>
            </div>
            <div className="ra4-skirmish-setting-row">
              <span className="ra4-skirmish-setting-label">СУПЕРОРУЖИЕ</span>
              <span className="ra4-skirmish-setting-value">ВКЛ.</span>
            </div>
          </div>
        </div>
      </div>

      <div className="ra4-skirmish-actions">
         <Button variant="primary" size="lg" onClick={() => navigate('/ingame/ussr')}>
            НАЧАТЬ БИТВУ
         </Button>
         <Button variant="secondary" size="lg">
            НАБЛЮДАТЬ
         </Button>
      </div>
    </div>
  );
};
