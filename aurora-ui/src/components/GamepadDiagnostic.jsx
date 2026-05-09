import React from 'react';

const GamepadDiagnostic = ({ mappingPlatform, setMappingPlatform, rebinding, setRebinding, controls, keyNames }) => {
  return (
    <div className="flex-col gap-m mb-l animate-fadeUp">
      <div className="flex-row justify-between align-center mb-m">
        <div className="flex-col">
          <label className="text-xs font-bold tracking-widest text-accent mb-xs">HARDWARE MAPPING INTERFACE</label>
          <span className="text-xxs opacity-30 uppercase letter-spacing-1">Interactive Controller Diagnostics v3.0</span>
        </div>

        <div className="flex-row gap-xs glass-panel p-xs" style={{ borderRadius: '12px', background: 'rgba(255,255,255,0.03)', border: '1px solid rgba(255,255,255,0.1)' }}>
          {['genesis', 'nes', 'snes'].map(p => (
            <button
              key={p}
              className={`nav-pill text-xxs ${mappingPlatform === p ? 'active' : ''}`}
              onClick={() => setMappingPlatform(p)}
              style={{
                padding: '8px 16px',
                background: mappingPlatform === p ? 'var(--text-primary)' : 'transparent',
                color: mappingPlatform === p ? '#000' : 'var(--text-primary)',
              }}
            >
              {p.toUpperCase()}
            </button>
          ))}
        </div>
      </div>

      <div className="glass-panel relative flex-col align-center justify-center py-xl overflow-hidden"
        style={{
          minHeight: '500px',
          background: 'radial-gradient(circle at center, rgba(20,20,20,1) 0%, rgba(5,5,5,1) 100%)',
          border: '1px solid rgba(255,255,255,0.05)',
          boxShadow: '0 30px 100px rgba(0,0,0,0.8)'
        }}>

        <div className="relative z-10" style={{ width: '100%', maxWidth: '650px', filter: 'drop-shadow(0 40px 80px rgba(0,0,0,0.9))' }}>
          <svg viewBox="0 0 600 300" fill="none" xmlns="http://www.w3.org/2000/svg">
            <defs>
              <linearGradient id="plasticGrad" x1="0%" y1="0%" x2="0%" y2="100%">
                <stop offset="0%" stopColor="#3a3a3a" />
                <stop offset="50%" stopColor="#2a2a2a" />
                <stop offset="100%" stopColor="#1a1a1a" />
              </linearGradient>
            </defs>

            {mappingPlatform === 'genesis' && (
              <g className="animate-fadeUp">
                <path d="M50 150C50 70 140 40 300 40C460 40 550 70 550 150C550 230 460 260 300 260C140 260 50 230 50 150Z" fill="url(#plasticGrad)" stroke="#111" strokeWidth="3" />
                <path d="M65 150C65 85 145 55 300 55C455 55 535 85 535 150C535 215 455 245 300 245C145 245 65 215 65 150Z" fill="#1e1e1e" opacity="0.5" />
                <circle cx="300" cy="140" r="75" fill="#151515" stroke="#000" strokeWidth="2" />
                <text x="300" y="90" textAnchor="middle" fill="rgba(255,255,255,0.1)" fontSize="10" fontWeight="900">16-BIT CONTROL PAD</text>
                <g transform="translate(140, 150)">
                  <circle r="42" fill="#0c0c0c" stroke="#222" />
                  <rect x="-8" y="-38" width="16" height="30" rx="2" fill={rebinding === 'gp_up' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_up')} style={{ cursor: 'pointer' }} />
                  <rect x="-8" y="8" width="16" height="30" rx="2" fill={rebinding === 'gp_down' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_down')} style={{ cursor: 'pointer' }} />
                  <rect x="-38" y="-8" width="30" height="16" rx="2" fill={rebinding === 'gp_left' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_left')} style={{ cursor: 'pointer' }} />
                  <rect x="8" y="-8" width="30" height="16" rx="2" fill={rebinding === 'gp_right' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_right')} style={{ cursor: 'pointer' }} />
                </g>
                {[
                  { id: 'gp_a', cx: 400, cy: 190, label: 'A' },
                  { id: 'gp_b', cx: 450, cy: 160, label: 'B' },
                  { id: 'gp_c', cx: 500, cy: 130, label: 'C' }
                ].map(btn => (
                  <g key={btn.id} onClick={() => setRebinding(btn.id)} style={{ cursor: 'pointer' }}>
                    <circle cx={btn.cx} cy={btn.cy} r="20" fill={rebinding === btn.id ? 'var(--accent)' : '#111'} stroke="#000" strokeWidth="2" />
                    <text x={btn.cx} y={btn.cy + 6} textAnchor="middle" fill={rebinding === btn.id ? '#000' : '#fff'} fontSize="14" fontWeight="900">{btn.label}</text>
                    <text x={btn.cx} y={btn.cy + 40} fill="#fff" fontSize="9" fontWeight="900" textAnchor="middle" opacity="0.6">{keyNames[controls[btn.id]]}</text>
                  </g>
                ))}
                <g transform="translate(300, 175)" onClick={() => setRebinding('gp_start')} style={{ cursor: 'pointer' }}>
                  <rect x="-20" y="-6" width="40" height="12" rx="6" fill={rebinding === 'gp_start' ? '#ff4d4d' : '#331111'} stroke="#000" />
                  <text x="0" y="22" textAnchor="middle" fill="#ff4d4d" fontSize="7" fontWeight="bold">START</text>
                </g>
              </g>
            )}

            {(mappingPlatform === 'nes' || mappingPlatform === 'snes') && (
               <g className="animate-fadeUp">
                  <rect x="100" y="60" width="400" height="180" rx="20" fill={mappingPlatform === 'nes' ? '#ccc' : '#ddd'} stroke="#999" strokeWidth="3" />
                  <rect x="120" y="80" width="360" height="140" rx="10" fill="#333" />
                  
                  {/* Common buttons for NES/SNES style */}
                  <g transform="translate(180, 150)">
                    <rect x="-10" y="-40" width="20" height="30" fill={rebinding === 'gp_up' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_up')} />
                    <rect x="-10" y="10" width="20" height="30" fill={rebinding === 'gp_down' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_down')} />
                    <rect x="-40" y="-10" width="30" height="20" fill={rebinding === 'gp_left' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_left')} />
                    <rect x="10" y="-10" width="30" height="20" fill={rebinding === 'gp_right' ? 'var(--accent)' : '#222'} onClick={() => setRebinding('gp_right')} />
                  </g>

                  <g transform="translate(420, 150)">
                    <circle cx="30" cy="0" r="20" fill={rebinding === 'gp_a' ? 'var(--accent)' : '#ff4d4d'} onClick={() => setRebinding('gp_a')} />
                    <circle cx="-20" cy="30" r="20" fill={rebinding === 'gp_b' ? 'var(--accent)' : '#ff4d4d'} onClick={() => setRebinding('gp_b')} />
                    <text x="30" y="40" textAnchor="middle" fill="#fff" fontSize="10">A: {keyNames[controls['gp_a']]}</text>
                    <text x="-20" y="70" textAnchor="middle" fill="#fff" fontSize="10">B: {keyNames[controls['gp_b']]}</text>
                  </g>
               </g>
            )}
          </svg>
        </div>
      </div>
    </div>
  );
};

export default GamepadDiagnostic;
