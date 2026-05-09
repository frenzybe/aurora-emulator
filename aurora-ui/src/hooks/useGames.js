import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getPlatform } from '../utils/platform';

export const useGames = (setErrorMsg) => {
  const [games, setGames] = useState([]);
  const [selectedGame, setSelectedGame] = useState(null);
  const [isLoading, setIsLoading] = useState(true);

  const loadGames = async () => {
    try {
      const data = await invoke('get_games');
      setGames(data);
      if (data.length > 0 && !selectedGame) setSelectedGame(data[0]);
      setIsLoading(false);

      // Async cover download loop
      for (const game of data) {
        if (!game.coverBase64) {
          console.log(`[Library] Checking/Hunting cover for: ${game.title}`);
          invoke('download_cover', { title: game.title, romPath: game.path }).then((b64) => {
            if (b64) {
              console.log(`[Library] Found and saved BLOB cover for ${game.title}`);
              setGames(prev => prev.map(g => g.path === game.path ? { ...g, coverBase64: b64 } : g));
              setSelectedGame(prev => prev?.path === game.path ? { ...prev, coverBase64: b64 } : prev);
            }
          });
        } else {
          console.log(`[Library] Game ${game.title} has cover data. BLOB size: ${game.coverBase64?.length || 0}`);
        }
      }
    } catch (e) {
      setErrorMsg('Failed to load library: ' + e);
    }
  };

  const addGame = async () => {
    try {
      const { open } = await import('@tauri-apps/plugin-dialog');
      const selected = await open({
        multiple: false,
        filters: [{ 
          name: 'Retro Games', 
          extensions: ['bin', 'gen', 'md', 'nes', 'sfc', 'smc', 'gb', 'gbc', 'gba', 'sms', 'gg', 'pce', 'cue', 'chd', 'ps1', 'psx', 'n64', 'nds', 'psp', 'iso', 'zip', '7z', 'cdi', 'gdi', 'gcm', 'rvz', 'dos', 'exe', 'conf', 'ss', 'saturn', '3do', 'a26', 'a78', 'lnx', 'ps2', '3ds', 'cia', 'msx', 'adf', 'scummvm', 'ws', 'wsc', 'ngp', 'ngc', 'vb', 'o2'] 
        }]
      });

      if (selected) {
        // Extract title from path
        const pathParts = selected.split('/');
        const filename = pathParts[pathParts.length - 1];
        const dotIndex = filename.lastIndexOf('.');
        const title = dotIndex !== -1 ? filename.substring(0, dotIndex) : filename;
        
        // Use the utility we already updated
        const platformInfo = getPlatform(selected);
        const platform = platformInfo.name;

        const newGame = await invoke('add_game', { 
          title, 
          romPath: selected, 
          platform 
        });
        setGames(prev => [...prev, newGame]);
        setSelectedGame(newGame);

        invoke('download_cover', { title: newGame.title, romPath: newGame.path }).then((coverData) => {
          if (coverData) {
            setGames(prev => prev.map(g => g.path === newGame.path ? { ...g, coverBase64: coverData } : g));
            setSelectedGame(prev => prev?.path === newGame.path ? { ...prev, coverBase64: coverData } : prev);
          }
        });
      }
    } catch (e) {
      setErrorMsg('Add game failed: ' + e);
    }
  };

  const deleteGame = async (game) => {
    if (!game) return;
    try {
      await invoke('remove_game', { romPath: game.path });
      const nextGames = games.filter(g => g.path !== game.path);
      setGames(nextGames);

      if (selectedGame?.path === game.path) {
        setSelectedGame(nextGames.length > 0 ? nextGames[0] : null);
      }
    } catch (e) {
      setErrorMsg('Delete failed: ' + e);
    }
  };

  const renameGame = async (game, newTitle) => {
    if (!game || !newTitle.trim()) return;
    try {
      await invoke('rename_game', { romPath: game.path, newTitle: newTitle });
      setGames(prev => prev.map(g => g.path === game.path ? { ...g, title: newTitle } : g));
      setSelectedGame(prev => prev?.path === game.path ? { ...prev, title: newTitle } : prev);

      const coverData = await invoke('download_cover', { title: newTitle, romPath: game.path });
      if (coverData) {
        setGames(prev => prev.map(g => g.path === game.path ? { ...g, coverBase64: coverData } : g));
        setSelectedGame(prev => prev?.path === game.path ? { ...prev, coverBase64: coverData } : prev);
      }
    } catch (e) {
      setErrorMsg('Rename failed: ' + e);
    }
  };

  useEffect(() => {
    loadGames();
  }, []);

  return { games, selectedGame, setSelectedGame, addGame, deleteGame, renameGame, isLoading, loadGames };
};
