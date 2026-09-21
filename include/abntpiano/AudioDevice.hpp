#pragma once

#include "abntpiano/SynthEngine.hpp"
#include <SDL.h>

namespace abntpiano {

// Abstração RAII sobre o dispositivo de áudio SDL2 (Audio Layer).
// Encapsula abertura, callback, fechamento e controle de execução.
class AudioDevice {
public:
    explicit AudioDevice(SynthEngine& synth);
    ~AudioDevice();

    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

    bool open(int sampleRate = 44100, int samples = 512);
    void close();

    void pause();
    void resume();

    bool isOpen() const { return deviceId_ != 0; }
    int sampleRate() const { return sampleRate_; }

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);

    SynthEngine& synth_;
    SDL_AudioDeviceID deviceId_ = 0;
    int sampleRate_ = 44100;
};

} // namespace abntpiano
