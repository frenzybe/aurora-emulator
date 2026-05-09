#ifndef AUDIO_PSG_HPP
#define AUDIO_PSG_HPP

#include "types.hpp"
#include <array>
#include <cstdint>

namespace md {

// Programmable Sound Generator (SN76489)
class PSG {
public:
    PSG();
    ~PSG();

    void init();
    void reset();
    void write_control(u8 value);
    void write_data(u8 value);
    
    // Генерация сэмпла (вызывается каждый цикл)
    void tick(int cycles);
    
    // Получить текущий сэмпл (смесь 4 каналов)
    f32 get_sample() const { return current_sample; }
    
    // Установка громкости каналов
    void set_volume(float global_volume) { volume = global_volume; }

private:
    // 4 канала: 3 square wave + 1 noise
    struct Channel {
        u8 tone = 0;        // Tone register (10-bit)
        u8 volume = 0;      // Volume (4-bit, 0-15)
        u8 noise_control = 0; // Noise mode
        int counter = 0;
        int frequency = 0;
        bool output = false;
    };

    std::array<Channel, 4> channels;
    
    // LFSR для noise канала
    u16 noise_lfsr = 0x8000;
    
    f32 current_sample = 0.0f;
    float volume = 1.0f;

    // Обновление одного канала
    void update_channel(Channel& ch, int cycles);
    
    // Square wave generation
    f32 generate_square(const Channel& ch);
    
    // Noise generation
    f32 generate_noise(const Channel& ch);
};

} // namespace md

#endif // AUDIO_PSG_HPP
