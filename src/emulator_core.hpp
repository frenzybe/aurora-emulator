#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include <dlfcn.h>
#include "../third_party/genplusgx/libretro/libretro-common/include/libretro.h"

class EmulatorCore {
public:
    struct AVInfo {
        int width = 320;
        int height = 224;
        double fps = 60.0;
        double sample_rate = 44100.0;
    };

    EmulatorCore() = default;
    ~EmulatorCore() { reset(); }

    bool load_core(const std::string& library_path);
    bool load_game(const std::string& rom_path);
    void run();
    void reset();

    // State management
    bool save_state(const std::string& path);
    bool load_state(const std::string& path);
    const std::vector<uint8_t>& get_rom_data() const { return rom_data; }
    
    // Provide memory access for RetroAchievements
    uint32_t read_memory_for_ra(uint32_t address, uint8_t* buffer, uint32_t num_bytes);
    
    // Callbacks setup
    static void set_video_cb(retro_video_refresh_t cb) { video_cb = cb; }
    static void set_audio_cb(retro_audio_sample_batch_t cb) { audio_cb = cb; }
    static void set_input_cb(retro_input_state_t cb) { input_cb = cb; }

    const AVInfo& get_av_info() const { return av_info; }
    int get_pixel_format() const { return pixel_format; }
    bool is_ready() const { return handle != nullptr && game_loaded; }

private:
    void* handle = nullptr;
    bool game_loaded = false;
    std::vector<uint8_t> rom_data; // Keep ROM data alive for the core
    
    // Cached memory pointers for RetroAchievements performance
    uint8_t* cached_sys_ram = nullptr;
    size_t cached_sys_size = 0;
    uint8_t* cached_save_ram = nullptr;
    size_t cached_save_size = 0;
    AVInfo av_info;

    // Libretro function pointers
    void (*retro_init)(void) = nullptr;
    void (*retro_deinit)(void) = nullptr;
    void (*retro_run)(void) = nullptr;
    bool (*retro_load_game)(const struct retro_game_info*) = nullptr;
    void (*retro_get_system_av_info)(struct retro_system_av_info*) = nullptr;
    size_t (*retro_serialize_size)(void) = nullptr;
    bool (*retro_serialize)(void*, size_t) = nullptr;
    bool (*retro_unserialize)(const void*, size_t) = nullptr;
    void* (*retro_get_memory_data)(unsigned id) = nullptr;
    size_t (*retro_get_memory_size)(unsigned id) = nullptr;

    static void core_log_callback(enum retro_log_level level, const char* fmt, ...);
    static retro_video_refresh_t video_cb;
    static retro_audio_sample_batch_t audio_cb;
    static retro_input_state_t input_cb;
    static int pixel_format;
    static bool core_environment(unsigned cmd, void* data);
};
