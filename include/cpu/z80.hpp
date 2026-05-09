#ifndef CPU_Z80_HPP
#define CPU_Z80_HPP

#include "types.hpp"
#include <array>
#include <cstdint>

namespace md {

class Bus;

// Zilog Z80 CPU (sub-processor для звука)
class CPUZ80 {
public:
    CPUZ80(Bus* bus);
    ~CPUZ80();

    void init();
    void reset();
    void step();  // Одна инструкция
    void run_cycles(int cycles);  // Выполнить N циклов

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wnested-anon-types"
    // Регистры
    struct {
        union {
            struct { u8 f, a; };  // Flags + Accumulator A
            u16 af;
        };
        union {
            struct { u8 c, b; };
            u16 bc;
        };
        union {
            struct { u8 e, d; };
            u16 de;
        };
        union {
            struct { u8 l, h; };
            u16 hl;
        };
        u16 ix, iy, sp, pc;
        u8 i, r, iff1, iff2;
        u16 af_prime, bc_prime, de_prime, hl_prime;
    } reg;
#pragma GCC diagnostic pop

    // Флаги
    enum Flag {
        FLAG_S = 0x80,  // Sign
        FLAG_Z = 0x40,  // Zero
        FLAG_5 = 0x20,  // Copy of bit 5
        FLAG_H = 0x10,  // Half Carry
        FLAG_3 = 0x08,  // Copy of bit 3
        FLAG_PV = 0x04, // Parity/Overflow
        FLAG_N = 0x02,  // Subtract
        FLAG_C = 0x01   // Carry
    };

    bool get_flag(u8 flag) const { return (reg.f & flag) != 0; }
    void set_flag(u8 flag, bool value) {
        if (value) reg.f |= flag;
        else reg.f &= ~flag;
    }

    // Состояние (пауза/резюме)
    void halt(bool h) { halted = h; }
    bool is_halted() const { return halted; }

private:
    Bus* bus;
    bool halted = false;
    [[maybe_unused]] u8 int_mode = 0;  // Mode 0, 1, 2

    u8 fetch_byte();
    u16 fetch_word();
    void execute_opcode(u8 opcode);

    // Memory access (с waitstates)
    u8 read_mem(u16 addr);
    void write_mem(u16 addr, u8 value);

    // I/O
    u8 read_io(u16 port);
    void write_io(u16 port, u8 value);

    // Прерывания
    void handle_interrupt();
    void reti();
    
    // Стек
    void push(u16 value);
    u16 pop();

    // Вспомогательные
    void update_flags_parity();
    u32 cycles_remaining = 0;
    
    // Управление циклами
    void add_cycles(int c) { cycles_remaining += c; }
    
    // Доступ к 8-битным регистрам по индексу
    u8 get_reg8(int idx) const;
    void set_reg8(int idx, u8 value);
};

} // namespace md

#endif // CPU_Z80_HPP
