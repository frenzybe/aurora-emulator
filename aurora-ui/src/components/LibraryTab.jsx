import React, { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Play, Trash2, Save, Search, LayoutGrid, Monitor, Gamepad2, Smartphone, Disc, Zap, Trophy, Cpu, FileText } from 'lucide-react';
import { convertFileSrc, invoke } from '@tauri-apps/api/core';
import { getPlatform } from '../utils/platform';
import Skeleton from './Skeleton';
import ConfirmationModal from './ConfirmationModal';

const LibraryTab = ({ 
  games, 
  selectedGame, 
  setSelectedGame, 
  onPlay, 
  onDelete, 
  onRename, 
  isPlaying,
  isLoading,
  onShowSaves,
  showToast
}) => {
  const [isEditingTitle, setIsEditingTitle] = useState(false);
  const [editedTitle, setEditedTitle] = useState('');
  const [gameToDelete, setGameToDelete] = useState(null);
  const [nfoContent, setNfoContent] = useState(null);
  const [isLoadingNfo, setIsLoadingNfo] = useState(false);
  
  // Filtering States
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedPlatform, setSelectedPlatform] = useState('ALL');

  // Platforms for filter with shortening logic
  const PLATFORM_MAP = {
    'NINTENDO ENTERTAINMENT SYSTEM': 'NES',
    'SUPER NINTENDO ENTERTAINMENT SYSTEM': 'SNES',
    'SEGA MEGA DRIVE - GENESIS': 'GENESIS',
    'SEGA MASTER SYSTEM - MARK III': 'SMS',
    'NEC - PC ENGINE - TURBOGRAFX 16': 'PCE',
    'NINTENDO GAME BOY': 'GB',
    'NINTENDO GAME BOY COLOR': 'GBC',
    'NINTENDO GAME BOY ADVANCE': 'GBA',
    'PLAYSTATION': 'PS1',
    'NINTENDO 64': 'N64',
    'ATARI 2600': 'A2600',
    'ATARI 7800': 'A7800',
    'ATARI LYNX': 'LYNX'
  };

  const platforms = ['ALL', ...new Set(games.map(g => {
    const name = getPlatform(g.path).name.toUpperCase();
    return PLATFORM_MAP[name] || name;
  }))];

  // Logic: Filter games
  const filteredGames = games.filter(game => {
    const matchesSearch = game.title.toLowerCase().includes(searchQuery.toLowerCase());
    const platformName = getPlatform(game.path).name.toUpperCase();
    const shortPlatformName = PLATFORM_MAP[platformName] || platformName;
    const matchesPlatform = selectedPlatform === 'ALL' || shortPlatformName === selectedPlatform;
    return matchesSearch && matchesPlatform;
  });

  // LOGIC: Get top 5 recently played games
  let recentGames = [...games]
    .filter(g => g.lastPlayed)
    .sort((a, b) => new Date(b.lastPlayed) - new Date(a.lastPlayed))
    .slice(0, 5);
  
  if (recentGames.length === 0) {
    recentGames = [...games].slice(0, 5);
  }

  if (isLoading) {
    return (
      <div className="animate-fadeUp">
        <div className="hero-content">
          <Skeleton width="120px" height="14px" className="mb-s" />
          <Skeleton width="60%" height="64px" className="mb-m" />
        </div>
        <div className="flex-row gap-m mt-xl overflow-hidden">
          {[1, 2, 3, 4, 5].map(i => (
            <Skeleton key={i} className="cover-card" width="220px" height="300px" />
          ))}
        </div>
      </div>
    );
  }

  if (games.length === 0) {
    return (
      <div className="flex-col justify-center align-center" style={{ height: '60vh', textAlign: 'center', color: 'var(--text-secondary)' }}>
        <p className="text-md mb-s">Your library is currently empty.</p>
        <p className="text-sm opacity-50">Click "+ Add Game" to begin your collection.</p>
      </div>
    );
  }

  const renderCover = (game) => {
    if (game.cover || game.coverBase64) {
      const src = game.coverBase64 ? `data:image/png;base64,${game.coverBase64}` : 
                  (game.cover.startsWith('http') || game.cover.startsWith('data:') ? game.cover : convertFileSrc(game.cover));
      return (
        <div className="cover-wrapper">
          <img src={src} alt={game.title} loading="lazy" />
        </div>
      );
    }
    return (
      <div className="cover-placeholder">
        <div className="flex-col align-center">
          <span className="text-xxs opacity-40 mb-xs">AURORA</span>
          <span className="text-xs font-bold px-s" style={{ textAlign: 'center' }}>{game.title}</span>
        </div>
      </div>
    );
  };

  const handleSelectNfo = async () => {
    if (!selectedGame) return;
    try {
      const nfoPath = await invoke('select_nfo_file');
      if (nfoPath) {
        await invoke('link_game_nfo', { romPath: selectedGame.path, nfoPath });
        const content = await invoke('get_game_nfo', { romPath: selectedGame.path });
        if (content) setNfoContent(content);
      }
    } catch (e) {
      console.error("Failed to link NFO:", e);
      showToast("NFO Error", e.toString(), "error");
    }
  };

  const handleViewNfo = async () => {
    if (!selectedGame || isLoadingNfo) return;
    setIsLoadingNfo(true);
    try {
      console.log("Fetching NFO for:", selectedGame.path);
      const content = await invoke('get_game_nfo', { romPath: selectedGame.path });
      if (content) {
        setNfoContent(content);
      } else {
        if (confirm("No .nfo file found automatically. Would you like to select one manually?")) {
          handleSelectNfo();
        }
      }
    } catch (e) {
      console.error("Failed to read NFO:", e);
      showToast("NFO Error", e.toString(), "error");
    } finally {
      setIsLoadingNfo(false);
    }
  };

  const cyclePlatform = async () => {
    if (!selectedGame) return;
    const commonIsoPlatforms = ["PlayStation Portable", "PlayStation", "PlayStation 2", "GameCube / Wii", "Sega Saturn", "Dreamcast"];
    const current = selectedGame.platform || getPlatform(selectedGame.path).name;
    let nextIdx = (commonIsoPlatforms.indexOf(current) + 1) % commonIsoPlatforms.length;
    const nextPlatform = commonIsoPlatforms[nextIdx];

    try {
      await invoke('update_game_platform', { romPath: selectedGame.path, platform: nextPlatform });
      // Update local state by modifying the game object in the list
      selectedGame.platform = nextPlatform;
      setSelectedGame({ ...selectedGame });
    } catch (e) {
      console.error("Failed to update platform:", e);
    }
  };

  const displayPlatform = selectedGame?.platform || (selectedGame ? getPlatform(selectedGame.path).name : 'AURORA ENGINE');

  return (
    <div className="library-scroll-container">
      <ConfirmationModal 
        isOpen={!!gameToDelete}
        title="Delete ROM?"
        message={`This will remove ${gameToDelete?.title} from your library permanently.`}
        onConfirm={() => {
          onDelete(gameToDelete);
          setGameToDelete(null);
        }}
        onCancel={() => setGameToDelete(null)}
      />

      {/* NFO VIEWER MODAL */}
      <AnimatePresence>
        {nfoContent && (
          <div className="modal-overlay" style={{ zIndex: 1000 }} onClick={() => setNfoContent(null)}>
            <motion.div 
              initial={{ opacity: 0, scale: 0.9, y: 20 }}
              animate={{ opacity: 1, scale: 1, y: 0 }}
              exit={{ opacity: 0, scale: 0.9, y: 20 }}
              className="glass-panel p-xl"
              style={{ 
                maxWidth: '1000px', 
                width: '95%', 
                maxHeight: '85vh', 
                display: 'flex',
                flexDirection: 'column',
                fontFamily: '"Courier New", Courier, monospace',
                fontSize: '13px',
                background: 'rgba(5, 7, 10, 0.98)',
                color: '#00ff41', 
                border: '1px solid rgba(0,255,65,0.4)',
                boxShadow: '0 0 50px rgba(0,255,65,0.15)',
                position: 'relative'
              }}
              onClick={e => e.stopPropagation()}
            >
              <div className="flex-row justify-between align-center mb-m" style={{ fontFamily: 'var(--font-main)', color: 'white', borderBottom: '1px solid rgba(255,255,255,0.1)', paddingBottom: '12px' }}>
                <div className="flex-row align-center gap-s">
                  <FileText size={18} className="text-accent" />
                  <span className="text-sm font-bold opacity-80 tracking-widest uppercase">Release Info: {selectedGame?.title}</span>
                </div>
                <div className="flex-row gap-s">
                  <button 
                    className="btn-secondary" 
                    style={{ padding: '6px 16px', fontSize: '10px' }}
                    onClick={() => {
                      navigator.clipboard.writeText(nfoContent);
                      alert("NFO content copied to clipboard!");
                    }}
                  >
                    COPY TEXT
                  </button>
                  <button 
                    className="btn-primary" 
                    style={{ padding: '6px 16px', fontSize: '10px' }} 
                    onClick={() => setNfoContent(null)}
                  >
                    CLOSE
                  </button>
                </div>
              </div>
              <div style={{ overflow: 'auto', flex: 1, paddingRight: '10px' }}>
                <pre style={{ 
                  whiteSpace: 'pre', 
                  margin: 0, 
                  lineHeight: '1.2',
                  letterSpacing: '0px'
                }}>
                  {nfoContent}
                </pre>
              </div>
            </motion.div>
          </div>
        )}
      </AnimatePresence>

      {/* 1. RECENTLY PLAYED SECTION (HERO) */}
      <motion.section 
        className="recent-section"
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.8 }}
      >
        <div className="hero-content">
          <AnimatePresence mode="wait">
            <motion.div 
              key={selectedGame?.path || 'none'}
              initial={{ opacity: 0, x: -20 }}
              animate={{ opacity: 1, x: 0 }}
              exit={{ opacity: 0, x: 20 }}
              transition={{ duration: 0.3 }}
              className="hero-text-block"
            >
              <div 
                className="platform-tag cursor-pointer hover-accent" 
                onClick={cyclePlatform}
                title="Click to cycle platform"
              >
                {displayPlatform.toUpperCase()}
              </div>
              
              {isEditingTitle ? (
                <input
                  autoFocus
                  className="input-field mb-m"
                  style={{ fontSize: 'var(--fs-lg)', fontWeight: '900', textTransform: 'uppercase', background: 'rgba(255,255,255,0.05)', border: '1px solid var(--accent)' }}
                  value={editedTitle}
                  onChange={(e) => setEditedTitle(e.target.value)}
                  onBlur={() => {
                    onRename(selectedGame, editedTitle);
                    setIsEditingTitle(false);
                  }}
                  onKeyDown={(e) => {
                    if (e.key === 'Enter') {
                      onRename(selectedGame, editedTitle);
                      setIsEditingTitle(false);
                    }
                    if (e.key === 'Escape') {
                      setIsEditingTitle(false);
                    }
                  }}
                />
              ) : (
                <h1 
                  className="hero-title cursor-pointer" 
                  onClick={() => {
                    if (selectedGame) {
                      setEditedTitle(selectedGame.title);
                      setIsEditingTitle(true);
                    }
                  }}
                >
                  {selectedGame?.title || 'SELECT A GAME'}
                </h1>
              )}
              
              {selectedGame && !isEditingTitle && (
                <div className="stats-row flex-row gap-m opacity-40 uppercase tracking-widest mb-m" style={{ fontSize: '10px', fontWeight: '900' }}>
                  <span>{selectedGame.playCount} SESSIONS</span>
                  {selectedGame.lastPlayed && <span>LAST: {new Date(selectedGame.lastPlayed).toLocaleDateString()}</span>}
                </div>
              )}
            </motion.div>
          </AnimatePresence>

          <div className="flex-row gap-m hero-actions">
            <motion.button
              whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}
              className="btn-primary"
              onClick={onPlay}
              disabled={isPlaying || !selectedGame}
              style={{ minWidth: '200px' }}
            >
              {isPlaying ? <div className="loader-ring" style={{ width: '16px', height: '16px' }} /> : <Play size={20} fill="currentColor" />}
              {isPlaying ? 'RUNNING' : 'START GAME'}
            </motion.button>
            
            <motion.button
              whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}
              className="btn-secondary"
              onClick={handleViewNfo}
              disabled={isPlaying || !selectedGame || isLoadingNfo}
              title="View Release Info"
            >
              {isLoadingNfo ? <div className="loader-ring" style={{ width: '16px', height: '16px' }} /> : <Disc size={20} />}
            </motion.button>

            <motion.button
              whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}
              className="btn-secondary"
              onClick={() => onShowSaves(selectedGame)}
              disabled={isPlaying || !selectedGame}
            >
              <Save size={20} />
            </motion.button>

            <motion.button
              whileHover={{ scale: 1.02 }} whileTap={{ scale: 0.98 }}
              className="btn-secondary"
              onClick={() => setGameToDelete(selectedGame)}
              disabled={isPlaying || !selectedGame}
              style={{ color: '#ff4d4d' }}
            >
              <Trash2 size={20} />
            </motion.button>
          </div>
        </div>

        <div className="recent-carousel mt-xl">
          <div className="section-label">CONTINUE PLAYING</div>
          <div className="carousel-track">
            {recentGames.map((game, idx) => (
              <motion.div
                key={`recent-${game.path || idx}`}
                whileHover={{ y: -10, scale: 1.05 }}
                whileTap={{ scale: 0.95 }}
                className={`recent-card ${selectedGame?.path === game.path ? 'active' : ''}`}
                onClick={() => setSelectedGame(game)}
              >
                {renderCover(game)}
                <div className="card-overlay">
                  <div className="card-title">{game.title}</div>
                </div>
              </motion.div>
            ))}
          </div>
        </div>
      </motion.section>

      {/* 2. ALL GAMES SECTION (GRID) */}
      <motion.section 
        className="collection-section mt-xl"
        initial={{ opacity: 0, y: 30 }}
        whileInView={{ opacity: 1, y: 0 }}
        viewport={{ once: true, margin: "-50px" }}
        transition={{ duration: 0.8, ease: "easeOut" }}
      >
        <div className="flex-row justify-between align-center mb-l">
          <div className="section-label" style={{ margin: 0 }}>YOUR COLLECTION ({filteredGames.length})</div>
          
          <div className="flex-row gap-m align-center">
            {/* Search Input */}
            <div className="search-box glass-panel">
              <Search size={14} className="opacity-40" />
              <input 
                type="text" 
                placeholder="Search games..." 
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="search-input"
              />
            </div>

            {/* Platform Filters */}
            <div className="flex-row gap-xs glass-panel p-xs" style={{ borderRadius: '12px', background: 'rgba(0,0,0,0.2)', padding: '4px' }}>
              {platforms.map((p, idx) => {
                const Icon = p === 'ALL' ? LayoutGrid : 
                            p === 'NES' ? Monitor :
                            p === 'SNES' ? Gamepad2 :
                            p === 'GENESIS' ? Zap :
                            p === 'GBA' ? Smartphone :
                            p === 'PS1' ? Disc : 
                            p.includes('ATARI') || p === 'A2600' || p === 'A7800' ? Cpu :
                            p === 'LYNX' ? Smartphone : Gamepad2;
                return (
                  <button
                    key={`${p}-${idx}`}
                    className={`nav-pill text-xxs ${selectedPlatform === p ? 'active' : ''}`}
                    style={{ height: '32px', padding: '0 16px', borderRadius: '8px' }}
                    onClick={() => setSelectedPlatform(p)}
                  >
                    <Icon size={12} className="pill-icon" />
                    <span>{p}</span>
                  </button>
                );
              })}
            </div>
          </div>
        </div>

        <motion.div 
          className="games-grid"
          key={selectedPlatform + searchQuery} // Re-animate on filter
          variants={{
            hidden: { opacity: 0 },
            show: {
              opacity: 1,
              transition: {
                staggerChildren: 0.03
              }
            }
          }}
          initial="hidden"
          whileInView="show"
          viewport={{ once: true }}
        >
          {filteredGames.map((game, idx) => (
            <motion.div
              key={`grid-${game.path || idx}`}
              variants={{
                hidden: { opacity: 0, scale: 0.9, y: 20 },
                show: { opacity: 1, scale: 1, y: 0 }
              }}
              whileHover={{ y: -8, scale: 1.02 }}
              whileTap={{ scale: 0.98 }}
              className={`grid-card ${selectedGame?.path === game.path ? 'active' : ''}`}
              onClick={() => setSelectedGame(game)}
            >
              <div className="grid-card-inner">
                {renderCover(game)}
              </div>
              <div className="grid-card-info">
                <div className="grid-game-title">{game.title}</div>
                <div className="grid-game-meta">{getPlatform(game.path).name}</div>
              </div>
            </motion.div>
          ))}
        </motion.div>
      </motion.section>
    </div>
  );
};

export default LibraryTab;
