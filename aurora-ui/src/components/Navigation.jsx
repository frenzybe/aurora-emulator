import React from 'react';
import { Gamepad2, Trophy, Settings, Plus, User, Wifi, WifiOff, Search } from 'lucide-react';

const Navigation = ({ activeTab, setActiveTab, connectedGamepads, onAddGame, settings, isOnline, profile }) => {
  return (
    <nav className="top-nav flex-row justify-between align-center">
      <div className="flex-row align-center gap-m">
        <div className="logo-text">AURORA</div>
        <div className="flex-col gap-xs">
          {['p1', 'p2'].map((player, idx) => {
            const inputMode = settings[`${player}_input`];
            const connectedGp = connectedGamepads[idx];
            const isDisconnected = inputMode === 'gamepad' && !connectedGp;
            
            const cleanName = (id) => {
              if (!id) return 'NONE';
              const name = id.toLowerCase();
              if (name.includes('dualsense') || name.includes('dualshock')) return 'DUALSENSE';
              if (name.includes('xbox')) return 'XBOX';
              if (name.includes('nintendo') || name.includes('pro controller')) return 'PRO CONTROLLER';
              return id.split(' ')[0].toUpperCase();
            };

            const deviceName = inputMode === 'gamepad' 
              ? (connectedGp ? cleanName(connectedGp.id) : 'WAKE UP GP') 
              : 'KEYBOARD';

            return (
              <div 
                key={player} 
                className={`gamepad-capsule glass-panel ${isDisconnected ? 'gp-disconnected' : ''}`}
                onClick={() => {
                  if (isDisconnected) {
                    window.dispatchEvent(new Event('gamepadconnected'));
                  }
                }}
                style={{ cursor: isDisconnected ? 'pointer' : 'default' }}
              >
                <span className="gamepad-label">P{idx + 1}</span>
                <span className={`gamepad-name ${isDisconnected ? 'animate-pulse text-red-400' : ''}`} style={{ opacity: isDisconnected ? 0.6 : 0.8 }}>
                  {deviceName}
                </span>
              </div>
            );
          })}
        </div>
      </div>
      
      <div className="flex-row gap-s" style={{ position: 'absolute', left: '50%', transform: 'translateX(-50%)' }}>
        <button 
          className={`nav-pill flex-row align-center gap-s ${activeTab === 'library' ? 'active' : ''}`} 
          onClick={() => setActiveTab('library')}
        >
          <Gamepad2 size={16} /> Library
        </button>
        <button 
          className={`nav-pill flex-row align-center gap-s ${activeTab === 'trophies' ? 'active' : ''}`} 
          onClick={() => setActiveTab('trophies')}
        >
          <Trophy size={16} /> Trophies
        </button>
        <button 
          className={`nav-pill flex-row align-center gap-s ${activeTab === 'settings' ? 'active' : ''}`} 
          onClick={() => setActiveTab('settings')}
        >
          <Settings size={16} /> Settings
        </button>
      </div>

      <div className="flex-row gap-m align-center">
        <button className="btn-add flex-row align-center gap-s" onClick={onAddGame}>
          <Plus size={16} /> Add Game
        </button>

        {/* Network Status */}
        <div 
          className={`flex-row align-center justify-center glass-panel ${!isOnline ? 'offline-pulse' : ''}`} 
          style={{ 
            width: '32px',
            height: '32px',
            borderRadius: '50%', 
            background: 'rgba(255, 255, 255, 0.03)',
            border: isOnline ? '1px solid rgba(255, 255, 255, 0.08)' : '1px solid rgba(255, 50, 50, 0.3)',
            color: isOnline ? '#fff' : '#ff4d4d',
            opacity: isOnline ? 0.8 : 1,
            transition: 'all 0.3s ease',
            boxShadow: !isOnline ? '0 0 10px rgba(255, 50, 50, 0.2)' : 'none'
          }}
          title={isOnline ? 'Online' : 'Offline - Check Connection'}
        >
          {isOnline ? <Wifi size={14} /> : <WifiOff size={14} />}
        </div>

        {/* Profile Avatar Button */}
        <div 
          className={`profile-avatar ${activeTab === 'account' ? 'pulse-border' : ''}`}
          style={{
            width: '40px', 
            height: '40px', 
            borderRadius: '50%', 
            cursor: 'pointer',
            overflow: 'hidden',
            border: activeTab === 'account' ? '2px solid rgba(255,255,255,0.8)' : '2px solid rgba(255,255,255,0.1)',
            transition: 'all 0.3s ease',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            background: 'rgba(0, 0, 0, 0.3)'
          }}
          onClick={() => setActiveTab('account')}
        >
          {settings?.ra_user && settings?.ra_key ? (
            <img 
              src={`https://media.retroachievements.org/UserPic/${settings.ra_user}.png`} 
              onError={(e) => { e.target.onerror = null; e.target.src = 'https://media.retroachievements.org/UserPic/RetroAchievements.png'; }}
              style={{ width: '100%', height: '100%', objectFit: 'cover' }} 
              alt="Profile" 
            />
          ) : (
            <User size={18} style={{ color: 'rgba(255,255,255,0.5)' }} />
          )}
        </div>
      </div>
    </nav>
  );
};

export default Navigation;
