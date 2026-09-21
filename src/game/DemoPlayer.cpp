#include "abntpiano/DemoPlayer.hpp"
#include <algorithm>

namespace abntpiano {

void DemoPlayer::reset(double) {
    cursor_ = 0;
    lastHitTime_ = -1.0;
    scheduledReleases_.clear();
}

void DemoPlayer::update(double currentPlayhead,
                        SongModeController& songMode,
                        std::set<char>& heldKeys,
                        std::map<char, HoldState>& holdStates) {
    const auto& pe = songMode.chart().playableEvents;

    while (cursor_ < pe.size()) {
        const auto& g = pe[cursor_];
        if (g.onset > currentPlayhead) {
            break;
        }

        if (!songMode.isGroupJudged(cursor_)) {
            int seed = static_cast<int>((cursor_ * 37 + 13) % 100);
            JudgementType judgeType = JudgementType::Perfect;
            if (seed < 75) {
                judgeType = JudgementType::Perfect;
            } else if (seed < 95) {
                judgeType = JudgementType::Great;
            } else {
                judgeType = JudgementType::Good;
            }

            songMode.triggerDemoHit(cursor_, judgeType, g.onset);

            for (size_t k = 0; k < g.keys.size(); ++k) {
                char key = static_cast<char>(std::toupper(static_cast<unsigned char>(g.keys[k])));
                double dur = (k < g.durations.size()) ? g.durations[k] : 0.25;
                heldKeys.insert(key);
                holdStates[key] = HoldState::Holding;
                scheduledReleases_[key] = currentPlayhead + std::max(dur - 0.02, 0.08);
            }
        }
        cursor_++;
    }

    for (auto it = scheduledReleases_.begin(); it != scheduledReleases_.end(); ) {
        if (currentPlayhead >= it->second) {
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
