#pragma once

#include "abntpiano/Note.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
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

    explicit SynthEngine(double sampleRate = 44100.0)
        : sampleRate_(sampleRate) {
        for (auto& v : voices_) v.sampleRate = sampleRate_;
        scheduled_.reserve(1024);
    }

    void setSampleRate(double sampleRate) {
        sampleRate_ = sampleRate;
        for (auto& v : voices_) v.sampleRate = sampleRate_;
    }

    // ── Relógio de áudio ─────────────────────────────────────────────────────
    // Segundos renderizados desde a abertura do device. Seguro na thread de jogo.
    double audioTime() const {
        return static_cast<double>(samplesRendered_.load(std::memory_order_relaxed)) / sampleRate_;
    }

    // ── Agendamento com precisão de amostra ──────────────────────────────────
    // atSeconds é tempo ABSOLUTO de áudio (mesma base de audioTime()).
    void scheduleNoteOn(double atSeconds, int midiNote, float velocity = 0.8f) {
        pushEvent(atSeconds, midiNote, velocity, true);
    }

    void scheduleNoteOff(double atSeconds, int midiNote) {
        pushEvent(atSeconds, midiNote, 0.0f, false);
    }

    // Descarta eventos futuros ainda não consumidos (troca de música, seek, speed).
    void clearSchedule() {
        std::lock_guard<std::mutex> lock(queueMutex_);
        scheduled_.clear();
    }

    // ── Disparo imediato (free play, UI) ─────────────────────────────────────
    void noteOn(int midiNote, float velocity = 0.8f) {
        std::lock_guard<std::mutex> lock(voiceMutex_);
        noteOnLocked(midiNote, velocity);
    }

    void noteOff(int midiNote) {
        std::lock_guard<std::mutex> lock(voiceMutex_);
        noteOffLocked(midiNote);
    }

    void setSustain(bool active) {
        std::lock_guard<std::mutex> lock(voiceMutex_);
        sustainActive_ = active;
        if (!sustainActive_) {
            for (auto& v : voices_) {
                if (v.active && v.markedForReleaseOnSustainUp) {
                    v.markedForReleaseOnSustainUp = false;
                    v.state = Voice::State::Release;
                }
            }
        }
    }

    bool isSustainActive() const { return sustainActive_; }

    void allNotesOff() {
        clearSchedule();
        std::lock_guard<std::mutex> lock(voiceMutex_);
        for (auto& v : voices_) {
            v.active = false;
            v.state = Voice::State::Off;
            v.envelope = 0.0;
            v.markedForReleaseOnSustainUp = false;
        }
    }

    // Libera suavemente todas as notas ativas para o Release (~0.25s)
    void releaseAllNotes() {
        clearSchedule();
        std::lock_guard<std::mutex> lock(voiceMutex_);
        for (auto& v : voices_) {
            if (v.active && v.state != Voice::State::Off) {
                v.markedForReleaseOnSustainUp = false;
                v.state = Voice::State::Release;
            }
        }
    }

    void setNaturalDecayTime(double seconds) { naturalDecayTime_ = std::max(0.2, seconds); }
    double naturalDecayTime() const { return naturalDecayTime_; }

    // ── Render (thread de áudio) ─────────────────────────────────────────────
    void render(float* buffer, size_t frames) {
        std::fill(buffer, buffer + frames, 0.0f);

        const uint64_t blockStart = samplesRendered_.load(std::memory_order_relaxed);

        // Copia os eventos deste bloco para fora do lock antes de sintetizar.
        pending_.clear();
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (!scheduled_.empty()) {
                const uint64_t blockEnd = blockStart + frames;
                size_t keep = 0;
                for (size_t i = 0; i < scheduled_.size(); ++i) {
                    const auto& e = scheduled_[i];
                    if (e.sample < blockEnd) {
                        // Eventos atrasados (sample < blockStart) tocam na amostra 0
                        // em vez de serem descartados: perde-se o alinhamento fino,
                        // nunca a nota.
                        uint64_t off = (e.sample > blockStart) ? (e.sample - blockStart) : 0;
                        pending_.push_back(PendingEvent{
                            static_cast<uint32_t>(off), e.midiNote, e.velocity, e.on
                        });
                    } else {
                        scheduled_[keep++] = e;
                    }
                }
                scheduled_.resize(keep);
            }
        }
        std::stable_sort(pending_.begin(), pending_.end(),
                         [](const PendingEvent& a, const PendingEvent& b) { return a.offset < b.offset; });

        std::lock_guard<std::mutex> lock(voiceMutex_);

        const double attackRate   = 1.0 / (0.005 * sampleRate_);
        const double decayRate    = 0.6 / (0.2 * sampleRate_);
        const double sustainLevel = 0.4;

        size_t evIdx = 0;
        for (size_t i = 0; i < frames; ++i) {
            // Consome eventos agendados exatamente nesta amostra
            while (evIdx < pending_.size() && pending_[evIdx].offset <= i) {
                const auto& e = pending_[evIdx++];
                if (e.on) noteOnLocked(e.midiNote, e.velocity);
                else      noteOffLocked(e.midiNote);
            }

            const double releaseRate = 1.0 / ((sustainActive_ ? 0.8 : 0.25) * sampleRate_);
            const double sustainDecayRate =
                sustainLevel / (naturalDecayTime_ * (sustainActive_ ? 1.6 : 1.0) * sampleRate_);

            double mix = 0.0;
            for (auto& v : voices_) {
                if (!v.active) continue;

                switch (v.state) {
                    case Voice::State::Attack:
                        v.envelope += attackRate;
                        if (v.envelope >= 1.0) { v.envelope = 1.0; v.state = Voice::State::Decay; }
                        break;
                    case Voice::State::Decay:
                        v.envelope -= decayRate;
                        if (v.envelope <= sustainLevel) { v.envelope = sustainLevel; v.state = Voice::State::Sustain; }
                        break;
                    case Voice::State::Sustain:
                        v.envelope -= sustainDecayRate;
                        if (v.envelope <= 0.0) { v.envelope = 0.0; v.active = false; v.state = Voice::State::Off; }
                        break;
                    case Voice::State::Release:
                        v.envelope -= releaseRate;
                        if (v.envelope <= 0.0) { v.envelope = 0.0; v.active = false; v.state = Voice::State::Off; }
                        break;
                    case Voice::State::Off:
                        v.active = false;
                        break;
                }
                if (!v.active) continue;

                // Timbre: fundamental + 2º e 3º harmônicos (piano elétrico)
                double sample = std::sin(v.phase) * 0.7 +
                                std::sin(v.phase * 2.0) * 0.2 +
                                std::sin(v.phase * 3.0) * 0.1;
                mix += sample * v.envelope * v.velocity;

                v.phase += v.phaseIncrement;
                if (v.phase >= 2.0 * M_PI) v.phase -= 2.0 * M_PI;
            }

            buffer[i] = static_cast<float>(std::tanh(mix * 0.16));
        }

        samplesRendered_.store(blockStart + frames, std::memory_order_relaxed);
    }

    size_t activeVoiceCount() const {
        size_t count = 0;
        for (const auto& v : voices_) if (v.active) count++;
        return count;
    }

    size_t pendingEventCount() const {
        std::lock_guard<std::mutex> lock(queueMutex_);
        return scheduled_.size();
    }

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

    void pushEvent(double atSeconds, int midiNote, float velocity, bool on) {
        uint64_t sample = static_cast<uint64_t>(std::max(0.0, atSeconds) * sampleRate_ + 0.5);
        std::lock_guard<std::mutex> lock(queueMutex_);
        scheduled_.push_back(ScheduledEvent{sample, midiNote, velocity, on});
    }

    // Requer voiceMutex_
    void noteOnLocked(int midiNote, float velocity) {
        for (auto& v : voices_) {
            if (v.active && v.midiNote == midiNote) {
                v.velocity = velocity;
                v.envelope = std::min(v.envelope, 0.9);
                v.state = Voice::State::Attack;
                v.startOrder = ++orderCounter_;
                return;
            }
        }

        Voice* target = nullptr;
        for (auto& v : voices_) if (!v.active) { target = &v; break; }

        if (!target) {
            // Rouba a voz mais dispensável. NUNCA rouba uma voz em Attack/Decay:
            // ela acabou de começar e roubá-la mata a nota mais recente
            // (era o bug que embaralhava passagens densas).
            double worst = 1e9;
            for (auto& v : voices_) {
                if (v.state == Voice::State::Attack || v.state == Voice::State::Decay) continue;
                double score = v.envelope * v.velocity;
                if (v.state == Voice::State::Release) score *= 0.25;
                if (score < worst) { worst = score; target = &v; }
            }
        }
        if (!target) {
            // Todas em Attack/Decay: sacrifica a mais antiga.
            uint64_t oldest = UINT64_MAX;
            for (auto& v : voices_) if (v.startOrder < oldest) { oldest = v.startOrder; target = &v; }
        }
        if (!target) target = &voices_[0];

        target->active = true;
        target->midiNote = midiNote;
        target->frequency = midiToFrequency(midiNote);
        target->phaseIncrement = (2.0 * M_PI * target->frequency) / sampleRate_;
        target->phase = 0.0;
        target->envelope = 0.0;
        target->velocity = velocity;
        target->startOrder = ++orderCounter_;
        target->state = Voice::State::Attack;
    }

    // Requer voiceMutex_
    void noteOffLocked(int midiNote) {
        for (auto& v : voices_) {
            if (v.active && v.midiNote == midiNote && v.state != Voice::State::Release) {
                if (sustainActive_) v.markedForReleaseOnSustainUp = true;
                else                v.state = Voice::State::Release;
            }
        }
    }

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
