#include "vdp/vdp.hpp"
#include "bus.hpp"
#include <algorithm>
#include <iostream>

namespace md {

VDP::VDP(Bus* bus) : bus(bus) {
    init();
}

VDP::~VDP() = default;

void VDP::init() {
    reset();
}

void VDP::reset() {
    std::fill(std::begin(reg.mode), std::end(reg.mode), 0);
    vram.fill(0);
    cram.fill(0);
    vsram.fill(0);
    state = {};
    ctrl = {};
    
    reg.mode[0] = 0x04;
    reg.mode[1] = 0x04;
    reg.mode[0xF] = 0x02; // Auto-increment
}

void VDP::step(int cycles) {
    state.dot_counter += cycles;
    if (state.dot_counter >= H_TOTAL) {
        state.dot_counter -= H_TOTAL;
        state.current_line++;
        
        if (state.current_line >= V_TOTAL) {
            state.current_line = 0;
            state.vblank = false;
        }
        
        if (state.current_line == V_BLANK_START) {
            state.vblank = true;
            if (reg.mode[1] & 0x20) {
                bus->request_interrupt(6);  // V-Blank interrupt
            }
        }
        
        if (state.current_line < V_BLANK_START) {
            render_scanline(state.current_line);
        }
    }
}

void VDP::render_scanline(int line) {
    if (line >= SCREEN_HEIGHT_NTSC) return;
    auto& buffer = line_buffer[line];
    buffer.clear();
    buffer.resize(SCREEN_WIDTH, {0, 0, 0});
    
    // Read background color
    u8 bg_color_idx = reg.mode[7] & 0x3F;
    u16 bg_color = cram[bg_color_idx];
    Pixel bg_pixel = { convert_color(bg_color, 0), convert_color(bg_color, 1), convert_color(bg_color, 2) };
    
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        buffer[x] = bg_pixel;
    }
    
    // MVP Plane A Rendering
    // Reg 2: Plane A Name Table base (Bits 3-5 -> shifted by 10)
    u16 plane_a_base = (reg.mode[2] & 0x38) << 10;
    
    // H-Scroll (Reg 13) and V-Scroll (VSRAM) ignored for this MVP
    int tile_y = line / 8;
    int pixel_y = line % 8;
    
    // Name table is typically 64x32 tiles (defined by Reg 16, assuming 64x32)
    for (int tx = 0; tx < 40; tx++) {
        u16 nt_addr = plane_a_base + (tile_y * 64 + tx) * 2;
        u16 tile_desc = (vram[nt_addr] << 8) | vram[nt_addr + 1];
        
        u16 tile_index = tile_desc & 0x7FF;
        u8 palette = (tile_desc >> 13) & 0x3;
        bool hflip = (tile_desc >> 11) & 1;
        bool vflip = (tile_desc >> 12) & 1;
        
        int py = vflip ? (7 - pixel_y) : pixel_y;
        u16 tile_data_addr = tile_index * 32 + (py * 4);
        
        // Read 4 bytes = 8 pixels (4 bits each)
        for (int px = 0; px < 8; px++) {
            int rx = hflip ? (7 - px) : px;
            u8 data_byte = vram[tile_data_addr + (rx / 2)];
            u8 color_idx = (rx % 2 == 0) ? (data_byte >> 4) : (data_byte & 0x0F);
            
            if (color_idx != 0) { // 0 is transparent
                u16 color = cram[(palette * 16) + color_idx];
                int screen_x = tx * 8 + px;
                if (screen_x < SCREEN_WIDTH) {
                    buffer[screen_x] = { convert_color(color, 0), convert_color(color, 1), convert_color(color, 2) };
                }
            }
        }
    }
}

void VDP::get_scanline(int line, std::vector<Pixel>& buffer) {
    if (line < SCREEN_HEIGHT_NTSC && !line_buffer[line].empty()) {
        buffer = line_buffer[line];
    }
}

void VDP::write_control_word(u16 value) {
    if (!ctrl.pending_write) {
        if ((value & 0xC000) == 0x8000) {
            // Register write
            u8 reg_id = (value >> 8) & 0x1F;
            u8 data = value & 0xFF;
            reg.mode[reg_id] = data;
        } else {
            // First half of command word
            ctrl.control_word = value;
            ctrl.pending_write = true;
        }
    } else {
        // Second half of command word
        u16 first = ctrl.control_word;
        u16 second = value;
        
        ctrl.code = ((first >> 14) & 3) | ((second >> 2) & 0x3C);
        ctrl.address = (first & 0x3FFF) | ((second & 3) << 14);
        
        if (ctrl.code & 0x20) {
            // DMA
            ctrl.dma_enabled = true;
            execute_dma();
        }
        ctrl.pending_write = false;
    }
}

u16 VDP::read_control_word() {
    ctrl.pending_write = false;
    u16 status = 0x3400; // Hardcoded mostly empty status
    if (state.vblank) status |= 0x0008;
    // ... FIFO empty, etc.
    return status;
}

void VDP::write_data_word(u16 value) {
    ctrl.pending_write = false;
    write_internal(value);
}

u16 VDP::read_data_word() {
    ctrl.pending_write = false;
    return read_internal();
}

void VDP::write_internal(u16 value) {
    if ((ctrl.code & 0x0F) == 0x01) { // VRAM
        vram[ctrl.address] = value >> 8;
        vram[ctrl.address ^ 1] = value & 0xFF;
    } else if ((ctrl.code & 0x0F) == 0x03) { // CRAM
        cram[(ctrl.address >> 1) & 0x3F] = value;
    } else if ((ctrl.code & 0x0F) == 0x05) { // VSRAM
        vsram[(ctrl.address >> 1) & 0x3F] = value;
    }
    ctrl.address += reg.mode[15];
}

u16 VDP::read_internal() {
    u16 val = 0;
    if ((ctrl.code & 0x0F) == 0x00) { // VRAM
        val = (vram[ctrl.address] << 8) | vram[ctrl.address ^ 1];
    } else if ((ctrl.code & 0x0F) == 0x08) { // CRAM
        val = cram[(ctrl.address >> 1) & 0x3F];
    } else if ((ctrl.code & 0x0F) == 0x04) { // VSRAM
        val = vsram[(ctrl.address >> 1) & 0x3F];
    }
    ctrl.address += reg.mode[15];
    return val;
}

void VDP::execute_dma() {
    u16 len = (reg.mode[19] | (reg.mode[20] << 8));
    if (len == 0) len = 0xFFFF;
    u32 src = (reg.mode[21] | (reg.mode[22] << 8) | ((reg.mode[23] & 0x7F) << 16)) << 1;
    
    u8 dma_type = reg.mode[23] >> 6;
    if (dma_type == 0 || dma_type == 1) { // Memory to VRAM
        for (u32 i = 0; i < len; i++) {
            u16 val = bus->read_word(src);
            src += 2;
            write_internal(val);
        }
    }
    // Update registers
    reg.mode[19] = 0; reg.mode[20] = 0;
    reg.mode[21] = src >> 1; reg.mode[22] = src >> 9; reg.mode[23] = (reg.mode[23] & 0x80) | ((src >> 17) & 0x7F);
    ctrl.dma_enabled = false;
}

u8 VDP::convert_color(u16 cram_color, u8 component) {
    u8 r = (cram_color & 0x000E) >> 1;
    u8 g = (cram_color & 0x00E0) >> 5;
    u8 b = (cram_color & 0x0E00) >> 9;
    
    r = (r * 255) / 7;
    g = (g * 255) / 7;
    b = (b * 255) / 7;
    
    switch (component) {
        case 0: return r;
        case 1: return g;
        case 2: return b;
        default: return 0;
    }
}

} // namespace md
