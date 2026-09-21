#include "abntpiano/ui/ResultsOverlay.hpp"
#include <iomanip>
#include <sstream>

namespace abntpiano::ui {

void ResultsOverlay::render(SDL_Renderer* ren,
                            const FontCollection& fonts,
                            const ExecutionSummary& summary) const {
    SDL_Rect overlay{kScreenWidth / 2 - 370, kScreenHeight / 2 - 210, 740, 420};
    SDL_SetRenderDrawColor(ren, 5, 8, 20, 230);
    SDL_RenderFillRect(ren, &overlay);

    SDL_Color borderCol =
        summary.grade == Grade::S ? SDL_Color{255, 200, 0, 255} :
        summary.grade == Grade::A ? SDL_Color{60, 220, 120, 255} :
        summary.grade == Grade::B ? SDL_Color{70, 180, 255, 255} :
        summary.grade == Grade::C ? SDL_Color{200, 180, 60, 255} :
                                    SDL_Color{200, 80, 80, 255};

    SDL_SetRenderDrawColor(ren, borderCol.r, borderCol.g, borderCol.b, 255);
    SDL_RenderDrawRect(ren, &overlay);
    SDL_Rect ov2{overlay.x + 1, overlay.y + 1, overlay.w - 2, overlay.h - 2};
    SDL_SetRenderDrawColor(ren, borderCol.r / 4, borderCol.g / 4, borderCol.b / 4, 180);
    SDL_RenderDrawRect(ren, &ov2);

    int cx = overlay.x + overlay.w / 2;

    renderText(ren, fonts.large, "MUSICA CONCLUIDA!", cx, overlay.y + 28, {255, 215, 0, 255}, true);

    std::string grStr = gradeToString(summary.grade);
    renderText(ren, fonts.huge, grStr, cx, overlay.y + 75, borderCol, true);

    SDL_SetRenderDrawColor(ren, 50, 65, 115, 220);
    SDL_RenderDrawLine(ren, overlay.x + 40, overlay.y + 160, overlay.x + overlay.w - 40, overlay.y + 160);
    SDL_SetRenderDrawColor(ren, 35, 48, 85, 150);
    SDL_RenderDrawLine(ren, overlay.x + 40, overlay.y + 161, overlay.x + overlay.w - 40, overlay.y + 161);

    {
        std::ostringstream ss;
        ss << "Pontuacao: " << summary.totalScore
           << "   Combo Max: " << summary.maxCombo
           << "   Precisao: " << std::fixed << std::setprecision(1)
           << (summary.accuracy * 100.0f) << "%";
        renderText(ren, fonts.small, ss.str(), cx, overlay.y + 178, {210, 218, 245, 255}, true);
    }

    struct StatCol {
        int cx;
        const char* label;
        int count;
        SDL_Color color;
    };
    int colY_label = overlay.y + 212;
    int colY_num   = overlay.y + 234;
    int colSpacing = overlay.w / 4;
    StatCol statCols[4] = {
        { overlay.x + colSpacing / 2,         "PERFECT", summary.perfectCount, {255, 215, 0, 255}   },
        { overlay.x + colSpacing + colSpacing / 2,   "GREAT",   summary.greatCount,   {60, 255, 100, 255}  },
        { overlay.x + 2 * colSpacing + colSpacing / 2, "GOOD",  summary.goodCount,    {70, 180, 255, 255}  },
        { overlay.x + 3 * colSpacing + colSpacing / 2, "MISS",  summary.missCount,    {255, 80, 80, 255}   },
    };

    for (const auto& col : statCols) {
        SDL_SetRenderDrawColor(ren, col.color.r / 8, col.color.g / 8, col.color.b / 8, 150);
        SDL_Rect colBg{col.cx - colSpacing / 2 + 8, colY_label - 6, colSpacing - 16, 105};
        SDL_RenderFillRect(ren, &colBg);
        SDL_SetRenderDrawColor(ren, col.color.r / 3, col.color.g / 3, col.color.b / 3, 160);
        SDL_RenderDrawRect(ren, &colBg);

        renderText(ren, fonts.small, col.label, col.cx, colY_label, col.color, true);
        renderText(ren, fonts.huge, std::to_string(col.count), col.cx, colY_num + 12, col.color, true);
    }

    SDL_SetRenderDrawColor(ren, 50, 65, 115, 180);
    SDL_RenderDrawLine(ren, overlay.x + 40, overlay.y + 355, overlay.x + overlay.w - 40, overlay.y + 355);

    renderText(ren, fonts.small,
        "[Enter] Reiniciar  |  [TAB] Proxima Musica  |  [1/2/3] Dificuldade",
        cx, overlay.y + 370, {100, 140, 210, 210}, true);
}

} // namespace abntpiano::ui
