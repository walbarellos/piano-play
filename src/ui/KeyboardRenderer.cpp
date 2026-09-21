#include "abntpiano/ui/KeyboardRenderer.hpp"
#include <algorithm>
#include <cctype>

namespace abntpiano::ui {

static constexpr float kKbHeight = static_cast<float>(kScreenHeight - kKeyboardY - 18);

SDL_Rect KeyboardRenderer::getKeyRect(char key) const {
    const auto* k = getPianoKey(key);
    if (!k) return {0, 0, 0, 0};

    float h = k->isSharp ? (kKbHeight * 0.62f) : kKbHeight;
    return {
        static_cast<int>(k->x - k->w / 2.0f),
        kKeyboardY,
        static_cast<int>(k->w),
        static_cast<int>(h)
    };
}

int KeyboardRenderer::getKeyCenterX(char key) const {
    const auto* k = getPianoKey(key);
    if (!k) return kScreenWidth / 2;
    return static_cast<int>(k->x);
}

void KeyboardRenderer::render(SDL_Renderer* ren,
                              const FontCollection& fonts,
                              const KeyboardMapper& /*mapper*/,
                              const std::set<char>& heldKeys,
                              const std::set<char>& /*keysAtHitLine*/,
                              const std::map<char, KeyFeedback>& feedbacks,
                              const std::map<char, HoldState>& holdStates,
                              double currentPlayhead) const {
    // 1. Moldura / Fundo do piano
    SDL_Rect kbBg{0, kKeyboardY - 6, kScreenWidth, kScreenHeight - (kKeyboardY - 6)};
    SDL_SetRenderDrawColor(ren, 10, 7, 15, 255); // #0A070F
    SDL_RenderFillRect(ren, &kbBg);

    // Friso de latão polido no topo do teclado
    SDL_SetRenderDrawColor(ren, 199, 154, 74, 76); // rgba(199,154,74,.30)
    SDL_RenderDrawLine(ren, 0, kKeyboardY - 6, kScreenWidth, kKeyboardY - 6);
    SDL_RenderDrawLine(ren, 0, kKeyboardY - 5, kScreenWidth, kKeyboardY - 5);

    const auto& keys = getPianoLayout();

    // 2. Renderizar em duas passagens: Naturais primeiro, Sustenidas por cima
    for (bool sharpPass : {false, true}) {
        for (const auto& k : keys) {
            if (k.isSharp != sharpPass) continue;

            bool isPhysPressed = heldKeys.count(k.ch) > 0;
            auto itHold = holdStates.find(k.ch);
            if (itHold != holdStates.end() && itHold->second == HoldState::Holding) {
                isPhysPressed = true;
            }

            auto itFb = feedbacks.find(k.ch);
            bool hasFb = (itFb != feedbacks.end() && itFb->second.expireTime > currentPlayhead);
            bool lit = isPhysPressed || hasFb;

            float hue = pitchHue(k.pc);

            // Squash físico: tecla acesa afunda 3px
            float press = lit ? 3.0f : 0.0f;
            float ky = static_cast<float>(kKeyboardY) + press;
            float h = (k.isSharp ? (kKbHeight * 0.62f) : kKbHeight) - press;
            float w = k.w;
            float x = k.x - w / 2.0f;

            float rTop = k.isSharp ? 3.0f : 4.0f;
            float rBot = k.isSharp ? (w * 0.42f) : (w * 0.22f);

            if (k.isSharp) {
                SDL_Color fill = lit ? hslToRgb(hue, 55, 52) : SDL_Color{21, 18, 28, 255}; // #15121C
                renderRoundRectSelective(ren, x, ky, w, h, rTop, rBot, fill, fill);
            } else {
                if (lit) {
                    SDL_Color fill = hslToRgb(hue, 58, 84);
                    renderRoundRectSelective(ren, x, ky, w, h, rTop, rBot, fill, fill);
                } else {
                    // Gradiente sutil das teclas de marfim
                    SDL_Color topCol{62, 55, 73, 255};
                    SDL_Color botCol{42, 37, 50, 255}; // #3A3444
                    renderRoundRectSelective(ren, x, ky, w, h, rTop, rBot, topCol, botCol);
                }
            }

            // Glow / Brilho interno ao tocar
            if (lit) {
                renderGlowDisc(ren, k.x, ky + h * 0.5f, std::max(w * 0.7f, 24.0f),
                               hslToRgb(hue, 80, 70, 70), hslToRgb(hue, 80, 70, 0));
            }

            // Tipografia
            if (k.isSharp) {
                SDL_Color letterCol = lit ? SDL_Color{10, 7, 15, 255} : SDL_Color{110, 127, 151, 255}; // #6E7F97
                std::string chStr(1, k.ch);
                renderText(ren, fonts.small, chStr, static_cast<int>(k.x), static_cast<int>(ky + h - 16), letterCol, true);
            } else {
                SDL_Color letterCol = lit ? hslToRgb(hue, 60, 14) : SDL_Color{185, 178, 196, 255}; // #B9B2C4
                std::string chStr(1, k.ch);
                renderText(ren, fonts.medium, chStr, static_cast<int>(k.x), static_cast<int>(ky + h - 26), letterCol, true);

                SDL_Color nameCol = lit ? hslToRgb(hue, 40, 22, 180) : SDL_Color{140, 132, 155, 160};
                renderText(ren, fonts.tiny, k.name, static_cast<int>(k.x), static_cast<int>(ky + h - 10), nameCol, true);
            }
        }
    }
}

} // namespace abntpiano::ui
