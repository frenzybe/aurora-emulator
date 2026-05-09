#include "../include/emulator.hpp"
#include "../include/bus.hpp"
#include "../include/cpu/m68k.hpp"
#include "../include/cpu/z80.hpp"
#include "../include/vdp/vdp.hpp"
#include "../include/audio/audio.hpp"
#include "../include/input/input.hpp"
#include "../include/renderer/software.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace md {

Emulator::Emulator() = default;

Emulator::~Emulator() = default;

bool Emulator::init(int argc, char** argv) {
    (void)argc; (void)argv;
    
    std::cout << "Initializing Mega Drive Emulator...\n";
    
    bus = std::make_unique<Bus>();
    cpu_68k = std::make_unique<CPU68K>(bus.get());
    cpu_z80 = std::make_unique<CPUZ80>(bus.get());
    vdp = std::make_unique<VDP>(bus.get());
    audio = std::make_unique<Audio>(bus.get());
    input = std::make_unique<InputManager>();
    renderer = std::make_unique<SoftwareRenderer>(vdp.get());
    
    bus->set_cpu68k(cpu_68k.get());
    bus->set_cpu_z80(cpu_z80.get());
    bus->set_vdp(vdp.get());
    bus->set_psg(audio->get_psg());
    bus->set_fm(audio->get_fm());
    
    cpu_68k->init();
    cpu_z80->init();
    vdp->init();
    audio->init();
    input->init();
    renderer->init(SCREEN_WIDTH, SCREEN_HEIGHT_NTSC);
    
    std::cout << "Emulator initialized.\n";
    return true;
}

void Emulator::shutdown() {
    running = false;
}

bool Emulator::load_rom(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Cannot open ROM: " << path << "\n";
        return false;
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<u8> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    bus->load_rom(buffer, static_cast<u32>(size));
    cpu_68k->reset(); // Инициализация векторов сброса после загрузки ROM
    
    std::cout << "ROM loaded: " << size << " bytes\n";
    return true;
}

bool Emulator::load_bios(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Cannot open BIOS: " << path << "\n";
        return false;
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<u8> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    
    bus->load_bios(buffer, static_cast<u32>(size));
    return true;
}

void Emulator::run() {
    running = true;
    emulation_loop();
}

void Emulator::emulation_loop() {
    while (running) {
        process_events();
        if (!paused) frame();
        SDL_Delay(16);
    }
}

void Emulator::frame() {
    input->update();
    cpu_68k->run_frame();
    cpu_z80->run_cycles(15000);
    
    for (int line = 0; line < static_cast<int>(SCREEN_HEIGHT_NTSC); line++) {
        vdp->step(114);
        vdp->render_scanline(line);
    }
    
    audio->tick(15000);
    std::vector<float> buf(1024);
    audio->mix(buf, 1024);
    
    renderer->present();
}

void Emulator::process_events() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) running = false;
        else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
            running = false;
        }
        else if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_p) {
            paused = !paused;
        }
    }
}

void Emulator::set_scale(float s) {
    if (renderer) renderer->set_scale(s);
}

void Emulator::set_volume(float v) {
    if (audio) audio->set_volume(v);
}

void Emulator::save_state(const std::string& /*path*/) {}
void Emulator::load_state(const std::string& /*path*/) {}
void Emulator::serialize(std::ostream& /*os*/) {}
void Emulator::deserialize(std::istream& /*is*/) {}

} // namespace md
