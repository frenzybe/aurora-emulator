#include "audio/fm.hpp"
#include <cmath>
#include <algorithm>

namespace md {

FM::FM() = default;

FM::~FM() = default;

void FM::init() {
    reset();
}

void FM::reset() {
    for (auto& ch : channels) {
        for (auto& op : ch.ops) {
            op.mult = 0;
            op.detune = 0;
            op.total_level = 0;
            op.key_scale = 0;
            op.attack_rate = 0;
            op.amplitude_mod = 0;
            op.decay_rate = 0;
            op.sustain_level = 0;
            op.release_rate = 0;
            op.ssg_eg = 0;
            op.phase = 0;
            op.phase_step = 0;
            op.env = 0;
            op.env_step = 0;
            op.env_state = Operator::ENV_OFF;
        }
        ch.algorithm = 0;
        ch.feedback = 0;
        ch.key_on = false;
    }
    lfo_counter = 0;
    lfo_active = false;
    current_sample = 0.0f;
}

void FM::write_reg(u8 reg, u8 data) {
    // YM2612 имеет 2 банка регистров
    // Упрощённо: только первый банк
    u8 ch = reg & 0x03;
    [[maybe_unused]] u8 op = (reg >> 2) & 0x03;
    u8 param = (reg >> 4) & 0x0F;
    
    if (ch < 6) {
        auto& channel = channels[ch];
        
        switch (param) {
            case 0x0: // LFO
                lfo_active = (data & 0x80) != 0;
                channel.lfo_freq = data & 0x07;
                break;
                
            case 0x1: // Timer 1
            case 0x2: // Timer 2
                break;
                
            case 0x4: // Key on/off
                channel.key_on = (data & 0x0F) != 0;
                if (channel.key_on) {
                    // Reset envelope
                    for (auto& op : channel.ops) {
                        op.env = 0;
                        op.env_state = Operator::ENV_ATTACK;
                    }
                }
                break;
                
            case 0x8: // Algorithm/Feedback
                channel.algorithm = data & 0x07;
                channel.feedback = (data >> 3) & 0x07;
                break;
                
            default:
                // Параметры операторов (20 на канал)
                // Упрощённо: только несколько
                if (param >= 0x20 && param <= 0x3F) {
                    u8 op_idx = (param - 0x20) / 4;
                    if (op_idx < 4) {
                        auto& oper = channel.ops[op_idx];
                        switch ((param - 0x20) % 4) {
                            case 0: // DT/MUL
                                oper.detune = (data >> 4) & 0x07;
                                oper.mult = data & 0x0F;
                                oper.calculate_phase_step();
                                break;
                            case 1: // TL
                                oper.total_level = data & 0x7F;
                                break;
                            case 2: // KS/AR
                                oper.key_scale = (data >> 6) & 0x03;
                                oper.attack_rate = data & 0x1F;
                                break;
                            case 3: // AM/DR
                                oper.amplitude_mod = (data >> 7) & 0x01;
                                oper.decay_rate = data & 0x1F;
                                break;
                        }
                    }
                }
                break;
        }
    }
}

u8 FM::read_reg(u8 /*reg*/) {
    // Чтение статуса (упрощённо)
    return 0x00;  // TODO: реализовать
}

void FM::tick(int cycles) {
    // Обновление LFO
    lfo_counter += cycles;
    if (lfo_counter >= 1024) {  // Примерная частота LFO
        lfo_counter -= 1024;
        // LFO modulation
    }
    
    // Обновление огибающих
    update_envelopes();
    
    // Генерация сэмпла для каждого канала
    f32 sample = 0.0f;
    for (int ch = 0; ch < 6; ch++) {
        generate_channel(ch);
        sample += channels[ch].ops[0].phase / 32768.0f;  // Упрощённо
    }
    
    current_sample = sample / 6.0f * global_volume;
}

void FM::generate_channel(int ch_idx) {
    auto& ch = channels[ch_idx];
    
    // Алгоритм 4-операторного FM
    // Упрощённо: только оператор 1
    auto& op1 = ch.ops[0];
    
    // Phase accumulation
    op1.phase += op1.phase_step;
    if (op1.phase >= (1 << 20)) op1.phase -= (1 << 20);
}

void FM::update_envelopes() {
    for (auto& ch : channels) {
        for (auto& op : ch.ops) {
            if (op.env_state == Operator::ENV_OFF) continue;
            
            op.env_step += 1;  // Зависит от clock
            
            switch (op.env_state) {
                case Operator::ENV_ATTACK:
                    if (op.env < 0x3FF) {
                        op.env += op.env_step;  // Упрощённо
                        if (op.env >= 0x3FF) {
                            op.env = 0x3FF;
                            op.env_state = Operator::ENV_DECAY;
                        }
                    }
                    break;
                case Operator::ENV_DECAY:
                    if (op.env > op.sustain_level * 0x3FF / 15) {
                        op.env -= op.env_step;
                    } else {
                        op.env_state = Operator::ENV_SUSTAIN;
                    }
                    break;
                case Operator::ENV_SUSTAIN:
                    // Держим уровень
                    break;
                case Operator::ENV_RELEASE:
                    if (op.env > 0) {
                        op.env -= op.env_step;
                    } else {
                        op.env = 0;
                        op.env_state = Operator::ENV_OFF;
                    }
                    break;
                default: break;
            }
        }
    }
}

void FM::Operator::calculate_phase_step() {
    // Заглушка
    phase_step = 1;
}

void FM::Operator::update_envelope() {
    // Заглушка
}

} // namespace md
