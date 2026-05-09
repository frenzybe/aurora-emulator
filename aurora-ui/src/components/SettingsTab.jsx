import React, { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import { motion } from 'framer-motion';
import { Monitor, Trophy, Volume2, Gamepad, Keyboard, ExternalLink, ShieldCheck, Zap, FolderOpen, Check, XCircle, Cpu, Download, RefreshCw } from 'lucide-react';

const EngineStatus = ({ showToast }) => {
  const [status, setStatus] = React.useState([]);
  const [loading, setLoading] = React.useState(true);
  const [downloadProgress, setDownloadProgress] = useState(null);

  const fetchStatus = async () => {
    try {
      const data = await invoke('get_system_status');
      setStatus(data);
    } catch (e) {
      console.error(e);
    } finally {
      setLoading(false);
    }
  };

  React.useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 10000);
    
    let unlisten;
    const setupListener = async () => {
      unlisten = await listen('download-progress', (event) => {
        setDownloadProgress(event.payload);
        if (event.payload.progress === 100) {
          setTimeout(() => setDownloadProgress(null), 3000);
          fetchStatus();
        }
      });
    };
    setupListener();

    return () => {
      clearInterval(interval);
      if (unlisten) unlisten();
    };
  }, []);

  if (loading) return null;

  return (
    <div style={{
      marginBottom: '40px',
      background: 'rgba(255, 255, 255, 0.02)',
      borderRadius: '24px',
      padding: '32px',
      border: '1px solid rgba(255, 255, 255, 0.05)'
    }}>
      <div className="flex-row justify-between align-center mb-xl">
        <label style={{
          display: 'flex',
          alignItems: 'center',
          gap: '12px',
          fontSize: '11px',
          fontWeight: '800',
          letterSpacing: '0.15em',
          color: 'rgba(255, 255, 255, 0.4)',
          textTransform: 'uppercase',
          margin: 0
        }}><Cpu size={14} /> Aurora Engine Status</label>
        <span className="text-xxs opacity-20 font-bold uppercase tracking-widest">Real-time Audit</span>
      </div>

      {/* Progress Overlay */}
      {downloadProgress && (
        <motion.div 
          initial={{ opacity: 0, scale: 0.95 }}
          animate={{ opacity: 1, scale: 1 }}
          exit={{ opacity: 0, scale: 0.95 }}
          style={{
            marginBottom: '24px',
            padding: '20px',
            background: 'rgba(59, 130, 246, 0.05)',
            border: '1px solid rgba(59, 130, 246, 0.2)',
            borderRadius: '16px',
            backdropFilter: 'blur(10px)'
          }}
        >
          <div className="flex-row justify-between align-center mb-s">
            <div className="flex-row align-center gap-s">
              <RefreshCw size={16} className={`text-blue-400 ${downloadProgress.progress < 100 ? 'animate-spin' : ''}`} />
              <div className="flex-col">
                <span className="text-xs font-black text-white">{downloadProgress.status}</span>
                <span className="text-xxs opacity-40 font-bold tracking-tight">{downloadProgress.file}</span>
              </div>
            </div>
            <span className="text-xs font-black text-blue-400">{Math.round(downloadProgress.progress)}%</span>
          </div>
          <div style={{ height: '4px', background: 'rgba(255,255,255,0.05)', borderRadius: '2px', overflow: 'hidden' }}>
            <motion.div 
              initial={{ width: 0 }}
              animate={{ width: `${downloadProgress.progress}%` }}
              style={{ height: '100%', background: 'linear-gradient(90deg, #3b82f6, #60a5fa)', borderRadius: '2px' }}
            />
          </div>
        </motion.div>
      )}

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: '16px' }}>
        {status.map(s => (
          <div key={s.id} className="flex-row justify-between align-center p-m glass-panel" style={{ borderRadius: '16px', background: 'rgba(255,255,255,0.01)' }}>
            <div className="flex-col">
              <span className="text-xs font-black italic tracking-tight">{s.name}</span>
              <div className="flex-row gap-s mt-xs">
                <div className="flex-row align-center gap-xs">
                  {s.core_ok ? <Check size={10} className="text-green-400" /> : <XCircle size={10} className="text-red-400" />}
                  <span className="text-xxs opacity-40 font-bold uppercase tracking-tighter">Core</span>
                </div>
                {s.bios_required && (
                  <div className="flex-row align-center gap-xs">
                    {s.bios_ok ? <Check size={10} className="text-green-400" /> : <XCircle size={10} className="text-red-400" />}
                    <span className="text-xxs opacity-40 font-bold uppercase tracking-tighter">BIOS</span>
                  </div>
                )}
              </div>
            </div>
            {!s.core_ok && (
              <span className="text-xxs font-black px-s py-xs rounded bg-red-500/10 text-red-400/80">SETUP NEEDED</span>
            )}
            {s.core_ok && (!s.bios_required || s.bios_ok) && (
              <Check size={14} className="opacity-20 text-green-400" />
            )}
          </div>
        ))}
      </div>
    </div>
  );
};

const SettingsTab = ({ 
  settings, 
  setSettings, 
  controls, 
  setControls, 
  connectedGamepads, 
  rebinding, 
  setRebinding, 
  mappingPlatform, 
  setMappingPlatform, 
  keyNames, 
  showToast 
}) => {
  const [activeSubTab, setActiveSubTab] = React.useState('general');
  const [activePlayer, setActivePlayer] = React.useState('p1');
  
  const subTabs = [
    { id: 'general', label: 'General', icon: Monitor },
    { id: 'engine', label: 'Engine', icon: Zap },
    { id: 'controls', label: 'Controls', icon: Gamepad },
    { id: 'accounts', label: 'Accounts', icon: Trophy },
  ];

  const sectionStyle = {
    background: 'rgba(255, 255, 255, 0.02)',
    borderRadius: '24px',
    padding: '32px',
    border: '1px solid rgba(255, 255, 255, 0.05)',
    minHeight: '400px'
  };

  const labelStyle = {
    display: 'flex',
    alignItems: 'center',
    gap: '12px',
    fontSize: '11px',
    fontWeight: '800',
    letterSpacing: '0.15em',
    color: 'rgba(255, 255, 255, 0.4)',
    marginBottom: '24px',
    textTransform: 'uppercase'
  };

  const currentInput = activePlayer === 'p1' ? settings.p1_input : settings.p2_input;

  const renderControlCard = (item) => {
    const controlId = currentInput === 'keyboard' ? item.id : item.gp;
    const isRebinding = rebinding && typeof rebinding === 'object' && rebinding.player === activePlayer && rebinding.id === controlId;
    
    if (!controls || !controls[activePlayer]) return null;
    const currentValue = controls[activePlayer][controlId];

    return (
      <div 
        key={item.id}
        onClick={() => setRebinding({ player: activePlayer, id: controlId })}
        className={`transition-all ${isRebinding ? 'active' : ''}`}
        style={{
          background: isRebinding ? 'rgba(255, 255, 255, 0.1)' : 'rgba(255, 255, 255, 0.02)',
          border: '1px solid',
          borderColor: isRebinding ? 'rgba(255, 255, 255, 0.3)' : 'rgba(255, 255, 255, 0.05)',
          borderRadius: '16px',
          padding: '16px 20px',
          cursor: 'pointer',
          display: 'flex',
          justifyContent: 'space-between',
          alignItems: 'center'
        }}
      >
        <span className="text-xxs opacity-40 font-bold flex-1">{item.label}</span>
        <span className="text-xs font-black tracking-widest" style={{ color: isRebinding ? '#fff' : 'rgba(255,255,255,0.8)' }}>
          {isRebinding ? '???' : (currentInput === 'keyboard' ? (keyNames[currentValue] || currentValue) : `BTN ${currentValue}`)}
        </span>
      </div>
    );
  };

  return (
    <motion.div 
      initial={{ opacity: 0, y: 20 }}
      animate={{ opacity: 1, y: 0 }}
      className="settings-container w-full" 
      style={{ maxWidth: '840px', margin: '0 auto', paddingBottom: '64px' }}
    >
      <header className="mb-xl flex-row justify-between align-center">
        <div>
          <h1 className="text-xl font-black italic m-0 tracking-tight">Configuration</h1>
          <p className="text-xxs opacity-30 mt-xs uppercase tracking-widest">Personalize your setup</p>
        </div>
        
        {/* Sub-tab Navigation */}
        <div className="flex-row gap-xs glass-panel p-xs" style={{ borderRadius: '14px', background: 'rgba(255,255,255,0.03)' }}>
          {subTabs.map(tab => (
            <button
              key={tab.id}
              onClick={() => setActiveSubTab(tab.id)}
              style={{
                display: 'flex',
                alignItems: 'center',
                gap: '8px',
                padding: '8px 16px',
                borderRadius: '10px',
                border: 'none',
                background: activeSubTab === tab.id ? 'rgba(255,255,255,0.08)' : 'transparent',
                color: activeSubTab === tab.id ? '#fff' : 'rgba(255,255,255,0.4)',
                fontSize: '10px',
                fontWeight: '800',
                cursor: 'pointer',
                transition: 'all 0.2s'
              }}
            >
              <tab.icon size={12} />
              {tab.label.toUpperCase()}
            </button>
          ))}
        </div>
      </header>

      <div style={sectionStyle}>
        <motion.div
          key={activeSubTab}
          initial={{ opacity: 0, x: 10 }}
          animate={{ opacity: 1, x: 0 }}
          transition={{ duration: 0.2 }}
        >
          {activeSubTab === 'general' && (
            <div className="flex-col gap-xl">
              <section>
                <label style={labelStyle}><Monitor size={14} /> Display & Visuals</label>
                <div className="flex-row gap-m">
                  {['none', 'scanlines', 'crt'].map(s => (
                    <button 
                      key={s} 
                      className="flex-1 transition-all"
                      style={{ 
                        background: settings.shader === s ? '#fff' : 'rgba(255,255,255,0.03)',
                        color: settings.shader === s ? '#000' : '#fff',
                        border: 'none',
                        borderRadius: '16px',
                        padding: '16px',
                        fontWeight: '800',
                        fontSize: '12px',
                        letterSpacing: '0.05em',
                        cursor: 'pointer',
                        boxShadow: settings.shader === s ? '0 10px 30px -10px rgba(255,255,255,0.3)' : 'none'
                      }}
                      onClick={() => setSettings({ ...settings, shader: s })}
                    >
                      {s.toUpperCase()}
                    </button>
                  ))}
                </div>
              </section>

              <section>
                <label style={labelStyle}><Volume2 size={16} /> System Audio</label>
                <div className="flex-row align-center gap-m p-m glass-panel" style={{ borderRadius: '16px' }}>
                  <input 
                    type="range" 
                    min="0" max="100" 
                    className="flex-1"
                    style={{ accentColor: '#fff', cursor: 'pointer' }}
                    value={settings.volume} 
                    onChange={(e) => setSettings({...settings, volume: e.target.value})}
                  />
                  <span className="text-xs font-bold opacity-40 w-8">{settings.volume}%</span>
                </div>
              </section>
            </div>
          )}

          {activeSubTab === 'engine' && (
            <div className="flex-col gap-m">
              <div className="flex-row justify-between align-center mb-m">
                <div className="flex-col">
                  <h3 className="text-xs font-black italic m-0">Library Maintenance</h3>
                  <p className="text-xxs opacity-30 mt-xs uppercase">Keep your cores and bios up to date</p>
                </div>
                <div className="flex-row gap-s">
                  <button
                    onClick={async (e) => {
                      const btn = e.currentTarget;
                      btn.disabled = true;
                      showToast('BIOS', 'Downloading BIOS Pack...', 'info');
                      try {
                        await invoke('download_bios_pack');
                        showToast('Success', 'BIOS Pack installed!', 'success');
                      } catch (e) {
                        showToast('Error', e.toString(), 'error');
                      } finally {
                        btn.disabled = false;
                      }
                    }}
                    className="btn-secondary flex-row align-center gap-s"
                    style={{ padding: '8px 16px', borderRadius: '10px', fontSize: '9px', background: 'rgba(255,255,255,0.05)' }}
                  >
                    <Download size={12} /> BIOS
                  </button>
                  <button
                    onClick={async () => {
                      showToast('Cores', 'Updating all cores...', 'info');
                      try {
                        await invoke('download_cores');
                        showToast('Success', 'Cores updated!', 'success');
                      } catch (e) {
                        showToast('Error', e.toString(), 'error');
                      }
                    }}
                    className="btn-secondary flex-row align-center gap-s"
                    style={{ padding: '8px 16px', borderRadius: '10px', fontSize: '9px', background: 'rgba(255,255,255,0.05)' }}
                  >
                    <Zap size={12} /> CORES
                  </button>
                </div>
              </div>
              <EngineStatus showToast={showToast} />
            </div>
          )}

          {activeSubTab === 'accounts' && (
            <div className="flex-col gap-m">
              <div className="flex-row justify-between align-center mb-m">
                <label style={{ ...labelStyle, marginBottom: 0 }}><Trophy size={14} /> RetroAchievements</label>
                <div className="flex-row align-center gap-xs text-xxs opacity-40">
                  <ShieldCheck size={12} /> SECURE
                </div>
              </div>
              
              <div className="flex-col gap-l glass-panel p-m" style={{ borderRadius: '20px' }}>
                <div>
                  <div className="flex-row justify-between align-center mb-s">
                    <label className="text-xxs opacity-30 uppercase tracking-widest block m-0">Web API Key</label>
                    <a href="https://retroachievements.org/controlpanel.php" target="_blank" rel="noreferrer" className="text-xxs opacity-40 hover:opacity-100 transition-opacity flex-row align-center gap-xs">
                      GET KEY <ExternalLink size={10} />
                    </a>
                  </div>
                  <div className="flex-row gap-s">
                    <input
                      type="password"
                      className="flex-1 input-dark"
                      placeholder="API Key..."
                      value={settings.ra_key || ''}
                      onChange={(e) => setSettings({ ...settings, ra_key: e.target.value })}
                    />
                    <button
                      className="btn-action"
                      style={{ padding: '0 20px', borderRadius: '12px', fontSize: '10px' }}
                      onClick={async () => {
                        if (!settings.ra_key) {
                          showToast('Input Required', 'API Key needed', 'error');
                          return;
                        }
                        showToast('Connection', 'Verifying...', 'info');
                        try {
                          const user = settings.ra_user || 'test';
                          const url = `https://retroachievements.org/API/API_GetUserSummary.php?z=${user}&y=${encodeURIComponent(settings.ra_key)}&u=${user}`;
                          const res = await invoke('ra_proxy', { url });
                          if (res && (res.Success !== false && res.success !== false)) {
                            showToast('Success', 'Verified!', 'success');
                          } else {
                            showToast('API Error', res.Message || "Invalid Key", 'error');
                          }
                        } catch (e) {
                          showToast('Network Error', e.toString(), 'error');
                        }
                      }}
                    >
                      VERIFY
                    </button>
                  </div>
                </div>

                <div>
                  <label className="text-xxs opacity-30 uppercase tracking-widest block mb-s">Game Password</label>
                  <input
                    type="password"
                    className="w-full input-dark"
                    placeholder="RA Token..."
                    value={settings.ra_password || ''}
                    onChange={(e) => setSettings({ ...settings, ra_password: e.target.value })}
                  />
                </div>
                
                <p className="text-xxs opacity-20 m-0 leading-relaxed italic">
                  * Credentials are encrypted and stored locally.
                </p>
              </div>
            </div>
          )}

          {activeSubTab === 'controls' && (
            <div className="flex-col gap-m">
              {/* Gamepad Status Bar */}
              {connectedGamepads && connectedGamepads.length > 0 && (
                <div className="flex-row gap-s mb-m p-s glass-panel" style={{ background: 'rgba(59, 130, 246, 0.1)', borderRadius: '12px' }}>
                  <Gamepad size={14} className="text-blue-400" />
                  <div className="flex-row gap-m overflow-hidden">
                    {connectedGamepads.map((gp, idx) => (
                      <span key={idx} className="text-xxs font-bold text-blue-300 whitespace-nowrap opacity-80">
                        {idx}: {gp.id.split(' (')[0]}
                      </span>
                    ))}
                  </div>
                </div>
              )}

              <div className="flex-row justify-between align-center mb-m">
                <div className="flex-row gap-s">
                  {['p1', 'p2'].map(p => (
                    <button 
                      key={p}
                      onClick={() => setActivePlayer(p)}
                      className={`nav-pill ${activePlayer === p ? 'active' : ''}`}
                    >
                      PLAYER {p.toUpperCase().slice(1)}
                    </button>
                  ))}
                </div>
                
                <div className="flex-row gap-xs glass-panel p-xs" style={{ borderRadius: '12px', background: 'rgba(0,0,0,0.2)' }}>
                  <button 
                    onClick={() => setSettings({...settings, [`${activePlayer}_input`]: 'keyboard', [`${activePlayer}_input_manual`]: true})}
                    className={`nav-pill text-xxs ${currentInput === 'keyboard' ? 'active' : ''}`}
                  >
                    KEYBOARD
                  </button>
                  <button 
                    onClick={() => setSettings({...settings, [`${activePlayer}_input`]: 'gamepad', [`${activePlayer}_input_manual`]: true})}
                    className={`nav-pill text-xxs ${currentInput === 'gamepad' ? 'active' : ''}`}
                  >
                    GAMEPAD
                  </button>
                </div>
              </div>

              <div className="flex-col gap-l">
                <div>
                  <div className="text-xxs opacity-20 font-black tracking-widest mb-m">D-PAD</div>
                  <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: '10px' }}>
                    {[{ id: 'up', label: 'UP', gp: 'gp_up' }, { id: 'down', label: 'DOWN', gp: 'gp_down' },
                      { id: 'left', label: 'LEFT', gp: 'gp_left' }, { id: 'right', label: 'RIGHT', gp: 'gp_right' }].map(renderControlCard)}
                  </div>
                </div>

                <div>
                  <div className="text-xxs opacity-20 font-black tracking-widest mb-m">ACTIONS</div>
                  <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: '10px' }}>
                    {[{ id: 'a', label: 'A', gp: 'gp_a' }, { id: 'b', label: 'B', gp: 'gp_b' },
                      { id: 'x', label: 'X', gp: 'gp_x' }, { id: 'y', label: 'Y', gp: 'gp_y' }].map(renderControlCard)}
                  </div>
                </div>

                <div>
                  <div className="text-xxs opacity-20 font-black tracking-widest mb-m">SYSTEM</div>
                  <div style={{ display: 'grid', gridTemplateColumns: 'repeat(2, 1fr)', gap: '10px' }}>
                    {[{ id: 'start', label: 'START', gp: 'gp_start' }, { id: 'select', label: 'SELECT', gp: 'gp_select' }].map(renderControlCard)}
                  </div>
                </div>
              </div>
            </div>
          )}
        </motion.div>
      </div>
    </motion.div>
  );
};

export default SettingsTab;
