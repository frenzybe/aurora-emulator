import React, { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { open } from "@tauri-apps/plugin-shell";
import { motion } from "framer-motion";
import {
  User,
  ExternalLink,
  Trophy,
  Star,
  Shield,
  ArrowRight,
  Clock,
  WifiOff,
  RefreshCw,
} from "lucide-react";
import { clearProfileCache } from "../hooks/useProfile";
import { clearAchievementCache } from "../hooks/useAchievements";

const AccountTab = ({
  settings,
  setSettings,
  showToast,
  isOnline,
  profile,
  profileLoading,
  refreshProfile,
  checkNetwork,
}) => {
  const handleConnect = async (e) => {
    e.preventDefault();
    if (!isOnline) {
      setErrorMsg("Internet connection required!");
      return;
    }
    if (!settings.ra_user || !settings.ra_key) {
      setErrorMsg("Username and Web API Key required!");
      return;
    }

    refreshProfile();
  };

  const handleLogout = () => {
    setSettings((prev) => ({
      ...prev,
      ra_user: "",
      ra_key: "",
      ra_password: "",
    }));
    clearProfileCache();
    clearAchievementCache();
  };

  if (!isOnline && !profile) {
    return (
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        className="w-full flex-col align-center justify-center p-m"
        style={{ flex: 1 }}
      >
        <div
          className="glass-panel flex-col align-center text-center"
          style={{
            width: "100%",
            maxWidth: "480px",
            borderRadius: "32px",
            background: "rgba(10, 10, 12, 0.4)",
            border: "1px solid rgba(255, 255, 255, 0.05)",
            backdropFilter: "blur(40px)",
            padding: "64px 40px",
            position: "relative",
            overflow: "hidden",
          }}
        >
          <div
            style={{
              position: "absolute",
              top: "-10%",
              left: "-10%",
              width: "120%",
              height: "120%",
              background:
                "radial-gradient(circle at center, rgba(255,255,255,0.03) 0%, transparent 70%)",
              zIndex: 0,
            }}
          />

          <div className="mb-xl" style={{ position: "relative", zIndex: 1 }}>
            <div
              style={{
                width: "80px",
                height: "80px",
                borderRadius: "24px",
                background: "rgba(255,255,255,0.03)",
                border: "1px solid rgba(255,255,255,0.08)",
                display: "flex",
                alignItems: "center",
                justifyContent: "center",
                margin: "0 auto",
                boxShadow: "0 10px 30px rgba(0,0,0,0.2)",
              }}
            >
              <WifiOff size={32} style={{ color: "rgba(255,255,255,0.4)" }} />
            </div>
          </div>

          <h2
            className="m-0 font-bold mb-s tracking-tight"
            style={{
              color: "#fff",
              fontSize: "28px",
              position: "relative",
              zIndex: 1,
            }}
          >
            Offline Mode
          </h2>
          <p
            className="opacity-40 mb-xl font-medium"
            style={{
              fontSize: "15px",
              lineHeight: "1.6",
              maxWidth: "300px",
              position: "relative",
              zIndex: 1,
            }}
          >
            Syncing with RetroAchievements requires an active connection.
          </p>

          <button
            onClick={checkNetwork}
            className="flex-row justify-center align-center gap-s transition-all"
            style={{
              background: "rgba(255,255,255,0.05)",
              color: "#fff",
              border: "1px solid rgba(255,255,255,0.1)",
              borderRadius: "16px",
              padding: "16px 32px",
              fontWeight: "700",
              fontSize: "13px",
              cursor: "pointer",
              position: "relative",
              zIndex: 1,
            }}
            onMouseEnter={(e) => {
              e.currentTarget.style.background = "rgba(255,255,255,0.1)";
              e.currentTarget.style.transform = "translateY(-2px)";
            }}
            onMouseLeave={(e) => {
              e.currentTarget.style.background = "rgba(255,255,255,0.05)";
              e.currentTarget.style.transform = "translateY(0)";
            }}
          >
            <RefreshCw size={16} /> RETRY CONNECTION
          </button>
        </div>
      </motion.div>
    );
  }

  if (profile) {
    const lastGame =
      profile.RecentlyPlayed && profile.RecentlyPlayed.length > 0
        ? profile.RecentlyPlayed[0]
        : null;
    const bannerUrl = lastGame
      ? `https://media.retroachievements.org${lastGame.ImageIcon}`
      : "";

    return (
      <motion.div
        initial={{ opacity: 0, y: 10 }}
        animate={{ opacity: 1, y: 0 }}
        className="w-full flex-col align-center justify-center p-m"
        style={{ flex: 1 }}
      >
        <div
          style={{
            width: "100%",
            maxWidth: "800px",
            borderRadius: "24px",
            overflow: "hidden",
            border: "1px solid rgba(255, 255, 255, 0.08)",
            background: "rgba(10, 10, 12, 0.5)",
            backdropFilter: "blur(30px)",
            position: "relative",
            boxShadow: "0 20px 50px rgba(0,0,0,0.5)",
          }}
        >
          {/* Hero Banner */}
          <div
            style={{
              height: "220px",
              width: "100%",
              position: "relative",
              overflow: "hidden",
              background: "#111",
            }}
          >
            {bannerUrl && (
              <div
                style={{
                  position: "absolute",
                  inset: -40,
                  backgroundImage: `url(${bannerUrl})`,
                  backgroundSize: "cover",
                  backgroundPosition: "center",
                  filter: "blur(40px) brightness(0.5) saturate(1.2)",
                  zIndex: 0,
                }}
              />
            )}
            <div
              style={{
                position: "absolute",
                inset: 0,
                background:
                  "linear-gradient(to bottom, transparent 0%, rgba(10, 10, 12, 0.8) 70%, rgba(10, 10, 12, 1) 100%)",
                zIndex: 1,
              }}
            />
          </div>

          <div
            style={{
              padding: "0 40px 40px 40px",
              position: "relative",
              zIndex: 2,
              marginTop: "-80px",
            }}
          >
            {/* Header: Avatar, Info & Sign Out */}
            <div
              className="flex-row justify-between align-end mb-xl"
              style={{ gap: "20px", flexWrap: "wrap" }}
            >
              <div
                className="flex-row align-end gap-l"
                style={{ flexWrap: "wrap" }}
              >
                <div
                  style={{
                    width: "140px",
                    height: "140px",
                    borderRadius: "50%",
                    overflow: "hidden",
                    border: "4px solid rgba(20,20,25,1)",
                    boxShadow: "0 10px 30px rgba(0,0,0,0.8)",
                    background: "#111",
                    position: "relative",
                  }}
                >
                  <img
                    src={`https://media.retroachievements.org/UserPic/${profile.User}.png`}
                    onError={(e) =>
                      (e.target.src =
                        "https://media.retroachievements.org/UserPic/RetroAchievements.png")
                    }
                    style={{
                      width: "100%",
                      height: "100%",
                      objectFit: "cover",
                    }}
                    alt="Avatar"
                  />
                  {profile.Rank && (
                    <div
                      style={{
                        position: "absolute",
                        bottom: 0,
                        left: 0,
                        right: 0,
                        background: "rgba(0,0,0,0.6)",
                        backdropFilter: "blur(8px)",
                        color: "#fff",
                        textAlign: "center",
                        fontSize: "10px",
                        fontWeight: "900",
                        padding: "6px 0",
                        letterSpacing: "0.15em",
                      }}
                    >
                      TOP {(parseInt(profile.Rank) / 1000).toFixed(1)}K
                    </div>
                  )}
                </div>

                <div className="flex-col pb-s">
                  <h2
                    className="m-0 font-black tracking-widest"
                    style={{
                      fontSize: "clamp(24px, 5vw, 36px)",
                      textShadow: "0 2px 20px rgba(0,0,0,0.8)",
                    }}
                  >
                    {profile.User.toUpperCase()}
                  </h2>
                  <div
                    className="text-xs opacity-60 font-bold uppercase tracking-widest mt-xs"
                    style={{
                      display: "flex",
                      alignItems: "center",
                      gap: "8px",
                    }}
                  >
                    <Shield size={14} /> {profile.Motto || "RETRO GAMER"}
                  </div>
                </div>
              </div>

              <div className="pb-s">
                <button
                  onClick={handleLogout}
                  className="btn-secondary transition-all"
                  style={{
                    background: "rgba(255,255,255,0.05)",
                    border: "1px solid rgba(255,255,255,0.1)",
                    color: "rgba(255,255,255,0.7)",
                    borderRadius: "100px",
                    padding: "10px 24px",
                    fontSize: "11px",
                    fontWeight: "800",
                    letterSpacing: "0.1em",
                    cursor: "pointer",
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.background = "rgba(255,50,50,0.2)";
                    e.currentTarget.style.color = "#ff6b6b";
                    e.currentTarget.style.borderColor = "rgba(255,50,50,0.4)";
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.background = "rgba(255,255,255,0.05)";
                    e.currentTarget.style.color = "rgba(255,255,255,0.7)";
                    e.currentTarget.style.borderColor = "rgba(255,255,255,0.1)";
                  }}
                >
                  SIGN OUT
                </button>
              </div>
            </div>

            {/* Content Split: Stats & Games */}
            <div
              className="flex-row gap-xl w-full"
              style={{ flexWrap: "wrap" }}
            >
              {/* Left Column: Stats Grid */}
              <div
                className="flex-col gap-m"
                style={{ minWidth: "220px", flex: 1 }}
              >
                <div
                  className="flex-col gap-s p-l"
                  style={{
                    background: "rgba(255,255,255,0.03)",
                    borderRadius: "20px",
                    border: "1px solid rgba(255,255,255,0.04)",
                  }}
                >
                  <div className="text-xs opacity-40 font-bold uppercase tracking-widest flex-row align-center gap-xs">
                    <Trophy size={14} /> GLOBAL RANK
                  </div>
                  <div
                    className="font-black"
                    style={{ fontSize: "clamp(24px, 4vw, 32px)" }}
                  >
                    {profile.Rank ? `#${profile.Rank}` : "N/A"}
                  </div>
                </div>
                <div
                  className="flex-col gap-s p-l"
                  style={{
                    background: "rgba(255,255,255,0.03)",
                    borderRadius: "20px",
                    border: "1px solid rgba(255,255,255,0.04)",
                  }}
                >
                  <div className="text-xs opacity-40 font-bold uppercase tracking-widest flex-row align-center gap-xs">
                    <Star size={14} /> TOTAL POINTS
                  </div>
                  <div
                    className="font-black"
                    style={{ fontSize: "clamp(24px, 4vw, 32px)" }}
                  >
                    {profile.TotalPoints}
                  </div>
                </div>
              </div>

              {/* Right Column: Recently Played */}
              <div className="flex-col" style={{ flex: 2, minWidth: "280px" }}>
                <div className="text-xs opacity-50 mb-m font-bold uppercase tracking-widest flex-row align-center gap-xs">
                  <Clock size={14} /> RECENTLY PLAYED
                </div>

                {profile.RecentlyPlayed && profile.RecentlyPlayed.length > 0 ? (
                  <div
                    className="flex-row gap-m"
                    style={{ overflowX: "auto", paddingBottom: "16px" }}
                  >
                    {profile.RecentlyPlayed.slice(0, 4).map((game) => (
                      <div
                        key={game.GameID}
                        className="flex-col gap-s"
                        style={{
                          width: "130px",
                          flexShrink: 0,
                          cursor: "pointer",
                        }}
                      >
                        <div
                          style={{
                            width: "100%",
                            height: "130px",
                            borderRadius: "20px",
                            background: "rgba(255,255,255,0.05)",
                            overflow: "hidden",
                            border: "1px solid rgba(255,255,255,0.05)",
                            position: "relative",
                          }}
                        >
                          <img
                            src={`https://media.retroachievements.org${game.ImageIcon}`}
                            alt={game.Title}
                            style={{
                              width: "100%",
                              height: "100%",
                              objectFit: "cover",
                            }}
                          />
                        </div>
                        <div
                          className="text-xs opacity-80 font-medium"
                          style={{
                            whiteSpace: "nowrap",
                            overflow: "hidden",
                            textOverflow: "ellipsis",
                            width: "100%",
                            padding: "0 4px",
                          }}
                          title={game.Title}
                        >
                          {game.Title}
                        </div>
                      </div>
                    ))}
                  </div>
                ) : (
                  <div
                    className="text-sm opacity-30 mt-m p-l text-center"
                    style={{
                      background: "rgba(255,255,255,0.02)",
                      borderRadius: "16px",
                      border: "1px dashed rgba(255,255,255,0.1)",
                    }}
                  >
                    No recently played games found. Time to play!
                  </div>
                )}
              </div>
            </div>
          </div>
        </div>
      </motion.div>
    );
  }

  return (
    <motion.div
      initial={{ opacity: 0, y: 10 }}
      animate={{ opacity: 1, y: 0 }}
      className="w-full flex-col align-center justify-center"
      style={{ flex: 1, padding: "var(--space-4)" }}
    >
      <div
        className="glass-panel"
        style={{
          width: "100%",
          maxWidth: "440px",
          borderRadius: "24px",
          background: "rgba(20, 20, 25, 0.4)",
          border: "1px solid rgba(255, 255, 255, 0.05)",
          backdropFilter: "blur(30px)",
          padding: "clamp(24px, 5vw, 40px)",
        }}
      >
        <div className="flex-col align-center text-center mb-xl">
          <div
            className="mb-m"
            style={{
              width: "clamp(48px, 8vw, 64px)",
              height: "clamp(48px, 8vw, 64px)",
              borderRadius: "50%",
              background: "rgba(255,255,255,0.03)",
              border: "1px solid rgba(255,255,255,0.08)",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
            }}
          >
            <User size={24} style={{ color: "rgba(255,255,255,0.8)" }} />
          </div>
          <h2
            className="m-0 font-light tracking-wide"
            style={{ color: "#fff", fontSize: "clamp(18px, 4vw, 24px)" }}
          >
            RetroAchievements
          </h2>
          <p
            className="opacity-40 mt-xs font-medium"
            style={{ fontSize: "clamp(10px, 2vw, 12px)" }}
          >
            Link your account to sync progress
          </p>
        </div>

        <form
          onSubmit={handleConnect}
          className="flex-col w-full"
          style={{ marginTop: "clamp(16px, 4vw, 24px)" }}
        >
          <div
            style={{
              background: "rgba(0,0,0,0.3)",
              borderRadius: "16px",
              border: "1px solid rgba(255,255,255,0.08)",
              overflow: "hidden",
              display: "flex",
              flexDirection: "column",
            }}
          >
            <input
              type="text"
              className="input-field"
              style={{
                background: "transparent",
                border: "none",
                borderBottom: "1px solid rgba(255,255,255,0.05)",
                borderRadius: "0",
                padding: "clamp(12px, 3vw, 16px) 20px",
                color: "#fff",
                fontSize: "clamp(13px, 3vw, 15px)",
                width: "100%",
                outline: "none",
                boxSizing: "border-box",
              }}
              placeholder="Username"
              value={settings.ra_user || ""}
              onChange={(e) =>
                setSettings({ ...settings, ra_user: e.target.value })
              }
              required
            />
            <input
              type="password"
              className="input-field"
              style={{
                background: "transparent",
                border: "none",
                borderBottom: "1px solid rgba(255,255,255,0.05)",
                borderRadius: "0",
                padding: "clamp(12px, 3vw, 16px) 20px",
                color: "#fff",
                fontSize: "clamp(13px, 3vw, 15px)",
                width: "100%",
                outline: "none",
                boxSizing: "border-box",
              }}
              placeholder="Web API Key"
              value={settings.ra_key || ""}
              onChange={(e) =>
                setSettings({ ...settings, ra_key: e.target.value })
              }
              required
            />
            <input
              type="password"
              className="input-field"
              style={{
                background: "transparent",
                border: "none",
                borderRadius: "0",
                padding: "clamp(12px, 3vw, 16px) 20px",
                color: "#fff",
                fontSize: "clamp(13px, 3vw, 15px)",
                width: "100%",
                outline: "none",
                boxSizing: "border-box",
              }}
              placeholder="Password"
              value={settings.ra_password || ""}
              onChange={(e) =>
                setSettings({ ...settings, ra_password: e.target.value })
              }
              required
            />
            <div
              style={{
                position: "relative",
                display: "flex",
                alignItems: "center",
              }}
            >
              <input
                type="password"
                className="input-field"
                style={{
                  background: "transparent",
                  border: "none",
                  borderRadius: "0",
                  padding: "clamp(12px, 3vw, 16px) 20px",
                  paddingRight: "clamp(90px, 20vw, 120px)",
                  color: "#fff",
                  fontSize: "clamp(13px, 3vw, 15px)",
                  width: "100%",
                  outline: "none",
                  boxSizing: "border-box",
                }}
                placeholder="Web API Key"
                value={settings.ra_key || ""}
                onChange={(e) =>
                  setSettings({ ...settings, ra_key: e.target.value })
                }
                required
              />
              <span
                className="opacity-60 hover:opacity-100 transition-opacity flex-row align-center gap-xs"
                style={{
                  position: "absolute",
                  right: "16px",
                  cursor: "pointer",
                  fontWeight: "bold",
                  fontSize: "clamp(9px, 2vw, 11px)",
                }}
                onClick={() =>
                  open("https://retroachievements.org/controlpanel.php")
                }
              >
                Find Key <ExternalLink size={10} />
              </span>
            </div>
          </div>

          <button
            type="submit"
            className="w-full"
            style={{
              background: "#fff",
              color: "#000",
              border: "none",
              borderRadius: "16px",
              padding: "clamp(14px, 3vw, 16px)",
              fontWeight: "800",
              fontSize: "clamp(12px, 2vw, 13px)",
              letterSpacing: "0.05em",
              cursor: "pointer",
              transition: "all 0.2s",
              marginTop: "clamp(16px, 4vw, 24px)",
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
              gap: "8px",
            }}
            disabled={profileLoading}
          >
            {profileLoading ? "CONNECTING..." : "SIGN IN"}{" "}
            {!profileLoading && <ArrowRight size={16} />}
          </button>
        </form>

        <div
          className="flex-row justify-center pt-m"
          style={{ marginTop: "clamp(16px, 4vw, 24px)" }}
        >
          <button
            className="flex-row align-center gap-s"
            style={{
              background: "rgba(255,255,255,0.03)",
              border: "1px solid rgba(255,255,255,0.1)",
              borderRadius: "100px",
              padding: "clamp(8px, 2vw, 12px) clamp(16px, 4vw, 24px)",
              color: "#fff",
              cursor: "pointer",
              transition: "all 0.2s",
              outline: "none",
            }}
            onMouseEnter={(e) => {
              e.currentTarget.style.background = "rgba(255,255,255,0.08)";
              e.currentTarget.style.transform = "scale(1.02)";
            }}
            onMouseLeave={(e) => {
              e.currentTarget.style.background = "rgba(255,255,255,0.03)";
              e.currentTarget.style.transform = "scale(1)";
            }}
            onClick={() =>
              open("https://retroachievements.org/createaccount.php")
            }
          >
            <span
              style={{
                fontSize: "clamp(10px, 2vw, 12px)",
                opacity: 0.5,
                fontWeight: 500,
              }}
            >
              Don't have an account?
            </span>
            <span
              style={{
                fontSize: "clamp(11px, 2.5vw, 13px)",
                fontWeight: 800,
                borderBottom: "1px solid rgba(255,255,255,0.3)",
                paddingBottom: "1px",
              }}
            >
              Register Here
            </span>
            <ArrowRight size={12} style={{ opacity: 0.8, marginLeft: "4px" }} />
          </button>
        </div>
      </div>
    </motion.div>
  );
};

export default AccountTab;
