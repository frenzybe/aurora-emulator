#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "types.hpp"
#include <string>
#include <memory>
#include <vector>

namespace md {

class Bus;
class CPU68K;
class CPUZ80;
class VDP;
class Audio;
class InputManager;
class Renderer;

enum class Region {
    NTSC = 0,
    PAL = 1
};

class Emulator {
public:
    Emulator();
    ~Emulator();

    bool init(int argc, char** argv);
    void shutdown();
    
    bool load_rom(const std::string& path);
    bool load_bios(const std::string& path);
    
    void run();
    void stop() { running = false; }
    
    void pause() { paused = true; }
    void resume() { paused = false; }
    bool is_paused() const { return paused; }

    void set_region(Region region) { current_region = region; }
    void set_scale(float s);
    void set_volume(float v);

    Bus* get_bus() const { return bus.get(); }
    CPU68K* get_cpu68k() const { return cpu_68k.get(); }
    CPUZ80* get_cpuz80() const { return cpu_z80.get(); }
    VDP* get_vdp() const { return vdp.get(); }
    Audio* get_audio() const { return audio.get(); }
    InputManager* get_input() const { return input.get(); }
    Renderer* get_renderer() const { return renderer.get(); }

    void save_state(const std::string& path);
    void load_state(const std::string& path);

private:
    std::unique_ptr<Bus> bus;
    std::unique_ptr<CPU68K> cpu_68k;
    std::unique_ptr<CPUZ80> cpu_z80;
    std::unique_ptr<VDP> vdp;
    std::unique_ptr<Audio> audio;
    std::unique_ptr<InputManager> input;
    std::unique_ptr<Renderer> renderer;

    bool running = false;
    bool paused = false;
    Region current_region = Region::NTSC;
    
    [[maybe_unused]] double frame_time = 0.0;
    [[maybe_unused]] int frame_count = 0;
    
    void emulation_loop();
    void frame();
    void process_events();
    
    void serialize(std::ostream& os);
    void deserialize(std::istream& is);
};

} // namespace md

#endif // EMULATOR_HPP
