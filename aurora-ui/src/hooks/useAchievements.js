import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { getPlatform } from '../utils/platform';

const gameListCache = {};
let achievementCache = {};

export const clearAchievementCache = () => {
  for (const key in gameListCache) delete gameListCache[key];
  for (const key in achievementCache) delete achievementCache[key];
};

export const useAchievements = (selectedGame, settings, activeTab, setErrorMsg, isOnline) => {
  const [achievements, setAchievements] = useState([]);
  const [raProgress, setRaProgress] = useState(null);
  const [raStatus, setRaStatus] = useState(null);
  const [lastFetchedGameId, setLastFetchedGameId] = useState(null);



  const fetchAchievements = async (gameTitle) => {
    const gamePathAtStart = selectedGame?.path;
    if (!isOnline) {
      setRaStatus("Offline Mode: Sync unavailable");
      return;
    }
    
    // Prevent redundant calls if we're already viewing this game's achievements
    if (selectedGame?.path === lastFetchedGameId && achievements.length > 0) return;

    setAchievements([]);
    setRaProgress(null);
    
    if (!settings.ra_user || !settings.ra_key) {
      setRaStatus("Setup RetroAchievements in Settings to see trophies");
      return;
    }

    try {
      const user = encodeURIComponent(settings.ra_user);
      const key = encodeURIComponent(settings.ra_key);
      const platform = getPlatform(selectedGame.path);
      if (!platform) return;

      let gameId = null;
      try {
        setRaStatus("Identifying game hash...");
        gameId = await invoke('get_game_id_by_hash', { 
          romPath: selectedGame.path,
          raUser: settings.ra_user,
          raKey: settings.ra_key
        });
        console.log("[RA-Sync] Hash identification successful:", gameId);
      } catch (hashError) {
        console.warn("[RA-Sync] Hash sync failed, falling back to title search:", hashError);
        setRaStatus("Syncing via title match...");
        
        const consoleId = platform.raId || 1;
        let gamesArray = gameListCache[consoleId];

        if (!gamesArray) {
          const ts = Date.now();
          const listUrl = `https://retroachievements.org/API/API_GetGameList.php?z=${user}&y=${key}&u=${user}&i=${consoleId}&_t=${ts}`;
          const gamesList = await invoke('ra_proxy', { url: listUrl });
          if (!gamesList || gamesList.error) throw new Error(gamesList?.error || "Sync Failed");
          gamesArray = Array.isArray(gamesList) ? gamesList : Object.values(gamesList);
          gameListCache[consoleId] = gamesArray;
        }

        const cleanTitle = gameTitle.trim().toLowerCase().replace(/[^\w\s]/gi, '');
        const match = gamesArray.find(g => {
          const t = g.Title.toLowerCase().replace(/[^\w\s]/gi, '');
          return t === cleanTitle || t.includes(cleanTitle) || cleanTitle.includes(t);
        });

        if (match) {
          gameId = match.ID;
          console.log("[RA-Sync] Title-based match found:", match.Title, "(ID:", gameId, ")");
        } else {
          throw new Error(`Game "${gameTitle}" not recognized`);
        }
      }

      setLastFetchedGameId(selectedGame.path);

      // Check achievement cache with ID
      if (achievementCache[gameId]) {
        const cached = achievementCache[gameId];
        setAchievements(cached.list);
        setRaProgress(cached.progress);
        setRaStatus(null);
        return;
      }

      setRaStatus(`Loading trophies for Game ID ${gameId}...`);
      const progressUrl = `https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?z=${user}&y=${key}&u=${user}&g=${gameId}`;
      const data = await invoke('ra_proxy', { url: progressUrl });

      if (data && data.Achievements && Object.keys(data.Achievements).length > 0) {
        const list = Object.values(data.Achievements).sort((a, b) => b.DateEarned ? 1 : -1);
        const earnedCount = list.filter(a => a.DateEarned).length;
        const progress = { earned: earnedCount, total: list.length };
        
        // Save to cache
        achievementCache[gameId] = { list, progress };
        
        // Double check if this game is still the active one before setting state
        if (selectedGame.path === gamePathAtStart) {
          setAchievements(list);
          setRaProgress(progress);
          setRaStatus(null);
        }
      } else {
        if (selectedGame.path === gamePathAtStart) {
          setRaStatus(`No achievements found for this game version`);
        }
      }
    } catch (e) {
      if (selectedGame.path === gamePathAtStart) {
        console.error("RA Error:", e);
        setRaStatus(`SYNC ERROR`);
        setErrorMsg(`RetroAchievements Sync Failed: ${e.message || e}`);
      }
    }
  };

  useEffect(() => {
    console.log("useAchievements effect triggered. isOnline:", isOnline, "activeTab:", activeTab);
    if (selectedGame && activeTab === 'trophies' && isOnline) {
      fetchAchievements(selectedGame.title);
    } else if (!isOnline) {
      console.log("useAchievements: Connection lost, clearing data");
      setRaStatus("Connection lost. Trophies hidden.");
      setAchievements([]);
      setRaProgress(null);
    }
  }, [selectedGame, activeTab, settings.ra_user, settings.ra_key, isOnline]);

  return { achievements, raProgress, raStatus };
};
