import React, { useState } from 'react';

export interface AssetGalleryProps {
  onClose: () => void;
}

export const ASSET_CATALOG = [
  { id: 'SU_GranitMBT', name: 'ОБТ-92 «Гранит»', category: 'unit', tris: 5800, matCount: 3, sockets: ['TurretYaw', 'GunPitch', 'Muzzle', 'SelectionAnchor'] },
  { id: 'SU_BogatyrOreCarrier', name: 'ГРМ-8 «Богатырь»', category: 'unit', tris: 4500, matCount: 3, sockets: ['HarvesterContainer', 'SelectionAnchor'] },
  { id: 'SU_RubezhRifleman', name: 'МС-12 «Рубеж»', category: 'unit', tris: 5900, matCount: 3, sockets: ['AssaultRifle', 'Muzzle', 'SelectionAnchor'] },
  { id: 'SU_HeavyFactory', name: 'Тяжёлый завод', category: 'building', tris: 4600, matCount: 3, sockets: ['SmokeStack1', 'VehicleBay', 'SelectionAnchor'] },
  { id: 'SU_Pillbox', name: 'Пулемётный дот', category: 'building', tris: 4500, matCount: 3, sockets: ['TwinTurret', 'SelectionAnchor'] },
  { id: 'pine_tree_01', name: 'Сосна высокое разрешение', category: 'environment', tris: 3000, matCount: 2, sockets: ['Canopy'] },
  { id: 'coast_rocks_01', name: 'Прибрежная скала', category: 'environment', tris: 1600, matCount: 1, sockets: ['RockMesh'] },
  { id: 'concrete_road_barrier', name: 'Бетонный барьер', category: 'prop', tris: 1600, matCount: 1, sockets: ['BarrierMesh'] },
  { id: 'old_military_crate', name: 'Военный ящик', category: 'prop', tris: 1600, matCount: 1, sockets: ['CrateMesh'] }
];

export const AssetGallery: React.FC<AssetGalleryProps> = ({ onClose }) => {
  const [selectedAssetId, setSelectedAssetId] = useState(ASSET_CATALOG[0].id);
  const [wireframe, setWireframe] = useState(false);
  const [showBoundingBox, setShowBoundingBox] = useState(true);

  const selectedAsset = ASSET_CATALOG.find(a => a.id === selectedAssetId) ?? ASSET_CATALOG[0];

  return (
    <div style={overlayStyle}>
      <div style={sidebarStyle}>
        <h2 style={{ color: '#00ffc8', margin: '0 0 15px 0', fontSize: '20px' }}>🔍 RA4 3D ASSET GALLERY</h2>
        <div style={{ display: 'flex', flexDirection: 'column', gap: '8px', overflowY: 'auto', flex: 1 }}>
          {ASSET_CATALOG.map(asset => (
            <button
              key={asset.id}
              onClick={() => setSelectedAssetId(asset.id)}
              style={{
                textAlign: 'left',
                padding: '10px 14px',
                background: selectedAssetId === asset.id ? 'rgba(0, 255, 200, 0.2)' : 'rgba(255, 255, 255, 0.05)',
                border: selectedAssetId === asset.id ? '1px solid #00ffc8' : '1px solid rgba(255, 255, 255, 0.1)',
                borderRadius: '4px',
                color: '#fff',
                cursor: 'pointer'
              }}
            >
              <div style={{ fontWeight: 'bold', fontSize: '13px' }}>{asset.name}</div>
              <div style={{ fontSize: '11px', color: '#888' }}>{asset.id} ({asset.category})</div>
            </button>
          ))}
        </div>
        <button style={closeBtnStyle} onClick={onClose}>
          ✕ ЗАКРЫТЬ ГАЛЕРЕЮ
        </button>
      </div>

      <div style={detailsPanelStyle}>
        <h3 style={{ color: '#00ffc8', marginTop: 0 }}>Ассемблинг и Спецификация 3D GLB</h3>
        <table style={tableStyle}>
          <tbody>
            <tr><td>ID Ассета:</td><td style={valStyle}>{selectedAsset.id}</td></tr>
            <tr><td>Категория:</td><td style={valStyle}>{selectedAsset.category}</td></tr>
            <tr><td>Треугольники (LOD0):</td><td style={{ ...valStyle, color: '#ffd700' }}>{selectedAsset.tris.toLocaleString()} tris</td></tr>
            <tr><td>Количество материалов:</td><td style={valStyle}>{selectedAsset.matCount} PBR set</td></tr>
            <tr><td>Узлы / Точки привязки (Sockets):</td><td style={valStyle}>{selectedAsset.sockets.join(', ')}</td></tr>
            <tr><td>Статус Лицензии:</td><td style={{ ...valStyle, color: '#00ffc8' }}>APPROVED (CC0 / Custom RA4 PBR)</td></tr>
          </tbody>
        </table>

        <div style={{ marginTop: '20px', display: 'flex', gap: '15px' }}>
          <label style={checkLabelStyle}>
            <input type="checkbox" checked={wireframe} onChange={e => setWireframe(e.target.checked)} />
            Каркасный режим (Wireframe)
          </label>
          <label style={checkLabelStyle}>
            <input type="checkbox" checked={showBoundingBox} onChange={e => setShowBoundingBox(e.target.checked)} />
            Габаритный контейнер (Bounding Box)
          </label>
        </div>
      </div>
    </div>
  );
};

const overlayStyle: React.CSSProperties = {
  position: 'absolute',
  top: 0,
  left: 0,
  width: '100%',
  height: '100%',
  backgroundColor: 'rgba(5, 8, 15, 0.92)',
  display: 'flex',
  zIndex: 3000,
  fontFamily: 'Inter, system-ui, sans-serif'
};

const sidebarStyle: React.CSSProperties = {
  width: '320px',
  backgroundColor: '#0c101a',
  borderRight: '1px solid #1e293b',
  padding: '20px',
  display: 'flex',
  flexDirection: 'column'
};

const detailsPanelStyle: React.CSSProperties = {
  flex: 1,
  padding: '30px',
  color: '#fff'
};

const closeBtnStyle: React.CSSProperties = {
  marginTop: '15px',
  padding: '12px',
  background: '#ff4444',
  color: '#fff',
  border: 'none',
  borderRadius: '4px',
  fontWeight: 'bold',
  cursor: 'pointer'
};

const tableStyle: React.CSSProperties = {
  width: '100%',
  maxWidth: '600px',
  borderCollapse: 'collapse',
  background: 'rgba(255, 255, 255, 0.03)',
  borderRadius: '6px'
};

const valStyle: React.CSSProperties = {
  fontWeight: 'bold',
  padding: '8px 12px'
};

const checkLabelStyle: React.CSSProperties = {
  display: 'flex',
  alignItems: 'center',
  gap: '8px',
  cursor: 'pointer',
  fontSize: '14px'
};
