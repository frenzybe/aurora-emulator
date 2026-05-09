#ifndef AUDIO_FM_HPP
#define AUDIO_FM_HPP

#include "types.hpp"
#include <array>
#include <cstdint>

namespace md {

// YM2612 (OPN2) FM Synthesis Chip
class FM {
public:
    FM();
    ~FM();

    void init();
    void reset();
    
    // Регистры 0x00-0xFF
    void write_reg(u8 reg, u8 data);
    u8 read_reg(u8 reg);
    
    // Генерация сэмпла
    void tick(int cycles);
    f32 get_sample() const { return current_sample; }
    
    void set_volume(float vol) { global_volume = vol; }

private:
    // OP (Operator) структура
    struct Operator {
        // 20 параметров оператора
        u8 mult = 0;         // Множитель частоты
        u8 detune = 0;       // Детюн
        u8 total_level = 0;  // Общая громкость
        u8 key_scale = 0;    // Ключевая шкала
        u8 attack_rate = 0;  // Скорость атаки
        u8 amplitude_mod = 0; // Амплитудная модуляция
        u8 decay_rate = 0;   // Скорость затухания
        u8 sustain_level = 0; // Уровень сустейна
        u8 release_rate = 0;  // Скорость релиза
        u8 ssg_eg = 0;       // SSG-EG
        
        // Phase
        int phase = 0;
        int phase_step = 0;
        
        // Envelope
        int env = 0;
        int env_step = 0;
        enum { ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE, ENV_OFF } env_state;
        
        void calculate_phase_step();
        void update_envelope();
    };

    // Channel (3 оператора + 1 ADPCM)
    struct Channel {
        Operator ops[4];  // 4 оператора на канал (YM2612: 6 FM + 3 SSG)
        u8 algorithm = 0; // Алгоритм связи операторов
        u8 feedback = 0;  // Обратная связь (1-3)
        u8 lfo_enable = 0;
        u8 lfo_freq = 0;
        bool key_on = false;
    };

    std::array<Channel, 6> channels;  // 6 FM каналов
    std::array<Channel, 3> ssg_channels; // 3 SSG канала
    
    // LFO
    u16 lfo_counter = 0;
    bool lfo_active = false;
    
    f32 current_sample = 0.0f;
    float global_volume = 1.0f;
    
    // Внутренние методы
    void generate_channel(int ch);
    f32 calculate_operator(const Operator& op, int phase);
    void update_envelopes();
};

} // namespace md

#endif // AUDIO_FM_HPP
