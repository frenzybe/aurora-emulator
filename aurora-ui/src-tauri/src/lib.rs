use tauri::{Manager, Emitter};
use std::fs;

mod db;
mod utils;
mod commands;

use commands::library::*;
use commands::emulator::*;
use commands::achievements::*;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_log::Builder::new()
            .level(log::LevelFilter::Info)
            .filter(|metadata| {
                !metadata.target().starts_with("tao") && 
                !metadata.target().starts_with("wry") &&
                !metadata.target().starts_with("reqwest")
            })
            .build())
        .plugin(tauri_plugin_fs::init())
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            let app_data = app.path().app_data_dir().expect("failed to get app data dir");
            fs::create_dir_all(app_data.join("roms")).expect("failed to create roms dir");
            fs::create_dir_all(app_data.join("system")).expect("failed to create BIOS dir");
            fs::create_dir_all(app_data.join("third_party").join("cores")).expect("failed to create cores dir");
            
            // Initialize SQLite DB
            if let Err(e) = db::init_db(app.handle()) {
                eprintln!("[DB] Failed to initialize: {}", e);
            }

            // Native Gamepad Thread
            let handle = app.handle().clone();
            std::thread::spawn(move || {
                use gilrs::Gilrs;
                let gilrs = Gilrs::new().ok();
                if let Some(mut g) = gilrs {
                    println!("[Native Input] Polling driver started.");
                    
                    loop {
                        while let Some(_ev) = g.next_event() {}
                        
                        for (_id, gamepad) in g.gamepads() {
                            for (code, btn_data) in gamepad.state().buttons() {
                                if btn_data.is_pressed() {
                                    let btn_idx = code.into_u32();
                                    let _ = handle.emit("native-gamepad-event", serde_json::json!({
                                        "type": "button",
                                        "gp_name": gamepad.name().to_string(),
                                        "button": btn_idx
                                    }));
                                }
                            }
                        }
                        std::thread::sleep(std::time::Duration::from_millis(16));
                    }
                }
            });
            
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            get_games,
            add_game,
            remove_game,
            rename_game,
            play_game,
            download_cores,
            download_cover,
            get_game_id_by_hash,
            get_ra_user_summary,
            get_ra_game_info_and_progress,
            ra_proxy,
            open_bios_folder,
            get_system_status,
            download_bios_pack,
            get_save_states,
            delete_save_state,
            get_game_nfo,
            select_nfo_file,
            link_game_nfo,
            update_game_platform
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
