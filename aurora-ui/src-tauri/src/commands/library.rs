use tauri::{AppHandle, Runtime, Manager};
use rusqlite::params;
use std::fs;
use chrono;
use crate::db::{get_db_conn, Game};
use base64;
use urlencoding;
use reqwest;
use futures;

pub fn internal_get_games(conn: &rusqlite::Connection) -> Result<Vec<Game>, String> {
    let mut stmt = conn.prepare("SELECT id, title, path, cover, cover_data, platform, play_count, last_played, added_date, nfo_path FROM games ORDER BY title ASC")
        .map_err(|e| e.to_string())?;
    
    let game_iter = stmt.query_map([], |row| {
        Ok(Game {
            id: Some(row.get(0)?),
            title: row.get(1)?,
            path: row.get(2)?,
            cover: row.get(3)?,
            cover_base64: row.get::<_, Option<Vec<u8>>>(4)?.map(|b| base64::Engine::encode(&base64::engine::general_purpose::STANDARD, b)),
            platform: row.get(5)?,
            play_count: row.get(6)?,
            last_played: row.get(7)?,
            added_date: row.get(8)?,
            nfo_path: row.get(9)?,
        })
    }).map_err(|e| e.to_string())?;

    let mut games = Vec::new();
    for game_res in game_iter {
        let mut game = game_res.map_err(|e| e.to_string())?;
        
        // Lazy migration to BLOB (logic remains here but could be further extracted)
        if game.cover_base64.is_none() {
            if let Some(ref path) = game.cover {
                if let Ok(bytes) = fs::read(path) {
                    let _ = conn.execute("UPDATE games SET cover_data = ?1 WHERE id = ?2", params![bytes, game.id]);
                    game.cover_base64 = Some(base64::Engine::encode(&base64::engine::general_purpose::STANDARD, bytes));
                }
            }
        }
        
        games.push(game);
    }
    Ok(games)
}

#[tauri::command]
pub async fn get_games<R: Runtime>(app: AppHandle<R>) -> Result<Vec<Game>, String> {
    let conn = get_db_conn(&app)?;
    internal_get_games(&conn)
}

#[tauri::command]
pub async fn add_game<R: Runtime>(app: AppHandle<R>, title: String, rom_path: String, platform: String) -> Result<Game, String> {
    let app_data = app.path().app_data_dir().expect("failed to get app data dir");
    let roms_dir = app_data.join("roms");
    
    let filename = std::path::Path::new(&rom_path).file_name().ok_or("Invalid path")?;
    let dest_path = roms_dir.join(filename);
    
    if !dest_path.exists() {
        fs::copy(&rom_path, &dest_path).map_err(|e| e.to_string())?;
    }

    let dest_path_str = dest_path.to_string_lossy().to_string();
    let cover_path = dest_path.with_extension("png");
    let cover = if cover_path.exists() { Some(cover_path.to_string_lossy().to_string()) } else { None };
    let added_date = chrono::Local::now().format("%Y-%m-%d %H:%M:%S").to_string();

    let conn = get_db_conn(&app)?;
    conn.execute(
        "INSERT OR IGNORE INTO games (title, path, cover, platform, added_date) VALUES (?1, ?2, ?3, ?4, ?5)",
        params![title, dest_path_str, cover, platform, added_date],
    ).map_err(|e| e.to_string())?;

    let id = conn.last_insert_rowid();

    Ok(Game {
        id: Some(id),
        title,
        path: dest_path_str,
        cover,
        cover_base64: None,
        platform: Some(platform.to_string()),
        play_count: 0,
        last_played: None,
        added_date,
        nfo_path: None,
    })
}

#[tauri::command]
pub async fn remove_game<R: Runtime>(app: AppHandle<R>, rom_path: String) -> Result<(), String> {
    let conn = get_db_conn(&app)?;
    conn.execute("DELETE FROM games WHERE path = ?1", params![rom_path])
        .map_err(|e| e.to_string())?;
    
    let rom_path_obj = std::path::Path::new(&rom_path);
    if rom_path_obj.exists() { let _ = fs::remove_file(rom_path_obj); }
    let cover_path = rom_path_obj.with_extension("png");
    if cover_path.exists() { let _ = fs::remove_file(cover_path); }
    Ok(())
}

#[tauri::command]
pub async fn download_cover<R: Runtime>(app: AppHandle<R>, title: String, rom_path: String) -> Result<Option<String>, String> {
    println!("[Backend] download_cover request for: {}", title);
    let rom_path_obj = std::path::Path::new(&rom_path);
    let cover_path = rom_path_obj.with_extension("png");
    let cover_path_str = cover_path.to_string_lossy().to_string();

    if cover_path.exists() {
        let bytes = fs::read(&cover_path).ok();
        let b64 = bytes.as_ref().map(|b| base64::Engine::encode(&base64::engine::general_purpose::STANDARD, b));
        let conn = get_db_conn(&app)?;
        let _ = conn.execute("UPDATE games SET cover = ?1, cover_data = ?2 WHERE path = ?3", params![&cover_path_str, bytes, &rom_path]);
        return Ok(b64);
    }

    let mut clean_title = title.clone();
    if let Some(pos) = clean_title.find('(') { clean_title = clean_title[..pos].trim().to_string(); }
    if let Some(pos) = clean_title.find('[') { clean_title = clean_title[..pos].trim().to_string(); }

    let mut bases = vec![title.clone(), clean_title.clone()];
    // Add no-space variant (Libretro often uses DuckTales instead of Duck Tales)
    let no_space = clean_title.replace(" ", "");
    if no_space != clean_title { bases.push(no_space); }
    
    let regions = vec!["", " (World)", " (USA)", " (Europe)", " (USA, Europe)", " (Japan, USA)", " (Japan)"];
    let mut futures = Vec::new();
    
    let ext = rom_path.split('.').last().unwrap_or("").to_lowercase();
    let system_folder = match ext.as_str() {
        "nes" => "Nintendo%20-%20Nintendo%20Entertainment%20System",
        "sfc" | "smc" => "Nintendo%20-%20Super%20Nintendo%20Entertainment%20System",
        "gb" => "Nintendo%20-%20Game%20Boy",
        "gbc" => "Nintendo%20-%20Game%20Boy%20Color",
        "gba" => "Nintendo%20-%20Game%20Boy%20Advance",
        "sms" => "Sega%20-%20Master%20System%20-%20Mark%20III",
        "gg" => "Sega%20-%20Game%20Gear",
        "pce" => "NEC%20-%20PC%20Engine%20-%20TurboGrafx%2016",
        _ => "Sega%20-%20Mega%20Drive%20-%20Genesis",
    };

    for b in bases {
        for r in &regions {
            let variant = format!("{}{}", b, r);
            let url = format!("https://thumbnails.libretro.com/{}/Named_Boxarts/{}.png", system_folder, urlencoding::encode(&variant));
            futures.push(Box::pin(async move {
                println!("[Backend] Trying URL: {}", url);
                if let Ok(res) = reqwest::get(&url).await {
                    if res.status() == 200 { 
                        if let Ok(bytes) = res.bytes().await { 
                            println!("[Backend] Success! Found cover for {}", variant);
                            return Ok((bytes, variant)); 
                        } 
                    } else {
                        // println!("[Backend] Skip ({}): {}", res.status(), url);
                    }
                }
                Err(())
            }));
        }
    }

    if !futures.is_empty() {
        if let Ok(((bytes, variant), _)) = futures::future::select_ok(futures).await {
            println!("[Backend] Finalizing cover for: {}", variant);
            if fs::write(&cover_path, &bytes).is_ok() {
                let conn = get_db_conn(&app)?;
                let _ = conn.execute("UPDATE games SET cover = ?1, cover_data = ?2 WHERE path = ?3", params![&cover_path_str, &bytes.to_vec(), &rom_path]);
                return Ok(Some(base64::Engine::encode(&base64::engine::general_purpose::STANDARD, &bytes)));
            }
        }
    }

    Ok(None)    
}

fn decode_cp437(bytes: &[u8]) -> String {
    let cp437_map: [char; 128] = [
        'Ç', 'ü', 'é', 'â', 'ä', 'à', 'å', 'ç', 'ê', 'ë', 'è', 'ï', 'î', 'ì', 'Ä', 'Å',
        'É', 'æ', 'Æ', 'ô', 'ö', 'ò', 'û', 'ù', 'ÿ', 'Ö', 'Ü', '¢', '£', '¥', '₧', 'ƒ',
        'á', 'í', 'ó', 'ú', 'ñ', 'Ñ', 'ª', 'º', '¿', '⌐', '¬', '½', '¼', '¡', '«', '»',
        '░', '▒', '▓', '│', '┤', '╡', '╢', '╖', '╕', '╣', '║', '╗', '╝', '╜', '╛', '┐',
        '└', '┴', '┬', '├', '─', '┼', '╞', '╟', '╚', '╔', '╩', '╦', '╠', '═', '╬', '╧',
        '╨', '╤', '╥', '╙', '╘', '╒', '╓', '╫', '╪', '┘', '┌', '█', '▄', '▌', '▐', '▀',
        'α', 'ß', 'Γ', 'π', 'Σ', 'σ', 'µ', 'τ', 'Φ', 'Θ', 'Ω', 'δ', '∞', 'φ', 'ε', '∩',
        '≡', '±', '≥', '≤', '⌠', '⌡', '÷', '≈', '°', '∙', '·', '√', 'ⁿ', '²', '■', ' '
    ];

    bytes.iter().map(|&b| {
        if b < 128 {
            b as char
        } else {
            cp437_map[(b - 128) as usize]
        }
    }).collect()
}

#[tauri::command]
pub async fn get_game_nfo<R: Runtime>(app: AppHandle<R>, rom_path: String) -> Result<Option<String>, String> {
    println!("[Backend] get_game_nfo for: {}", rom_path);
    let conn = get_db_conn(&app)?;
    
    // Check if we have a linked NFO path in DB first
    let mut stmt = conn.prepare("SELECT nfo_path FROM games WHERE path = ?1").map_err(|e| e.to_string())?;
    let linked_nfo: Option<String> = stmt.query_row([&rom_path], |row| row.get(0)).ok().flatten();

    let nfo_path = if let Some(ref path) = linked_nfo {
        println!("[Backend] Using linked NFO: {}", path);
        std::path::PathBuf::from(path)
    } else {
        let rom_path_obj = std::path::Path::new(&rom_path);
        let auto_path = rom_path_obj.with_extension("nfo");
        println!("[Backend] Checking auto NFO: {:?}", auto_path);
        auto_path
    };

    if nfo_path.exists() {
        println!("[Backend] Found NFO at: {:?}", nfo_path);
        match fs::read(nfo_path) {
            Ok(bytes) => {
                // Use custom CP437 decoder for high-fidelity ASCII art
                let content = decode_cp437(&bytes);
                Ok(Some(content))
            },
            Err(e) => Err(e.to_string())
        }
    } else {
        println!("[Backend] NFO not found");
        Ok(None)
    }
}

#[tauri::command]
pub async fn select_nfo_file<R: Runtime>(app: AppHandle<R>) -> Result<Option<String>, String> {
    println!("[Backend] select_nfo_file dialog opening...");
    use tauri_plugin_dialog::DialogExt;
    let file = app.dialog().file().add_filter("NFO Files", &["nfo", "txt"]).blocking_pick_file();
    println!("[Backend] Dialog result: {:?}", file);
    Ok(file.map(|f| f.to_string()))
}

#[tauri::command]
pub async fn link_game_nfo<R: Runtime>(app: AppHandle<R>, rom_path: String, nfo_path: String) -> Result<(), String> {
    let conn = get_db_conn(&app)?;
    conn.execute("UPDATE games SET nfo_path = ?1 WHERE path = ?2", params![nfo_path, rom_path])
        .map_err(|e| e.to_string())?;
    Ok(())
}

#[tauri::command]
pub async fn update_game_platform<R: Runtime>(app: AppHandle<R>, rom_path: String, platform: String) -> Result<(), String> {
    let conn = get_db_conn(&app)?;
    conn.execute("UPDATE games SET platform = ?1 WHERE path = ?2", params![platform, rom_path])
        .map_err(|e| e.to_string())?;
    Ok(())
}

#[tauri::command]
pub async fn rename_game<R: Runtime>(app: AppHandle<R>, rom_path: String, new_title: String) -> Result<(), String> {
    let conn = get_db_conn(&app)?;
    conn.execute("UPDATE games SET title = ?1 WHERE path = ?2", params![new_title, rom_path])
        .map_err(|e| e.to_string())?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use rusqlite::Connection;

    #[test]
    fn test_filename_extraction() {
        let path = "/Users/test/games/sonic.bin";
        let filename = std::path::Path::new(path).file_name().unwrap().to_str().unwrap();
        assert_eq!(filename, "sonic.bin");
    }

    #[test]
    fn test_cover_path_gen() {
        let rom_path = std::path::Path::new("/roms/mario.nes");
        let cover_path = rom_path.with_extension("png");
        assert_eq!(cover_path.to_str().unwrap(), "/roms/mario.png");
    }

    #[test]
    fn test_date_format() {
        let date = chrono::Local::now().format("%Y-%m-%d").to_string();
        assert!(date.contains("-"));
        assert_eq!(date.len(), 10);
    }

    #[test]
    fn test_internal_get_games() {
        let conn = Connection::open_in_memory().unwrap();
        conn.execute("CREATE TABLE games (id INTEGER PRIMARY KEY, title TEXT, path TEXT, cover TEXT, cover_data BLOB, platform TEXT, play_count INTEGER DEFAULT 0, last_played TEXT, added_date TEXT)", []).unwrap();
        conn.execute("INSERT INTO games (title, path, play_count, added_date) VALUES ('Zelda', '/zelda.nes', 0, '2024-01-01')", []).unwrap();
        
        let games = internal_get_games(&conn).unwrap();
        assert_eq!(games.len(), 1);
        assert_eq!(games[0].title, "Zelda");
    }
}
