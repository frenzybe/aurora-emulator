#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <memory>

namespace md {

// Базовые типы
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using f32 = float;

// 24-битный адрес (Mega Drive)
using addr_t = u32;

// Константы
constexpr u32 MD_CLOCK_NTSC = 53693175;  // 7.89 MHz * 68/12
constexpr u32 MD_CLOCK_PAL = 53203425;   // 7.67 MHz * 68/12

constexpr u32 MD_RAM_SIZE = 0x10000;     // 64KB
constexpr u32 MD_VRAM_SIZE = 0x10000;    // 64KB
constexpr u32 MD_ZRAM_SIZE = 0x2000;     // 8KB Z80 RAM

// Разрешение экрана
constexpr u32 SCREEN_WIDTH = 320;
constexpr u32 SCREEN_HEIGHT_NTSC = 224;
constexpr u32 SCREEN_HEIGHT_PAL = 240;

// Frame rate
constexpr double FRAME_RATE_NTSC = 60.0;
constexpr double FRAME_RATE_PAL = 50.0;

} // namespace md

#endif // TYPES_HPP
