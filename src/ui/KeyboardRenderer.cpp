#include "abntpiano/ui/KeyboardRenderer.hpp"
#include <algorithm>
#include <cctype>

namespace abntpiano::ui {

KeyboardRenderer::KeyboardRenderer() {
    rowOffsets_[0] = (kScreenWidth - 10 * (kKeyWidth + kKeyGap) + kKeyGap) / 2;
    rowOffsets_[1] = (kScreenWidth - 9 * (kKeyWidth + kKeyGap) + kKeyGap) / 2 + (kKeyWidth + kKeyGap) / 2;
    rowOffsets_[2] = (kScreenWidth - 7 * (kKeyWidth + kKeyGap) + kKeyGap) / 2 + (kKeyWidth + kKeyGap);
}

SDL_Rect KeyboardRenderer::getKeyRect(char key) const {
    char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
    for (const auto& vk : kVisualKeys) {
        if (vk.key == norm) {
            int x = rowOffsets_[vk.row] + vk.col * (kKeyWidth + kKeyGap);
            int y = kKeyboardY + vk.row * (kKeyHeight + kKeyGap);
            return {x, y, kKeyWidth, kKeyHeight};
        }
    }
    return {0, 0, 0, 0};
}

int KeyboardRenderer::getKeyCenterX(char key) const {
    SDL_Rect r = getKeyRect(key);
    return r.x + r.w / 2;
}

void KeyboardRenderer::render(SDL_Renderer* ren,
                              const FontCollection& fonts,
                              const KeyboardMapper& mapper,
                              const std::set<char>& heldKeys,
                              const std::set<char>& keysAtHitLine,
                              const std::map<char, KeyFeedback>& feedbacks,
                              const std::map<char, HoldState>& holdStates,
                              double currentPlayhead) const {
    // Fundo da zona do teclado
    {
        SDL_Rect kbBg{0, kKeyboardY - 8, kScreenWidth, kScreenHeight - (kKeyboardY - 8)};
        SDL_SetRenderDrawColor(ren, 8, 10, 18, 255);
        SDL_RenderFillRect(ren, &kbBg);
        SDL_SetRenderDrawColor(ren, 30, 40, 65, 255);
        SDL_RenderDrawLine(ren, 0, kKeyboardY - 8, kScreenWidth, kKeyboardY - 8);
    }

    for (const auto& vk : kVisualKeys) {
        SDL_Rect r = getKeyRect(vk.key);
        bool isPhysPressed = heldKeys.count(vk.key) > 0 || (holdStates.count(vk.key) > 0 && holdStates.at(vk.key) == HoldState::Holding);

        auto itFb = feedbacks.find(vk.key);
        bool hasFb = (itFb != feedbacks.end() && itFb->second.expireTime > currentPlayhead);

        auto itHold = holdStates.find(vk.key);
        HoldState hState = (itHold != holdStates.end()) ? itHold->second : HoldState::Idle;

        // Visual 3D key depression when physically held
        if (isPhysPressed) {
            r.y += 2;
            r.h -= 2;
        }

        Uint8 rr = 22, gg = 27, bb = 44;
        if (isPhysPressed) {
            if (hasFb) {
                switch (itFb->second.type) {
                    case JudgementType::Perfect: rr = 255; gg = 210; bb = 30;  break;
                    case JudgementType::Great:   rr = 45;  gg = 230; bb = 100; break;
                    case JudgementType::Good:    rr = 40;  gg = 170; bb = 255; break;
                    case JudgementType::Miss:    rr = 230; gg = 30;  bb = 55;  break;
                }
            } else {
                rr = 25; gg = 205; bb = 85;
            }
        } else if (hasFb) {
            // Subtle residual glow after key release
            switch (itFb->second.type) {
                case JudgementType::Perfect: rr = 70;  gg = 60;  bb = 20;  break;
                case JudgementType::Great:   rr = 20;  gg = 65;  bb = 30;  break;
                case JudgementType::Good:    rr = 20;  gg = 45;  bb = 75;  break;
                case JudgementType::Miss:    rr = 75;  gg = 20;  bb = 25;  break;
            }
        } else if (hState == HoldState::Missed) {
            rr = 90; gg = 20; bb = 30;
        }

        renderGradientRect(ren, r,
            {static_cast<Uint8>(std::min(255, rr + 20)),
             static_cast<Uint8>(std::min(255, gg + 20)),
             static_cast<Uint8>(std::min(255, bb + 20)), 255},
            {rr, gg, bb, 255});

        // 3D Bevel highlight
        SDL_SetRenderDrawColor(ren,
            std::min(255, rr + 50), std::min(255, gg + 50), std::min(255, bb + 60), 180);
        SDL_RenderDrawLine(ren, r.x + 2, r.y + 1, r.x + r.w - 3, r.y + 1);

        // Borda
        if (isPhysPressed) {
            if (hasFb && itFb->second.type == JudgementType::Perfect) {
                SDL_SetRenderDrawColor(ren, 255, 255, 200, 255);
            } else if (hasFb && itFb->second.type == JudgementType::Great) {
                SDL_SetRenderDrawColor(ren, 160, 255, 180, 255);
            } else if (hasFb && itFb->second.type == JudgementType::Good) {
                SDL_SetRenderDrawColor(ren, 120, 220, 255, 255);
            } else {
                SDL_SetRenderDrawColor(ren, 80, 255, 140, 255);
            }
        } else if (hasFb && itFb->second.type == JudgementType::Miss) {
            SDL_SetRenderDrawColor(ren, 255, 60, 80, 255);
        } else {
            SDL_SetRenderDrawColor(ren, 50, 62, 95, 255);
        }
        SDL_RenderDrawRect(ren, &r);

        SDL_Rect innerBorder{r.x + 1, r.y + 1, r.w - 2, r.h - 2};
        SDL_SetRenderDrawColor(ren, 30, 38, 60, 120);
        SDL_RenderDrawRect(ren, &innerBorder);

        // Letra
        SDL_Color keyLetterColor =
            isPhysPressed ? SDL_Color{255, 255, 255, 255} :
            hasFb         ? SDL_Color{240, 240, 240, 255} :
                            SDL_Color{200, 210, 230, 255};
        std::string kStr(1, vk.key);
        renderText(ren, fonts.medium, kStr, r.x + r.w / 2, r.y + 18, keyLetterColor, true);

        // Nota
        auto note = mapper.noteForKey(vk.key);
        if (note) {
            SDL_Color noteNameColor =
                (isPhysPressed || hState == HoldState::Holding) ? SDL_Color{160, 255, 190, 220} :
                SDL_Color{100, 120, 160, 200};
            renderText(ren, fonts.tiny, note->name, r.x + r.w / 2, r.y + 42, noteNameColor, true);
        }
    }
}

} // namespace abntpiano::ui
