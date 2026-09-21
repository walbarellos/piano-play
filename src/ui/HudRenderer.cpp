#include "abntpiano/ui/HudRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace abntpiano::ui {

HudRenderer::HudRenderer() = default;

void HudRenderer::render(SDL_Renderer* ren,
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
                         double phraseExpireTime) const {
    // Fundo do HUD com gradiente
    SDL_Rect hudBg{0, 0, kScreenWidth, kHudHeight};
    renderGradientRect(ren, hudBg, {14, 18, 32, 255}, {10, 14, 26, 255});
    SDL_SetRenderDrawColor(ren, 50, 70, 130, 255);
    SDL_RenderDrawLine(ren, 0, kHudHeight, kScreenWidth, kHudHeight);
    SDL_SetRenderDrawColor(ren, 70, 100, 180, 100);
    SDL_RenderDrawLine(ren, 0, kHudHeight - 1, kScreenWidth, kHudHeight - 1);

    if (isFreePlay) {
        renderText(ren, fonts.medium, "♪  FREE PLAY — Toque Livre", 18, 6, {100, 200, 255, 255});
        renderText(ren, fonts.small,
            "[F2-F8] Músicas  [1/2/3] Dificuldade  [TAB] Próxima  [F1] Song Mode  [ESC] Sair",
            18, 34, {100, 120, 165, 255});
        return;
    }

    renderSongInfo(ren, fonts, currentSong, currentDifficulty, lookahead);
    renderStats(ren, fonts, scoring, playhead, isDemoMode);
    renderProgressBar(ren, playhead, totalSongDuration);

    // Combo phrase centralizada
    if (!comboPhrase.empty() && phraseExpireTime > playhead) {
        float t = static_cast<float>(std::min(1.0, phraseExpireTime - playhead));
        Uint8 a = static_cast<Uint8>(t * 255.0f);
        renderText(ren, fonts.small, "★ " + comboPhrase + " ★", kScreenWidth / 2, 8, {255, 215, 0, a}, true);
    }
}

void HudRenderer::renderSongInfo(SDL_Renderer* ren, const FontCollection& fonts, const Song& song,
                                 int difficulty, double lookahead) const {
    renderText(ren, fonts.medium, "♪  " + song.title, 18, 4, {255, 215, 0, 255});
    renderText(ren, fonts.tiny, song.composer, 18, 30, {160, 165, 200, 200});

    const char* diffName = (difficulty == 1) ? "EASY" : (difficulty == 2) ? "NORMAL" : "HARD";
    SDL_Color diffBadgeCol =
        (difficulty == 1) ? SDL_Color{30, 200, 100, 255} :
        (difficulty == 2) ? SDL_Color{70, 180, 255, 255} :
                            SDL_Color{255, 140, 40, 255};

    int badgeX = 430;
    SDL_Rect badgeBg{badgeX, 6, static_cast<int>(std::strlen(diffName) * 9 + 18), 22};
    SDL_SetRenderDrawColor(ren, diffBadgeCol.r / 5, diffBadgeCol.g / 5, diffBadgeCol.b / 5, 220);
    SDL_RenderFillRect(ren, &badgeBg);
    SDL_SetRenderDrawColor(ren, diffBadgeCol.r, diffBadgeCol.g, diffBadgeCol.b, 255);
    SDL_RenderDrawRect(ren, &badgeBg);
    renderText(ren, fonts.small, diffName, badgeX + 8, 10, diffBadgeCol);

    std::ostringstream nav;
    nav << "[F2-F8] Musica  [1/2/3] Dif  [TAB] Prox  [-/+] Queda:"
        << std::fixed << std::setprecision(1) << lookahead << "s  [Enter] Reiniciar  [F10] Guia  [F11] Acomp  [F12] DEMO";
    renderText(ren, fonts.tiny, nav.str(), 18, 48, {80, 95, 145, 200});
}

void HudRenderer::renderStats(SDL_Renderer* ren, const FontCollection& fonts, const ScoringEngine& scoring,
                              double playhead, bool isDemoMode) const {
    int64_t score = scoring.currentScore();
    int combo     = scoring.currentCombo();
    float acc     = scoring.currentAccuracy() * 100.0f;
    std::string grade = gradeToString(scoring.currentGrade());

    SDL_SetRenderDrawColor(ren, 15, 20, 40, 160);
    SDL_Rect statsBg{kScreenWidth - 470, 1, 468, kHudHeight - 2};
    SDL_RenderFillRect(ren, &statsBg);
    SDL_SetRenderDrawColor(ren, 40, 55, 95, 180);
    SDL_RenderDrawLine(ren, kScreenWidth - 470, 1, kScreenWidth - 470, kHudHeight - 2);

    const int COL_SCORE = kScreenWidth - 455;
    const int COL_COMBO = kScreenWidth - 330;
    const int COL_ACC   = kScreenWidth - 195;
    const int COL_GRADE = kScreenWidth - 60;

    // SCORE
    renderText(ren, fonts.tiny, "SCORE", COL_SCORE, 5, {110, 130, 185, 200});
    renderText(ren, fonts.large, std::to_string(score), COL_SCORE, 20, {240, 245, 255, 255});

    // COMBO
    SDL_Color comboCol = (combo >= 10) ? SDL_Color{255, 150, 30, 255} : SDL_Color{70, 200, 255, 255};
    renderText(ren, fonts.tiny, "COMBO", COL_COMBO, 5, {110, 130, 185, 200});
    renderText(ren, fonts.large, "x" + std::to_string(combo), COL_COMBO, 20, comboCol);

    // ACCURACY
    SDL_Color accCol = (acc >= 95.0f) ? SDL_Color{30, 255, 120, 255} :
                       (acc >= 70.0f) ? SDL_Color{220, 220, 60, 255} :
                                        SDL_Color{220, 100, 60, 255};
    renderText(ren, fonts.tiny, "PRECISAO", COL_ACC, 5, {110, 130, 185, 200});
    std::ostringstream accStream;
    accStream << std::fixed << std::setprecision(1) << acc << "%";
    renderText(ren, fonts.large, accStream.str(), COL_ACC, 20, accCol);

    // GRADE
    SDL_Color gradeCol =
        (grade == "S") ? SDL_Color{255, 200, 0, 255} :
        (grade == "A") ? SDL_Color{60, 220, 120, 255} :
        (grade == "B") ? SDL_Color{70, 180, 255, 255} :
        (grade == "C") ? SDL_Color{200, 180, 60, 255} :
                         SDL_Color{200, 80, 80, 255};
    renderText(ren, fonts.tiny, "GRADE", COL_GRADE, 5, {110, 130, 185, 200});
    renderText(ren, fonts.large, grade, COL_GRADE, 20, gradeCol);

    // DEMO indicator
    if (isDemoMode) {
        bool blinkOn = (std::fmod(playhead, 0.5) < 0.35);
        if (blinkOn) {
            SDL_Rect demoBg{kScreenWidth / 2 - 48, 6, 96, 28};
            SDL_SetRenderDrawColor(ren, 180, 30, 220, 220);
            SDL_RenderFillRect(ren, &demoBg);
            SDL_SetRenderDrawColor(ren, 255, 100, 255, 255);
            SDL_RenderDrawRect(ren, &demoBg);
            renderText(ren, fonts.small, "◉ DEMO", kScreenWidth / 2, 20, {255, 255, 255, 255}, true);
        }
    }
}

void HudRenderer::renderProgressBar(SDL_Renderer* ren, double playhead, double totalDuration) const {
    if (totalDuration <= 0.0) return;
    double progress = std::clamp(playhead / totalDuration, 0.0, 1.0);
    int barX = 18, barY = kHudHeight - 7, barW = kScreenWidth - 36, barH = 4;
    SDL_SetRenderDrawColor(ren, 30, 40, 65, 255);
    SDL_Rect bgBar{barX, barY, barW, barH};
    SDL_RenderFillRect(ren, &bgBar);
    SDL_SetRenderDrawColor(ren, 255, 215, 0, 200);
    SDL_Rect progBar{barX, barY, static_cast<int>(barW * progress), barH};
    SDL_RenderFillRect(ren, &progBar);
}

} // namespace abntpiano::ui
