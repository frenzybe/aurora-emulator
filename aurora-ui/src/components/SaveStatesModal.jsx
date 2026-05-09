import React, { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { X, Play, Clock, Save, Trash2, CameraOff } from 'lucide-react';
import { invoke, convertFileSrc } from '@tauri-apps/api/core';

const SaveStatesModal = ({ isOpen, onClose, game, onLoadState }) => {
  const [states, setStates] = useState([]);
  const [selectedIndex, setSelectedIndex] = useState(0);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    if (isOpen && game) {
      loadStates();
    }
  }, [isOpen, game]);

  const loadStates = async () => {
    setLoading(true);
    try {
      const result = await invoke('get_save_states', { romPath: game.path });
      setStates(result);
      if (result.length > 0) setSelectedIndex(0);
    } catch (e) {
      console.error("Failed to load save states:", e);
    } finally {
      setLoading(false);
    }
  };

  const handleDelete = async (e, statePath) => {
    e.stopPropagation();
    // Use window.confirm explicitly to avoid Tauri interception issues
    if (!window.confirm('Are you sure you want to delete this save state?')) return;
    
    try {
      await invoke('delete_save_state', { statePath });
      const newStates = states.filter(s => s.path !== statePath);
      setStates(newStates);
      if (selectedIndex >= newStates.length) setSelectedIndex(Math.max(0, newStates.length - 1));
    } catch (e) {
      console.error("Failed to delete save state:", e);
    }
  };

  if (!isOpen) return null;

  const selectedState = states[selectedIndex];

  return (
    <div className="modal-overlay" style={{ zIndex: 3000 }}>
      <motion.div 
        initial={{ opacity: 0, scale: 0.95 }}
        animate={{ opacity: 1, scale: 1 }}
        exit={{ opacity: 0, scale: 0.95 }}
        className="modal-content"
        style={{ 
          width: '1100px', 
          maxWidth: '95vw', 
          height: '700px',
          maxHeight: '90vh',
          background: 'rgba(10, 10, 15, 0.98)',
          backdropFilter: 'blur(40px)',
          border: '1px solid rgba(255, 255, 255, 0.08)',
          padding: 0,
          overflow: 'hidden',
          display: 'flex',
          flexDirection: 'row',
          borderRadius: '24px'
        }}
      >
        {/* LEFT PANEL: TIMELINE LIST */}
        <div style={{ 
          width: '320px', 
          borderRight: '1px solid rgba(255, 255, 255, 0.05)',
          display: 'flex',
          flexDirection: 'column',
          background: 'rgba(0, 0, 0, 0.2)'
        }}>
          <div style={{ padding: '30px 24px', borderBottom: '1px solid rgba(255, 255, 255, 0.05)' }}>
            <h2 className="text-md font-black uppercase tracking-tighter mb-xs">Timeline</h2>
            <p className="text-xxxxs opacity-40 uppercase tracking-widest">History of {game?.title}</p>
          </div>

          <div style={{ flex: 1, overflowY: 'auto', padding: '12px' }} className="custom-scrollbar">
            {loading ? (
               <div className="flex-col align-center py-xl opacity-20"><div className="loader-ring" /></div>
            ) : states.length > 0 ? (
              states.map((state, idx) => (
                <motion.div
                  key={`${state.path}-${idx}`}
                  onClick={() => setSelectedIndex(idx)}
                  whileHover={{ x: 4 }}
                  style={{
                    padding: '12px',
                    borderRadius: '12px',
                    cursor: 'pointer',
                    background: selectedIndex === idx ? 'rgba(255, 255, 255, 0.05)' : 'transparent',
                    border: '1px solid',
                    borderColor: selectedIndex === idx ? 'rgba(255, 255, 255, 0.1)' : 'transparent',
                    marginBottom: '8px',
                    display: 'flex',
                    gap: '12px',
                    alignItems: 'center',
                    transition: 'all 0.2s ease'
                  }}
                >
                  <div style={{ width: '80px', aspectRatio: '4/3', borderRadius: '6px', overflow: 'hidden', background: '#000' }}>
                    {state.screenshot && <img src={state.screenshot} style={{ width: '100%', height: '100%', objectFit: 'cover' }} />}
                  </div>
                  <div style={{ flex: 1 }}>
                    <div className="text-xxxxs font-bold opacity-40 mb-xs">SLOT {state.slot.length > 8 ? 'AUTO' : state.slot}</div>
                    <div className="text-xxs font-black opacity-80" style={{ fontSize: '10px' }}>{state.timestamp.split(' ')[1]}</div>
                  </div>
                </motion.div>
              ))
            ) : (
              <div className="text-xxxxs opacity-20 text-center mt-xl uppercase tracking-widest">No memories found</div>
            )}
          </div>
        </div>

        {/* RIGHT PANEL: LARGE PREVIEW */}
        <div style={{ flex: 1, position: 'relative', display: 'flex', flexDirection: 'column' }}>
          <motion.button 
            whileHover={{ scale: 1.1, opacity: 1 }}
            whileTap={{ scale: 0.9 }}
            onClick={onClose} 
            style={{ 
              position: 'absolute', top: '24px', right: '24px', zIndex: 100, 
              background: 'none',
              border: 'none',
              cursor: 'pointer',
              color: 'white',
              opacity: 0.5,
              transition: 'opacity 0.2s ease'
            }}
          >
            <X size={32} strokeWidth={1.5} />
          </motion.button>

          {selectedState ? (
            <>
              {/* Massive Preview Image */}
              <div style={{ flex: 1, position: 'relative', overflow: 'hidden', background: '#000' }}>
                <AnimatePresence mode="wait">
                  <motion.img
                    key={selectedState.path}
                    initial={{ opacity: 0, scale: 1.05 }}
                    animate={{ opacity: 1, scale: 1 }}
                    exit={{ opacity: 0 }}
                    transition={{ duration: 0.4 }}
                    src={selectedState.screenshot}
                    style={{ width: '100%', height: '100%', objectFit: 'contain' }}
                  />
                </AnimatePresence>
                
                {/* Vignette & Gradient Overlay */}
                <div style={{ 
                  position: 'absolute', top: 0, left: 0, right: 0, bottom: 0,
                  background: 'linear-gradient(to top, rgba(10,10,15,1) 0%, rgba(10,10,15,0) 40%)'
                }} />
              </div>

              {/* Bottom Info Bar */}
              <div style={{ padding: '40px', background: 'rgba(10,10,15,1)', position: 'relative' }}>
                 <div className="flex-row justify-between align-end">
                    <div>
                      <div className="flex-row align-center gap-s mb-s opacity-40">
                         <Clock size={14} />
                         <span className="text-xxs font-bold uppercase tracking-widest">{selectedState.timestamp}</span>
                      </div>
                      <h3 className="text-xl font-black uppercase tracking-tighter leading-none">
                        Resume Session
                      </h3>
                    </div>

                    <div className="flex-row gap-m">
                      <button 
                        onClick={(e) => handleDelete(e, selectedState.path)}
                        className="btn-secondary"
                        style={{ border: '1px solid rgba(255, 50, 50, 0.3)', color: '#ff5555', height: '56px' }}
                      >
                        <Trash2 size={18} />
                      </button>
                      
                      <button 
                        onClick={() => onLoadState(selectedState)}
                        className="btn-primary"
                        style={{ padding: '0 40px', height: '56px' }}
                      >
                        <div className="flex-row align-center gap-m">
                           <Play size={20} fill="currentColor" />
                           <span className="text-xs font-black italic">RESUME NOW</span>
                        </div>
                      </button>
                    </div>
                 </div>
              </div>
            </>
          ) : (
            <div className="flex-col align-center justify-center h-full opacity-10">
               <Save size={120} />
               <span className="text-md font-black uppercase mt-m tracking-tighter">Temporal Void</span>
            </div>
          )}
        </div>
      </motion.div>
    </div>
  );
};

export default SaveStatesModal;
