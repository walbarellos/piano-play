#include "abntpiano/AudioDevice.hpp"
#include <iostream>

namespace abntpiano {

AudioDevice::AudioDevice(SynthEngine& synth)
    : synth_(synth) {}

AudioDevice::~AudioDevice() {
    close();
}

bool AudioDevice::open(int sampleRate, int samples) {
    close();

    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};
    desired.freq = sampleRate;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = static_cast<Uint16>(samples);
    desired.callback = audioCallback;
    desired.userdata = this;

    deviceId_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (deviceId_ == 0) {
        std::cerr << "Erro ao abrir áudio SDL: " << SDL_GetError() << "\n";
        return false;
    }

    sampleRate_ = obtained.freq;
    synth_.setSampleRate(sampleRate_);
    resume();
    return true;
}

void AudioDevice::close() {
    if (deviceId_ != 0) {
        SDL_CloseAudioDevice(deviceId_);
        deviceId_ = 0;
    }
}

void AudioDevice::pause() {
    if (deviceId_ != 0) {
        SDL_PauseAudioDevice(deviceId_, 1);
    }
}

void AudioDevice::resume() {
    if (deviceId_ != 0) {
        SDL_PauseAudioDevice(deviceId_, 0);
    }
}

void AudioDevice::audioCallback(void* userdata, Uint8* stream, int len) {
    auto* self = static_cast<AudioDevice*>(userdata);
    float* floatStream = reinterpret_cast<float*>(stream);
    size_t frames = len / sizeof(float);
    self->synth_.render(floatStream, frames);
}

} // namespace abntpiano
