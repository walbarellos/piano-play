#pragma once

#include "abntpiano/SongModeController.hpp"
#include "abntpiano/ui/KeyboardRenderer.hpp"
#include "abntpiano/ui/RenderTypes.hpp"
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
                double lookahead) const;

private:
    void renderLanes(SDL_Renderer* ren, const KeyboardRenderer& keyboard, const std::set<char>& keysAtHitLine) const;
    void renderNotes(SDL_Renderer* ren, const FontCollection& fonts, const KeyboardRenderer& keyboard,
                     const std::vector<VisibleNote>& visNotes, double lookahead) const;
    void renderHitLine(SDL_Renderer* ren, const KeyboardRenderer& keyboard, const std::set<char>& keysAtHitLine) const;
};

} // namespace abntpiano::ui
