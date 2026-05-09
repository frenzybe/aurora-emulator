#include "emulator_core.hpp"
#include <fstream>
#include <iostream>
#include <cstdarg>

retro_video_refresh_t EmulatorCore::video_cb = nullptr;
retro_audio_sample_batch_t EmulatorCore::audio_cb = nullptr;
retro_input_state_t EmulatorCore::input_cb = nullptr;
int EmulatorCore::pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;

bool EmulatorCore::core_environment(unsigned cmd, void* data) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_CAN_DUPE: *(bool*)data = true; return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
            pixel_format = *(int*)data;
            return true;
        }
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: *(const char**)data = "."; return true;
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
            auto log = (struct retro_log_callback*)data;
            log->log = core_log_callback;
            return true;
        }
    }
    return false;
}

bool EmulatorCore::load_core(const std::string& library_path) {
    reset();
    handle = dlopen(library_path.c_str(), RTLD_NOW);
    if (!handle) {
        const char* err = dlerror();
        std::cerr << "[Core-Error] Failed to load library " << library_path << ": " << (err ? err : "Unknown error") << std::endl;
        return false;
    }

    auto load_sym = [this](const char* name) -> void* {
        void* sym = dlsym(handle, name);
        if (!sym) std::cerr << "[Core-Warning] Missing required symbol: " << name << std::endl;
        return sym;
    };

    auto set_env = (void (*)(retro_environment_t))load_sym("retro_set_environment");
    auto set_vid = (void (*)(retro_video_refresh_t))load_sym("retro_set_video_refresh");
    auto set_aud = (void (*)(retro_audio_sample_batch_t))load_sym("retro_set_audio_sample_batch");
    auto set_pol = (void (*)(retro_input_poll_t))load_sym("retro_set_input_poll");
    auto set_inp = (void (*)(retro_input_state_t))load_sym("retro_set_input_state");

    retro_init = (void (*)(void))load_sym("retro_init");
    retro_deinit = (void (*)(void))load_sym("retro_deinit");
    retro_run = (void (*)(void))load_sym("retro_run");
    retro_load_game = (bool (*)(const struct retro_game_info*))load_sym("retro_load_game");
    retro_get_system_av_info = (void (*)(struct retro_system_av_info*))load_sym("retro_get_system_av_info");
    retro_serialize_size = (size_t (*)(void))load_sym("retro_serialize_size");
    retro_serialize = (bool (*)(void*, size_t))load_sym("retro_serialize");
    retro_unserialize = (bool (*)(const void*, size_t))load_sym("retro_unserialize");
    retro_get_memory_data = (void* (*)(unsigned))load_sym("retro_get_memory_data");
    retro_get_memory_size = (size_t (*)(unsigned))load_sym("retro_get_memory_size");

    // Critical symbols check
    if (!retro_init || !retro_run || !retro_load_game) {
        std::cerr << "[Core-Error] Core is missing critical Libretro functions. Aborting." << std::endl;
        reset();
        return false;
    }

    if (set_env) set_env(core_environment);
    if (set_vid) set_vid(video_cb);
    if (set_aud) set_aud(audio_cb);
    if (set_pol) set_pol([](){});
    if (set_inp) set_inp(input_cb);
    
    retro_init();
    std::cout << "[Core] Successfully initialized core: " << library_path << std::endl;
    return true;
}

bool EmulatorCore::load_game(const std::string& rom_path) {
    if (!handle) return false;
    std::ifstream rf(rom_path, std::ios::binary | std::ios::ate);
    if (!rf.is_open()) return false;
    
    size_t sz = rf.tellg(); rf.seekg(0);
    rom_data.resize(sz);
    rf.read((char*)rom_data.data(), sz);

    retro_game_info gi = {rom_path.c_str(), rom_data.data(), sz, ""};
    if (!retro_load_game || !retro_load_game(&gi)) {
        rom_data.clear();
        return false;
    }

    struct retro_system_av_info av;
    if (retro_get_system_av_info) retro_get_system_av_info(&av);
    av_info.width = av.geometry.base_width;
    av_info.height = av.geometry.base_height;
    av_info.fps = av.timing.fps;
    av_info.sample_rate = av.timing.sample_rate;
    
    // Cache memory pointers immediately after loading
    if (retro_get_memory_data && retro_get_memory_size) {
        cached_sys_ram = (uint8_t*)retro_get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
        cached_sys_size = retro_get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
        cached_save_ram = (uint8_t*)retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        cached_save_size = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        std::cout << "[Core] Memory pointers cached. System RAM: " << cached_sys_size << " bytes, Save RAM: " << cached_save_size << " bytes." << std::endl;
    }

    game_loaded = true;
    return true;
}

void EmulatorCore::core_log_callback(enum retro_log_level level, const char* fmt, ...) {
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    
    const char* prefix = "[Core]";
    if (level == RETRO_LOG_ERROR) prefix = "[Core-ERROR]";
    else if (level == RETRO_LOG_WARN) prefix = "[Core-WARN]";
    else if (level == RETRO_LOG_DEBUG) prefix = "[Core-DEBUG]";
    
    std::cout << prefix << " " << buf;
}

void EmulatorCore::run() {
    if (retro_run && game_loaded) retro_run();
}

void EmulatorCore::reset() {
    if (handle) {
        if (retro_deinit) retro_deinit();
        dlclose(handle);
    }
    handle = nullptr;
    game_loaded = false;
    retro_run = nullptr;
    rom_data.clear();
    cached_sys_ram = nullptr;
    cached_save_ram = nullptr;
    cached_sys_size = 0;
    cached_save_size = 0;
}

bool EmulatorCore::save_state(const std::string& path) {
    if (!retro_serialize || !retro_serialize_size) return false;
    size_t sz = retro_serialize_size();
    std::vector<uint8_t> buf(sz);
    if (retro_serialize(buf.data(), sz)) {
        std::ofstream f(path, std::ios::binary);
        f.write((char*)buf.data(), sz);
        return true;
    }
    return false;
}

bool EmulatorCore::load_state(const std::string& path) {
    if (!retro_unserialize || !retro_serialize_size) return false;
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return false;
    size_t sz = f.tellg(); f.seekg(0);
    std::vector<uint8_t> buf(sz);
    f.read((char*)buf.data(), sz);
    return retro_unserialize(buf.data(), sz);
}

uint32_t EmulatorCore::read_memory_for_ra(uint32_t address, uint8_t* buffer, uint32_t num_bytes) {
    if (!game_loaded) return 0;
    
    // Handle NES specifically for mirrors and WRAM
    if (cached_sys_size == 0x0800) { // 2KB is typical for NES
        // NES Mirrors: 0x0800-0x1FFF are mirrors of 0x0000-0x07FF
        if (address < 0x2000) {
            uint32_t real_addr = address % 0x0800;
            uint32_t bytes_to_read = std::min((size_t)num_bytes, (size_t)0x0800 - real_addr);
            if (cached_sys_ram) {
                memcpy(buffer, cached_sys_ram + real_addr, bytes_to_read);
                return bytes_to_read;
            }
        }
        
        // NES WRAM/SRAM: 0x6000-0x7FFF
        if (address >= 0x6000 && address <= 0x7FFF) {
            if (cached_save_ram && cached_save_size > 0) {
                uint32_t offset = address - 0x6000;
                if (offset < cached_save_size) {
                    uint32_t bytes_to_read = std::min((size_t)num_bytes, cached_save_size - offset);
                    memcpy(buffer, cached_save_ram + offset, bytes_to_read);
                    return bytes_to_read;
                }
            }
        }
    }

    // Default generic fallback using cached pointers
    if (cached_sys_ram && address < cached_sys_size) {
        uint32_t bytes_to_read = std::min((size_t)num_bytes, (size_t)cached_sys_size - address);
        memcpy(buffer, cached_sys_ram + address, bytes_to_read);
        return bytes_to_read;
    }
    
    return 0;
}
