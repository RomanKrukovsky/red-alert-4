import React, { useState } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';

interface BuildItem {
  id: string;
  name: string;
  cost: number;
  icon: string;
  category: string;
  timeSec: number;
}

export const InGameHUD: React.FC = () => {
  const [searchParams, setSearchParams] = useSearchParams();
  const navigate = useNavigate();
  const mode = searchParams.get('mode') || 'ussr-base';

  // Mode determines background screenshot, faction aesthetic, and layout variant
  const modeConfig = {
    'ussr-base': {
      screenshot: '/screenshots/13.png',
      faction: 'ussr',
      factionName: 'СССР',
      title: 'ГЛАВНЫЙ ШТАБ',
      subtitle: 'Здание управления',
      accentColor: '#ff2222',
      themeClass: 'theme-ussr',
      hp: '5000 / 5000',
      hpPercent: 100,
      badge: '★',
      commander: 'Товарищ Командир',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '88 / 200'
    },
    'allies-airfield': {
      screenshot: '/screenshots/14.png',
      faction: 'allies',
      factionName: 'АЛЬЯНС',
      title: 'АЭРОДРОМ',
      subtitle: 'Производство авиации Альянса',
      accentColor: '#0088ff',
      themeClass: 'theme-allies',
      hp: '2500 / 2500',
      hpPercent: 100,
      badge: '🦅',
      commander: 'Президент Элеанор Уорд',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '128 / 200'
    },
    'ec-base': {
      screenshot: '/screenshots/15.png',
      faction: 'ec',
      factionName: 'ВОСТОЧНАЯ КОАЛИЦИЯ',
      title: 'ЦЕНТРАЛЬНЫЙ КОМПЛЕКС',
      subtitle: 'Главное строение коалиции',
      accentColor: '#00ff66',
      themeClass: 'theme-ec',
      hp: '4500 / 4500',
      hpPercent: 100,
      badge: '🐉',
      commander: 'Верховный Генерал Гао',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '116 / 200'
    },
    'chrono-base': {
      screenshot: '/screenshots/16.png',
      faction: 'chrono',
      factionName: 'ХРОНОЛЕГИОН',
      title: 'ГЛАВНЫЙ ХРОНОРЕАКТОР',
      subtitle: 'Сердце темпоральной базы',
      accentColor: '#aa00ff',
      themeClass: 'theme-chrono',
      hp: '5000 / 5000',
      hpPercent: 100,
      badge: '⏳',
      commander: 'Главнокомандующий Алексей',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '172 / 200'
    },
    'ussr-tank-assault': {
      screenshot: '/screenshots/20.png',
      faction: 'ussr',
      factionName: 'СССР',
      title: 'ТЯЖЁЛЫЙ ТАНК КВ-3',
      subtitle: 'Роль: штурмовой танк',
      accentColor: '#ff2222',
      themeClass: 'theme-ussr',
      hp: '1850 / 1850',
      hpPercent: 100,
      badge: '★',
      commander: 'Товарищ Командир',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '186 / 200'
    },
    'ussr-base-defense': {
      screenshot: '/screenshots/21.png',
      faction: 'ussr',
      factionName: 'СССР',
      title: 'КОМАНДНЫЙ ЦЕНТР',
      subtitle: 'База под массированной атакой!',
      accentColor: '#ff2222',
      themeClass: 'theme-ussr',
      hp: '2500 / 2500',
      hpPercent: 85,
      badge: '★',
      commander: 'Товарищ Командир',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '88 / 200'
    },
    'allies-naval': {
      screenshot: '/screenshots/22.png',
      faction: 'allies',
      factionName: 'АЛЬЯНС',
      title: 'ЭСМИНЕЦ «СВОБОДА»',
      subtitle: 'Эсминец класса «Арли Бёрк»',
      accentColor: '#0088ff',
      themeClass: 'theme-allies',
      hp: '3600 / 3600',
      hpPercent: 100,
      badge: '🦅',
      commander: 'Президент Элеанор Уорд',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '116 / 200'
    },
    'allies-air-battle': {
      screenshot: '/screenshots/23.png',
      faction: 'allies',
      factionName: 'АЛЬЯНС',
      title: 'ЭСКАДРИЛЬЯ «ОРЁЛ»',
      subtitle: 'Многоцелевой истребитель завоевания господства',
      accentColor: '#0088ff',
      themeClass: 'theme-allies',
      hp: '600 / 600',
      hpPercent: 100,
      badge: '🦅',
      commander: 'Президент Элеанор Уорд',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '128 / 200'
    },
    'chrono-superweapon': {
      screenshot: '/screenshots/24.png',
      faction: 'chrono',
      factionName: 'ХРОНОЛЕГИОН',
      title: 'ХРОНОКОЛЛАПС «ВЕЧНОСТЬ»',
      subtitle: 'СУПЕРОРУЖИЕ АКТИВИРОВАНО!',
      accentColor: '#aa00ff',
      themeClass: 'theme-chrono',
      hp: '5000 / 5000',
      hpPercent: 100,
      badge: '⏳',
      commander: 'Главнокомандующий Алексей',
      credits: '23 450',
      energy: '17 820',
      tech: '9 680',
      pop: '172 / 200'
    }
  }[mode] || {
    screenshot: '/screenshots/13.png',
    faction: 'ussr',
    factionName: 'СССР',
    title: 'ГЛАВНЫЙ ШТАБ',
    subtitle: 'Здание управления',
    accentColor: '#ff2222',
    themeClass: 'theme-ussr',
    hp: '5000 / 5000',
    hpPercent: 100,
    badge: '★',
    commander: 'Товарищ Командир',
    credits: '23 450',
    energy: '17 820',
    tech: '9 680',
    pop: '88 / 200'
  };

  // State for sidebar tabs and production queue
  const [activeTab, setActiveTab] = useState('СТРОИТЬ');
  const [productionQueue, setProductionQueue] = useState([
    { id: '1', name: 'ТАНК Т-34', time: '00:12', progress: 65 },
    { id: '2', name: 'ШТУРМОВИКИ', time: '00:08', progress: 0 }
  ]);
  const [selectedGroup, setSelectedGroup] = useState<number | null>(1);
  const [superweaponReady, setSuperweaponReady] = useState(true);

  // Production cards list
  const buildItems: BuildItem[] = [
    { id: 'b1', name: 'ЭНЕРГОСТАНЦИЯ', cost: 800, icon: '⚡', category: 'СТРОИТЬ', timeSec: 10 },
    { id: 'b2', name: 'БАРАКИ', cost: 600, icon: '🏠', category: 'СТРОИТЬ', timeSec: 8 },
    { id: 'b3', name: 'ВОЕННЫЙ ЗАВОД', cost: 2000, icon: '🏭', category: 'СТРОИТЬ', timeSec: 25 },
    { id: 'b4', name: 'ОЧИСТИТЕЛЬ РУДЫ', cost: 1500, icon: '⛏', category: 'СТРОИТЬ', timeSec: 18 },
    { id: 'b5', name: 'ЗЕНИТНАЯ ПУШКА', cost: 900, icon: '🎯', category: 'СТРОИТЬ', timeSec: 12 },
    { id: 'b6', name: 'РАКЕТНАЯ ШАХТА', cost: 1200, icon: '🚀', category: 'СТРОИТЬ', timeSec: 20 },
    { id: 'u1', name: 'ШТУРМОВИКИ', cost: 300, icon: '💂', category: 'ВОЙСКА', timeSec: 6 },
    { id: 'u2', name: 'ТАНК Т-34', cost: 900, icon: '🚜', category: 'ВОЙСКА', timeSec: 14 },
    { id: 'u3', name: 'ТЯЖ. ТАНК КВ-3', cost: 1400, icon: '🛡', category: 'ВОЙСКА', timeSec: 22 },
    { id: 'u4', name: 'РСЗО «СМЕРЧ»', cost: 1100, icon: '☄', category: 'ВОЙСКА', timeSec: 16 },
    { id: 'u5', name: 'ТЕСЛА-ТАНК', cost: 1200, icon: '⚡', category: 'ВОЙСКА', timeSec: 18 },
    { id: 'u6', name: 'МАМОНТ МК2', cost: 2200, icon: '🐘', category: 'ВОЙСКА', timeSec: 30 }
  ];

  const handleQueueItem = (item: BuildItem) => {
    setProductionQueue(q => [...q, { id: Date.now().toString(), name: item.name, time: `00:${item.timeSec}`, progress: 0 }]);
  };

  const handleRemoveQueueItem = (id: string) => {
    setProductionQueue(q => q.filter(item => item.id !== id));
  };

  return (
    <div
      className={modeConfig.themeClass}
      style={{
        width: '100vw',
        height: '100vh',
        position: 'relative',
        background: `url('${modeConfig.screenshot}') no-repeat center center`,
        backgroundSize: 'cover',
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'space-between',
        padding: '12px 20px',
        boxSizing: 'border-box',
        overflow: 'hidden',
        fontFamily: "'Oswald', sans-serif"
      }}
    >
      {/* =========================================================================
          TOP STRIP: Commander Profile, Objectives/Alerts, Resources, Minimap
          ========================================================================= */}
      <div style={{
        display: 'flex',
        justifyContent: 'space-between',
        alignItems: 'flex-start',
        zIndex: 20
      }}>
        {/* Top-Left: Player & Mission Objectives */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', width: '340px' }}>
          {/* Commander Card */}
          <div className="ra4-panel clip-bevel-sm" style={{
            padding: '6px 14px',
            display: 'flex',
            alignItems: 'center',
            gap: '12px',
            border: `1px solid ${modeConfig.accentColor}`
          }}>
            <div style={{
              width: '32px',
              height: '32px',
              borderRadius: '4px',
              background: 'rgba(0,0,0,0.6)',
              border: `1px solid ${modeConfig.accentColor}`,
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: modeConfig.accentColor,
              fontSize: '18px'
            }}>
              {modeConfig.badge}
            </div>
            <div>
              <div style={{ color: modeConfig.accentColor, fontSize: '14px', fontWeight: 800 }}>
                {modeConfig.commander}
              </div>
              <div style={{ color: '#aaa', fontSize: '10px' }}>УРОВЕНЬ 45 ★ ВЕТЕРАН</div>
            </div>
          </div>

          {/* Objectives Card */}
          <div className="ra4-panel clip-bevel-sm" style={{
            padding: '10px 14px',
            border: '1px solid rgba(255,255,255,0.15)',
            fontSize: '11px',
            color: '#ddd'
          }}>
            <div style={{ color: modeConfig.accentColor, fontWeight: 700, letterSpacing: '1px', marginBottom: '4px' }}>
              ОСНОВНЫЕ ЗАДАЧИ
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '3px', fontFamily: "'Inter', sans-serif" }}>
              <div style={{ color: '#00ff66' }}>☑ Уничтожить базу противника</div>
              <div>☐ Захватить Хранилище ресурсов</div>
              <div>☐ Отразить воздушный налёт</div>
            </div>
          </div>

          {/* Tactical Alert Notification (Conditional on mode) */}
          {(mode === 'ussr-base' || mode === 'ussr-base-defense') && (
            <div className="alert-pulse clip-bevel-sm" style={{
              padding: '6px 12px',
              color: '#ffffff',
              fontSize: '12px',
              fontWeight: 800,
              letterSpacing: '1px',
              display: 'flex',
              alignItems: 'center',
              gap: '8px',
              boxShadow: '0 0 15px rgba(255,0,0,0.8)'
            }}>
              <span>🚨</span>
              <span>НАШУ БАЗУ АТАКУЮТ!</span>
            </div>
          )}

          {mode === 'chrono-superweapon' && (
            <div className="clip-bevel-sm" style={{
              background: 'linear-gradient(90deg, #aa00ff 0%, #440066 100%)',
              border: '1px solid #cc44ff',
              padding: '6px 12px',
              color: '#ffffff',
              fontSize: '12px',
              fontWeight: 800,
              display: 'flex',
              alignItems: 'center',
              gap: '8px',
              boxShadow: '0 0 20px rgba(170,0,255,0.8)'
            }}>
              <span>⚡</span>
              <span>СУПЕРОРУЖИЕ АКТИВИРОВАНО!</span>
            </div>
          )}
        </div>

        {/* Center Alert/Superweapon Banner (Conditional) */}
        {mode === 'ussr-base-defense' && (
          <div className="alert-pulse clip-bevel-md" style={{
            padding: '10px 40px',
            border: '2px solid #ff2222',
            textAlign: 'center',
            color: '#ffffff',
            boxShadow: '0 0 30px rgba(255,0,0,0.9)'
          }}>
            <div style={{ fontSize: '18px', fontWeight: 900, letterSpacing: '3px' }}>
              ТРЕВОГА!
            </div>
            <div style={{ fontSize: '12px', letterSpacing: '1px', fontFamily: "'Inter', sans-serif" }}>
              База подвергается атаке противника! Разверните комплексы ПВО.
            </div>
          </div>
        )}

        {mode === 'chrono-superweapon' && (
          <div className="clip-bevel-md" style={{
            background: 'rgba(20, 5, 30, 0.92)',
            border: '2px solid #cc44ff',
            padding: '10px 40px',
            textAlign: 'center',
            color: '#ffffff',
            boxShadow: '0 0 30px rgba(170,0,255,0.9)'
          }}>
            <div style={{ color: '#cc44ff', fontSize: '18px', fontWeight: 900, letterSpacing: '3px' }}>
              ХРОНОКОЛЛАПС «ВЕЧНОСТЬ»
            </div>
            <div style={{ color: '#00ffcc', fontSize: '12px', letterSpacing: '2px', fontWeight: 700 }}>
              ЦЕЛЬ ПОРАЖЕНА
            </div>
          </div>
        )}

        {/* Top-Right: Resource Counters Strip */}
        <div className="ra4-panel clip-bevel-sm" style={{
          padding: '8px 20px',
          display: 'flex',
          gap: '24px',
          alignItems: 'center',
          border: `1px solid ${modeConfig.accentColor}`
        }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: '6px', color: '#ffdd00', fontSize: '15px', fontWeight: 700 }}>
            <span>💰</span> {modeConfig.credits}
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '6px', color: '#00ffcc', fontSize: '15px', fontWeight: 700 }}>
            <span>⚡</span> {modeConfig.energy}
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '6px', color: '#00ccff', fontSize: '15px', fontWeight: 700 }}>
            <span>⚛</span> {modeConfig.tech}
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '6px', color: '#ff8800', fontSize: '15px', fontWeight: 700 }}>
            <span>👥</span> {modeConfig.pop}
          </div>
          <button
            onClick={() => navigate('/menu')}
            style={{
              background: 'rgba(50,20,20,0.6)',
              border: '1px solid rgba(255,255,255,0.2)',
              color: '#fff',
              padding: '4px 8px',
              borderRadius: '3px',
              cursor: 'pointer',
              fontSize: '12px'
            }}
          >
            ⚙
          </button>
        </div>
      </div>

      {/* =========================================================================
          BOTTOM HUD: Unit Inspector, Command Grid, Production Sidebar
          ========================================================================= */}
      <div style={{
        display: 'grid',
        gridTemplateColumns: '380px 1fr 340px',
        gap: '16px',
        alignItems: 'flex-end',
        zIndex: 20
      }}>
        {/* Bottom-Left: Selected Unit / Building Inspector */}
        <div className="ra4-panel clip-bevel-md" style={{
          padding: '16px',
          border: `1px solid ${modeConfig.accentColor}`,
          display: 'flex',
          flexDirection: 'column',
          gap: '10px',
          background: 'rgba(15, 8, 12, 0.95)'
        }}>
          {/* Header & HP */}
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
            <div>
              <div style={{ color: modeConfig.accentColor, fontSize: '16px', fontWeight: 800, letterSpacing: '1px' }}>
                {modeConfig.title}
              </div>
              <div style={{ color: '#aaa', fontSize: '11px' }}>
                {modeConfig.subtitle}
              </div>
            </div>
            <span style={{ color: modeConfig.accentColor, fontSize: '24px' }}>{modeConfig.badge}</span>
          </div>

          {/* Health Bar */}
          <div>
            <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '10px', color: '#aaa', marginBottom: '2px' }}>
              <span>ПРОЧНОСТЬ</span>
              <strong style={{ color: '#00ff66' }}>{modeConfig.hp}</strong>
            </div>
            <div style={{
              width: '100%',
              height: '8px',
              background: 'rgba(0,0,0,0.8)',
              borderRadius: '2px',
              overflow: 'hidden',
              border: '1px solid rgba(255,255,255,0.15)'
            }}>
              <div style={{
                width: `${modeConfig.hpPercent}%`,
                height: '100%',
                background: 'linear-gradient(90deg, #00ff66, #ffcc00)'
              }} />
            </div>
          </div>

          {/* Unit / Ability Hotkey Buttons (Q, W, E, R) */}
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(4, 1fr)', gap: '6px', marginTop: '4px' }}>
            {['Q', 'W', 'E', 'R'].map((hk, i) => (
              <button
                key={hk}
                className="clip-bevel-sm"
                style={{
                  background: 'linear-gradient(180deg, rgba(40,15,15,0.9) 0%, rgba(15,5,5,0.9) 100%)',
                  border: `1px solid ${modeConfig.accentColor}`,
                  color: '#fff',
                  padding: '6px 0',
                  fontSize: '11px',
                  fontWeight: 700,
                  cursor: 'pointer'
                }}
              >
                [{hk}] {i === 0 ? 'АТАКА' : (i === 1 ? 'ЩИТ' : (i === 2 ? 'РЕМОНТ' : 'НАВЫК'))}
              </button>
            ))}
          </div>

          {/* Control Group Quick Buttons (1..0) */}
          <div style={{ display: 'flex', gap: '3px', marginTop: '2px' }}>
            {[1, 2, 3, 4, 5, 6, 7, 8, 9, 0].map(grp => (
              <div
                key={grp}
                onClick={() => setSelectedGroup(grp)}
                style={{
                  flex: 1,
                  textAlign: 'center',
                  background: selectedGroup === grp ? modeConfig.accentColor : 'rgba(20,20,20,0.8)',
                  color: selectedGroup === grp ? '#000' : '#888',
                  fontSize: '10px',
                  fontWeight: 700,
                  padding: '2px 0',
                  borderRadius: '2px',
                  cursor: 'pointer',
                  border: `1px solid ${selectedGroup === grp ? '#fff' : 'rgba(255,255,255,0.1)'}`
                }}
              >
                {grp}
              </div>
            ))}
          </div>
        </div>

        {/* Bottom-Center: Production Queue & Command Bar Grid */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '10px', alignItems: 'center' }}>
          {/* Active Production Queue Card */}
          <div className="ra4-panel clip-bevel-md" style={{
            width: '100%',
            padding: '10px 16px',
            border: `1px solid ${modeConfig.accentColor}`,
            background: 'rgba(15, 8, 12, 0.95)'
          }}>
            <div style={{ color: modeConfig.accentColor, fontSize: '11px', fontWeight: 700, letterSpacing: '1.5px', marginBottom: '6px' }}>
              ОЧЕРЕДЬ ПРОИЗВОДСТВА ({productionQueue.length})
            </div>
            <div style={{ display: 'flex', gap: '8px', overflowX: 'auto' }}>
              {productionQueue.map((item, idx) => (
                <div
                  key={item.id}
                  style={{
                    flex: 1,
                    background: 'rgba(30,10,15,0.8)',
                    border: '1px solid rgba(255,255,255,0.15)',
                    borderRadius: '4px',
                    padding: '4px 8px',
                    display: 'flex',
                    flexDirection: 'column',
                    justifyContent: 'space-between'
                  }}
                >
                  <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '11px' }}>
                    <span style={{ color: '#fff', fontWeight: 700 }}>{idx + 1}. {item.name}</span>
                    <span
                      onClick={() => handleRemoveQueueItem(item.id)}
                      style={{ color: '#ff3333', cursor: 'pointer', fontWeight: 800 }}
                    >
                      ✕
                    </span>
                  </div>
                  <div style={{
                    width: '100%',
                    height: '4px',
                    background: 'rgba(0,0,0,0.8)',
                    borderRadius: '2px',
                    marginTop: '4px',
                    overflow: 'hidden'
                  }}>
                    <div style={{
                      width: `${item.progress > 0 ? item.progress : 25}%`,
                      height: '100%',
                      background: modeConfig.accentColor
                    }} />
                  </div>
                </div>
              ))}
            </div>
          </div>

          {/* Command Buttons Strip (Move, Attack, Shield, Stop, Scatter, Deploy) */}
          <div style={{ display: 'flex', gap: '6px' }}>
            {[
              { key: 'Q', label: 'ДВИЖЕНИЕ', icon: '⮞' },
              { key: 'W', label: 'АТАКА', icon: '⚔' },
              { key: 'E', label: 'ОБОРОНА', icon: '🛡' },
              { key: 'R', label: 'СТОП', icon: '🛑' },
              { key: 'T', label: 'ПАТРУЛЬ', icon: '🔄' },
              { key: 'A', label: 'РАССРЕДОТОЧИТЬ', icon: '↔' },
              { key: 'S', label: 'РАЗВЕРНУТЬ', icon: '⚓' }
            ].map(cmd => (
              <button
                key={cmd.key}
                className="clip-bevel-sm"
                style={{
                  background: 'linear-gradient(180deg, #251012 0%, #120608 100%)',
                  border: `1px solid ${modeConfig.accentColor}`,
                  color: '#ffffff',
                  padding: '8px 12px',
                  fontSize: '11px',
                  fontWeight: 700,
                  cursor: 'pointer',
                  display: 'flex',
                  flexDirection: 'column',
                  alignItems: 'center',
                  gap: '2px'
                }}
              >
                <span style={{ fontSize: '14px' }}>{cmd.icon}</span>
                <span>[{cmd.key}] {cmd.label}</span>
              </button>
            ))}
          </div>
        </div>

        {/* Bottom-Right: Production Sidebar Tabs & Build Grid */}
        <div className="ra4-panel clip-bevel-md" style={{
          padding: '14px',
          border: `1px solid ${modeConfig.accentColor}`,
          display: 'flex',
          flexDirection: 'column',
          gap: '10px',
          background: 'rgba(15, 8, 12, 0.95)',
          maxHeight: '360px'
        }}>
          {/* Tabs */}
          <div style={{ display: 'flex', borderBottom: '1px solid rgba(255,255,255,0.15)', paddingBottom: '6px' }}>
            {['СТРОИТЬ', 'ВОЙСКА', 'УЛУЧШЕНИЯ', 'ДОКТРИНЫ'].map(tab => (
              <div
                key={tab}
                onClick={() => setActiveTab(tab)}
                style={{
                  flex: 1,
                  textAlign: 'center',
                  fontSize: '11px',
                  fontWeight: 700,
                  color: activeTab === tab ? modeConfig.accentColor : '#888',
                  cursor: 'pointer',
                  borderBottom: activeTab === tab ? `2px solid ${modeConfig.accentColor}` : 'none'
                }}
              >
                {tab}
              </div>
            ))}
          </div>

          {/* Build Grid Cards */}
          <div style={{
            display: 'grid',
            gridTemplateColumns: '1fr 1fr',
            gap: '8px',
            overflowY: 'auto',
            maxHeight: '240px',
            paddingRight: '4px'
          }}>
            {buildItems
              .filter(item => item.category === activeTab || (activeTab !== 'СТРОИТЬ' && activeTab !== 'ВОЙСКА'))
              .map(item => (
                <div
                  key={item.id}
                  onClick={() => handleQueueItem(item)}
                  className="clip-bevel-sm"
                  style={{
                    background: 'linear-gradient(180deg, rgba(30,15,18,0.9) 0%, rgba(15,5,8,0.9) 100%)',
                    border: `1px solid ${modeConfig.accentColor}`,
                    padding: '8px',
                    display: 'flex',
                    flexDirection: 'column',
                    justifyContent: 'space-between',
                    height: '60px',
                    cursor: 'pointer',
                    position: 'relative'
                  }}
                >
                  <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                    <span style={{ fontSize: '16px' }}>{item.icon}</span>
                    <span style={{ color: '#ffdd00', fontSize: '11px', fontWeight: 700 }}>💰 {item.cost}</span>
                  </div>
                  <div style={{ color: '#ffffff', fontSize: '11px', fontWeight: 700, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>
                    {item.name}
                  </div>
                </div>
              ))}
          </div>
        </div>
      </div>
    </div>
  );
};
