import { useState, useEffect } from 'react';
import { invoke } from '@tauri-apps/api/core';

let profileCache = { data: null, timestamp: 0 };

export const clearProfileCache = () => {
  profileCache = { data: null, timestamp: 0 };
};

export const useProfile = (settings, isOnline) => {
  const [profile, setProfile] = useState(profileCache.data);
  const [loading, setLoading] = useState(false);

  const fetchProfile = async (force = false) => {
    if (!isOnline) return;
    if (!settings.ra_user || !settings.ra_key) {
      setProfile(null);
      return;
    }

    const now = Date.now();
    // Cache valid for 10 minutes unless forced
    if (!force && profileCache.data && (now - profileCache.timestamp < 600000)) {
      if (!profile) setProfile(profileCache.data);
      return;
    }

    setLoading(true);
    try {
      const url = `https://retroachievements.org/API/API_GetUserSummary.php?z=${encodeURIComponent(settings.ra_user)}&y=${encodeURIComponent(settings.ra_key)}&u=${encodeURIComponent(settings.ra_user)}&g=5&a=5`;
      const res = await invoke('ra_proxy', { url });
      if (res && res.User) {
        profileCache.data = res;
        profileCache.timestamp = now;
        setProfile(res);
      } else {
        setProfile(null);
      }
    } catch (e) {
      console.error("Profile fetch error:", e);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchProfile();
  }, [settings.ra_user, settings.ra_key, isOnline]);

  return { profile, loading, refreshProfile: () => fetchProfile(true) };
};
