#pragma once

#include "abntpiano/SongModeController.hpp"
#include "abntpiano/ui/KeyboardRenderer.hpp"
#include "abntpiano/ui/RenderTypes.hpp"
#include <map>
#include <set>
#include <vector>

namespace abntpiano::ui {

class HighwayRenderer {
public:
    HighwayRenderer() = default;

    void render(SDL_Renderer* ren,
                const FontCollection& fonts,
                const KeyboardRenderer& keyboard,
                const std::vector<VisibleNote>& visNotes,
                const std::set<char>& keysAtHitLine,
                const std::map<char, KeyFeedback>& feedbacks,
                double lookahead,
                double playhead) const;

private:
    void renderPlayfieldBackground(SDL_Renderer* ren, const std::vector<VisibleNote>& visNotes) const;
    void renderLanes(SDL_Renderer* ren) const;
    void renderNotes(SDL_Renderer* ren, const FontCollection& fonts,
                     const std::vector<VisibleNote>& visNotes, double lookahead) const;
    void renderFeltRail(SDL_Renderer* ren, const std::set<char>& keysAtHitLine,
                        const std::map<char, KeyFeedback>& feedbacks, double playhead) const;
};

} // namespace abntpiano::ui
