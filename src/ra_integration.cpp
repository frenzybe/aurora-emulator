#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include "ra_integration.hpp"
#include "emulator_core.hpp"
#include "rc_hash.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <cmath>
#include <SDL_image.h>
#include <algorithm>

static std::map<std::string, std::vector<uint8_t>> badge_cache;
static std::mutex badge_mutex;

// Define httplib implementation here
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

static RA_Manager* g_ra_manager = nullptr;

RA_Manager::RA_Manager() : rc(nullptr), active_core(nullptr) {
    g_ra_manager = this;
#ifdef _WIN32
    font_title = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 20);
    font_desc = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 14);
#else
    font_title = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", 20);
    font_desc = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", 14);
#endif
    if (!font_title) {
        std::cout << "[RA] Failed to load font: " << TTF_GetError() << std::endl;
    }
}

RA_Manager::~RA_Manager() {
    if (font_title) TTF_CloseFont(font_title);
    if (font_desc) TTF_CloseFont(font_desc);
    for (auto& popup : active_popups) {
        if (popup.title_tex) SDL_DestroyTexture(popup.title_tex);
        if (popup.desc_tex) SDL_DestroyTexture(popup.desc_tex);
    }
    
    if (rc) {
        rc_client_destroy(rc);
    }
    g_ra_manager = nullptr;
}

void RA_Manager::init(const std::string& username, const std::string& token) {
    this->user = username;
    this->token = token;

    rc = rc_client_create(RA_Manager::read_memory, RA_Manager::http_request_handler);
    rc_client_set_event_handler(rc, RA_Manager::event_handler);
    
    rc_client_enable_logging(rc, RC_CLIENT_LOG_LEVEL_VERBOSE, [](const char* message, const rc_client_t* client) {
        std::cout << "[RA] " << message << std::endl;
    });

    std::cout << "[RA-DEBUG] Attempting login with API Key as password for user: " << username << std::endl;
    // Try to login using the provided API Key as a password.
    rc_client_begin_login_with_password(rc, username.c_str(), token.c_str(), [](int result, const char* error_message, rc_client_t* client, void* userdata) {
        if (result == RC_OK) {
            std::cout << "[RA] Login successful!" << std::endl;
        } else {
            std::cout << "[RA] Login failed: " << (error_message ? error_message : "Unknown error") << std::endl;
        }
    }, this);
}

void RA_Manager::load_game(const std::string& rom_path, EmulatorCore* core) {
    active_core = core;
    if (!core) return;

    std::cout << "[RA] Identifying game for achievements..." << std::endl;
    
    const auto& buffer = core->get_rom_data();
    if (buffer.empty()) {
        std::cout << "[RA] ROM data is empty, cannot identify." << std::endl;
        return;
    }
    
    uint32_t console_id = RC_CONSOLE_MEGA_DRIVE;
    std::string ext = rom_path.substr(rom_path.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "nes") console_id = RC_CONSOLE_NINTENDO;
    else if (ext == "sfc" || ext == "smc") console_id = RC_CONSOLE_SUPER_NINTENDO;
    else if (ext == "gb") console_id = RC_CONSOLE_GAMEBOY;
    else if (ext == "gbc") console_id = RC_CONSOLE_GAMEBOY_COLOR;
    else if (ext == "gba") console_id = RC_CONSOLE_GAMEBOY_ADVANCE;
    else if (ext == "md" || ext == "bin" || ext == "gen") console_id = RC_CONSOLE_MEGA_DRIVE;

    std::cout << "[RA] Console ID detected: " << console_id << " (ext: " << ext << ")" << std::endl;

    rc_client_begin_identify_and_load_game(rc, console_id, rom_path.c_str(), buffer.data(), buffer.size(), [](int result, const char* error_message, rc_client_t* client, void* userdata) {
        if (result == RC_OK) {
            const rc_client_game_t* game = rc_client_get_game_info(client);
            if (game) {
                std::cout << "[RA] Game identified: " << (game->title ? game->title : "Unknown") << " (ID: " << game->id << ")" << std::endl;
            }
            std::cout << "[RA] Achievements loaded successfully!" << std::endl;
        } else {
            std::cout << "[RA] Failed to load game: " << (error_message ? error_message : "Unknown error") << std::endl;
        }
    }, this);
}

void RA_Manager::update() {
    if (rc) {
        rc_client_do_frame(rc);
    }
}

void RA_Manager::render(SDL_Renderer* renderer) {
    std::lock_guard<std::mutex> lock(popups_mutex);
    for (auto& popup : active_popups) {
        if (popup.frames_left > 0) {
            popup.frames_left--;

            // Lazy initialization of textures
            if (!popup.title_tex && font_title) {
                SDL_Color c = {255, 255, 255, 255};
                SDL_Surface* ts = TTF_RenderUTF8_Blended(font_title, popup.title.c_str(), c);
                if (ts) {
                    popup.title_tex = SDL_CreateTextureFromSurface(renderer, ts);
                    popup.title_w = ts->w;
                    popup.title_h = ts->h;
                    SDL_FreeSurface(ts);
                }
            }

            if (!popup.desc_tex && font_desc) {
                SDL_Color c = {200, 200, 200, 255};
                SDL_Surface* ds = TTF_RenderUTF8_Blended(font_desc, popup.description.c_str(), c);
                if (ds) {
                    popup.desc_tex = SDL_CreateTextureFromSurface(renderer, ds);
                    popup.desc_w = ds->w;
                    popup.desc_h = ds->h;
                    SDL_FreeSurface(ds);
                }
            }

            if (!popup.badge_tex && !popup.badge_name.empty()) {
                std::lock_guard<std::mutex> lock(badge_mutex);
                auto it = badge_cache.find(popup.badge_name);
                if (it != badge_cache.end() && !it->second.empty()) {
                    SDL_RWops* rw = SDL_RWFromConstMem(it->second.data(), it->second.size());
                    if (rw) {
                        SDL_Surface* bs = IMG_Load_RW(rw, 1);
                        if (bs) {
                            popup.badge_tex = SDL_CreateTextureFromSurface(renderer, bs);
                            SDL_FreeSurface(bs);
                        }
                    }
                    popup.badge_name = ""; // Prevent retrying
                }
            }

            int win_w, win_h;
            SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
            
            int rect_w = 400;
            int rect_h = 72;
            int rect_x = (win_w - rect_w) / 2;
            int rect_y = win_h - rect_h - 40;

            // Smooth cubic ease out/in animation
            float t = 1.0f;
            if (popup.frames_left > 150) {
                t = (180 - popup.frames_left) / 30.0f; // 0.0 to 1.0
                t = 1.0f - std::pow(1.0f - t, 3.0f); // Cubic ease out
            } else if (popup.frames_left < 30) {
                t = popup.frames_left / 30.0f;
                t = t * t * t; // Cubic ease in
            }
            rect_y = win_h - (rect_h + 40) * t + 20 * (1.0f - t);
            
            int alpha = (int)(255 * t);

            // Draw dark translucent background
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, (alpha * 240) / 255);
            SDL_Rect bg = {rect_x, rect_y, rect_w, rect_h};
            SDL_RenderFillRect(renderer, &bg);
            
            // Accent line on the left (golden)
            SDL_SetRenderDrawColor(renderer, 255, 215, 0, alpha);
            SDL_Rect accent = {rect_x, rect_y, 4, rect_h};
            SDL_RenderFillRect(renderer, &accent);

            // Draw sleek outline
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, (alpha * 40) / 255);
            SDL_RenderDrawRect(renderer, &bg);

            int text_x = rect_x + 20;

            // Draw badge
            if (popup.badge_tex) {
                SDL_SetTextureAlphaMod(popup.badge_tex, alpha);
                SDL_Rect b_dst = {rect_x + 16, rect_y + 12, 48, 48};
                SDL_RenderCopy(renderer, popup.badge_tex, nullptr, &b_dst);
                text_x += 56;
            }

            // Draw text
            if (popup.title_tex) {
                SDL_SetTextureAlphaMod(popup.title_tex, alpha);
                SDL_Rect dst = {text_x, rect_y + 14, popup.title_w, popup.title_h};
                SDL_RenderCopy(renderer, popup.title_tex, nullptr, &dst);
            }

            if (popup.desc_tex) {
                SDL_SetTextureAlphaMod(popup.desc_tex, alpha);
                SDL_Rect dst = {text_x, rect_y + 40, popup.desc_w, popup.desc_h};
                SDL_RenderCopy(renderer, popup.desc_tex, nullptr, &dst);
            }
        }
    }

    // Erase expired popups
    active_popups.erase(
        std::remove_if(active_popups.begin(), active_popups.end(), [](Popup& p) {
            if (p.frames_left <= 0) {
                if (p.title_tex) SDL_DestroyTexture(p.title_tex);
                if (p.desc_tex) SDL_DestroyTexture(p.desc_tex);
                if (p.badge_tex) SDL_DestroyTexture(p.badge_tex);
                return true;
            }
            return false;
        }),
        active_popups.end()
    );
}

uint32_t RA_Manager::read_memory(uint32_t address, uint8_t* buffer, uint32_t num_bytes, rc_client_t* client) {
    if (g_ra_manager && g_ra_manager->active_core) {
        return g_ra_manager->active_core->read_memory_for_ra(address, buffer, num_bytes);
    }
    return 0;
}

void RA_Manager::http_request_handler(const rc_api_request_t* request, rc_client_server_callback_t callback, void* callback_data, rc_client_t* client) {
    // This needs to happen asynchronously so we don't block the emulator
    std::string url = request->url;
    std::string post_data = request->post_data ? request->post_data : "";
    std::string content_type = request->content_type ? request->content_type : "";

    std::thread([url, post_data, content_type, callback, callback_data, client]() {
        // Extract protocol and host from URL
        std::string protocol = "https://";
        size_t host_start = url.find("://");
        if (host_start != std::string::npos) {
            protocol = url.substr(0, host_start + 3);
            host_start += 3;
        } else {
            host_start = 0;
        }
        
        size_t host_end = url.find("/", host_start);
        std::string host = url.substr(host_start, host_end - host_start);
        std::string path = url.substr(host_end);

        httplib::Client cli(protocol + host);
        cli.set_connection_timeout(5);
        cli.enable_server_certificate_verification(false);
        
        std::cout << "[RA-HTTP] Request Path: " << path << std::endl;
        if (!post_data.empty()) {
            std::cout << "[RA-HTTP] Post Data: " << post_data.substr(0, 50) << "..." << std::endl;
        }

        httplib::Result res;
        httplib::Headers headers = {
            { "User-Agent", "MegaDriveEmu/1.0" }
        };
        
        if (!post_data.empty()) {
            res = cli.Post(path.c_str(), headers, post_data, content_type.c_str());
        } else {
            res = cli.Get(path.c_str(), headers);
        }

        rc_api_server_response_t response;
        if (res && res->status == 200) {
            std::cout << "[RA] HTTP 200 OK: " << path.substr(0, 30) << "..." << std::endl;
            response.body = res->body.c_str();
            response.body_length = res->body.length();
            response.http_status_code = res->status;
        } else {
            std::cout << "[RA] HTTP Error: " << (res ? std::to_string(res->status) : "connection failed") << " for " << path << std::endl;
            response.body = "";
            response.body_length = 0;
            response.http_status_code = res ? res->status : 0;
        }

        callback(&response, callback_data);
    }).detach();
}

void RA_Manager::event_handler(const rc_client_event_t* event, rc_client_t* client) {
    if (event->type == RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED) {
        std::cout << "[RA] Achievement Unlocked: " << event->achievement->title << std::endl;
        if (g_ra_manager) {
            std::string badge = event->achievement->badge_name;
            
            bool needs_download = false;
            {
                std::lock_guard<std::mutex> lock(badge_mutex);
                if (!badge.empty() && badge_cache.find(badge) == badge_cache.end()) {
                    needs_download = true;
                    badge_cache[badge] = std::vector<uint8_t>();
                }
            }
            
            if (needs_download) {
                std::thread([badge]() {
                    httplib::Client cli("https://media.retroachievements.org");
                    cli.set_connection_timeout(5);
                    cli.enable_server_certificate_verification(false);
                    auto res = cli.Get("/Badge/" + badge + ".png");
                    if (res && res->status == 200) {
                        std::lock_guard<std::mutex> lock(badge_mutex);
                        badge_cache[badge].assign(res->body.begin(), res->body.end());
                    } else {
                        std::lock_guard<std::mutex> lock(badge_mutex);
                        badge_cache.erase(badge);
                    }
                }).detach();
            }

            std::lock_guard<std::mutex> lock(g_ra_manager->popups_mutex);
            g_ra_manager->active_popups.push_back({
                event->achievement->title,
                event->achievement->description,
                180,
                nullptr, nullptr, 0, 0, 0, 0,
                badge, {}, false, nullptr, 0.0f
            });
        }
    } else if (event->type == RC_CLIENT_EVENT_LEADERBOARD_STARTED) {
        std::cout << "[RA] Leaderboard Started" << std::endl;
    } else {
        std::cout << "[RA] Event triggered: " << event->type << std::endl;
    }
}
