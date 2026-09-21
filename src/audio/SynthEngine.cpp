#include "abntpiano/SynthEngine.hpp"

#include <algorithm>
#include <cmath>

namespace abntpiano {

SynthEngine::SynthEngine(double sampleRate)
    : sampleRate_(sampleRate) {
    for (auto& v : voices_) v.sampleRate = sampleRate_;
    scheduled_.reserve(1024);
}

void SynthEngine::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    for (auto& v : voices_) v.sampleRate = sampleRate_;
}

double SynthEngine::audioTime() const {
    return static_cast<double>(samplesRendered_.load(std::memory_order_relaxed)) / sampleRate_;
}

void SynthEngine::scheduleNoteOn(double atSeconds, int midiNote, float velocity) {
    pushEvent(atSeconds, midiNote, velocity, true);
}

void SynthEngine::scheduleNoteOff(double atSeconds, int midiNote) {
    pushEvent(atSeconds, midiNote, 0.0f, false);
}

void SynthEngine::clearSchedule() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    scheduled_.clear();
}

void SynthEngine::noteOn(int midiNote, float velocity) {
    std::lock_guard<std::mutex> lock(voiceMutex_);
    noteOnLocked(midiNote, velocity);
}

void SynthEngine::noteOff(int midiNote) {
    std::lock_guard<std::mutex> lock(voiceMutex_);
    noteOffLocked(midiNote);
}

void SynthEngine::setSustain(bool active) {
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

void SynthEngine::allNotesOff() {
    clearSchedule();
    std::lock_guard<std::mutex> lock(voiceMutex_);
    for (auto& v : voices_) {
        v.active = false;
        v.state = Voice::State::Off;
        v.envelope = 0.0;
        v.markedForReleaseOnSustainUp = false;
    }
}

void SynthEngine::releaseAllNotes() {
    clearSchedule();
    std::lock_guard<std::mutex> lock(voiceMutex_);
    for (auto& v : voices_) {
        if (v.active && v.state != Voice::State::Off) {
            v.markedForReleaseOnSustainUp = false;
            v.state = Voice::State::Release;
        }
    }
}

void SynthEngine::setNaturalDecayTime(double seconds) {
    naturalDecayTime_ = std::max(0.2, seconds);
}

void SynthEngine::render(float* buffer, size_t frames) {
    std::fill(buffer, buffer + frames, 0.0f);

    const uint64_t blockStart = samplesRendered_.load(std::memory_order_relaxed);

    pending_.clear();
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (!scheduled_.empty()) {
            const uint64_t blockEnd = blockStart + frames;
            size_t keep = 0;
            for (size_t i = 0; i < scheduled_.size(); ++i) {
                const auto& e = scheduled_[i];
                if (e.sample < blockEnd) {
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

size_t SynthEngine::activeVoiceCount() const {
    size_t count = 0;
    for (const auto& v : voices_) if (v.active) count++;
    return count;
}

size_t SynthEngine::pendingEventCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return scheduled_.size();
}

void SynthEngine::pushEvent(double atSeconds, int midiNote, float velocity, bool on) {
    uint64_t sample = static_cast<uint64_t>(std::max(0.0, atSeconds) * sampleRate_ + 0.5);
    std::lock_guard<std::mutex> lock(queueMutex_);
    scheduled_.push_back(ScheduledEvent{sample, midiNote, velocity, on});
}

void SynthEngine::noteOnLocked(int midiNote, float velocity) {
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
        double worst = 1e9;
        for (auto& v : voices_) {
            if (v.state == Voice::State::Attack || v.state == Voice::State::Decay) continue;
            double score = v.envelope * v.velocity;
            if (v.state == Voice::State::Release) score *= 0.25;
            if (score < worst) { worst = score; target = &v; }
        }
    }
    if (!target) {
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

void SynthEngine::noteOffLocked(int midiNote) {
    for (auto& v : voices_) {
        if (v.active && v.midiNote == midiNote && v.state != Voice::State::Release) {
            if (sustainActive_) v.markedForReleaseOnSustainUp = true;
            else                v.state = Voice::State::Release;
        }
    }
}

} // namespace abntpiano
