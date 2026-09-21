#include "abntpiano/ui/HudRenderer.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace abntpiano::ui {

HudRenderer::HudRenderer() = default;

void HudRenderer::render(SDL_Renderer* ren,
                         const FontCollection& fonts,
                         const Song& currentSong,
                         int currentDifficulty,
                         double /*lookahead*/,
                         const ScoringEngine& scoring,
                         double playhead,
                         double totalSongDuration,
                         bool isFreePlay,
                         bool isTeacherMode,
                         const std::string& comboPhrase,
                         double phraseExpireTime,
                         bool showShortcutsOverlay) const {
    // 1. Fundo do HUD com acabamento em ébano e friso de latão
    SDL_Rect hudBg{0, 0, kScreenWidth, kHudHeight};
    SDL_SetRenderDrawColor(ren, 10, 7, 15, 255); // #0A070F
    SDL_RenderFillRect(ren, &hudBg);

    SDL_SetRenderDrawColor(ren, 199, 154, 74, 56); // rgba(199,154,74,.22)
    SDL_RenderDrawLine(ren, 0, kHudHeight - 1, kScreenWidth, kHudHeight - 1);

    if (isFreePlay) {
        renderText(ren, fonts.medium, "FREE PLAY - Toque Livre", 26, 20, {239, 230, 214, 255});
        renderText(ren, fonts.small, "Pressione [?] para atalhos", 26, 48, {110, 104, 128, 255});
        if (showShortcutsOverlay) renderShortcutsOverlay(ren, fonts);
        return;
    }

    renderSongInfo(ren, fonts, currentSong, currentDifficulty, isTeacherMode);
    renderStats(ren, fonts, scoring);
    renderProgressBar(ren, playhead, totalSongDuration);

    // Frase de combo temporária
    if (!comboPhrase.empty() && phraseExpireTime > playhead) {
        float t = static_cast<float>(std::min(1.0, phraseExpireTime - playhead));
        Uint8 a = static_cast<Uint8>(t * 255.0f);
        renderText(ren, fonts.small, "* " + comboPhrase + " *", kScreenWidth / 2, 16, {199, 154, 74, a}, true);
    }

    if (showShortcutsOverlay) {
        renderShortcutsOverlay(ren, fonts);
    }
}

void HudRenderer::renderSongInfo(SDL_Renderer* ren, const FontCollection& fonts, const Song& song,
                                 int difficulty, bool isTeacherMode) const {
    // Título da música
    renderText(ren, fonts.medium, song.title, 26, 14, {239, 230, 214, 255}); // #EFE6D6

    // Compositor / Artista
    std::string comp = song.composer.empty() ? "Tradicional" : song.composer;
    renderText(ren, fonts.small, comp, 26, 42, {110, 104, 128, 255}); // #6E6880

    // Pílula de dificuldade colocada imediatamente após o nome do compositor
    int compW = static_cast<int>(comp.size() * 8 + 8);
    int pillX = 26 + compW + 12;
    int pillY = 40;
    int pillW = 54;
    int pillH = 19;

    const char* diffLabel = (difficulty == 1) ? "FÁCIL" : (difficulty == 2) ? "MÉDIO" : "DIFÍCIL";
    renderCapsuleOutline(ren, static_cast<float>(pillX), static_cast<float>(pillY),
                         static_cast<float>(pillW), static_cast<float>(pillH),
                         {199, 154, 74, 128}); // rgba(199,154,74,.5)

    renderText(ren, fonts.tiny, diffLabel, pillX + pillW / 2, pillY + pillH / 2,
               {199, 154, 74, 255}, true); // #C79A4A

    // Indicador PROFESSOR (TEACHER)
    if (isTeacherMode) {
        int badgeX = pillX + pillW + 12;
        int badgeW = 135;
        int badgeH = 20;
        SDL_Color badgeCol{199, 154, 74, 255}; // Dourado
        renderCapsule(ren, static_cast<float>(badgeX), static_cast<float>(pillY),
                      static_cast<float>(badgeW), static_cast<float>(badgeH),
                      {85, 24, 110, 220}, {55, 14, 75, 220});
        renderCapsuleOutline(ren, static_cast<float>(badgeX), static_cast<float>(pillY),
                             static_cast<float>(badgeW), static_cast<float>(badgeH),
                             badgeCol);
        renderText(ren, fonts.tiny, "PROFESSOR [F12]", badgeX + badgeW / 2, pillY + badgeH / 2,
                   {255, 250, 240, 255}, true);
    }

    // Indicador discreto de atalhos
    renderText(ren, fonts.tiny, "?  atalhos", 26, kHudHeight - 16, {74, 68, 88, 255}); // #4A4458
}

void HudRenderer::renderStats(SDL_Renderer* ren, const FontCollection& fonts, const ScoringEngine& scoring) const {
    int judged = scoring.perfectCount() + scoring.greatCount() + scoring.goodCount() + scoring.missCount();

    std::string strScore = std::to_string(scoring.currentScore());
    std::string strCombo = (judged > 0 && scoring.currentCombo() > 0) ? ("×" + std::to_string(scoring.currentCombo())) : "—";

    std::ostringstream accStream;
    if (judged > 0) {
        accStream << std::fixed << std::setprecision(1) << (scoring.currentAccuracy() * 100.0f) << "%";
    } else {
        accStream << "—";
    }
    std::string strAcc = accStream.str();
    std::string strGrade = (judged > 0) ? gradeToString(scoring.currentGrade()) : "—";

    struct StatItem {
        std::string label;
        std::string value;
    };

    std::vector<StatItem> stats = {
        {"Pontos",   strScore},
        {"Combo",    strCombo},
        {"Precisão", strAcc},
        {"Nota",     strGrade}
    };

    int curX = kScreenWidth - 28;
    for (int i = static_cast<int>(stats.size()) - 1; i >= 0; i--) {
        const auto& item = stats[i];
        int valW = 0, lblW = 0;
        if (fonts.large) TTF_SizeUTF8(fonts.large, item.value.c_str(), &valW, nullptr);
        if (fonts.tiny)  TTF_SizeUTF8(fonts.tiny,  item.label.c_str(), &lblW, nullptr);
        int colW = std::max(valW, lblW);

        // Valor em grande destaque (#EFE6D6)
        renderTextRight(ren, fonts.large, item.value, curX, 14, {239, 230, 214, 255});

        // Label secundário (#6E6880)
        renderTextRight(ren, fonts.tiny, item.label, curX, 48, {110, 104, 128, 255});

        curX -= (colW + 44);
    }
}

void HudRenderer::renderProgressBar(SDL_Renderer* ren, double playhead, double totalDuration) const {
    if (totalDuration <= 0.0) return;
    float prog = static_cast<float>(std::clamp(playhead / totalDuration, 0.0, 1.0));

    // Base da barra de progresso (4px de altura)
    int barY = kHudHeight - 4;
    int barH = 4;

    // Trilho de fundo
    SDL_Rect track{0, barY, kScreenWidth, barH};
    SDL_SetRenderDrawColor(ren, 239, 230, 214, 18); // rgba(239,230,214,.07)
    SDL_RenderFillRect(ren, &track);

    // Barra de feltro vermelho
    int fillW = static_cast<int>(static_cast<float>(kScreenWidth) * prog);
    if (fillW > 0) {
        SDL_Rect barFill{0, barY, fillW, barH};
        SDL_SetRenderDrawColor(ren, 179, 46, 66, 255); // #B32E42
        SDL_RenderFillRect(ren, &barFill);
    }
}

void HudRenderer::renderShortcutsOverlay(SDL_Renderer* ren, const FontCollection& fonts) const {
    // Backdrop escuro semitransparente
    SDL_Rect backdrop{0, 0, kScreenWidth, kScreenHeight};
    SDL_SetRenderDrawColor(ren, 7, 5, 11, 230);
    SDL_RenderFillRect(ren, &backdrop);

    // Cartão central
    int cardW = 540;
    int cardH = 390;
    int cardX = (kScreenWidth - cardW) / 2;
    int cardY = (kScreenHeight - cardH) / 2;

    SDL_Color cardBg{18, 14, 26, 255};
    renderRoundRectSelective(ren, static_cast<float>(cardX), static_cast<float>(cardY),
                             static_cast<float>(cardW), static_cast<float>(cardH),
                             8.0f, 8.0f, cardBg, cardBg);

    // Moldura dourada
    SDL_SetRenderDrawColor(ren, 199, 154, 74, 180);
    SDL_Rect cardRect{cardX, cardY, cardW, cardH};
    SDL_RenderDrawRect(ren, &cardRect);

    // Título do modal
    renderText(ren, fonts.medium, "ATALHOS DO TECLADO", cardX + cardW / 2, cardY + 22, {239, 230, 214, 255}, true);

    const std::pair<std::string, std::string> shortcuts[] = {
        {"F2 – F6",       "Escolher obra completa do repertório"},
        {"TAB",           "Alternar para a próxima música"},
        {"1 / 2 / 3",     "Dificuldade: Fácil / Normal / Difícil"},
        {"T / F12",       "Modo Professor (ON: assiste / OFF: toca)"},
        {"- / +",         "Ajustar velocidade de queda das notas"},
        {"[ / ]",         "Ajustar andamento de reprodução"},
        {"F10 / F11",     "Alternar Guia de Melodia / Acompanhamento"},
        {"Enter",         "Reiniciar peça do início"},
        {"F1",            "Modo Free Play (tocar livremente)"},
        {"ESC",           "Voltar ao Menu Principal"}
    };

    int startY = cardY + 68;
    for (const auto& item : shortcuts) {
        renderText(ren, fonts.small, item.first, cardX + 36, startY, {199, 154, 74, 255}, false);
        renderText(ren, fonts.small, item.second, cardX + 175, startY, {220, 214, 230, 255}, false);
        startY += 26;
    }

    renderText(ren, fonts.tiny, "Pressione [?] ou [ESC] para fechar", cardX + cardW / 2, cardY + cardH - 24, {110, 104, 128, 255}, true);
}

} // namespace abntpiano::ui
