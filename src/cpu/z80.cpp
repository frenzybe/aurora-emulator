#include "cpu/z80.hpp"
#include "bus.hpp"
#include <iostream>

namespace md {

CPUZ80::CPUZ80(Bus* bus) : bus(bus) {}

CPUZ80::~CPUZ80() = default;

void CPUZ80::init() {
    reset();
}

void CPUZ80::reset() {
    reg.af = 0;
    reg.bc = 0;
    reg.de = 0;
    reg.hl = 0;
    reg.ix = 0;
    reg.iy = 0;
    reg.sp = 0x2000;  // Z80 stack
    reg.pc = 0;       // Начальный PC
    reg.i = 0;
    reg.r = 0;
    reg.iff1 = reg.iff2 = 0;
    halted = false;
    cycles_remaining = 0;
}

void CPUZ80::step() {
    if (halted) {
        // В режиме HALT потребляем циклы, но не выполняем инструкций
        cycles_remaining -= 4;
        return;
    }
    
    u8 opcode = fetch_byte();
    execute_opcode(opcode);
}

void CPUZ80::run_cycles(int cycles) {
    cycles_remaining = cycles;
    while (cycles_remaining > 0) {
        step();
    }
}

u8 CPUZ80::fetch_byte() {
    u8 value = read_mem(reg.pc);
    reg.pc++;
    return value;
}

u16 CPUZ80::fetch_word() {
    u8 lo = fetch_byte();
    u8 hi = fetch_byte();
    return lo | (hi << 8);
}

void CPUZ80::execute_opcode(u8 opcode) {
    // Упрощённый декодер Z80 инструкций
    // Z80 имеет очень много инструкций, для MVP реализуем только базовые
    
    switch (opcode) {
        case 0x00: // NOP
            add_cycles(4);
            break;
            
        case 0x01: // LD BC, nn
            reg.c = fetch_byte();
            reg.b = fetch_byte();
            add_cycles(10);
            break;
            
        case 0x06: // LD B, n
            reg.b = fetch_byte();
            add_cycles(7);
            break;
            
        case 0x0E: // LD C, n
            reg.c = fetch_byte();
            add_cycles(7);
            break;
            
        case 0x11: // LD DE, nn
            reg.e = fetch_byte();
            reg.d = fetch_byte();
            add_cycles(10);
            break;
            
        case 0x16: // LD D, n
            reg.d = fetch_byte();
            add_cycles(7);
            break;
            
        case 0x1E: // LD E, n
            reg.e = fetch_byte();
            add_cycles(7);
            break;
            
        case 0x21: // LD HL, nn
            reg.l = fetch_byte();
            reg.h = fetch_byte();
            add_cycles(10);
            break;
            
        case 0x26: // LD H, n
            reg.h = fetch_byte();
            add_cycles(7);
            break;
            
        case 0x2E: // LD L, n
            reg.l = fetch_byte();
            add_cycles(7);
            break;
            
        case 0x31: // LD SP, nn
            reg.sp = fetch_word();
            add_cycles(10);
            break;
            
        case 0x36: // LD (HL), n
            write_mem(reg.hl, fetch_byte());
            add_cycles(10);
            break;
            
        case 0x3E: // LD A, n
            reg.a = fetch_byte();
            add_cycles(7);
            break;
            
        case 0x40: // LD B, B
        case 0x41: // LD B, C
        case 0x42: // LD B, D
        case 0x43: // LD B, E
        case 0x44: // LD B, H
        case 0x45: // LD B, L
            reg.b = get_reg8((opcode >> 3) & 0x7);
            add_cycles(4);
            break;
            
        case 0x46: // LD B, (HL)
            reg.b = read_mem(reg.hl);
            add_cycles(7);
            break;
            
        case 0x48: // LD C, B etc.
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
            reg.c = get_reg8((opcode >> 3) & 0x7);
            add_cycles(4);
            break;
            
        case 0x4E: // LD C, (HL)
            reg.c = read_mem(reg.hl);
            add_cycles(7);
            break;
            
        case 0x50: // LD D, B etc.
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
            reg.d = get_reg8((opcode >> 3) & 0x7);
            add_cycles(4);
            break;
            
        case 0x56: // LD D, (HL)
            reg.d = read_mem(reg.hl);
            add_cycles(7);
            break;
            
        case 0x58: // LD E, B etc.
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
            reg.e = get_reg8((opcode >> 3) & 0x7);
            add_cycles(4);
            break;
            
        case 0x5E: // LD E, (HL)
            reg.e = read_mem(reg.hl);
            add_cycles(7);
            break;
            
        case 0x60: // LD H, B etc.
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
            reg.h = get_reg8((opcode >> 3) & 0x7);
            add_cycles(4);
            break;
            
        case 0x66: // LD H, (HL)
            reg.h = read_mem(reg.hl);
            add_cycles(7);
            break;
            
        case 0x68: // LD L, B etc.
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
            reg.l = get_reg8((opcode >> 3) & 0x7);
            add_cycles(4);
            break;
            
        case 0x6E: // LD L, (HL)
            reg.l = read_mem(reg.hl);
            add_cycles(7);
            break;
            
        case 0x70: // LD (HL), B etc.
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
            write_mem(reg.hl, get_reg8((opcode >> 3) & 0x7));
            add_cycles(7);
            break;
            
        case 0x76: // HALT
            halted = true;
            add_cycles(4);
            break;
            
        case 0x77: // LD (HL), A
            write_mem(reg.hl, reg.a);
            add_cycles(7);
            break;
            
        case 0x78: // LD A, B etc.
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
            reg.a = get_reg8((opcode >> 3) & 0x7);
            add_cycles(4);
            break;
            
        case 0x7E: // LD A, (HL)
            reg.a = read_mem(reg.hl);
            add_cycles(7);
            break;
            
        case 0x86: // ADD A, (HL)
            {
                u8 val = read_mem(reg.hl);
                u32 result = reg.a + val;
                reg.a = static_cast<u8>(result);
                set_flag(FLAG_C, result > 0xFF);
                set_flag(FLAG_Z, reg.a == 0);
                set_flag(FLAG_H, ((reg.a & 0x0F) + (val & 0x0F)) > 0x0F);
                set_flag(FLAG_N, false);
                update_flags_parity();
                add_cycles(7);
            }
            break;
            
        case 0xC3: // JP nn
            reg.pc = fetch_word();
            add_cycles(10);
            break;
            
        case 0xC9: // RET
            reg.pc = read_mem(reg.sp);
            reg.sp += 2;
            add_cycles(10);
            break;
            
        case 0xE6: // AND n
            reg.a &= fetch_byte();
            set_flag(FLAG_Z, reg.a == 0);
            set_flag(FLAG_N, false);
            set_flag(FLAG_H, true);
            update_flags_parity();
            add_cycles(7);
            break;
            
        case 0xEE: // XOR n
            reg.a ^= fetch_byte();
            set_flag(FLAG_Z, reg.a == 0);
            set_flag(FLAG_N, false);
            set_flag(FLAG_H, false);
            update_flags_parity();
            add_cycles(7);
            break;
            
        case 0xF3: // DI
            reg.iff1 = reg.iff2 = 0;
            add_cycles(4);
            break;
            
        case 0xFB: // EI
            reg.iff1 = reg.iff2 = 1;
            add_cycles(4);
            break;
            
        default:
            // Неизвестная инструкция — для MVP пропускаем
            // В будущем: добавить полный набор Z80 инструкций
            break;
    }
}

u8 CPUZ80::read_mem(u16 addr) {
    // Z80 может обращаться к своей RAM и к 68K RAM (с waitstates)
    if (addr < 0x2000) {
        return bus->get_zram_ptr()[addr];
    } else {
        // Доступ к 68K RAM через шину (с задержкой)
        return bus->read(addr);
    }
}

void CPUZ80::write_mem(u16 addr, u8 value) {
    if (addr < 0x2000) {
        bus->get_zram_ptr()[addr] = value;
    } else {
        bus->write(addr, value);
    }
}

u8 CPUZ80::read_io(u16 /*port*/) {
    // I/O порты Z80
    // Для MVP: заглушка
    return 0xFF;
}

void CPUZ80::write_io(u16 port, u8 /*value*/) {
    // I/O порты Z80
    // PSG управляется через Z80 I/O
    if ((port & 0xFF) == 0x7F) {
        // PSG register select/write
        // TODO: передать в PSG
    }
}

void CPUZ80::handle_interrupt() {
    if (reg.iff1 && !halted) {
        // Z80 mode 1 interrupt (вектор 0x38)
        reg.iff1 = reg.iff2 = 0;
        push(reg.pc);
        reg.pc = 0x0038;  // Vector 1
        add_cycles(13);
    }
}

void CPUZ80::reti() {
    // Return from interrupt
    reg.pc = pop();
    reg.iff1 = reg.iff2 = 1;
    add_cycles(14);
}

void CPUZ80::update_flags_parity() {
    // Parity flag: 1 if even number of bits set
    u8 bits = reg.a;
    bits ^= bits >> 4;
    bits ^= bits >> 2;
    bits ^= bits >> 1;
    set_flag(FLAG_PV, (bits & 1) == 0);
}

void CPUZ80::push(u16 value) {
    reg.sp -= 2;
    write_mem(reg.sp, static_cast<u8>(value & 0xFF));
    write_mem(reg.sp + 1, static_cast<u8>(value >> 8));
}

u16 CPUZ80::pop() {
    u16 value = read_mem(reg.sp);
    value |= read_mem(reg.sp + 1) << 8;
    reg.sp += 2;
    return value;
}

// Реализация доступа к 8-битным регистрам по индексу
u8 CPUZ80::get_reg8(int idx) const {
    switch (idx & 0x7) {
        case 0: return reg.b;
        case 1: return reg.c;
        case 2: return reg.d;
        case 3: return reg.e;
        case 4: return reg.h;
        case 5: return reg.l;
        case 6: return reg.a;
        case 7: return reg.f; // редко используется
        default: return 0;
    }
}

void CPUZ80::set_reg8(int idx, u8 value) {
    switch (idx & 0x7) {
        case 0: reg.b = value; break;
        case 1: reg.c = value; break;
        case 2: reg.d = value; break;
        case 3: reg.e = value; break;
        case 4: reg.h = value; break;
        case 5: reg.l = value; break;
        case 6: reg.a = value; break;
        case 7: reg.f = value; break;
        default: break;
    }
}

} // namespace md
