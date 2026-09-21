#pragma once

#include "abntpiano/HoldState.hpp"
#include "abntpiano/SongModeController.hpp"
#include <map>
#include <set>

namespace abntpiano {

class DemoPlayer {
public:
    DemoPlayer() = default;

    void reset(double playhead = 0.0);

    // Executa a lógica de auto-player até targetPlayheadTime.
    // Avança o playhead de songMode com hits humanizados.
    void update(double targetPlayheadTime,
                SongModeController& songMode,
                std::set<char>& heldKeys,
                std::map<char, HoldState>& holdStates);

    size_t cursor() const { return cursor_; }

private:
    size_t cursor_ = 0;
    double lastHitTime_ = -1.0;
    std::map<char, double> scheduledReleases_;
};

} // namespace abntpiano
