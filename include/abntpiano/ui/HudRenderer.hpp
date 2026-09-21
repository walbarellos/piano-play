#pragma once

#include "abntpiano/ScoringEngine.hpp"
#include "abntpiano/Song.hpp"
#include "abntpiano/ui/RenderTypes.hpp"
#include <string>

namespace abntpiano::ui {

class HudRenderer {
public:
    HudRenderer();

    void render(SDL_Renderer* ren,
                const FontCollection& fonts,
                const Song& currentSong,
                int currentDifficulty,
                double lookahead,
                const ScoringEngine& scoring,
                double playhead,
                double totalSongDuration,
                bool isFreePlay,
                bool isDemoMode,
                const std::string& comboPhrase,
                double phraseExpireTime) const;

private:
    void renderSongInfo(SDL_Renderer* ren, const FontCollection& fonts, const Song& song,
                        int difficulty, double lookahead) const;
    void renderStats(SDL_Renderer* ren, const FontCollection& fonts, const ScoringEngine& scoring,
                     double playhead, bool isDemoMode) const;
    void renderProgressBar(SDL_Renderer* ren, double playhead, double totalDuration) const;
};

} // namespace abntpiano::ui
