#ifndef BUS_HPP
#define BUS_HPP

#include "types.hpp"
#include <array>
#include <functional>

namespace md {

// Forward declarations
class CPU68K;
class CPUZ80;
class VDP;
class PSG;
class FM;
class Controller;

// Шина памяти и I/O
class Bus {
public:
    Bus();
    ~Bus();

    // Инициализация
    void init();
    void reset();

    // Загрузка ROM
    void load_rom(const std::vector<u8>& data, u32 size);
    void load_bios(const std::vector<u8>& data, u32 size);

    // Чтение/запись из/в память (24-bit адрес)
    u8 read(addr_t addr);
    u16 read_word(addr_t addr);
    u32 read_long(addr_t addr);
    
    void write(addr_t addr, u8 value);
    void write_word(addr_t addr, u16 value);
    void write_long(addr_t addr, u32 value);

    // Прямой доступ к памяти (для DMA)
    u8* get_ram_ptr() { return ram.data(); }
    u8* get_vram_ptr() { return vram.data(); }
    u8* get_zram_ptr() { return zram.data(); }
    const std::vector<u8>& get_rom() const { return rom; }

    // Регистры I/O
    struct {
        // VDP регистры (0x00-0x1F)
        u8 vdp_ctrl[0x20];
        // PSG регистры (0x00-0x0F через write)
        u8 psg_ctrl[0x10];
        // FM регистры (0x00-0xFF)
        u8 fm_ctrl[0x100];
        // Controller ports
        u8 controller_1;
        u8 controller_2;
        u8 controller_3; // expansion
        u8 controller_4; // expansion
    } io;

    // Подключение устройств
    void set_cpu68k(CPU68K* cpu) { cpu_68k = cpu; }
    void set_cpu_z80(CPUZ80* cpu) { cpu_z80 = cpu; }
    void set_vdp(VDP* vdp) { vdp_dev = vdp; }
    void set_psg(PSG* psg) { psg_dev = psg; }
    void set_fm(FM* fm) { fm_dev = fm; }
    void set_controller_1(Controller* port) { ctrl1 = port; }
    void set_controller_2(Controller* port) { ctrl2 = port; }

    // Callback для прерываний
    using InterruptCallback = std::function<void(int level)>;
    void set_interrupt_callback(InterruptCallback cb) { int_cb = std::move(cb); }

    // Запрос прерывания
    void request_interrupt(int level);

private:
    // Память
    std::array<u8, MD_RAM_SIZE> ram;      // 64KB RAM
    std::array<u8, MD_VRAM_SIZE> vram;    // 64KB VRAM
    std::array<u8, MD_ZRAM_SIZE> zram;    // 8KB Z80 RAM
    std::vector<u8> rom;                  // ROM (до 4MB)
    std::vector<u8> bios;                 // BIOS (16KB)

    // Устройства
    CPU68K* cpu_68k = nullptr;
    CPUZ80* cpu_z80 = nullptr;
    VDP* vdp_dev = nullptr;
    PSG* psg_dev = nullptr;
    FM* fm_dev = nullptr;
    Controller* ctrl1 = nullptr;
    Controller* ctrl2 = nullptr;

    // Прерывания
    InterruptCallback int_cb;

    // Внутренние методы
    u8 read_memory(addr_t addr);
    u16 read_memory_word(addr_t addr);
    u32 read_memory_long(addr_t addr);
    
    void write_memory(addr_t addr, u8 value);
    void write_memory_word(addr_t addr, u16 value);
    void write_memory_long(addr_t addr, u32 value);

    u8 read_io(addr_t addr);
    u16 read_io_word(addr_t addr);
    void write_io(addr_t addr, u8 value);
    void write_io_word(addr_t addr, u16 value);

    // Z80 shared memory access
    u8 z80_read(addr_t addr);
    void z80_write(addr_t addr, u8 value);
};

} // namespace md

#endif // BUS_HPP
