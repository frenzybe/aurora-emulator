#ifndef CPU_M68K_HPP
#define CPU_M68K_HPP

#include "types.hpp"

namespace md {

class Bus;

// Motorola 68000 CPU (Musashi wrapper)
class CPU68K {
public:
    CPU68K(Bus* bus);
    ~CPU68K();

    void init();
    void reset();
    void step();  // Выполнить одну инструкцию
    void run_frame();  // Выполнить один кадр

    u32 get_pc() const;
    void set_pc(u32 addr);

private:
    Bus* bus;
    u32 cycles = 0;
};

} // namespace md

#endif // CPU_M68K_HPP
