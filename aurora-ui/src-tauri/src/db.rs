use chrono;
use rusqlite::{params, Connection};
use serde::{Deserialize, Serialize};
use std::fs;
use tauri::{AppHandle, Manager, Runtime};

#[derive(Serialize, Deserialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct Game {
    pub id: Option<i64>,
    pub title: String,
    pub path: String,
    pub cover: Option<String>,
    pub cover_base64: Option<String>,
    pub platform: Option<String>,
    pub play_count: i32,
    pub last_played: Option<String>,
    pub added_date: String,
    pub nfo_path: Option<String>,
}

pub fn get_db_path<R: Runtime>(app: &AppHandle<R>) -> std::path::PathBuf {
    app.path()
        .app_data_dir()
        .unwrap_or_default()
        .join("aurora.db")
}

pub fn get_db_conn<R: Runtime>(app: &AppHandle<R>) -> Result<Connection, String> {
    Connection::open(get_db_path(app)).map_err(|e| e.to_string())
}

pub fn init_db<R: Runtime>(app: &AppHandle<R>) -> Result<(), String> {
    let conn = get_db_conn(app)?;
    conn.execute(
        "CREATE TABLE IF NOT EXISTS games (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            path TEXT NOT NULL UNIQUE,
            cover TEXT,
            cover_data BLOB,
            platform TEXT,
            play_count INTEGER DEFAULT 0,
            last_played TEXT,
            added_date TEXT,
            nfo_path TEXT
        )",
        [],
    )
    .map_err(|e| e.to_string())?;

    conn.execute(
        "CREATE TABLE IF NOT EXISTS save_states (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            game_path TEXT NOT NULL,
            state_path TEXT NOT NULL UNIQUE,
            thumb_path TEXT,
            state_data BLOB,
            screenshot BLOB,
            timestamp TEXT NOT NULL,
            slot TEXT NOT NULL,
            FOREIGN KEY(game_path) REFERENCES games(path) ON DELETE CASCADE
        )",
        [],
    )
    .map_err(|e| e.to_string())?;

    // Migration for existing DBs
    let _ = conn.execute("ALTER TABLE games ADD COLUMN cover_data BLOB", []);
    let _ = conn.execute("ALTER TABLE games ADD COLUMN nfo_path TEXT", []);
    let _ = conn.execute("ALTER TABLE save_states ADD COLUMN screenshot BLOB", []);
    let _ = conn.execute("ALTER TABLE save_states ADD COLUMN state_data BLOB", []);

    // Check if migration is needed
    let pref_dir = app.path().app_data_dir().unwrap_or_default();
    let library_path = pref_dir.join("library.txt");
    if library_path.exists() {
        println!("[DB] Found legacy library.txt, migrating...");
        migrate_from_txt(app, &conn, &library_path)?;
    }

    Ok(())
}

pub fn migrate_from_txt<R: Runtime>(
    _app: &AppHandle<R>,
    conn: &Connection,
    path: &std::path::Path,
) -> Result<(), String> {
    let content = fs::read_to_string(path).map_err(|e| e.to_string())?;
    let added_date = chrono::Local::now().format("%Y-%m-%d %H:%M:%S").to_string();

    for line in content.lines() {
        let line = line.trim();
        if line.is_empty() {
            continue;
        }

        if let Some((title, path_desc)) = line.split_once('|') {
            let (rom_path, platform) = if let Some((p, d)) = path_desc.split_once('|') {
                (p.trim(), d.trim())
            } else {
                (path_desc.trim(), "Unknown")
            };

            let rom_path_obj = std::path::Path::new(rom_path);
            let cover_path = rom_path_obj.with_extension("png");
            let cover = if cover_path.exists() {
                Some(cover_path.to_string_lossy().to_string())
            } else {
                None
            };
            let cover_data = if let Some(ref p) = cover {
                fs::read(p).ok()
            } else {
                None
            };

            let _ = conn.execute(
                "INSERT OR IGNORE INTO games (title, path, cover, cover_data, platform, added_date) VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
                params![title.trim(), rom_path, cover, cover_data, platform, added_date],
            );
        }
    }

    // Rename old file to prevent double migration
    let _ = fs::rename(path, path.with_extension("txt.bak"));
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_game_struct_serialization() {
        let game = Game {
            id: Some(1),
            title: "Sonic".to_string(),
            path: "/path/to/sonic.bin".to_string(),
            cover: None,
            cover_base64: None,
            platform: Some("Genesis".to_string()),
            play_count: 5,
            last_played: None,
            added_date: "2023-01-01".to_string(),
        };
        let json = serde_json::to_string(&game).unwrap();
        assert!(json.contains("\"title\":\"Sonic\""));
        assert!(json.contains("\"playCount\":5")); // Test camelCase
    }

    #[test]
    fn test_in_memory_db() {
        let conn = Connection::open_in_memory().unwrap();
        conn.execute(
            "CREATE TABLE games (id INTEGER PRIMARY KEY, title TEXT, path TEXT UNIQUE)",
            [],
        )
        .unwrap();

        conn.execute(
            "INSERT INTO games (title, path) VALUES (?1, ?2)",
            params!["Mario", "/mario.nes"],
        )
        .unwrap();

        let mut stmt = conn
            .prepare("SELECT title FROM games WHERE path = ?1")
            .unwrap();
        let title: String = stmt.query_row(params!["/mario.nes"], |r| r.get(0)).unwrap();
        assert_eq!(title, "Mario");
    }

    #[test]
    fn test_db_unique_constraint() {
        let conn = Connection::open_in_memory().unwrap();
        conn.execute("CREATE TABLE games (path TEXT UNIQUE)", [])
            .unwrap();
        conn.execute("INSERT INTO games (path) VALUES ('/test.bin')", [])
            .unwrap();
        let res = conn.execute("INSERT INTO games (path) VALUES ('/test.bin')", []);
        assert!(res.is_err()); // Path must be unique
    }

    #[test]
    fn test_game_full_deserialization() {
        let json = r#"{
            "title": "Contra",
            "path": "/contra.nes",
            "playCount": 10,
            "addedDate": "2024-05-06"
        }"#;
        let game: Game = serde_json::from_str(json).unwrap();
        assert_eq!(game.title, "Contra");
        assert_eq!(game.play_count, 10);
    }

    #[test]
    fn test_default_play_count() {
        let conn = Connection::open_in_memory().unwrap();
        conn.execute(
            "CREATE TABLE games (id INTEGER PRIMARY KEY, play_count INTEGER DEFAULT 0)",
            [],
        )
        .unwrap();
        conn.execute("INSERT INTO games (id) VALUES (1)", [])
            .unwrap();
        let count: i32 = conn
            .query_row("SELECT play_count FROM games", [], |r| r.get(0))
            .unwrap();
        assert_eq!(count, 0);
    }

    #[test]
    fn test_game_serialization_empty_fields() {
        let game = Game {
            id: None,
            title: "".to_string(),
            path: "".to_string(),
            cover: None,
            cover_base64: None,
            platform: None,
            play_count: 0,
            last_played: None,
            added_date: "".to_string(),
        };
        let json = serde_json::to_string(&game).unwrap();
        assert!(json.contains("\"id\":null"));
    }

    #[test]
    fn test_migrate_empty_file() {
        let conn = Connection::open_in_memory().unwrap();
        conn.execute("CREATE TABLE games (title TEXT, path TEXT)", [])
            .unwrap();

        let temp_file = std::env::temp_dir().join("empty_lib.txt");
        fs::write(&temp_file, "").unwrap();

        // This shouldn't crash
        let _ = conn.execute("DELETE FROM games", []);

        let _ = fs::remove_file(temp_file);
    }
}
