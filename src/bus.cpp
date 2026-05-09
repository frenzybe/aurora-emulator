#include "bus.hpp"
#include "cpu/m68k.hpp"
#include "cpu/z80.hpp"
#include "vdp/vdp.hpp"
#include "audio/psg.hpp"
#include "audio/fm.hpp"
#include "input/controller.hpp"
#include <iostream>

extern "C" {
#include "m68k.h"
}

namespace md {

Bus::Bus() {
    init();
}

Bus::~Bus() = default;

void Bus::init() {
    reset();
}

void Bus::reset() {
    ram.fill(0);
    vram.fill(0);
    zram.fill(0);
    rom.clear();
    bios.clear();
    
    std::fill(std::begin(io.vdp_ctrl), std::end(io.vdp_ctrl), 0);
    std::fill(std::begin(io.psg_ctrl), std::end(io.psg_ctrl), 0);
    std::fill(std::begin(io.fm_ctrl), std::end(io.fm_ctrl), 0);
    io.controller_1 = 0;
    io.controller_2 = 0;
}

void Bus::load_rom(const std::vector<u8>& data, u32 size) {
    rom.resize(size);
    std::copy(data.begin(), data.begin() + size, rom.begin());
    std::cout << "ROM loaded: " << size << " bytes\n";
}

void Bus::load_bios(const std::vector<u8>& data, u32 size) {
    bios.resize(size);
    std::copy(data.begin(), data.begin() + size, bios.begin());
}

u8 Bus::read(addr_t addr) {
    addr &= 0xFFFFFF;
    if (addr < 0x400000) {
        if (addr < rom.size()) return rom[addr];
        return 0;
    }
    if ((addr & 0xE00000) == 0xE00000) { // RAM (mirrored every 64KB, typically accessed at 0xFF0000)
        return ram[addr & 0xFFFF];
    }
    if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
        // Z80 RAM / YM2612
        return 0xFF; // TODO
    }
    if (addr >= 0xA10000 && addr <= 0xA1001F) {
        return read_io(addr);
    }
    if (addr >= 0xC00000 && addr <= 0xC0001F) {
        // VDP (Byte reads)
        if (vdp_dev) {
            u16 word = 0;
            if ((addr & 0x1E) == 0x00) word = vdp_dev->read_data_word();
            else if ((addr & 0x1E) == 0x04) word = vdp_dev->read_control_word();
            return (addr & 1) ? (word & 0xFF) : (word >> 8);
        }
    }
    return 0xFF;
}

u16 Bus::read_word(addr_t addr) {
    addr &= 0xFFFFFF;
    if (addr >= 0xC00000 && addr <= 0xC0001F) {
        if (vdp_dev) {
            if ((addr & 0x1E) == 0x00) return vdp_dev->read_data_word();
            if ((addr & 0x1E) == 0x04) return vdp_dev->read_control_word();
        }
        return 0xFFFF;
    }
    return (read(addr) << 8) | read(addr + 1);
}

u32 Bus::read_long(addr_t addr) {
    return (read_word(addr) << 16) | read_word(addr + 2);
}

void Bus::write(addr_t addr, u8 value) {
    addr &= 0xFFFFFF;
    if ((addr & 0xE00000) == 0xE00000) { // RAM
        ram[addr & 0xFFFF] = value;
        return;
    }
    if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
        // Z80 RAM
        return;
    }
    if (addr >= 0xA10000 && addr <= 0xA1001F) {
        write_io(addr, value);
        return;
    }
    if (addr >= 0xC00000 && addr <= 0xC0001F) {
        // VDP byte write
        if (vdp_dev) {
            u16 word = (value << 8) | value;
            if ((addr & 0x1E) == 0x00) vdp_dev->write_data_word(word);
            else if ((addr & 0x1E) == 0x04) vdp_dev->write_control_word(word);
        }
    }
}

void Bus::write_word(addr_t addr, u16 value) {
    addr &= 0xFFFFFF;
    if (addr >= 0xC00000 && addr <= 0xC0001F) {
        if (vdp_dev) {
            if ((addr & 0x1E) == 0x00) vdp_dev->write_data_word(value);
            else if ((addr & 0x1E) == 0x04) vdp_dev->write_control_word(value);
        }
        return;
    }
    write(addr, value >> 8);
    write(addr + 1, value & 0xFF);
}

void Bus::write_long(addr_t addr, u32 value) {
    write_word(addr, value >> 16);
    write_word(addr + 2, value & 0xFFFF);
}

u8 Bus::read_io(addr_t addr) {
    switch (addr & 0x1F) {
        case 0x03: // Controller 1 data
            return ctrl1 ? (ctrl1->get_md_button(BTN_A).pressed ? 0x00 : 0x7F) : 0x7F;
        case 0x05: // Controller 2 data
            return ctrl2 ? (ctrl2->get_md_button(BTN_A).pressed ? 0x00 : 0x7F) : 0x7F;
        default: return 0xFF;
    }
}

void Bus::write_io(addr_t addr, u8 value) {
    // Controller select, TMSS, etc
}

void Bus::request_interrupt(int level) {
    m68k_set_irq(level);
}

// Dummy methods to satisfy compiler if anything uses them internally
u8 Bus::read_memory(addr_t addr) { return read(addr); }
void Bus::write_memory(addr_t addr, u8 value) { write(addr, value); }

} // namespace md
