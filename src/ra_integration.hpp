#pragma once

#include <string>
#include <vector>
#include <SDL.h>
#include <SDL_ttf.h>
#include <mutex>
#include "rc_client.h"

// Forward declarations
class EmulatorCore;

class RA_Manager {
public:
    RA_Manager();
    ~RA_Manager();

    // Initialize with RetroAchievements credentials
    void init(const std::string& username, const std::string& token);

    // Call this when a ROM is loaded to identify the game
    void load_game(const std::string& rom_path, EmulatorCore* core);

    // Call this every frame to process memory conditions
    void update();

    // Render achievement popups
    void render(SDL_Renderer* renderer);

    // Static memory callback for rcheevos
    static uint32_t read_memory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client);

private:
    rc_client_t* rc;
    std::string user;
    std::string token;
    EmulatorCore* active_core;
    
    // For handling HTTP requests in background threads
    static void http_request_handler(const rc_api_request_t* request, rc_client_server_callback_t callback, void* callback_data, rc_client_t* client);
    
    struct Popup {
        std::string title;
        std::string description;
        int frames_left;
        SDL_Texture* title_tex = nullptr;
        SDL_Texture* desc_tex = nullptr;
        int title_w = 0, title_h = 0;
        int desc_w = 0, desc_h = 0;
        
        // Badge
        std::string badge_name;
        std::vector<uint8_t> badge_data;
        bool badge_downloaded = false;
        SDL_Texture* badge_tex = nullptr;
        
        // Animation
        float y_offset = 0.0f;
    };
    std::vector<Popup> active_popups;
    mutable std::mutex popups_mutex;

    TTF_Font* font_title = nullptr;
    TTF_Font* font_desc = nullptr;

    static void event_handler(const rc_client_event_t* event, rc_client_t* client);
};
