#include "audio/psg.hpp"
#include <cmath>

namespace md {

PSG::PSG() = default;

PSG::~PSG() = default;

void PSG::init() {
    reset();
}

void PSG::reset() {
    for (auto& ch : channels) {
        ch.tone = 0;
        ch.volume = 0;
        ch.noise_control = 0;
        ch.counter = 0;
        ch.frequency = 0;
        ch.output = false;
    }
    noise_lfsr = 0x8000;
    current_sample = 0.0f;
}

void PSG::write_control(u8 value) {
    // SN76489: latch/data bit (bit 7)
    if (value & 0x80) {
        // Latch: select channel and register type
        u8 channel = (value >> 5) & 0x3;
        u8 reg_type = (value >> 4) & 0x1;  // 0=tone, 1=volume/noise
        
        if (reg_type == 0) {
            // Tone register (10-bit)
            channels[channel].tone = (channels[channel].tone & 0x400) | (value & 0x3F);
        } else {
            // Volume register (4-bit)
            channels[channel].volume = value & 0x0F;
        }
    } else {
        // Data: continuation of tone register (bit 9-10)
        for (auto& ch : channels) {
            if (ch.tone & 0x400) {
                ch.tone = (ch.tone & 0x3F) | ((value & 0x03) << 6);
                break;
            }
        }
    }
}

void PSG::write_data(u8 value) {
    write_control(value);
}

void PSG::tick(int cycles) {
    f32 sample = 0.0f;
    
    for (auto& ch : channels) {
        update_channel(ch, cycles);
        f32 chan_sample = generate_square(ch);
        sample += chan_sample;
    }
    
    current_sample = sample / 4.0f * volume;
}

void PSG::update_channel(Channel& ch, int cycles) {
    if (ch.tone == 0) {
        ch.counter = 0;
        ch.output = false;
        return;
    }
    
    ch.counter += cycles;
    int period = (ch.tone + 1) * 2;
    
    if (ch.counter >= period) {
        ch.counter -= period;
        ch.output = !ch.output;
    }
}

f32 PSG::generate_square(const Channel& ch) {
    if (ch.volume == 0) return 0.0f;
    
    float amplitude = ch.output ? 1.0f : 0.0f;
    float vol = static_cast<float>(ch.volume) / 15.0f;
    return amplitude * vol;
}

} // namespace md
