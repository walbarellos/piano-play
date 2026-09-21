#include "abntpiano/ui/HighwayRenderer.hpp"
#include <algorithm>
#include <cmath>

namespace abntpiano::ui {

static constexpr int kNoteHeadH = 28;

void HighwayRenderer::render(SDL_Renderer* ren,
                             const FontCollection& fonts,
                             const KeyboardRenderer& keyboard,
                             const std::vector<VisibleNote>& visNotes,
                             const std::set<char>& keysAtHitLine,
                             double lookahead) const {
    // Playfield background
    SDL_Rect bgZone{0, kHudHeight, kScreenWidth, kHitY - kHudHeight};
    renderGradientRect(ren, bgZone, {10, 14, 26, 255}, {6, 9, 18, 255});

    renderLanes(ren, keyboard, keysAtHitLine);
    renderNotes(ren, fonts, keyboard, visNotes, lookahead);
    renderHitLine(ren, keyboard, keysAtHitLine);
}

void HighwayRenderer::renderLanes(SDL_Renderer* ren, const KeyboardRenderer& keyboard, const std::set<char>& keysAtHitLine) const {
    for (const auto& vk : kVisualKeys) {
        int cx = keyboard.getKeyCenterX(vk.key);
        bool atHit = keysAtHitLine.count(vk.key) > 0;
        if (atHit) {
            SDL_SetRenderDrawColor(ren, 40, 90, 40, 60);
            SDL_Rect laneRect{cx - kKeyWidth / 2, kHorizonY, kKeyWidth, kHitY - kHorizonY};
            SDL_RenderFillRect(ren, &laneRect);
        }
        SDL_SetRenderDrawColor(ren, atHit ? 50 : 25, atHit ? 70 : 30, atHit ? 50 : 50, atHit ? 120 : 50);
        SDL_RenderDrawLine(ren, cx, kHorizonY, cx, kHitY);
    }
}

void HighwayRenderer::renderNotes(SDL_Renderer* ren, const FontCollection& fonts, const KeyboardRenderer& keyboard,
                                  const std::vector<VisibleNote>& visNotes, double lookahead) const {
    for (const auto& vn : visNotes) {
        double frac = vn.timeToHit / lookahead;
        int noteBottomY = kHorizonY + static_cast<int>((1.0 - frac) * (kHitY - kHorizonY));
        int noteY = noteBottomY - kNoteHeadH;
        bool isTouching = (std::abs(vn.timeToHit) <= 0.13);

        // Barra de conexão de acordes
        if (vn.keys.size() > 1 && !vn.isJudged) {
            int minX = kScreenWidth, maxX = 0;
            for (char k : vn.keys) {
                int cx = keyboard.getKeyCenterX(k);
                minX = std::min(minX, cx);
                maxX = std::max(maxX, cx);
            }
            SDL_SetRenderDrawColor(ren, 80, 200, 255, 120);
            for (int d = -1; d <= 1; d++) {
                SDL_RenderDrawLine(ren, minX, noteY + kNoteHeadH / 2 + d, maxX, noteY + kNoteHeadH / 2 + d);
            }
        }

        for (size_t k = 0; k < vn.keys.size(); k++) {
            char keyChar = vn.keys[k];
            double dur = (k < vn.durations.size()) ? vn.durations[k] : 0.3;
            SDL_Rect keyBox = keyboard.getKeyRect(keyChar);
            int nW = kKeyWidth - 6;
            int nX = keyBox.x + 3;
            int tailH = static_cast<int>((dur / lookahead) * (kHitY - kHorizonY));
            bool isLongNote = (dur >= 0.5);

            // Cauda (hold ribbon)
            if (tailH > 12) {
                int tailTopY = noteBottomY - tailH;
                int cTop = std::max(kHorizonY, tailTopY);
                int cBot = std::min(kHitY + 18, noteY + kNoteHeadH);

                if (cBot > cTop) {
                    int tw = nW - 14;
                    SDL_Rect tailR{nX + 7, cTop, tw, cBot - cTop};

                    if (isTouching) {
                        SDL_SetRenderDrawColor(ren, 0, 180, 80, 190);
                        SDL_RenderFillRect(ren, &tailR);
                        SDL_SetRenderDrawColor(ren, 0, 255, 100, 240);
                    } else {
                        renderGradientRect(ren, tailR, {50, 130, 230, 180}, {20, 80, 180, 160});
                        SDL_SetRenderDrawColor(ren, 100, 200, 255, 180);
                    }
                    SDL_RenderDrawLine(ren, tailR.x, tailR.y, tailR.x, tailR.y + tailR.h);
                    SDL_RenderDrawLine(ren, tailR.x + tailR.w - 1, tailR.y, tailR.x + tailR.w - 1, tailR.y + tailR.h);
                    if (tailTopY >= kHorizonY) {
                        SDL_RenderDrawLine(ren, tailR.x, tailR.y, tailR.x + tailR.w, tailR.y);
                        SDL_RenderDrawLine(ren, tailR.x, tailR.y + 1, tailR.x + tailR.w, tailR.y + 1);
                    }
                }
            }

            // Cabeça da nota
            if (!vn.isJudged && noteY + kNoteHeadH >= kHorizonY && noteY <= kHitY + 28) {
                SDL_Rect head{nX, noteY, nW, kNoteHeadH};

                if (isTouching) {
                    renderGradientRect(ren, head, {30, 255, 110, 255}, {0, 200, 80, 255});
                    SDL_SetRenderDrawColor(ren, 220, 255, 230, 255);
                    SDL_RenderDrawRect(ren, &head);
                    SDL_Rect halo{head.x - 2, head.y - 2, head.w + 4, head.h + 4};
                    SDL_SetRenderDrawColor(ren, 0, 255, 100, 80);
                    SDL_RenderDrawRect(ren, &halo);
                } else if (isLongNote) {
                    renderGradientRect(ren, head, {40, 150, 240, 255}, {20, 100, 200, 255});
                    SDL_SetRenderDrawColor(ren, 255, 200, 80, 255);
                    SDL_RenderDrawRect(ren, &head);
                } else {
                    renderGradientRect(ren, head, {50, 160, 255, 245}, {30, 120, 220, 245});
                    SDL_SetRenderDrawColor(ren, 120, 220, 255, 255);
                    SDL_RenderDrawRect(ren, &head);
                }

                std::string letter(1, keyChar);
                renderText(ren, fonts.small, letter, nX + nW / 2, noteY + kNoteHeadH / 2, {240, 248, 255, 255}, true);
            }
        }
    }
}

void HighwayRenderer::renderHitLine(SDL_Renderer* ren, const KeyboardRenderer& keyboard, const std::set<char>& keysAtHitLine) const {
    // Linha de horizonte
    SDL_SetRenderDrawColor(ren, 40, 70, 130, 200);
    SDL_RenderDrawLine(ren, 0, kHorizonY, kScreenWidth, kHorizonY);
    SDL_SetRenderDrawColor(ren, 60, 100, 160, 80);
    SDL_RenderDrawLine(ren, 0, kHorizonY + 1, kScreenWidth, kHorizonY + 1);

    // Linha de impacto dourada neon
    SDL_SetRenderDrawColor(ren, 255, 215, 0, 18);
    for (int d = 1; d <= 8; d++) {
        SDL_RenderDrawLine(ren, 0, kHitY + d, kScreenWidth, kHitY + d);
        SDL_RenderDrawLine(ren, 0, kHitY - d, kScreenWidth, kHitY - d);
    }
    SDL_SetRenderDrawColor(ren, 255, 215, 0, 255);
    SDL_RenderDrawLine(ren, 0, kHitY - 1, kScreenWidth, kHitY - 1);
    SDL_RenderDrawLine(ren, 0, kHitY,     kScreenWidth, kHitY);
    SDL_RenderDrawLine(ren, 0, kHitY + 1, kScreenWidth, kHitY + 1);
    SDL_SetRenderDrawColor(ren, 255, 248, 200, 180);
    SDL_RenderDrawLine(ren, 0, kHitY, kScreenWidth, kHitY);

    // Sensores nas teclas
    for (const auto& vk : kVisualKeys) {
        int cx = keyboard.getKeyCenterX(vk.key);
        bool onHit = keysAtHitLine.count(vk.key) > 0;
        if (onHit) {
            SDL_SetRenderDrawColor(ren, 0, 255, 100, 255);
            SDL_Rect s{cx - 14, kHitY - 8, 28, 17};
            SDL_RenderFillRect(ren, &s);
            SDL_SetRenderDrawColor(ren, 220, 255, 230, 255);
            SDL_RenderDrawRect(ren, &s);
            SDL_SetRenderDrawColor(ren, 0, 255, 100, 60);
            SDL_Rect sHalo{s.x - 3, s.y - 3, s.w + 6, s.h + 6};
            SDL_RenderDrawRect(ren, &sHalo);
        } else {
            SDL_SetRenderDrawColor(ren, 200, 165, 30, 120);
            SDL_Rect s{cx - 9, kHitY - 3, 18, 7};
            SDL_RenderFillRect(ren, &s);
        }
    }
}

} // namespace abntpiano::ui
