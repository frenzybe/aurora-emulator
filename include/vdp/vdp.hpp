#ifndef VDP_HPP
#define VDP_HPP

#include "types.hpp"
#include <array>
#include <vector>
#include <cstdint>

namespace md {

class Bus;

class VDP {
public:
    VDP(Bus* bus);
    ~VDP();

    void init();
    void reset();
    void step(int cycles);
    void render_scanline(int line);

    struct {
        u8 mode[0x20];
    } reg;

    static constexpr u32 VRAM_SIZE = 0x10000;
    static constexpr u32 CRAM_SIZE = 0x80;
    static constexpr u32 VSRAM_SIZE = 0x40;

    std::array<u8, VRAM_SIZE> vram;
    std::array<u16, CRAM_SIZE> cram;
    std::array<u16, VSRAM_SIZE> vsram;

    struct {
        bool hblank = false;
        bool vblank = false;
        int current_line = 0;
        int dot_counter = 0;
        u16 vcounter = 0;
        u16 hcounter = 0;
    } state;

    struct {
        u16 control_word = 0;
        bool pending_write = false;
        u8 code = 0;
        u16 address = 0;
        
        bool dma_enabled = false;
        u8 dma_type = 0;
        u32 dma_source = 0;
        u16 dma_length = 0;
    } ctrl;

    struct Pixel {
        u8 r, g, b;
    };
    void get_scanline(int line, std::vector<Pixel>& buffer);

    // 16-bit access
    void write_control_word(u16 value);
    void write_data_word(u16 value);
    u16 read_control_word();
    u16 read_data_word();

    // Legacy byte access wrappers
    void write_control(u8 value) { write_control_word((value << 8) | value); }
    void write_data(u8 value) { write_data_word((value << 8) | value); }
    u8 read_control() { return read_control_word() >> 8; }
    u8 read_data() { return read_data_word() >> 8; }

    bool is_vblank() const { return state.vblank; }
    bool is_hblank() const { return state.hblank; }

private:
    Bus* bus;

    static constexpr int H_TOTAL = 3420;
    static constexpr int V_TOTAL = 262;
    static constexpr int H_BLANK_START = 2560;
    static constexpr int H_BLANK_END = 3200;
    static constexpr int V_BLANK_START = 224;
    static constexpr int V_BLANK_END = 262;

    void execute_dma();
    void write_internal(u16 value);
    u16 read_internal();
    
    u8 convert_color(u16 cram_value, u8 component);

    std::vector<Pixel> line_buffer[SCREEN_HEIGHT_NTSC];
};

} // namespace md

#endif // VDP_HPP
