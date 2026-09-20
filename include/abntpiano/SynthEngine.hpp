#pragma once

#include "abntpiano/Note.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace abntpiano {

// Sintetizador polifônico com envelope ADSR (RF03, RF04, RF05, ADR-02)
class SynthEngine {
public:
    static constexpr size_t kMaxVoices = 16;

    explicit SynthEngine(double sampleRate = 44100.0)
        : sampleRate_(sampleRate) {
        for (auto& v : voices_) {
            v.sampleRate = sampleRate_;
        }
    }

    void setSampleRate(double sampleRate) {
        sampleRate_ = sampleRate;
        for (auto& v : voices_) {
            v.sampleRate = sampleRate_;
        }
    }

    void noteOn(int midiNote, float velocity = 0.8f) {
        // Se a nota já está tocando, reinicia com a nova velocity
        for (auto& v : voices_) {
            if (v.active && v.midiNote == midiNote) {
                v.velocity = velocity;
                v.state = Voice::State::Attack;
                return;
            }
        }

        // Procura voz livre
        Voice* target = nullptr;
        for (auto& v : voices_) {
            if (!v.active) {
                target = &v;
                break;
            }
        }

        // Se todas ativas, rouba a voz em release mais adiantada
        if (!target) {
            double lowestEnv = 2.0;
            for (auto& v : voices_) {
                if (v.state == Voice::State::Release && v.envelope < lowestEnv) {
                    lowestEnv = v.envelope;
                    target = &v;
                }
            }
        }

        if (!target) target = &voices_[0]; // fallback voz mais antiga

        target->active = true;
        target->midiNote = midiNote;
        target->frequency = midiToFrequency(midiNote);
        target->phase = 0.0;
        target->velocity = velocity;
        target->state = Voice::State::Attack;
    }

    void noteOff(int midiNote) {
        for (auto& v : voices_) {
            if (v.active && v.midiNote == midiNote && v.state != Voice::State::Release) {
                if (sustainActive_) {
                    v.markedForReleaseOnSustainUp = true;
                } else {
                    v.state = Voice::State::Release;
                }
            }
        }
    }

    void setSustain(bool active) {
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
        for (auto& v : voices_) {
            v.active = false;
            v.state = Voice::State::Off;
            v.envelope = 0.0;
            v.markedForReleaseOnSustainUp = false;
        }
    }

    // Libera suavemente todas as notas ativas para o Release (~0.25s) sem corte abrupto
    void releaseAllNotes() {
        for (auto& v : voices_) {
            if (v.active && v.state != Voice::State::Off) {
                v.markedForReleaseOnSustainUp = false;
                v.state = Voice::State::Release;
            }
        }
    }

    // Configura tempo de vida / decaimento natural da nota (em segundos)
    void setNaturalDecayTime(double seconds) {
        naturalDecayTime_ = std::max(0.2, seconds);
    }
    double naturalDecayTime() const { return naturalDecayTime_; }

    // Renderiza áudio PCM mono em float [-1.0, 1.0]
    void render(float* buffer, size_t frames) {
        std::fill(buffer, buffer + frames, 0.0f);

        for (auto& v : voices_) {
            if (!v.active) continue;

            double phaseIncrement = (2.0 * M_PI * v.frequency) / sampleRate_;
            
            // Taxas do envelope ADSR em segundos
            // Attack: 0.005s, Decay: 0.2s, Sustain level: 0.4
            double attackRate   = 1.0 / (0.005 * sampleRate_);
            double decayRate    = 0.6 / (0.2 * sampleRate_);
            double sustainLevel = 0.4;
            double releaseRate  = 1.0 / ((sustainActive_ ? 0.8 : 0.25) * sampleRate_);
            // Decaimento contínuo natural (tempo de vida realista da vibração da corda)
            double sustainDecayRate = sustainLevel / (naturalDecayTime_ * (sustainActive_ ? 1.6 : 1.0) * sampleRate_);

            for (size_t i = 0; i < frames; ++i) {
                // Atualiza ADSR
                switch (v.state) {
                    case Voice::State::Attack:
                        v.envelope += attackRate;
                        if (v.envelope >= 1.0) {
                            v.envelope = 1.0;
                            v.state = Voice::State::Decay;
                        }
                        break;
                    case Voice::State::Decay:
                        v.envelope -= decayRate;
                        if (v.envelope <= sustainLevel) {
                            v.envelope = sustainLevel;
                            v.state = Voice::State::Sustain;
                        }
                        break;
                    case Voice::State::Sustain:
                        v.envelope -= sustainDecayRate;
                        if (v.envelope <= 0.0) {
                            v.envelope = 0.0;
                            v.active = false;
                            v.state = Voice::State::Off;
                        }
                        break;
                    case Voice::State::Release:
                        v.envelope -= releaseRate;
                        if (v.envelope <= 0.0) {
                            v.envelope = 0.0;
                            v.active = false;
                            v.state = Voice::State::Off;
                        }
                        break;
                    case Voice::State::Off:
                        v.active = false;
                        break;
                }

                if (!v.active) break;

                // Timbre: Fundamental + 2º harmônico suave + 3º harmônico sutil (som tipo piano elétrico)
                double sample = std::sin(v.phase) * 0.7 +
                                std::sin(v.phase * 2.0) * 0.2 +
                                std::sin(v.phase * 3.0) * 0.1;
                
                buffer[i] += static_cast<float>(sample * v.envelope * v.velocity * 0.25);

                v.phase += phaseIncrement;
                if (v.phase >= 2.0 * M_PI) {
                    v.phase -= 2.0 * M_PI;
                }
            }
        }

        // Soft clipper / limiter para evitar distorção digital
        for (size_t i = 0; i < frames; ++i) {
            buffer[i] = std::clamp(buffer[i], -1.0f, 1.0f);
        }
    }

    size_t activeVoiceCount() const {
        size_t count = 0;
        for (const auto& v : voices_) {
            if (v.active) count++;
        }
        return count;
    }

private:
    struct Voice {
        enum class State { Attack, Decay, Sustain, Release, Off };
        bool active = false;
        bool markedForReleaseOnSustainUp = false;
        int midiNote = 0;
        double frequency = 440.0;
        double phase = 0.0;
        double sampleRate = 44100.0;
        double envelope = 0.0;
        float velocity = 0.8f;
        State state = State::Off;
    };

    double sampleRate_ = 44100.0;
    double naturalDecayTime_ = 2.0; // 2 segundos de sustentação natural antes do silêncio
    bool sustainActive_ = false;
    std::array<Voice, kMaxVoices> voices_;
};

} // namespace abntpiano
