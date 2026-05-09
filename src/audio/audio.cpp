#include "audio/audio.hpp"
#include "audio/psg.hpp"
#include "audio/fm.hpp"

namespace md {

Audio::Audio(Bus* bus) : bus(bus) {
    psg = new PSG();
    fm = new FM();
}

Audio::~Audio() {
    delete psg;
    delete fm;
}

void Audio::init() {
    psg->init();
    fm->init();
    sample_buffer.resize(4096);  // Буфер на ~100ms при 44.1kHz
}

void Audio::reset() {
    psg->reset();
    fm->reset();
    sample_buffer.clear();
}

void Audio::tick(int cycles) {
    // Обновляем PSG и FM
    psg->tick(cycles);
    fm->tick(cycles);
}

void Audio::mix(std::vector<float>& buffer, int samples) {
    // Смешиваем PSG и FM в выходной буфер
    for (int i = 0; i < samples; i++) {
        float psg_sample = psg->get_sample();
        float fm_sample = fm->get_sample();
        
        // Смешивание (можно настроить громкость каждого канала)
        float mixed = (psg_sample * 0.4f + fm_sample * 0.6f) * volume;
        
        // Ограничение
        mixed = std::clamp(mixed, -1.0f, 1.0f);
        
        buffer[i] = mixed;
    }
}

} // namespace md
