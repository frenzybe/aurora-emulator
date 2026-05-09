import React, { useState, useEffect } from 'react';
import { invoke, convertFileSrc } from '@tauri-apps/api/core';
import { listen } from '@tauri-apps/api/event';
import { motion, AnimatePresence } from 'framer-motion';
import { Search, Plus, Play, Trash2, Settings as SettingsIcon, Download, RotateCcw, Monitor, Volume2, Gamepad2, Info, Loader2 } from 'lucide-react';

// Components
import Navigation from './components/Navigation';
import LibraryTab from './components/LibraryTab';
import TrophiesTab from './components/TrophiesTab';
import SettingsTab from './components/SettingsTab';
import AccountTab from './components/AccountTab';
import SaveStatesModal from './components/SaveStatesModal';

// Hooks
import { useGames } from './hooks/useGames';
import { useAchievements } from './hooks/useAchievements';
import { useProfile } from './hooks/useProfile';

// Utils
import { getPlatform } from './utils/platform';

import './App.css';
import Toast from './components/Toast';

function App() {
  const [activeTab, setActiveTab] = useState('library');
  const [connectedGamepads, setConnectedGamepads] = useState([]);
  const [notifications, setNotifications] = useState([]);

  const showToast = (title, message, type = 'info') => {
    const id = Date.now();
    setNotifications(prev => [...prev, { id, title, message, type }]);
  };

  const removeNotification = (id) => {
    setNotifications(prev => prev.filter(n => n.id !== id));
  };

  const [settings, setSettings] = useState(() => {
    const saved = localStorage.getItem('aurora_settings');
    const defaults = { shader: 'none', volume: 80, p1_input: 'keyboard', p2_input: 'gamepad', ra_user: '', ra_key: '', ra_password: '' };
    return saved ? { ...defaults, ...JSON.parse(saved) } : defaults;
  });

  const [controls, setControls] = useState(() => {
    const saved = localStorage.getItem('aurora_controls');
    const p1_defaults = {
      up: 38, down: 40, left: 37, right: 39, a: 90, b: 88, c: 67, x: 83, y: 65, l: 81, r: 87, 
      l2: 49, r2: 50, l3: 51, r3: 52, // Defaults to numbers 1,2,3,4
      select: 16, start: 13,
      gp_up: 12, gp_down: 13, gp_left: 14, gp_right: 15, gp_a: 0, gp_b: 1, gp_c: 2, gp_x: 2, gp_y: 3, gp_l: 4, gp_r: 5, 
      gp_l2: 6, gp_r2: 7, gp_l3: 10, gp_r3: 11,
      gp_select: 8, gp_start: 9
    };
    const p2_defaults = {
      up: 104, down: 101, left: 100, right: 102, a: 103, b: 105, c: 106, x: 107, y: 108, l: 109, r: 110, 
      l2: 53, r2: 54, l3: 55, r3: 56, // Defaults to 5,6,7,8
      select: 111, start: 13,
      gp_up: 12, gp_down: 13, gp_left: 14, gp_right: 15, gp_a: 0, gp_b: 1, gp_c: 2, gp_x: 2, gp_y: 3, gp_l: 4, gp_r: 5, 
      gp_l2: 6, gp_r2: 7, gp_l3: 10, gp_r3: 11,
      gp_select: 8, gp_start: 9
    };

    if (saved) {
      const parsed = JSON.parse(saved);
      // Migration: Handle old flat format or missing new buttons (L2, R2, etc.)
      const p1 = parsed.p1 || (parsed.up ? parsed : p1_defaults);
      const p2 = parsed.p2 || p2_defaults;
      
      return { 
        p1: { ...p1_defaults, ...p1 }, 
        p2: { ...p2_defaults, ...p2 } 
      };
    }
    return { p1: p1_defaults, p2: p2_defaults };
  });

  const [rebinding, setRebinding] = useState(null); // { player: 'p1', id: 'up' }
  const [mappingPlatform, setMappingPlatform] = useState('genesis');
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLaunching, setIsLaunching] = useState(false);
  const [showSavesModal, setShowSavesModal] = useState(null); // Will hold game object
  const [isOnline, setIsOnline] = useState(window.navigator.onLine);

  const checkStatus = async () => {
    if (!window.navigator.onLine) {
      setIsOnline(false);
      return;
    }

    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 2500);
    
    try {
      await fetch('https://connectivitycheck.gstatic.com/generate_204', { 
        mode: 'no-cors', 
        cache: 'no-store',
        signal: controller.signal
      });
      clearTimeout(timeoutId);
      setIsOnline(true);
    } catch (e) {
      setIsOnline(false);
    }
  };

  useEffect(() => {
    const handleStatusChange = () => {
      checkStatus();
    };
    window.addEventListener('online', handleStatusChange);
    window.addEventListener('offline', handleStatusChange);
    
    checkStatus();
    const interval = setInterval(checkStatus, 10000);

    return () => {
      window.removeEventListener('online', handleStatusChange);
      window.removeEventListener('offline', handleStatusChange);
      clearInterval(interval);
    };
  }, []);

  // Use Custom Hooks
  const { games, selectedGame, setSelectedGame, addGame, deleteGame, renameGame, isLoading, loadGames } = useGames((msg) => showToast('Library Error', msg, 'error'));
  const { achievements, raProgress, raStatus } = useAchievements(selectedGame, settings, activeTab, (msg) => showToast('Achievements Error', msg, 'error'), isOnline);
  const { profile, loading: profileLoading, refreshProfile } = useProfile(settings, isOnline);

  const keyNames = {
    // Basic Navigation
    82: 'UP', 81: 'DOWN', 80: 'LEFT', 79: 'RIGHT',
    38: 'UP', 40: 'DOWN', 37: 'LEFT', 39: 'RIGHT',
    13: 'ENTER', 32: 'SPACE', 27: 'ESC', 9: 'TAB', 8: 'BACK', 16: 'SHIFT', 17: 'CTRL', 18: 'ALT',
    // Letters A-Z
    65: 'A', 66: 'B', 67: 'C', 68: 'D', 69: 'E', 70: 'F', 71: 'G', 72: 'H', 73: 'I', 74: 'J', 
    75: 'K', 76: 'L', 77: 'M', 78: 'N', 79: 'O', 80: 'P', 81: 'Q', 82: 'R', 83: 'S', 84: 'T', 
    85: 'U', 86: 'V', 87: 'W', 88: 'X', 89: 'Y', 90: 'Z',
    // Numbers 0-9
    48: '0', 49: '1', 50: '2', 51: '3', 52: '4', 53: '5', 54: '6', 55: '7', 56: '8', 57: '9',
    // Special Emulator Codes (if any)
    29: 'Z', 27: 'X', 6: 'C', 22: 'S', 20: 'Q', 26: 'W', 225: 'L-SHIFT',
    // Gamepad Buttons
    0: 'A', 1: 'B', 2: 'X', 3: 'Y', 4: 'L1', 5: 'R1', 6: 'L2', 7: 'R2', 8: 'SELECT', 9: 'START', 10: 'L3', 11: 'R3'
  };

  // Persistence
  useEffect(() => { localStorage.setItem('aurora_controls', JSON.stringify(controls)); }, [controls]);
  useEffect(() => { localStorage.setItem('aurora_settings', JSON.stringify(settings)); }, [settings]);

  // Input Handling (Keyboard/Mouse focus)
  useEffect(() => {
    // We can add global keyboard shortcuts here if needed later
  }, []);

  const pollGamepads = () => {
    const gamepads = navigator.getGamepads();
    const currentGps = [];
    
    // Debug log for all slots
    let foundSomething = false;
    for (let i = 0; i < gamepads.length; i++) {
      if (gamepads[i]) {
        console.log(`[Diagnostic] Slot ${i}: Found ${gamepads[i].id}`);
        currentGps.push({ index: gamepads[i].index, id: gamepads[i].id });
        foundSomething = true;
      }
    }
    if (!foundSomething && Math.random() < 0.01) { // Log occasionally even if empty
       console.log("[Diagnostic] No gamepads detected by browser.");
    }
    
    setConnectedGamepads(prev => {
      if (prev.length !== currentGps.length || 
          prev.some((p, idx) => p.index !== currentGps[idx]?.index)) {
        return currentGps;
      }
      return prev;
    });
  };

  useEffect(() => {
    const unlisten = listen('emulator-exit', () => {
      console.log("[Launcher] Emulator exit detected, resetting state.");
      setIsPlaying(false);
    });

    const handleGpConnect = (e) => {
      console.log(`[Input] Gamepad connected in Browser: Slot ${e.gamepad.index}, ID: ${e.gamepad.id}`);
      // Log all connected ones
      const all = navigator.getGamepads();
      for(let i=0; i<all.length; i++) {
        if(all[i]) console.log(`  -> Slot ${i}: ${all[i].id}`);
      }
      pollGamepads();
    };

    window.addEventListener("gamepadconnected", handleGpConnect);
    window.addEventListener("gamepaddisconnected", pollGamepads);

    const interval = setInterval(pollGamepads, 1000);

    return () => { 
      unlisten.then(f => f()); 
      clearInterval(interval);
      window.removeEventListener("gamepadconnected", handleGpConnect);
      window.removeEventListener("gamepaddisconnected", pollGamepads);
    };
  }, []);

  // Rebinding Logic
  useEffect(() => {
    if (!rebinding) return;
    const { player, id } = rebinding;

    const unlistenNative = listen('native-gamepad-event', (event) => {
      const { type, gp_name, button } = event.payload;
      console.log(`[Native Input] ${type} from ${gp_name}: ${button}`);
      
      if (id.startsWith('gp_')) {
          setControls(p => ({ ...p, [player]: { ...p[player], [id]: button } })); 
          setRebinding(null); 
          showToast('Native Mapped', `Assigned BTN ${button} from ${gp_name}`, 'success');
      }
    });

    if (id.startsWith('gp_')) {
      let raf;
      let initialButtons = null;
      
      const poll = () => {
        const gamepads = navigator.getGamepads();
        
        // Capture baseline on first run to ignore buttons already held down
        if (initialButtons === null) {
          initialButtons = [];
          for (let g = 0; g < gamepads.length; g++) {
            if (gamepads[g]) {
              initialButtons[g] = gamepads[g].buttons.map(b => b.pressed || b.value > 0.3);
            }
          }
        }

        for (let g = 0; g < gamepads.length; g++) {
          const gp = gamepads[g];
          if (gp) {
            // 1. Check Buttons
            gp.buttons.forEach((btn, idx) => {
              const isPressed = btn.pressed || btn.value > 0.6;
              if (isPressed && (!initialButtons[g] || !initialButtons[g][idx])) { 
                setControls(p => ({ ...p, [player]: { ...p[player], [id]: idx } })); 
                setRebinding(null); 
                showToast('Control Mapped', `Assigned BTN ${idx} to ${id.replace('gp_', '').toUpperCase()}`, 'success');
              }
              if (!isPressed && initialButtons[g] && initialButtons[g][idx]) {
                initialButtons[g][idx] = false;
              }
            });

            // 2. Check Axes (for D-Pad on generic controllers)
            gp.axes.forEach((val, idx) => {
              if (Math.abs(val) > 0.7) { // Strong movement
                // We use a convention: axes are stored as 100 + index (positive) or 200 + index (negative)
                // But for now, let's just log and see. 
                // Actually, Defender Omega usually has DPAD on axes 4/5 or similar.
                // To keep it simple for main.cpp, let's just stick to buttons if possible, 
                // but allow mapping axes to DPAD IDs.
                if (id.includes('up') || id.includes('down') || id.includes('left') || id.includes('right')) {
                    // For now, let's just show a toast that Axis was detected
                    // but most users want to map buttons.
                }
              }
            });
          }
        }
        if (rebinding) raf = requestAnimationFrame(poll);
      };
      raf = requestAnimationFrame(poll);
      return () => {
        if (raf) cancelAnimationFrame(raf);
        unlistenNative.then(f => f());
      };
    } else {
      const handle = (e) => {
        e.preventDefault();
        if (e.keyCode) { 
          setControls(p => ({ ...p, [player]: { ...p[player], [id]: e.keyCode } })); 
          setRebinding(null); 
          showToast('Key Mapped', `Assigned ${keyNames[e.keyCode] || e.keyCode} to ${id.toUpperCase()}`, 'success');
        }
      };
      window.addEventListener('keydown', handle);
      return () => {
        window.removeEventListener('keydown', handle);
        unlistenNative.then(f => f());
      };
    }
  }, [rebinding]);

  const handlePlay = async (statePath = null) => {
    if (!selectedGame || isPlaying) return;
    try {
      if (showSavesModal) setShowSavesModal(null); // Close modal if open
      setIsLaunching(true);
      const BROWSER_TO_SDL = { 0: 0, 1: 1, 2: 2, 3: 3, 4: 9, 5: 10, 6: 15, 7: 16, 10: 7, 11: 8, 8: 4, 9: 6, 12: 11, 13: 12, 14: 13, 15: 14 };
      
      const JS_TO_SDL = {
        // Arrows
        38: 82, 40: 81, 37: 80, 39: 79,
        // WASD
        87: 26, 83: 22, 65: 4, 68: 7,
        // Action Buttons (ZXCV...)
        90: 29, 88: 27, 67: 6, 86: 25, 83: 22, 65: 4, 81: 20, 87: 26,
        // System
        13: 40, 32: 44, 27: 41, 8: 42, 9: 43, 16: 225, 17: 224, 18: 226,
        // Numbers
        48: 39, 49: 30, 50: 31, 51: 32, 52: 33, 53: 34, 54: 35, 55: 36, 56: 37, 57: 38
      };

      const processControls = (pControls) => {
        let processed = {};
        Object.keys(pControls).forEach(k => { 
          // Convert snake_case to camelCase for the backend struct
          const camelKey = k.replace(/_([a-z])/g, (g) => g[1].toUpperCase());
          let val = pControls[k];
          
          if (k.startsWith('gp_')) {
            val = BROWSER_TO_SDL[val] ?? val; 
          } else {
            // Always convert from JS KeyCode to SDL Scancode
            val = JS_TO_SDL[val] ?? val;
          }
          
          processed[camelKey] = val;
        });
        return processed;
      };

      // Start the game (this will await until exit)
      const playPromise = invoke('play_game', {
        romPath: selectedGame.path,
        volume: parseFloat(settings.volume) / 100.0,
        fastForward: false,
        saveState: false,
        shader: settings.shader,
        p1Input: settings.p1_input,
        p1Controls: processControls(controls.p1),
        p2Input: settings.p2_input,
        p2Controls: processControls(controls.p2),
        raUser: settings.ra_user || null,
        raToken: settings.ra_password || settings.ra_key || null,
        statePath: typeof statePath === 'string' ? statePath : null
      });
      
      // Aesthetics: show splash for at least 1.5s
      await new Promise(r => setTimeout(r, 1500));
      
      setIsLaunching(false);
      setIsPlaying(true);
      
      // Wait for game to actually finish
      await playPromise;
      setIsPlaying(false);
    } catch (e) {
      showToast('Emulator Error', e.toString(), 'error');
      setIsLaunching(false);
      setIsPlaying(false);
    }
  };

  return (
    <div className="hero-layout">
      <AnimatePresence>
        <SaveStatesModal 
          isOpen={!!showSavesModal}
          game={showSavesModal}
          onClose={() => setShowSavesModal(null)}
          onLoadState={(state) => handlePlay(state.path)}
        />

        {isLaunching && (
          <motion.div 
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            className="launch-overlay"
            style={{
              position: 'fixed', top: 0, left: 0, right: 0, bottom: 0,
              zIndex: 2000, background: 'black',
              display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center',
              overflow: 'hidden'
            }}
          >
            {/* Blurred Background for Splash */}
            <motion.div 
              initial={{ scale: 1.1, opacity: 0 }}
              animate={{ scale: 1, opacity: 0.3 }}
              transition={{ duration: 1 }}
              style={{
                position: 'absolute', top: 0, left: 0, right: 0, bottom: 0,
                backgroundImage: selectedGame?.cover || selectedGame?.coverBase64 ? (() => {
                  if (selectedGame.coverBase64) return `url("data:image/png;base64,${selectedGame.coverBase64}")`;
                  let src = selectedGame.cover.startsWith('http') || selectedGame.cover.startsWith('data:') ? selectedGame.cover : convertFileSrc(selectedGame.cover);
                  if (!src.includes('://') && !src.startsWith('data:')) {
                      src = `asset://localhost${encodeURI(src.startsWith('/') ? src : '/' + src)}`;
                  }
                  return `url("${src}")`;
                })() : 'none',
                backgroundSize: 'cover', backgroundPosition: 'center',
                filter: 'blur(60px) grayscale(100%)',
                zIndex: -1
              }}
            />

            <motion.div
              initial={{ scale: 0.8, opacity: 0 }}
              animate={{ scale: 1, opacity: 1 }}
              transition={{ delay: 0.2 }}
              style={{ textAlign: 'center' }}
            >
              <h1 className="text-xl font-black italic tracking-tighter mb-m">AURORA</h1>
              <div className="flex-row align-center gap-m justify-center opacity-30">
                <Loader2 className="animate-spin" size={16} />
                <span className="text-xxs uppercase tracking-widest">Initialising Core...</span>
              </div>
              
              <div style={{ marginTop: '60px' }}>
                <span className="text-xxs opacity-20 uppercase tracking-widest block mb-s">Now Starting</span>
                <h2 className="text-m font-black">{selectedGame?.title}</h2>
              </div>
            </motion.div>
          </motion.div>
        )}

        <motion.div
          key={selectedGame?.cover || 'empty'}
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={{ duration: 1.2, ease: "easeInOut" }}
          className="hero-bg"
          style={{ backgroundImage: selectedGame?.cover || selectedGame?.coverBase64 ? (() => {
            if (selectedGame.coverBase64) return `url("data:image/png;base64,${selectedGame.coverBase64}")`;
            let src = selectedGame.cover.startsWith('http') || selectedGame.cover.startsWith('data:') ? selectedGame.cover : convertFileSrc(selectedGame.cover);
            if (!src.includes('://') && !src.startsWith('data:')) {
                src = `asset://localhost${encodeURI(src.startsWith('/') ? src : '/' + src)}`;
            }
            return `url("${src}")`;
          })() : 'none' }}
        />
        <motion.div
          key={selectedGame?.path + '_glow'}
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={{ duration: 0.8 }}
          className="hero-bg-glow"
          style={{ 
            position: 'absolute',
            top: 0, left: 0, right: 0, bottom: 0,
            background: `radial-gradient(circle at 15% 50%, rgba(255, 255, 255, 0.05) 0%, transparent 60%)`,
            zIndex: 1,
            pointerEvents: 'none'
          }}
        />
      </AnimatePresence>
      <div className="hero-overlay" style={{ zIndex: 2 }}></div>

      <Navigation
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        connectedGamepads={connectedGamepads}
        onAddGame={addGame}
        settings={settings}
        isOnline={isOnline}
        profile={profile}
      />

      <main className="main-stage">
        <AnimatePresence mode="wait">
          {activeTab === 'library' && (
            <motion.div key="library" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className='tab-wrapper'>
              <LibraryTab
                games={games}
                selectedGame={selectedGame}
                setSelectedGame={setSelectedGame}
                onPlay={handlePlay}
                onShowSaves={(game) => setShowSavesModal(game)}
                onDelete={deleteGame}
                onRename={renameGame}
                isPlaying={isPlaying}
                isLoading={isLoading}
                showToast={showToast}
              />
            </motion.div>
          )}


          {activeTab === 'trophies' && (
            <motion.div key="trophies" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className='tab-wrapper'>
              <TrophiesTab
                selectedGame={selectedGame}
                achievements={achievements}
                raProgress={raProgress}
                raStatus={raStatus}
                isLoggedIn={!!(settings?.ra_user && settings?.ra_key)}
                setActiveTab={setActiveTab}
                isOnline={isOnline}
                checkNetwork={checkStatus}
              />
            </motion.div>
          )}

          {activeTab === 'account' && (
            <motion.div key="account" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className='tab-wrapper'>
              <AccountTab 
                settings={settings} 
                setSettings={setSettings} 
                showToast={showToast}
                isOnline={isOnline}
                profile={profile}
                profileLoading={profileLoading}
                refreshProfile={refreshProfile}
                checkNetwork={checkStatus}
              />
            </motion.div>
          )}

          {activeTab === 'settings' && (
            <motion.div key="settings" initial={{ opacity: 0 }} animate={{ opacity: 1 }} exit={{ opacity: 0 }} className='tab-wrapper'>
              <SettingsTab
                settings={settings} setSettings={setSettings}
                controls={controls} setControls={setControls}
                connectedGamepads={connectedGamepads}
                rebinding={rebinding} setRebinding={setRebinding}
                mappingPlatform={mappingPlatform} setMappingPlatform={setMappingPlatform}
                keyNames={keyNames} showToast={showToast}
              />
            </motion.div>
          )}
        </AnimatePresence>
      </main>

      <Toast notifications={notifications} removeNotification={removeNotification} />
    </div>
  );
}

export default App;
