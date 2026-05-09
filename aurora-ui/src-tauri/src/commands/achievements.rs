use tauri::AppHandle;

pub fn get_hashable_data<'a>(ext: &str, buffer: &'a [u8]) -> &'a [u8] {
    if ext == "nes" && buffer.len() > 16 {
        &buffer[16..]
    } else {
        buffer
    }
}

#[tauri::command]
pub async fn get_game_id_by_hash(_app: AppHandle, rom_path: String, _ra_user: String, _ra_key: String) -> Result<u32, String> {
    let path = std::path::Path::new(&rom_path);
    let mut file = std::fs::File::open(path).map_err(|e| e.to_string())?;
    let mut buffer = Vec::new();
    std::io::Read::read_to_end(&mut file, &mut buffer).map_err(|e| e.to_string())?;

    let ext = path.extension().and_then(|s| s.to_str()).unwrap_or("").to_lowercase();
    let data_to_hash = get_hashable_data(&ext, &buffer);
    let hash = format!("{:x}", md5::compute(data_to_hash));

    let url = format!("https://retroachievements.org/dorequest.php?r=gameid&m={}", hash);
    let client = reqwest::Client::builder().user_agent("RetroArch/1.15.0").build().map_err(|e| e.to_string())?;
    let res = client.get(&url).send().await.map_err(|e| e.to_string())?;
    let json: serde_json::Value = res.json().await.map_err(|e| e.to_string())?;
    
    let id = json.get("GameID").and_then(|v| v.as_u64()).ok_or_else(|| format!("Could not find GameID"))? as u32;
    if id == 0 { return Err("Game not recognized by RetroAchievements".into()); }
    Ok(id)
}

#[tauri::command]
pub async fn get_ra_user_summary(user: String, key: String, target_user: String) -> Result<serde_json::Value, String> {
    let url = format!("https://retroachievements.org/API/API_GetUserSummary.php?u={}&y={}&z={}", user, key, target_user);
    let res = reqwest::get(&url).await.map_err(|e| e.to_string())?;
    res.json().await.map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn get_ra_game_info_and_progress(user: String, key: String, game_id: u32) -> Result<serde_json::Value, String> {
    let url = format!("https://retroachievements.org/API/API_GetGameInfoAndUserProgress.php?u={}&y={}&g={}", user, key, game_id);
    let res = reqwest::get(&url).await.map_err(|e| e.to_string())?;
    res.json().await.map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn ra_proxy(url: String) -> Result<serde_json::Value, String> {
    let client = reqwest::Client::builder()
        .user_agent("RetroArch/1.15.0")
        .build()
        .map_err(|e| e.to_string())?;
        
    let res = client.get(&url).send().await.map_err(|e| e.to_string())?;
    res.json().await.map_err(|e| e.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_nes_header_skip() {
        let mut buffer = vec![0u8; 32];
        for i in 0..32 { buffer[i] = i as u8; }
        
        let nes_data = get_hashable_data("nes", &buffer);
        assert_eq!(nes_data.len(), 16);
        assert_eq!(nes_data[0], 16);

        let bin_data = get_hashable_data("bin", &buffer);
        assert_eq!(bin_data.len(), 32);
        assert_eq!(bin_data[0], 0);
    }

    #[test]
    fn test_md5_hash() {
        let data = b"hello";
        let hash = format!("{:x}", md5::compute(data));
        assert_eq!(hash, "5d41402abc4b2a76b9719d911017c592");
    }
}
