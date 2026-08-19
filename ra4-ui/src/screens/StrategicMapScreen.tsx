import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';

interface MapNode {
  id: string;
  num: string;
  name: string;
  x: number; // percentage
  y: number; // percentage
  stars: number;
  status: 'completed' | 'active' | 'locked';
  description: string;
  rewards: { xp: number; credits: number; tech?: string };
}

const NODES: MapNode[] = [
  { id: 'warsaw', num: '01', name: 'ВАРШАВА', x: 12, y: 37, stars: 3, status: 'completed', description: 'Захват передового командного пункта Альянса в Польше.', rewards: { xp: 500, credits: 3000 } },
  { id: 'berlin', num: '02', name: 'БЕРЛИН', x: 15, y: 51, stars: 3, status: 'completed', description: 'Разгром бронетанкового клина Бундесвера.', rewards: { xp: 800, credits: 5000 } },
  { id: 'baltic', num: '03', name: 'ПРИБАЛТИКА', x: 29, y: 31, stars: 3, status: 'completed', description: 'Освобождение Рижского залива и береговых батарей.', rewards: { xp: 900, credits: 6000 } },
  { id: 'kiev', num: '04', name: 'КИЕВ', x: 30, y: 46, stars: 3, status: 'completed', description: 'Операция «Киев-86»: уничтожение экспедиционного корпуса.', rewards: { xp: 1100, credits: 7500 } },
  { id: 'leningrad', num: '05', name: 'ЛЕНИНГРАД', x: 44, y: 26, stars: 3, status: 'completed', description: 'Снятие блокады Финского залива и запуск верфей.', rewards: { xp: 1200, credits: 8000 } },
  { id: 'stalingrad', num: '06', name: 'СТАЛИНГРАД', x: 55, y: 43, stars: 3, status: 'completed', description: 'Оборона волжского логистического узла.', rewards: { xp: 1300, credits: 9000 } },
  { id: 'caucasus', num: '07', name: 'КАВКАЗ', x: 51, y: 58, stars: 3, status: 'completed', description: 'Зачистка горных перевалов и нефтяных скважин Баку.', rewards: { xp: 1400, credits: 9500 } },
  { id: 'tehran', num: '08', name: 'ТЕГЕРАН', x: 48, y: 78, stars: 2, status: 'completed', description: 'Подавление южного плацдарма коалиции.', rewards: { xp: 1450, credits: 9800 } },
  {
    id: 'hammer',
    num: '09',
    name: 'ОПЕРАЦИЯ «МОЛОТ»',
    x: 37,
    y: 60,
    stars: 0,
    status: 'active',
    description: 'Прорвите оборону NATO и захватите секретный исследовательский комплекс в Новосибирске. Уничтожьте все силы противника в регионе.',
    rewards: { xp: 1500, credits: 10000, tech: 'НОВАЯ ТЕХНОЛОГИЯ «ТЕСЛА-БАШНЯ»' }
  }
];

export const StrategicMapScreen: React.FC = () => {
  const navigate = useNavigate();
  const [selectedNodeId, setSelectedNodeId] = useState('hammer');

  const selectedNode = NODES.find(n => n.id === selectedNodeId) || NODES[8];

  const handleStartMission = () => {
    navigate('/briefing');
  };

  return (
    <div
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('/screenshots/8.png') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '16px 36px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Oswald', sans-serif"
      }}
    >
      {/* Top Resource & Command Strip */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderBottom: '1px solid rgba(255,50,50,0.3)',
        paddingBottom: '10px',
        zIndex: 10
      }}>
        {/* Left Faction Title */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '14px' }}>
          <div style={{
            fontSize: '26px',
            color: '#ff2222',
            filter: 'drop-shadow(0 0 8px rgba(255,0,0,0.8))'
          }}>
            ★
          </div>
          <div>
            <div style={{ color: '#ff2222', fontSize: '18px', fontWeight: 800, letterSpacing: '2px', lineHeight: 1 }}>
              СССР
            </div>
            <div style={{ color: '#aaa', fontSize: '10px', letterSpacing: '1px' }}>
              СЛАВА СОВЕТСКОМУ СОЮЗУ!
            </div>
          </div>
        </div>

        {/* Center Game Title */}
        <div style={{ textAlign: 'center' }}>
          <div style={{ color: '#888', fontSize: '10px', letterSpacing: '3px' }}>COMMAND & CONQUER™</div>
          <div style={{ color: '#ff2222', fontSize: '22px', fontWeight: 800, letterSpacing: '2px', lineHeight: 1 }}>
            RED ALERT 4
          </div>
        </div>

        {/* Right Player Stats & Currencies */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '8px' }}>
            <div style={{
              width: '28px',
              height: '28px',
              borderRadius: '50%',
              background: '#3a0808',
              border: '1px solid #ff3333',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: '#ff2222',
              fontSize: '14px'
            }}>
              ★
            </div>
            <div>
              <div style={{ color: '#fff', fontSize: '12px', fontWeight: 700 }}>ТОВАРИЩ КОМАНДИР</div>
              <div style={{ color: '#ff4444', fontSize: '10px' }}>УРОВЕНЬ 45 ★</div>
            </div>
          </div>

          <div style={{ display: 'flex', gap: '16px', borderLeft: '1px solid rgba(255,255,255,0.1)', paddingLeft: '16px' }}>
            <div style={{ color: '#ffdd00', fontSize: '13px', display: 'flex', alignItems: 'center', gap: '6px' }}>
              <span>💰</span> 125 840
            </div>
            <div style={{ color: '#ff4444', fontSize: '13px', display: 'flex', alignItems: 'center', gap: '6px' }}>
              <span>★</span> 8 450
            </div>
            <div style={{ color: '#00ccff', fontSize: '13px', display: 'flex', alignItems: 'center', gap: '6px' }}>
              <span>📦</span> 1 250
            </div>
          </div>
        </div>
      </div>

      {/* Chapter Subtitle Header */}
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', margin: '8px 0', zIndex: 10 }}>
        <div>
          <div style={{ color: '#ff3333', fontSize: '14px', fontWeight: 700, letterSpacing: '2px' }}>
            ★ КАМПАНИЯ: СССР
          </div>
          <div style={{ color: '#fff', fontSize: '18px', fontWeight: 800 }}>
            ГЛАВА 3: КРАСНЫЙ ШТОРМ
          </div>
        </div>
      </div>

      {/* Tactical Map Overlay Area with Connected Mission Nodes */}
      <div style={{
        position: 'relative',
        flex: 1,
        zIndex: 5,
        display: 'flex',
        alignItems: 'stretch'
      }}>
        {/* SVG connection lines between mission nodes */}
        <svg style={{
          position: 'absolute',
          top: 0,
          left: 0,
          width: '68%',
          height: '100%',
          pointerEvents: 'none',
          zIndex: 1
        }}>
          {/* Connecting lines matching screenshot 8 */}
          <line x1="12%" y1="37%" x2="15%" y2="51%" stroke="#ff2222" strokeWidth="2" strokeDasharray="4 2" opacity="0.7" />
          <line x1="12%" y1="37%" x2="29%" y2="31%" stroke="#ff2222" strokeWidth="2" opacity="0.8" />
          <line x1="15%" y1="51%" x2="30%" y2="46%" stroke="#ff2222" strokeWidth="2" opacity="0.8" />
          <line x1="29%" y1="31%" x2="44%" y2="26%" stroke="#ff2222" strokeWidth="2" opacity="0.8" />
          <line x1="30%" y1="46%" x2="44%" y2="26%" stroke="#ff2222" strokeWidth="1.5" strokeDasharray="3 3" opacity="0.6" />
          <line x1="44%" y1="26%" x2="55%" y2="43%" stroke="#ff2222" strokeWidth="2" opacity="0.8" />
          <line x1="30%" y1="46%" x2="37%" y2="60%" stroke="#ff3333" strokeWidth="2.5" />
          <line x1="55%" y1="43%" x2="51%" y2="58%" stroke="#ff2222" strokeWidth="2" opacity="0.8" />
          <line x1="51%" y1="58%" x2="48%" y2="78%" stroke="#ff2222" strokeWidth="2" strokeDasharray="4 2" opacity="0.8" />
          <line x1="48%" y1="78%" x2="37%" y2="60%" stroke="#ff3333" strokeWidth="2.5" strokeDasharray="3 2" />
        </svg>

        {/* Map Mission Node Markers */}
        <div style={{ position: 'relative', width: '68%', height: '100%', zIndex: 2 }}>
          {NODES.map(node => {
            const isSelected = selectedNodeId === node.id;
            return (
              <div
                key={node.id}
                onClick={() => setSelectedNodeId(node.id)}
                style={{
                  position: 'absolute',
                  left: `${node.x}%`,
                  top: `${node.y}%`,
                  transform: 'translate(-50%, -50%)',
                  cursor: 'pointer',
                  display: 'flex',
                  flexDirection: 'column',
                  alignItems: 'center',
                  gap: '4px',
                  zIndex: isSelected ? 10 : 3
                }}
              >
                {/* Glowing Outer Ring for Active Node */}
                <div style={{
                  width: isSelected ? '46px' : '32px',
                  height: isSelected ? '46px' : '32px',
                  borderRadius: '50%',
                  background: isSelected ? 'rgba(255, 0, 0, 0.4)' : 'rgba(30, 10, 10, 0.85)',
                  border: `2px solid ${isSelected ? '#ff3333' : '#aa2222'}`,
                  boxShadow: isSelected ? '0 0 25px #ff2222, inset 0 0 15px #ff0000' : '0 0 8px rgba(255,0,0,0.4)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                  color: isSelected ? '#ffff00' : '#ff4444',
                  fontSize: isSelected ? '20px' : '14px',
                  transition: 'all 0.2s ease',
                  animation: isSelected ? 'alert-flash 1.5s infinite' : 'none'
                }}>
                  ★
                </div>

                {/* Node Label & Stars */}
                <div style={{
                  background: 'rgba(0,0,0,0.85)',
                  padding: '2px 8px',
                  borderRadius: '3px',
                  border: `1px solid ${isSelected ? '#ff3333' : 'rgba(255,50,50,0.3)'}`,
                  textAlign: 'center',
                  whiteSpace: 'nowrap'
                }}>
                  <div style={{ color: isSelected ? '#ffffff' : '#ccc', fontSize: '11px', fontWeight: 700 }}>
                    {node.num}. {node.name}
                  </div>
                  {node.stars > 0 && (
                    <div style={{ color: '#ffdd00', fontSize: '10px', letterSpacing: '2px' }}>
                      {'★'.repeat(node.stars)}
                    </div>
                  )}
                </div>
              </div>
            );
          })}
        </div>

        {/* Right Mission Detail Card Panel */}
        <div style={{
          width: '32%',
          height: '100%',
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'space-between',
          zIndex: 10
        }}>
          <div className="ra4-panel clip-bevel-md" style={{
            padding: '20px',
            border: '1px solid #ff2222',
            display: 'flex',
            flexDirection: 'column',
            gap: '14px',
            flex: 1
          }}>
            {/* Header */}
            <div style={{ color: '#ff2222', fontSize: '18px', fontWeight: 800, letterSpacing: '1px' }}>
              {selectedNode.num}. {selectedNode.name}
            </div>

            {/* Mission Image Thumbnail */}
            <div style={{
              width: '100%',
              height: '130px',
              borderRadius: '4px',
              border: '1px solid rgba(255,50,50,0.4)',
              background: 'rgba(20,10,10,0.8)',
              overflow: 'hidden',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              position: 'relative'
            }}>
              <div style={{ color: '#ff4444', fontSize: '32px' }}>★</div>
              <div style={{
                position: 'absolute',
                bottom: '6px',
                right: '8px',
                background: 'rgba(0,0,0,0.8)',
                padding: '2px 6px',
                fontSize: '10px',
                color: '#aaa',
                borderRadius: '2px'
              }}>
                РАЙОН БОЕВЫХ ДЕЙСТВИЙ
              </div>
            </div>

            {/* Objective */}
            <div>
              <div style={{ color: '#ff3333', fontSize: '12px', fontWeight: 700, letterSpacing: '1px' }}>
                ЦЕЛЬ МИССИИ:
              </div>
              <div style={{ color: '#d0d0d0', fontSize: '12px', lineHeight: 1.5, marginTop: '4px', fontFamily: "'Inter', sans-serif" }}>
                {selectedNode.description}
              </div>
            </div>

            {/* Rewards */}
            <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px' }}>
              <div style={{ color: '#ff3333', fontSize: '12px', fontWeight: 700, letterSpacing: '1px', marginBottom: '6px' }}>
                НАГРАДЫ:
              </div>
              <div style={{ display: 'flex', gap: '14px', alignItems: 'center' }}>
                <div style={{ display: 'flex', alignItems: 'center', gap: '4px', color: '#ffdd00', fontSize: '12px' }}>
                  <span>★</span> {selectedNode.rewards.xp} XP
                </div>
                <div style={{ display: 'flex', alignItems: 'center', gap: '4px', color: '#ffdd00', fontSize: '12px' }}>
                  <span>💰</span> {selectedNode.rewards.credits}
                </div>
              </div>
              {selectedNode.rewards.tech && (
                <div style={{ color: '#00ffcc', fontSize: '11px', marginTop: '6px', display: 'flex', alignItems: 'center', gap: '6px' }}>
                  <span>⚛</span> {selectedNode.rewards.tech}
                </div>
              )}
            </div>

            {/* Difficulty Selector */}
            <div style={{ borderTop: '1px solid rgba(255,255,255,0.1)', paddingTop: '10px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <span style={{ color: '#aaa', fontSize: '12px' }}>СЛОЖНОСТЬ:</span>
                <span style={{ color: '#ff2222', fontWeight: 700, fontSize: '13px' }}>ВЕТЕРАН ★</span>
              </div>
            </div>

            {/* Launch Button */}
            <button
              onClick={handleStartMission}
              className="clip-bevel-sm"
              style={{
                background: 'linear-gradient(180deg, #ff2222 0%, #7a0b0b 100%)',
                border: '1px solid #ff4444',
                color: '#ffffff',
                padding: '12px',
                fontSize: '16px',
                fontWeight: 800,
                letterSpacing: '2px',
                cursor: 'pointer',
                boxShadow: '0 0 18px rgba(255,0,0,0.8)',
                marginTop: 'auto'
              }}
            >
              ★ НАЧАТЬ МИССИЮ
            </button>
          </div>
        </div>
      </div>

      {/* Bottom Progress Milestones & Navigation */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'center',
        borderTop: '1px solid rgba(255,255,255,0.1)',
        paddingTop: '8px',
        zIndex: 10
      }}>
        {/* Left Navigation */}
        <button
          onClick={() => navigate('/campaign/ussr')}
          className="ra4-btn-ussr clip-bevel-sm"
          style={{ padding: '8px 24px', fontSize: '14px' }}
        >
          ‹ НАЗАД
        </button>

        {/* Center Progress Milestones */}
        <div style={{ display: 'flex', alignItems: 'center', gap: '20px' }}>
          <span style={{ color: '#aaa', fontSize: '12px' }}>ПРОГРЕСС ГЛАВЫ: <strong>8 / 12</strong> МИССИЙ</span>
          <div style={{ display: 'flex', gap: '12px', alignItems: 'center' }}>
            <span style={{ color: '#ffdd00', fontSize: '12px' }}>📦 4 МИССИИ [✓]</span>
            <span style={{ color: '#ffdd00', fontSize: '12px' }}>🎖 8 МИССИЙ [✓]</span>
            <span style={{ color: '#666', fontSize: '12px' }}>🚜 12 МИССИЙ [ ]</span>
          </div>
        </div>

        {/* Right Tools */}
        <div style={{ display: 'flex', gap: '10px' }}>
          <button onClick={() => navigate('/briefing')} className="ra4-btn-ussr clip-bevel-sm" style={{ padding: '6px 14px', fontSize: '12px' }}>
            МИРОВАЯ ОБСТАНОВКА
          </button>
          <button onClick={() => navigate('/video-comms')} className="ra4-btn-ussr clip-bevel-sm" style={{ padding: '6px 14px', fontSize: '12px' }}>
            АРХИВ БРИФИНГОВ
          </button>
        </div>
      </div>
    </div>
  );
};
