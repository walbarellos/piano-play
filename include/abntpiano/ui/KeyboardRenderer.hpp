#pragma once

#include "abntpiano/KeyboardMapper.hpp"
#include "abntpiano/JudgementEngine.hpp"
#include "abntpiano/ui/RenderTypes.hpp"
#include <map>
#include <set>

namespace abntpiano::ui {

struct KeyFeedback {
    JudgementType type;
    double expireTime = 0.0;
};

class KeyboardRenderer {
public:
    KeyboardRenderer() = default;

    SDL_Rect getKeyRect(char key) const;
    int getKeyCenterX(char key) const;

    void render(SDL_Renderer* ren,
                const FontCollection& fonts,
                const KeyboardMapper& mapper,
                const std::set<char>& heldKeys,
                const std::set<char>& keysAtHitLine,
                const std::map<char, KeyFeedback>& feedbacks,
                const std::map<char, HoldState>& holdStates,
                double currentPlayhead) const;
};

} // namespace abntpiano::ui
