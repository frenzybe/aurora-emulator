#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>
#include <iomanip>
#include <bitset>

namespace md {

// Конвертация в hex-строку
inline std::string to_hex(u32 value, int width = 8) {
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(width) << std::hex << value;
    return ss.str();
}

// Конвертация в bin-строку
inline std::string to_bin(u8 value) {
    return std::bitset<8>(value).to_string();
}

// Bit manipulation helpers
template<typename T>
inline T get_bits(T value, u8 start, u8 length) {
    return (value >> start) & ((1 << length) - 1);
}

template<typename T>
inline T set_bits(T value, u8 start, u8 length, T bits) {
    T mask = ((1 << length) - 1) << start;
    return (value & ~mask) | ((bits << start) & mask);
}

// Little-endian/Big-endian конвертация
inline u16 swap16(u16 value) {
    return (value << 8) | (value >> 8);
}

inline u32 swap32(u32 value) {
    return (value << 24) |
           ((value << 8) & 0x00FF0000) |
           ((value >> 8) & 0x0000FF00) |
           (value >> 24);
}

// Clamp значение
template<typename T>
inline T clamp(T value, T min, T max) {
    return (value < min) ? min : (value > max) ? max : value;
}

} // namespace md

#endif // UTILS_HPP
