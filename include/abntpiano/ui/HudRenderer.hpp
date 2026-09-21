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
                bool isTeacherMode,
                const std::string& comboPhrase,
                double phraseExpireTime,
                bool showShortcutsOverlay = false) const;

    void renderShortcutsOverlay(SDL_Renderer* ren, const FontCollection& fonts) const;

private:
    void renderSongInfo(SDL_Renderer* ren, const FontCollection& fonts, const Song& song,
                        int difficulty, bool isTeacherMode) const;
    void renderStats(SDL_Renderer* ren, const FontCollection& fonts, const ScoringEngine& scoring) const;
    void renderProgressBar(SDL_Renderer* ren, double playhead, double totalDuration) const;
};

} // namespace abntpiano::ui
