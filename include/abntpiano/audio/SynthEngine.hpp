#pragma once

#include "abntpiano/Note.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace abntpiano {

// Sintetizador polifônico com envelope ADSR e agendamento com precisão de amostra
// (RF03, RF04, RF05, ADR-02).
//
// SINCRONIA: o relógio mestre é o contador de amostras renderizadas
// (audioTime()). A thread de jogo agenda eventos em tempo ABSOLUTO de áudio e o
// callback os consome amostra a amostra. Isso elimina o jitter de frame
// (±16ms a 60fps) e o drift entre o relógio de parede e o da placa de som.
class SynthEngine {
public:
    static constexpr size_t kMaxVoices = 64;

    explicit SynthEngine(double sampleRate = 44100.0);

    void setSampleRate(double sampleRate);
    double sampleRate() const { return sampleRate_; }

    // Relógio de áudio: segundos renderizados desde a abertura do device.
    double audioTime() const;

    // Agendamento com precisão de amostra (tempo absoluto de áudio)
    void scheduleNoteOn(double atSeconds, int midiNote, float velocity = 0.8f);
    void scheduleNoteOff(double atSeconds, int midiNote);
    void clearSchedule();

    // Disparo imediato (free play, UI)
    void noteOn(int midiNote, float velocity = 0.8f);
    void noteOff(int midiNote);
    void setSustain(bool active);
    bool isSustainActive() const { return sustainActive_; }

    void allNotesOff();
    void releaseAllNotes();

    void setNaturalDecayTime(double seconds);
    double naturalDecayTime() const { return naturalDecayTime_; }

    // Render (thread de áudio)
    void render(float* buffer, size_t frames);

    size_t activeVoiceCount() const;
    size_t pendingEventCount() const;

private:
    struct Voice {
        enum class State { Attack, Decay, Sustain, Release, Off };
        bool active = false;
        bool markedForReleaseOnSustainUp = false;
        int midiNote = 0;
        double frequency = 440.0;
        double phase = 0.0;
        double phaseIncrement = 0.0;
        double sampleRate = 44100.0;
        double envelope = 0.0;
        float velocity = 0.8f;
        uint64_t startOrder = 0;
        State state = State::Off;
    };

    struct ScheduledEvent { uint64_t sample; int midiNote; float velocity; bool on; };
    struct PendingEvent   { uint32_t offset; int midiNote; float velocity; bool on; };

    void pushEvent(double atSeconds, int midiNote, float velocity, bool on);
    void noteOnLocked(int midiNote, float velocity);
    void noteOffLocked(int midiNote);

    double sampleRate_ = 44100.0;
    double naturalDecayTime_ = 2.0;
    bool sustainActive_ = false;
    uint64_t orderCounter_ = 0;

    std::atomic<uint64_t> samplesRendered_{0};
    std::array<Voice, kMaxVoices> voices_;

    mutable std::mutex queueMutex_;
    mutable std::mutex voiceMutex_;
    std::vector<ScheduledEvent> scheduled_;
    std::vector<PendingEvent> pending_; // scratch da thread de áudio
};

} // namespace abntpiano
