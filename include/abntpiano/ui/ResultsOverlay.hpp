#pragma once

#include "abntpiano/ScoringEngine.hpp"
#include "abntpiano/ui/RenderTypes.hpp"

namespace abntpiano::ui {

class ResultsOverlay {
public:
    ResultsOverlay() = default;

    void render(SDL_Renderer* ren,
                const FontCollection& fonts,
                const ExecutionSummary& summary) const;
};

} // namespace abntpiano::ui
