use tauri::{AppHandle, Runtime, Manager, Emitter};
use tauri_plugin_shell::ShellExt;
use std::fs;
use std::path::Path;
use chrono::{self, DateTime, Local};
use crate::db::get_db_conn;

#[derive(serde::Serialize)]
pub struct SaveStateInfo {
    pub path: String,
    pub slot: String,
    pub timestamp: String,
    pub thumbnail: Option<String>,
    pub screenshot: Option<String>,
}

#[derive(serde::Deserialize)]
pub struct ControllerMap {
    pub up: i32,
    pub down: i32,
    pub left: i32,
    pub right: i32,
    pub a: i32,
    pub b: i32,
    pub c: i32,
    pub x: i32,
    pub y: i32,
    pub l: i32,
    pub r: i32,
    pub l2: i32,
    pub r2: i32,
    pub l3: i32,
    pub r3: i32,
    pub select: i32,
    pub start: i32,
    // Gamepad mappings
    #[serde(rename = "gpUp")]
    pub gp_up: i32,
    #[serde(rename = "gpDown")]
    pub gp_down: i32,
    #[serde(rename = "gpLeft")]
    pub gp_left: i32,
    #[serde(rename = "gpRight")]
    pub gp_right: i32,
    #[serde(rename = "gpA")]
    pub gp_a: i32,
    #[serde(rename = "gpB")]
    pub gp_b: i32,
    #[serde(rename = "gpC")]
    pub gp_c: i32,
    #[serde(rename = "gpX")]
    pub gp_x: i32,
    #[serde(rename = "gpY")]
    pub gp_y: i32,
    #[serde(rename = "gpL")]
    pub gp_l: i32,
    #[serde(rename = "gpR")]
    pub gp_r: i32,
    #[serde(rename = "gpL2")]
    pub gp_l2: i32,
    #[serde(rename = "gpR2")]
    pub gp_r2: i32,
    #[serde(rename = "gpL3")]
    pub gp_l3: i32,
    #[serde(rename = "gpR3")]
    pub gp_r3: i32,
    #[serde(rename = "gpSelect")]
    pub gp_select: i32,
    #[serde(rename = "gpStart")]
    pub gp_start: i32,
}

#[tauri::command]
pub async fn get_save_states<R: Runtime>(app: AppHandle<R>, rom_path: String) -> Result<Vec<SaveStateInfo>, String> {
    let app_dir = app.path().app_data_dir().map_err(|e| e.to_string())?;
    let raw_game_name = Path::new(&rom_path).file_stem().unwrap().to_str().unwrap();
    let game_name = raw_game_name.chars()
        .filter(|c| c.is_alphanumeric() || *c == ' ' || *c == '-' || *c == '_')
        .collect::<String>()
        .trim()
        .to_string();

    let states_dir = app_dir.join("states").join(&game_name);
    let conn = get_db_conn(&app)?;

    // 1. SYNC FROM FILESYSTEM TO DB
    if states_dir.exists() {
        if let Ok(entries) = fs::read_dir(&states_dir) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path.extension().and_then(|s| s.to_str()) == Some("state") {
                    let file_name = path.file_name().and_then(|s| s.to_str()).unwrap_or("unknown");
                    let file_name_str = file_name.to_string();
                    let thumb_path = path.with_extension("png");
                    let thumb_name = thumb_path.file_name().and_then(|s| s.to_str()).map(|s| s.to_string());
                    
                    let state_data = fs::read(&path).ok();
                    let screenshot_data = if thumb_path.exists() { fs::read(&thumb_path).ok() } else { None };
                    
                    let metadata = fs::metadata(&path).map_err(|e| e.to_string())?;
                    let modified: DateTime<Local> = metadata.modified().unwrap().into();
                    let timestamp = modified.format("%Y-%m-%d %H:%M:%S").to_string();
                    let slot = file_name.split('_').last().unwrap_or("0").replace(".state", "");

                    // Insert or Update in DB using game_name + file_name as a unique key for platform-agnosticism
                    let _ = conn.execute(
                        "INSERT INTO save_states (game_path, state_path, thumb_path, state_data, screenshot, timestamp, slot) 
                         VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)
                         ON CONFLICT(state_path) DO UPDATE SET 
                            state_data = COALESCE(?4, state_data),
                            screenshot = COALESCE(?5, screenshot),
                            timestamp = ?6",
                        rusqlite::params![rom_path, file_name_str, thumb_name, state_data, screenshot_data, timestamp, slot],
                    );
                }
            }
        }
    }

    // 2. READ FROM DB (DB is now the source of truth)
    let mut states = Vec::new();
    let mut stmt = conn.prepare(
        "SELECT state_path, thumb_path, screenshot, timestamp, slot FROM save_states WHERE game_path = ?1 ORDER BY timestamp DESC"
    ).map_err(|e| e.to_string())?;

    let rows = stmt.query_map(rusqlite::params![rom_path], |row| {
        let file_name: String = row.get(0)?;
        let thumb_name: Option<String> = row.get(1)?;
        
        // Reconstruct absolute paths for the frontend
        let full_state_path = states_dir.join(&file_name).to_string_lossy().to_string();
        let full_thumb_path = thumb_name.map(|n| states_dir.join(n).to_string_lossy().to_string());

        let screenshot_blob: Option<Vec<u8>> = row.get(2).ok();
        let mut screenshot_base64 = None;
        if let Some(data) = screenshot_blob {
            use base64::{Engine as _, engine::general_purpose};
            screenshot_base64 = Some(format!("data:image/png;base64,{}", general_purpose::STANDARD.encode(data)));
        }
 
        Ok(SaveStateInfo {
            path: full_state_path,
            thumbnail: full_thumb_path,
            screenshot: screenshot_base64,
            timestamp: row.get(3)?,
            slot: row.get(4)?,
        })
    }).map_err(|e| e.to_string())?;

    for state in rows {
        if let Ok(s) = state {
            states.push(s);
        }
    }

    Ok(states)
}

#[tauri::command]
pub async fn delete_save_state<R: Runtime>(app: AppHandle<R>, state_path: String) -> Result<(), String> {
    let path = Path::new(&state_path);
    
    // 1. Delete physical files (state and thumbnail)
    if path.exists() {
        fs::remove_file(path).map_err(|e| e.to_string())?;
    }

    let thumb_path = path.with_extension("png");
    if thumb_path.exists() {
        let _ = fs::remove_file(thumb_path);
    }

    // 2. Delete from DB using the FILENAME (since that's what we store now)
    let file_name = path.file_name().and_then(|s| s.to_str()).unwrap_or(&state_path);
    let conn = get_db_conn(&app)?;
    
    conn.execute("DELETE FROM save_states WHERE state_path = ?1", rusqlite::params![file_name])
        .map_err(|e| e.to_string())?;

    Ok(())
}

#[tauri::command]
pub async fn play_game<R: Runtime>(
    app: AppHandle<R>,
    rom_path: String,
    volume: f32,
    _fast_forward: bool,
    _save_state: bool,
    shader: String,
    p1_input: String,
    p1_controls: ControllerMap,
    p2_input: String,
    p2_controls: ControllerMap,
    ra_user: Option<String>,
    ra_token: Option<String>,
    state_path: Option<String>,
) -> Result<(), String> {
    println!("[Launcher] play_game called with state_path: {:?}", state_path);
    println!("[Launcher] Starting game: {}", rom_path);
    
    // Update play stats in DB
    let last_played = chrono::Local::now().format("%Y-%m-%d %H:%M:%S").to_string();
    let conn = get_db_conn(&app)?;
    let _ = conn.execute(
        "UPDATE games SET play_count = play_count + 1, last_played = ?1 WHERE path = ?2",
        rusqlite::params![last_played, rom_path],
    );

    let core_path = get_core_path(&app, &rom_path);
    
    // Check if core actually exists before trying to launch
    if !std::path::Path::new(&core_path).exists() {
        let msg = format!("Emulator core not found at: {}. Please download it in System Status.", core_path);
        eprintln!("[Launcher] Error: {}", msg);
        return Err(msg);
    }

    let app_dir = app.path().app_data_dir().unwrap_or_default();
    
    let app_system_dir = app_dir.join("system");
    let resource_system = app.path().resource_dir().unwrap_or_default().join("resources").join("system");
    let dev_resource_system = std::env::current_dir().unwrap_or_default().join("src-tauri").join("resources").join("system");
    
    let system_dir = if app_system_dir.exists() && fs::read_dir(&app_system_dir).map(|mut d| d.next().is_some()).unwrap_or(false) {
        // Use AppData system dir if it exists and is not empty (where downloaded BIOS go)
        app_system_dir.to_string_lossy().to_string()
    } else if resource_system.exists() {
        resource_system.to_string_lossy().to_string()
    } else if dev_resource_system.exists() {
        dev_resource_system.to_string_lossy().to_string()
    } else {
        app_system_dir.to_string_lossy().to_string()
    };
    
    println!("[Launcher][v2] Using system_dir for BIOS: {}", system_dir);
    
    let raw_game_name = Path::new(&rom_path).file_stem().unwrap().to_str().unwrap();
    let game_name = raw_game_name.chars()
        .filter(|c| c.is_alphanumeric() || *c == ' ' || *c == '-' || *c == '_')
        .collect::<String>()
        .trim()
        .to_string();

    let states_dir = app_dir.join("states").join(game_name);
    fs::create_dir_all(&states_dir).map_err(|e| e.to_string())?;

    // RESTORE ALL SAVES FOR THIS GAME FROM DB
    if let Ok(mut stmt) = conn.prepare("SELECT state_path, state_data, screenshot FROM save_states WHERE game_path = ?1") {
        if let Ok(mut rows) = stmt.query(rusqlite::params![rom_path]) {
            while let Ok(Some(row)) = rows.next() {
                let file_name: String = row.get(0).unwrap_or_default();
                let sd: Option<Vec<u8>> = row.get(1).ok();
                let sb: Option<Vec<u8>> = row.get(2).ok();
                
                if let Some(data) = sd {
                    let path = states_dir.join(&file_name);
                    let _ = fs::write(&path, data);
                    
                    if let Some(thumb) = sb {
                        let tp = path.with_extension("png");
                        let _ = fs::write(tp, thumb);
                    }
                }
            }
        }
    }

    let mut args = vec![
        rom_path.clone(),
        "--volume".to_string(), (volume * 100.0).to_string(),
        "--core".to_string(), core_path.clone(),
        "--shader".to_string(), shader,
        "--system-dir".to_string(), system_dir.clone(),
        "--p1-input".to_string(), p1_input,
        "--p2-input".to_string(), p2_input,
        "--states-dir".to_string(), states_dir.to_string_lossy().to_string(),
    ];

    if let Some(path) = state_path {
        if !path.is_empty() && path != "none" {
            args.push("--state".to_string());
            args.push(path);
        }
    }

    // ... (Keyboard mapping adds here)
    
    // MOVE LOG TO THE END OF ARG BUILDING (before extending with inputs)
    // Actually, let's put it right before command execution.

    // Add P1 Keyboard
    args.extend(vec!["--p1-key-up".to_string(), p1_controls.up.to_string()]);
    args.extend(vec!["--p1-key-down".to_string(), p1_controls.down.to_string()]);
    args.extend(vec!["--p1-key-left".to_string(), p1_controls.left.to_string()]);
    args.extend(vec!["--p1-key-right".to_string(), p1_controls.right.to_string()]);
    args.extend(vec!["--p1-key-a".to_string(), p1_controls.a.to_string()]);
    args.extend(vec!["--p1-key-b".to_string(), p1_controls.b.to_string()]);
    args.extend(vec!["--p1-key-c".to_string(), p1_controls.c.to_string()]);
    args.extend(vec!["--p1-key-x".to_string(), p1_controls.x.to_string()]);
    args.extend(vec!["--p1-key-y".to_string(), p1_controls.y.to_string()]);
    args.extend(vec!["--p1-key-l".to_string(), p1_controls.l.to_string()]);
    args.extend(vec!["--p1-key-r".to_string(), p1_controls.r.to_string()]);
    args.extend(vec!["--p1-key-l2".to_string(), p1_controls.l2.to_string()]);
    args.extend(vec!["--p1-key-r2".to_string(), p1_controls.r2.to_string()]);
    args.extend(vec!["--p1-key-l3".to_string(), p1_controls.l3.to_string()]);
    args.extend(vec!["--p1-key-r3".to_string(), p1_controls.r3.to_string()]);
    args.extend(vec!["--p1-key-select".to_string(), p1_controls.select.to_string()]);
    args.extend(vec!["--p1-key-start".to_string(), p1_controls.start.to_string()]);

    // Add P1 Gamepad
    args.extend(vec!["--p1-gp-up".to_string(), p1_controls.gp_up.to_string()]);
    args.extend(vec!["--p1-gp-down".to_string(), p1_controls.gp_down.to_string()]);
    args.extend(vec!["--p1-gp-left".to_string(), p1_controls.gp_left.to_string()]);
    args.extend(vec!["--p1-gp-right".to_string(), p1_controls.gp_right.to_string()]);
    args.extend(vec!["--p1-gp-a".to_string(), p1_controls.gp_a.to_string()]);
    args.extend(vec!["--p1-gp-b".to_string(), p1_controls.gp_b.to_string()]);
    args.extend(vec!["--p1-gp-c".to_string(), p1_controls.gp_c.to_string()]);
    args.extend(vec!["--p1-gp-x".to_string(), p1_controls.gp_x.to_string()]);
    args.extend(vec!["--p1-gp-y".to_string(), p1_controls.gp_y.to_string()]);
    args.extend(vec!["--p1-gp-l".to_string(), p1_controls.gp_l.to_string()]);
    args.extend(vec!["--p1-gp-r".to_string(), p1_controls.gp_r.to_string()]);
    args.extend(vec!["--p1-gp-l2".to_string(), p1_controls.gp_l2.to_string()]);
    args.extend(vec!["--p1-gp-r2".to_string(), p1_controls.gp_r2.to_string()]);
    args.extend(vec!["--p1-gp-l3".to_string(), p1_controls.gp_l3.to_string()]);
    args.extend(vec!["--p1-gp-r3".to_string(), p1_controls.gp_r3.to_string()]);
    args.extend(vec!["--p1-gp-select".to_string(), p1_controls.gp_select.to_string()]);
    args.extend(vec!["--p1-gp-start".to_string(), p1_controls.gp_start.to_string()]);

    // Add P2 Keyboard
    args.extend(vec!["--p2-key-up".to_string(), p2_controls.up.to_string()]);
    args.extend(vec!["--p2-key-down".to_string(), p2_controls.down.to_string()]);
    args.extend(vec!["--p2-key-left".to_string(), p2_controls.left.to_string()]);
    args.extend(vec!["--p2-key-right".to_string(), p2_controls.right.to_string()]);
    args.extend(vec!["--p2-key-a".to_string(), p2_controls.a.to_string()]);
    args.extend(vec!["--p2-key-b".to_string(), p2_controls.b.to_string()]);
    args.extend(vec!["--p2-key-c".to_string(), p2_controls.c.to_string()]);
    args.extend(vec!["--p2-key-x".to_string(), p2_controls.x.to_string()]);
    args.extend(vec!["--p2-key-y".to_string(), p2_controls.y.to_string()]);
    args.extend(vec!["--p2-key-l".to_string(), p2_controls.l.to_string()]);
    args.extend(vec!["--p2-key-r".to_string(), p2_controls.r.to_string()]);
    args.extend(vec!["--p2-key-l2".to_string(), p2_controls.l2.to_string()]);
    args.extend(vec!["--p2-key-r2".to_string(), p2_controls.r2.to_string()]);
    args.extend(vec!["--p2-key-l3".to_string(), p2_controls.l3.to_string()]);
    args.extend(vec!["--p2-key-r3".to_string(), p2_controls.r3.to_string()]);
    args.extend(vec!["--p2-key-select".to_string(), p2_controls.select.to_string()]);
    args.extend(vec!["--p2-key-start".to_string(), p2_controls.start.to_string()]);

    // Add P2 Gamepad
    args.extend(vec!["--p2-gp-up".to_string(), p2_controls.gp_up.to_string()]);
    args.extend(vec!["--p2-gp-down".to_string(), p2_controls.gp_down.to_string()]);
    args.extend(vec!["--p2-gp-left".to_string(), p2_controls.gp_left.to_string()]);
    args.extend(vec!["--p2-gp-right".to_string(), p2_controls.gp_right.to_string()]);
    args.extend(vec!["--p2-gp-a".to_string(), p2_controls.gp_a.to_string()]);
    args.extend(vec!["--p2-gp-b".to_string(), p2_controls.gp_b.to_string()]);
    args.extend(vec!["--p2-gp-c".to_string(), p2_controls.gp_c.to_string()]);
    args.extend(vec!["--p2-gp-x".to_string(), p2_controls.gp_x.to_string()]);
    args.extend(vec!["--p2-gp-y".to_string(), p2_controls.gp_y.to_string()]);
    args.extend(vec!["--p2-gp-l".to_string(), p2_controls.gp_l.to_string()]);
    args.extend(vec!["--p2-gp-r".to_string(), p2_controls.gp_r.to_string()]);
    args.extend(vec!["--p2-gp-l2".to_string(), p2_controls.gp_l2.to_string()]);
    args.extend(vec!["--p2-gp-r2".to_string(), p2_controls.gp_r2.to_string()]);
    args.extend(vec!["--p2-gp-l3".to_string(), p2_controls.gp_l3.to_string()]);
    args.extend(vec!["--p2-gp-r3".to_string(), p2_controls.gp_r3.to_string()]);
    args.extend(vec!["--p2-gp-select".to_string(), p2_controls.gp_select.to_string()]);
    args.extend(vec!["--p2-gp-start".to_string(), p2_controls.gp_start.to_string()]);

    if let Some(user) = ra_user {
        args.extend(vec!["--ra-user".to_string(), user]);
    }
    if let Some(token) = ra_token {
        args.extend(vec!["--ra-token".to_string(), token]);
    }

    // Verify BIOS presence for PS1
    if core_path.contains("pcsx_rearmed") {
        let scph = ["scph5501.bin", "scph5500.bin", "scph5502.bin"];
        let mut found = false;
        for b in scph {
            if Path::new(&system_dir).join(b).exists() {
                println!("[Launcher] Found PS1 BIOS: {}", b);
                found = true;
            }
        }
        if !found {
            println!("[Launcher] WARNING: No common PS1 BIOS found in {}", system_dir);
        }
    }

    let shell = app.shell();
    let cmd = shell.sidecar("MegaDriveEmu").map_err(|e| e.to_string())?.args(args);
    
    // Run the game and capture output for debugging
    let output = cmd.output().await.map_err(|e| e.to_string())?;
    
    let stdout = String::from_utf8_lossy(&output.stdout);
    let stderr = String::from_utf8_lossy(&output.stderr);

    println!("[Launcher] Emulator Output (stdout):\n{}", stdout);
    
    if !output.status.success() {
        eprintln!("[Launcher] Emulator Error (stderr):\n{}", stderr);
        
        // Analyze common errors to give better feedback
        if stdout.contains("unsupported/invalid CD image") || stderr.contains("unsupported/invalid CD image") {
            return Err("Эмулятор не поддерживает формат .iso для этой платформы. Пожалуйста, используйте .chd или .bin/.cue.".to_string());
        }
        
        if stdout.contains("No BIOS files found") {
            return Err("Эмулятор не нашел файлы BIOS. Проверьте, что они лежат в папке 'system' и названы маленькими буквами (например, scph5501.bin).".to_string());
        }

        return Err(format!("Эмулятор завершился с ошибкой. Проверьте консоль для деталей."));
    }
    
    println!("[Launcher] Game exited successfully");
    
    // Emit event as a fallback for frontend
    let _ = app.emit("emulator-exit", ());

    // FINAL SYNC: Move new saves to DB after game closes
    let _ = get_save_states(app, rom_path).await;

    Ok(())
}

fn get_core_extension() -> &'static str {
    #[cfg(target_os = "windows")] { "dll" }
    #[cfg(target_os = "linux")] { "so" }
    #[cfg(target_os = "macos")] { "dylib" }
    #[cfg(not(any(target_os = "windows", target_os = "linux", target_os = "macos")))] { "so" }
}

fn get_buildbot_platform() -> &'static str {
    #[cfg(all(target_os = "macos", target_arch = "aarch64"))] { "apple/osx/arm64" }
    #[cfg(all(target_os = "macos", target_arch = "x86_64"))] { "apple/osx/x86_64" }
    #[cfg(target_os = "windows")] { "windows/x86_64" }
    #[cfg(all(target_os = "linux", target_arch = "x86_64"))] { "linux/x86_64" }
    #[cfg(all(target_os = "linux", target_arch = "aarch64"))] { "linux/aarch64" }
    #[cfg(not(any(target_os = "windows", target_os = "linux", target_os = "macos")))] { "linux/x86_64" }
}

fn get_core_filename(base: &str) -> String {
    format!("{}_libretro.{}", base, get_core_extension())
}

fn get_core_path<R: Runtime>(app: &AppHandle<R>, rom_path: &str) -> String {
    let ext = rom_path.split('.').last().unwrap_or("").to_lowercase();
    let app_dir = app.path().app_data_dir().unwrap_or_default();
    let cores_dir = app_dir.join("third_party").join("cores");
    
    // Try to detect platform from DB to be more precise with generic extensions like .iso
    let platform = {
        let conn = get_db_conn(app).ok();
        conn.and_then(|c| {
            let mut stmt = c.prepare("SELECT platform FROM games WHERE path = ?1").ok()?;
            stmt.query_row([rom_path], |row| row.get::<_, String>(0)).ok()
        })
    };

    let core_base = match ext.as_str() {
        "nes" => "fceumm",
        "sfc" | "smc" => "snes9x",
        "gb" | "gbc" => "gambatte",
        "gba" => "mgba",
        "pce" | "tg16" => "beetle_pce_fast",
        "lnx" => "handy",
        "c64" | "d64" | "prg" => "vice_x64sc",
        "ps1" | "psx" | "cue" | "chd" => "pcsx_rearmed",
        "n64" | "v64" | "z64" => "mupen64plus_next",
        "nds" => "desmume",
        "psp" => "ppsspp",
        "iso" => {
            if let Some(p) = platform {
                if p.to_uppercase().contains("PLAYSTATION") && !p.to_uppercase().contains("PORTABLE") {
                    "pcsx_rearmed" // Use PS1 core for PS1 ISOs
                } else if p.to_uppercase().contains("PLAYSTATION 2") {
                    "play"
                } else {
                    "ppsspp" // Default ISO to PSP
                }
            } else {
                "ppsspp"
            }
        },
        "cdi" | "gdi" => "flycast",
        "gcm" | "rvz" => "dolphin",
        "dos" | "exe" | "conf" => "dosbox_pure",
        "ss" | "saturn" => "yabause",
        "3do" => "opera",
        "a26" | "bin" => {
            if let Some(p) = platform {
                if p.to_uppercase().contains("ATARI 2600") { "stella" }
                else { "genesis_plus_gx" } // Default .bin to Genesis if not sure
            } else { "stella" }
        },
        "a78" | "a7800" => "prosystem",
        "ps2" => "play",
        "3ds" | "cia" => "citra",
        "msx" | "msx1" | "msx2" | "dsk" => "fmsx",
        "adf" => "puae",
        "scummvm" => "scummvm",
        "ws" | "wsc" => "mednafen_wswan",
        "ngp" | "ngc" => "mednafen_ngp",
        "vb" => "mednafen_vb",
        "o2" => "o2em",
        "zip" | "7z" => "fbneo",
        _ => "genesis_plus_gx",
    };
    
    let core_name = get_core_filename(core_base);
    
    // Check resources first
    let res_path = app.path().resource_dir().unwrap_or_default().join("resources").join("cores").join(&core_name);
    if res_path.exists() {
        return res_path.to_string_lossy().to_string();
    }
    
    // Dev fallback
    let dev_res_path = std::env::current_dir().unwrap_or_default().join("src-tauri").join("resources").join("cores").join(&core_name);
    if dev_res_path.exists() {
        return dev_res_path.to_string_lossy().to_string();
    }
    
    cores_dir.join(core_name).to_string_lossy().to_string()
}

#[derive(Clone, serde::Serialize)]
struct DownloadPayload {
    file: String,
    progress: f64,
    status: String,
}

#[tauri::command]
pub async fn download_cores<R: Runtime>(app: AppHandle<R>) -> Result<(), String> {
    let platform = get_buildbot_platform();
    let base_url = format!("https://buildbot.libretro.com/nightly/{}/latest/", platform);
    
    let app_dir = app.path().app_data_dir().map_err(|e| e.to_string())?;
    let cores_dir = app_dir.join("third_party").join("cores");
    let _ = fs::create_dir_all(&cores_dir);

    let _ = app.emit("download-progress", DownloadPayload {
        file: "Fetching Index".to_string(),
        progress: 0.0,
        status: "Scanning Libretro Buildbot...".to_string(),
    });

    let client = reqwest::Client::builder()
        .user_agent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36")
        .build()
        .map_err(|e| e.to_string())?;
        
    let index_html = client.get(&base_url).send().await.map_err(|e| e.to_string())?.text().await.map_err(|e| e.to_string())?;
    
    // Improved parsing: search for anything inside href="..." that ends with .zip
    let mut core_links = Vec::new();
    
    // Look for both "href=" and "HREF=" and handle different quote types
    for line in index_html.lines() {
        let line_lower = line.to_lowercase();
        if line_lower.contains(".zip") {
            // Find all occurrences of href="..." in the line
            let parts: Vec<&str> = line.split("href=\"").collect();
            for part in parts.iter().skip(1) {
                if let Some(end) = part.find("\"") {
                    let link = &part[..end];
                    if link.ends_with(".zip") {
                        core_links.push(link.to_string());
                    }
                }
            }
            
            // Also try single quotes if double quotes failed for this part
            let parts_single: Vec<&str> = line.split("href='").collect();
            for part in parts_single.iter().skip(1) {
                if let Some(end) = part.find("'") {
                    let link = &part[..end];
                    if link.ends_with(".zip") && !core_links.contains(&link.to_string()) {
                        core_links.push(link.to_string());
                    }
                }
            }
        }
    }

    let total_cores = core_links.len() as f64;
    if total_cores == 0.0 {
        return Err("No cores found on the server. Please check your internet connection.".to_string());
    }

    for (index, core_zip) in core_links.iter().enumerate() {
        let url = format!("{}{}", base_url, core_zip);
        let progress = (index as f64 / total_cores) * 100.0;
        
        let _ = app.emit("download-progress", DownloadPayload {
            file: core_zip.clone(),
            progress,
            status: format!("Downloading Cores: {:.0}% ({}/{})", progress, index + 1, total_cores as i32),
        });

        let res = client.get(&url).send().await.map_err(|e| e.to_string())?;
        if !res.status().is_success() { continue; }

        let bytes = res.bytes().await.map_err(|e| e.to_string())?;
        let reader = std::io::Cursor::new(bytes);
        
        if let Ok(mut archive) = zip::ZipArchive::new(reader) {
            for i in 0..archive.len() {
                if let Ok(mut file) = archive.by_index(i) {
                    let outpath = cores_dir.join(file.name());
                    if let Ok(mut outfile) = fs::File::create(&outpath) {
                        let _ = std::io::copy(&mut file, &mut outfile);
                    }
                }
            }
        }
    }

    let _ = app.emit("download-progress", DownloadPayload {
        file: "Complete".to_string(),
        progress: 100.0,
        status: format!("Successfully installed {} cores!", total_cores as i32),
    });

    Ok(())
}

#[tauri::command]
pub async fn download_bios_pack<R: Runtime>(app: AppHandle<R>) -> Result<(), String> {
    let url = "https://github.com/Abdess/retrobios/releases/download/v2026.04.02/RetroArch_Lakka_v1.22.2_Platform_BIOS_Pack.zip";

    let app_dir = app.path().app_data_dir().map_err(|e| e.to_string())?;
    let system_dir = app_dir.join("system");
    let _ = fs::create_dir_all(&system_dir);

    app.emit("download-progress", DownloadPayload {
        file: "BIOS Pack".to_string(),
        progress: 0.0,
        status: "Starting BIOS download...".to_string(),
    }).map_err(|e: tauri::Error| e.to_string())?;

    let client = reqwest::Client::new();
    let res = client.get(url)
        .header("User-Agent", "Mozilla/5.0")
        .send().await.map_err(|e| e.to_string())?;
    
    if !res.status().is_success() {
        return Err(format!("Download failed with status: {}. Server busy.", res.status()));
    }

    let total_size = res.content_length().unwrap_or(0);
    let mut bytes = Vec::new();
    let mut downloaded: u64 = 0;
    let mut stream = res.bytes_stream();

    use futures::StreamExt;
    while let Some(item) = stream.next().await {
        let chunk = item.map_err(|e: reqwest::Error| e.to_string())?;
        downloaded += chunk.len() as u64;
        bytes.extend_from_slice(&chunk);
        
        if total_size > 0 {
            let progress = (downloaded as f64 / total_size as f64) * 100.0;
            app.emit("download-progress", DownloadPayload {
                file: "BIOS Pack".to_string(),
                progress,
                status: format!("Downloading BIOS: {:.1}%", progress),
            }).map_err(|e: tauri::Error| e.to_string())?;
        }
    }

    let reader = std::io::Cursor::new(bytes);
    let mut archive = zip::ZipArchive::new(reader).map_err(|e| e.to_string())?;

    for i in 0..archive.len() {
        let mut file = archive.by_index(i).map_err(|e| e.to_string())?;
        let file_name = file.name().to_string();
        
        if file.is_dir() { continue; }

        // Logic to flatten top-level redundant folders if any
        // If file is "RetroArch_BIOS_Pack/system/dc/dc_boot.bin", we might want "dc/dc_boot.bin"
        // For simplicity, let's just take the filename if it's a common BIOS, 
        // but preserve specific structures like "dc/" or "pcsx2/".
        
        let path_parts: Vec<&str> = file_name.split('/').collect();
        let target_name = if path_parts.len() > 1 {
            let first = path_parts[0].to_lowercase();
            if first.contains("bios") || first.contains("pack") || first == "system" {
                // If it's something like "system/dc/bios.bin", we want "dc/bios.bin"
                // If it's just "system/bios.bin", we want "bios.bin"
                path_parts[1..].join("/")
            } else {
                file_name
            }
        } else {
            file_name
        };

        let outpath = system_dir.join(target_name);
        
        if let Some(p) = outpath.parent() {
            fs::create_dir_all(&p).map_err(|e| e.to_string())?;
        }
        
        let mut outfile = fs::File::create(&outpath).map_err(|e| e.to_string())?;
        std::io::copy(&mut file, &mut outfile).map_err(|e| e.to_string())?;
    }

    app.emit("download-progress", DownloadPayload {
        file: "Complete".to_string(),
        progress: 100.0,
        status: "BIOS Pack installed!".to_string(),
    }).map_err(|e: tauri::Error| e.to_string())?;

    Ok(())
}

#[tauri::command]
pub async fn open_bios_folder<R: Runtime>(app: AppHandle<R>) -> Result<(), String> {
    let app_dir = app.path().app_data_dir().map_err(|e| e.to_string())?;
    let system_dir = app_dir.join("system");
    
    // Ensure it exists
    let _ = fs::create_dir_all(&system_dir);
    
    #[cfg(target_os = "macos")]
    {
        std::process::Command::new("open")
            .arg(system_dir)
            .spawn()
            .map_err(|e| e.to_string())?;
    }
    
    #[cfg(target_os = "windows")]
    {
        std::process::Command::new("explorer")
            .arg(system_dir)
            .spawn()
            .map_err(|e| e.to_string())?;
    }

    #[cfg(target_os = "linux")]
    {
        std::process::Command::new("xdg-open")
            .arg(system_dir)
            .spawn()
            .map_err(|e| e.to_string())?;
    }

    Ok(())
}

#[derive(serde::Serialize)]
pub struct SystemStatus {
    pub id: String,
    pub name: String,
    pub core_ok: bool,
    pub bios_ok: bool,
    pub bios_required: bool,
}

#[tauri::command]
pub async fn get_system_status<R: Runtime>(app: AppHandle<R>) -> Result<Vec<SystemStatus>, String> {
    let app_dir = app.path().app_data_dir().unwrap_or_default();
    let cores_dir = app_dir.join("third_party").join("cores");
    let system_dir = app_dir.join("system");
    
    // Base resource dir from Tauri
    let res_dir_base = app.path().resource_dir().unwrap_or_default();
    
    // Improved dev fallback: try both root/src-tauri/resources and ./resources (if CWD is src-tauri)
    let cwd = std::env::current_dir().unwrap_or_default();
    let dev_res_dir = if cwd.join("resources").exists() {
        cwd.join("resources")
    } else if cwd.join("src-tauri").join("resources").exists() {
        cwd.join("src-tauri").join("resources")
    } else {
        cwd // Fallback
    };
    
    println!("[Status Audit] AppData: {:?}", app_dir);
    println!("[Status Audit] ResourceBase: {:?}", res_dir_base);
    println!("[Status Audit] DevResDir: {:?}", dev_res_dir);

    let res_cores = res_dir_base.join("resources").join("cores");
    let res_system = res_dir_base.join("resources").join("system");

    let systems = vec![
        ("nes", "NES", "fceumm", vec![]),
        ("snes", "SNES", "snes9x", vec![]),
        ("gb", "Game Boy", "gambatte", vec![]),
        ("gba", "GBA", "mgba", vec![]),
        ("genesis", "Mega Drive", "genesis_plus_gx", vec![]),
        ("psx", "PlayStation", "pcsx_rearmed", vec!["scph5501.bin", "scph5500.bin", "scph5502.bin", "ps1_rom.bin"]),
        ("n64", "Nintendo 64", "mupen64plus_next", vec![]),
        ("nds", "Nintendo DS", "desmume", vec!["bios7.bin", "bios9.bin", "firmware.bin"]),
        ("psp", "PSP", "ppsspp", vec![]),
        ("dreamcast", "Dreamcast", "flycast", vec!["dc/dc_boot.bin", "dc_boot.bin"]),
        ("saturn", "Saturn", "yabause", vec!["saturn_bios.bin", "stlsbios.bin"]),
        ("pce", "PC Engine", "mednafen_pce_fast", vec![]),
        ("gamecube", "GameCube / Wii", "dolphin", vec![]),
        ("ps2", "PlayStation 2", "play", vec!["pcsx2/bios/ps2_bios.bin"]),
        ("3ds", "Nintendo 3DS", "citra", vec![]),
        ("msx", "MSX / MSX2", "fmsx", vec!["MSX.ROM", "MSX2.ROM"]),
        ("amiga", "Amiga", "puae", vec!["kick34005.A500"]),
        ("c64", "Commodore 64", "vice_x64sc", vec![]),
        ("scummvm", "ScummVM", "scummvm", vec![]),
        ("wswan", "WonderSwan", "mednafen_wswan", vec![]),
        ("ngp", "Neo Geo Pocket", "mednafen_ngp", vec![]),
        ("vb", "Virtual Boy", "mednafen_vb", vec![]),
        ("o2", "Odyssey 2", "o2em", vec!["o2rom.bin"]),
        ("gg", "Game Gear", "genesis_plus_gx", vec![]),
        ("sms", "Master System", "genesis_plus_gx", vec![]),
        ("a2600", "Atari 2600", "stella", vec![]),
        ("a7800", "Atari 7800", "prosystem", vec!["7800 BIOS (U).rom"]),
        ("lynx", "Atari Lynx", "handy", vec!["lynxboot.img"]),
        ("dos", "DOS", "dosbox_pure", vec![]),
        ("3do", "3DO", "opera", vec!["panafz10.bin", "3do_bios.bin"]),
        ("arcade", "Arcade", "fbneo", vec![]),
    ];

    let mut status_list = Vec::new();

    for (id, name, core_base, bios_files) in systems {
        let core_file = get_core_filename(core_base);
        let core_ok = cores_dir.join(&core_file).exists() || 
                      res_cores.join(&core_file).exists() || 
                      dev_res_dir.join("cores").join(&core_file).exists();
        
        let bios_ok = if bios_files.is_empty() {
            true
        } else {
            // Check if AT LEAST ONE of the required BIOS files exists
            bios_files.iter().any(|f| {
                system_dir.join(f).exists() || 
                res_system.join(f).exists() || 
                dev_res_dir.join("system").join(f).exists()
            })
        };

        status_list.push(SystemStatus {
            id: id.to_string(),
            name: name.to_string(),
            core_ok,
            bios_ok,
            bios_required: !bios_files.is_empty(),
        });
    }

    Ok(status_list)
}


#[cfg(test)]
mod tests {

    #[test]
    fn test_core_mapping() {
        let cases = vec![
            ("game.nes", "fceumm_libretro.dylib"),
            ("game.sfc", "snes9x_libretro.dylib"),
            ("game.gba", "mgba_libretro.dylib"),
            ("game.md", "genesis_plus_gx_libretro.dylib"),
        ];

        for (path, expected_core) in cases {
            let ext = path.split('.').last().unwrap_or("").to_lowercase();
            let core_name = match ext.as_str() {
                "nes" => "fceumm_libretro.dylib",
                "sfc" | "smc" => "snes9x_libretro.dylib",
                "gb" | "gbc" => "gambatte_libretro.dylib",
                "gba" => "mgba_libretro.dylib",
                _ => "genesis_plus_gx_libretro.dylib",
            };
            assert_eq!(core_name, expected_core);
        }
    }
}
