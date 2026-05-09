#include "cpu/m68k.hpp"
#include "bus.hpp"
#include <iostream>

extern "C" {
#include "m68k.h"
}

namespace md {

static Bus* g_bus = nullptr;

CPU68K::CPU68K(Bus* bus) : bus(bus) {
    g_bus = bus;
}

CPU68K::~CPU68K() {
    if (g_bus == bus) g_bus = nullptr;
}

void CPU68K::init() {
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    reset();
}

void CPU68K::reset() {
    m68k_pulse_reset();
    cycles = 0;
}

void CPU68K::step() {
    cycles += m68k_execute(1); // will execute minimum 1 instruction
}

void CPU68K::run_frame() {
    int frame_cycles = 30000;
    m68k_execute(frame_cycles);
}

u32 CPU68K::get_pc() const {
    return m68k_get_reg(nullptr, M68K_REG_PC);
}

void CPU68K::set_pc(u32 addr) {
    m68k_set_reg(M68K_REG_PC, addr);
}

} // namespace md

// Musashi callbacks (must be in C linkage)
extern "C" {

unsigned int m68k_read_memory_8(unsigned int address) {
    if (md::g_bus) return md::g_bus->read(address);
    return 0xFF;
}

unsigned int m68k_read_memory_16(unsigned int address) {
    if (md::g_bus) return md::g_bus->read_word(address);
    return 0xFFFF;
}

unsigned int m68k_read_memory_32(unsigned int address) {
    if (md::g_bus) return md::g_bus->read_long(address);
    return 0xFFFFFFFF;
}

void m68k_write_memory_8(unsigned int address, unsigned int value) {
    if (md::g_bus) md::g_bus->write(address, value & 0xFF);
}

void m68k_write_memory_16(unsigned int address, unsigned int value) {
    if (md::g_bus) md::g_bus->write_word(address, value & 0xFFFF);
}

void m68k_write_memory_32(unsigned int address, unsigned int value) {
    if (md::g_bus) md::g_bus->write_long(address, value);
}

// Optional read/write callbacks for instruction fetch
unsigned int m68k_read_disassembler_8(unsigned int address) { return m68k_read_memory_8(address); }
unsigned int m68k_read_disassembler_16(unsigned int address) { return m68k_read_memory_16(address); }
unsigned int m68k_read_disassembler_32(unsigned int address) { return m68k_read_memory_32(address); }

} // extern "C"
