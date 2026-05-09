#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "types.hpp"
#include <vector>
#include <cstdint>

namespace md {

class Bus;
class PSG;
class FM;

// Audio Subsystem (сведение PSG + FM)
class Audio {
public:
    Audio(Bus* bus);
    ~Audio();

    void init();
    void reset();
    
    void tick(int cycles);
    void mix(std::vector<float>& buffer, int samples);
    
    void set_volume(float vol) { volume = vol; }
    void set_sample_rate(int rate) { sample_rate = rate; }
    
    // Геттеры для доступа к компонентам
    PSG* get_psg() const { return psg; }
    FM* get_fm() const { return fm; }

private:
    Bus* bus;
    PSG* psg = nullptr;
    FM* fm = nullptr;
    
    float volume = 1.0f;
    int sample_rate = 44100;
    
    std::vector<float> sample_buffer;
    int buffer_pos = 0;
    
    void resample_and_mix();
};

} // namespace md

#endif // AUDIO_HPP
