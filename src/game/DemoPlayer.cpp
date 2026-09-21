#include "abntpiano/DemoPlayer.hpp"
#include <algorithm>

namespace abntpiano {

void DemoPlayer::reset(double) {
    cursor_ = 0;
    lastHitTime_ = -1.0;
    scheduledReleases_.clear();
}

void DemoPlayer::update(double targetPlayheadTime,
                        SongModeController& songMode,
                        std::set<char>& heldKeys,
                        std::map<char, HoldState>& holdStates) {
    const auto& pe = songMode.chart().playableEvents;
    const auto& hw = songMode.chart().difficulty.hitWindow;

    while (cursor_ < pe.size()) {
        const auto& g = pe[cursor_];
        double nextGap = (cursor_ + 1 < pe.size()) ? (pe[cursor_ + 1].onset - g.onset) : 1.0;
        if (nextGap < 0.001) nextGap = 0.001;

        int seed = static_cast<int>((cursor_ * 37 + 13) % 100);
        double humanOffset = 0.0;
        if (seed < 72) {
            double frac = ((seed % 19) - 9) / 9.0;
            humanOffset = frac * (hw.perfect * 0.40) / 1000.0;
        } else if (seed < 92) {
            double sign = (seed % 2 == 0) ? 1.0 : -1.0;
            double span = hw.great - hw.perfect;
            humanOffset = sign * (hw.perfect + 0.15 * span + ((seed % 7) / 7.0) * (0.50 * span)) / 1000.0;
        } else {
            double sign = (seed % 2 == 0) ? 1.0 : -1.0;
            double span = hw.good - hw.great;
            humanOffset = sign * (hw.great + 0.15 * span + ((seed % 7) / 7.0) * (0.45 * span)) / 1000.0;
        }

        if (nextGap < 0.25) {
            double cap = nextGap * 0.35;
            humanOffset = std::clamp(humanOffset, -cap, cap);
        }

        double hitTime = g.onset + humanOffset;
        if (hitTime < lastHitTime_ + 0.010) hitTime = lastHitTime_ + 0.010;
        double maxSafe = g.onset + (hw.good - 25.0) / 1000.0;
        if (hitTime > maxSafe) hitTime = maxSafe;

        if (hitTime > targetPlayheadTime) {
            break;
        }

        double step = hitTime - songMode.playhead();
        if (step > 0.0) {
            songMode.update(step);
        }

        if (!songMode.isGroupJudged(cursor_)) {
            JudgementType judgeType = JudgementType::Perfect;
            double absDeltaMs = std::abs(hitTime - g.onset) * 1000.0;
            if (absDeltaMs <= hw.perfect) {
                judgeType = JudgementType::Perfect;
            } else if (absDeltaMs <= hw.great) {
                judgeType = JudgementType::Great;
            } else {
                judgeType = JudgementType::Good;
            }

            songMode.triggerDemoHit(cursor_, judgeType, hitTime);

            for (size_t k = 0; k < g.keys.size(); ++k) {
                char key = static_cast<char>(std::toupper(static_cast<unsigned char>(g.keys[k])));
                double dur = (k < g.durations.size()) ? g.durations[k] : 0.3;
                heldKeys.insert(key);
                holdStates[key] = HoldState::Holding;
                scheduledReleases_[key] = songMode.playhead() + std::max(dur - 0.03, 0.06);
            }
        }
        lastHitTime_ = songMode.playhead();
        cursor_++;
    }

    double rem = targetPlayheadTime - songMode.playhead();
    if (rem > 0.0) {
        songMode.update(rem);
    }

    double phNow = songMode.playhead();
    for (auto it = scheduledReleases_.begin(); it != scheduledReleases_.end(); ) {
        if (phNow >= it->second) {
            char key = it->first;
            heldKeys.erase(key);
            auto hsIt = holdStates.find(key);
            if (hsIt != holdStates.end() && hsIt->second == HoldState::Holding) {
                hsIt->second = HoldState::Idle;
            }
            it = scheduledReleases_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace abntpiano
