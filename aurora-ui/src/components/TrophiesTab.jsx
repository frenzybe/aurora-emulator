import { Trophy, WifiOff, RefreshCw } from 'lucide-react';
import { motion } from 'framer-motion';
import Skeleton from './Skeleton';

const TrophyCard = ({ ach, index }) => (
  <motion.div
    initial={{ opacity: 0, x: -20 }}
    animate={{ opacity: 1, x: 0 }}
    transition={{ delay: index * 0.05 }}
    className={`trophy-card ${ach.DateEarned ? 'earned' : 'locked'}`}
  >
    <div className="trophy-badge-wrapper">
      <img
        src={`https://media.retroachievements.org/Badge/${ach.BadgeName}.png`}
        alt={ach.Title}
        className="trophy-badge"
      />
      {ach.DateEarned && <div className="trophy-flare" />}
    </div>
    <div className="trophy-info">
      <h4 className="text-xs font-bold mb-xs">{ach.Title}</h4>
      <p className="text-xxs opacity-50 leading-tight mb-s">{ach.Description}</p>
      <div className="flex-row justify-between align-center mt-auto">
        <span className="points-tag">{ach.Points} PTS</span>
        {ach.DateEarned && <span className="date-tag">{new Date(ach.DateEarned).toLocaleDateString()}</span>}
      </div>
    </div>
  </motion.div>
);

const TrophiesTab = ({ selectedGame, achievements, raProgress, raStatus, isLoggedIn, setActiveTab, isOnline, checkNetwork }) => {
  if (!isOnline) {
    return (
      <motion.div 
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        className="flex-col align-center justify-center w-full"
        style={{ flex: 1 }}
      >
        <div className="glass-panel flex-col align-center text-center" style={{ width: '100%', maxWidth: '480px', borderRadius: '32px', background: 'rgba(10, 10, 12, 0.4)', border: '1px solid rgba(255, 255, 255, 0.05)', backdropFilter: 'blur(40px)', padding: '64px 40px', position: 'relative', overflow: 'hidden' }}>
          
          <div style={{ position: 'absolute', top: '-10%', left: '-10%', width: '120%', height: '120%', background: 'radial-gradient(circle at center, rgba(255,255,255,0.03) 0%, transparent 70%)', zIndex: 0 }} />

          <div className="mb-xl" style={{ position: 'relative', zIndex: 1 }}>
            <div style={{ width: '80px', height: '80px', borderRadius: '24px', background: 'rgba(255,255,255,0.03)', border: '1px solid rgba(255,255,255,0.08)', display: 'flex', alignItems: 'center', justifyContent: 'center', margin: '0 auto', boxShadow: '0 10px 30px rgba(0,0,0,0.2)' }}>
              <WifiOff size={32} style={{ color: 'rgba(255,255,255,0.4)' }} />
            </div>
          </div>
          
          <h2 className="m-0 font-bold mb-s tracking-tight" style={{ color: '#fff', fontSize: '28px', position: 'relative', zIndex: 1 }}>Connection Lost</h2>
          <p className="opacity-40 mb-xl font-medium" style={{ fontSize: '15px', lineHeight: '1.6', maxWidth: '300px', position: 'relative', zIndex: 1 }}>
            Trophies are currently hidden because the network is unavailable.
          </p>

          <button 
            onClick={checkNetwork}
            className="flex-row justify-center align-center gap-s transition-all"
            style={{ 
              background: 'rgba(255,255,255,0.05)', 
              color: '#fff', 
              border: '1px solid rgba(255,255,255,0.1)', 
              borderRadius: '16px', 
              padding: '16px 32px', 
              fontWeight: '700', 
              fontSize: '13px', 
              cursor: 'pointer',
              position: 'relative',
              zIndex: 1
            }}
            onMouseEnter={(e) => { e.currentTarget.style.background = 'rgba(255,255,255,0.1)'; e.currentTarget.style.transform = 'translateY(-2px)'; }}
            onMouseLeave={(e) => { e.currentTarget.style.background = 'rgba(255,255,255,0.05)'; e.currentTarget.style.transform = 'translateY(0)'; }}
          >
            <RefreshCw size={16} /> RETRY CONNECTION
          </button>
        </div>
      </motion.div>
    );
  }

  if (!isLoggedIn) {
    return (
      <motion.div 
        initial={{ opacity: 0, scale: 0.95 }}
        animate={{ opacity: 1, scale: 1 }}
        className="flex-col align-center justify-center w-full"
        style={{ flex: 1 }}
      >
        <div className="glass-panel flex-col align-center text-center" style={{ width: '100%', maxWidth: '420px', borderRadius: '24px', background: 'rgba(10, 10, 12, 0.6)', border: '1px solid rgba(255, 255, 255, 0.08)', backdropFilter: 'blur(40px)', padding: '48px 32px', boxShadow: '0 20px 40px rgba(0,0,0,0.5)' }}>
          
          <div className="mb-l" style={{ width: '72px', height: '72px', borderRadius: '50%', background: 'linear-gradient(135deg, rgba(255,255,255,0.1), rgba(255,255,255,0.02))', border: '1px solid rgba(255,255,255,0.1)', display: 'flex', alignItems: 'center', justifyContent: 'center', boxShadow: 'inset 0 2px 10px rgba(255,255,255,0.05)' }}>
            <Trophy size={32} style={{ color: 'rgba(255,255,255,0.8)' }} />
          </div>
          
          <h2 className="m-0 font-bold mb-s tracking-wide" style={{ color: '#fff', fontSize: '22px' }}>Trophies Locked</h2>
          
          <p className="opacity-50 mb-xl" style={{ fontSize: '13px', lineHeight: '1.6', maxWidth: '85%' }}>
            Connect your RetroAchievements account to view, track, and unlock trophies in real-time.
          </p>
          
          <button 
            onClick={() => setActiveTab('account')}
            className="w-full flex-row justify-center align-center transition-all"
            style={{ background: '#fff', color: '#000', border: 'none', borderRadius: '100px', padding: '16px', fontWeight: '800', fontSize: '12px', letterSpacing: '0.1em', cursor: 'pointer' }}
            onMouseEnter={(e) => e.currentTarget.style.transform = 'scale(1.02)'}
            onMouseLeave={(e) => e.currentTarget.style.transform = 'scale(1)'}
          >
            CONNECT ACCOUNT
          </button>
        </div>
      </motion.div>
    );
  }

  if (!selectedGame) {
    return (
      <div className="flex-col align-center justify-center h-full opacity-30 mt-xl">
        <Trophy size={64} className="mb-m" />
        <p className="text-l uppercase tracking-widest font-bold">Select a game to see achievements</p>
      </div>
    );
  }

  if (achievements.length === 0) {
    return (
      <div className="flex-col align-center justify-center w-full" style={{ flex: 1, minHeight: '60vh' }}>
        {raStatus && (raStatus.includes('Loading') || raStatus.includes('Syncing')) ? (
          <>
            <p className="text-xxs uppercase tracking-widest animate-pulse opacity-50 mb-xl">
              {raStatus}
            </p>
            <div className="trophy-grid w-full">
              {[1, 2, 3, 4, 5, 6, 7, 8].map(i => (
                <div key={i} className="trophy-card" style={{ border: 'none', background: 'rgba(255,255,255,0.02)' }}>
                  <Skeleton width="48px" height="48px" borderRadius="12px" />
                  <div className="flex-1">
                    <Skeleton width="60%" height="14px" className="mb-s" />
                    <Skeleton width="90%" height="10px" />
                  </div>
                </div>
              ))}
            </div>
          </>
        ) : (
          <motion.div 
            initial={{ opacity: 0, scale: 0.9 }}
            animate={{ opacity: 1, scale: 1 }}
            className="flex-col align-center text-center"
          >
            <div className="mb-l" style={{ width: '80px', height: '80px', borderRadius: '50%', background: 'rgba(255,255,255,0.02)', border: '1px solid rgba(255,255,255,0.05)', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
              <Trophy size={32} style={{ opacity: 0.1 }} />
            </div>
            <h3 className="text-m font-bold mb-xs">No Trophies Available</h3>
            <p className="text-xs opacity-30 max-w-xs leading-relaxed">
              This game might not have a trophy set yet, or there was a problem syncing with RetroAchievements.
            </p>
          </motion.div>
        )}
      </div>
    );
  }

  return (
    <div className="trophy-room animate-fadeUp">
      <header className="trophy-header mb-xl">
        <div className="flex-row justify-between align-center">
          <div>
            <label className="text-xxs text-accent uppercase tracking-widest mb-s block">Achievement Showcase</label>
            <h1 className="text-xl font-black italic">{selectedGame.title}</h1>
          </div>
          {raProgress && raProgress.total > 0 && (
            <div className="stats-badge p-m flex-row align-center gap-m">
              <div className="stat">
                <div className="text-xxs text-dim uppercase">Completion</div>
                <div className="text-l font-black">
                  {Math.round((raProgress.earned / raProgress.total) * 100)}%
                </div>
              </div>
              <div className="stat-divider" />
              <div className="stat">
                <div className="text-xxs text-dim uppercase">Trophies</div>
                <div className="text-l font-black">{raProgress.earned} / {raProgress.total}</div>
              </div>
            </div>
          )}
        </div>

        {raProgress && (
          <div className="progress-track mt-l">
            <div className="progress-bar" style={{ width: `${(raProgress.earned / raProgress.total) * 100}%` }}>
              <div className="progress-glow" />
            </div>
          </div>
        )}
      </header>

      <div className="trophy-grid pb-xl">
        {achievements.map((ach, i) => (
          <TrophyCard key={ach.ID} ach={ach} index={i} />
        ))}
      </div>
    </div>
  );
};

export default TrophiesTab;
