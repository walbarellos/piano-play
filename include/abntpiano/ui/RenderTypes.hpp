#pragma once

#include "abntpiano/HoldState.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>

namespace abntpiano::ui {

// Dimensões do jogo
constexpr int kScreenWidth  = 1280;
constexpr int kScreenHeight = 800;

// Layout vertical
constexpr int kHudHeight    = 80;
constexpr int kHorizonY     = kHudHeight + 2;
constexpr int kHitY         = 595;
constexpr int kKeyboardY    = kHitY + 12;

// Dimensões das teclas visuais
constexpr int kKeyWidth     = 66;
constexpr int kKeyHeight    = 62;
constexpr int kKeyGap       = 5;

// Layout do teclado visual ABNT (3 fileiras alinhadas)
struct KeyVisualPos {
    char key;
    int row;
    int col;
};

extern const std::vector<KeyVisualPos> kVisualKeys;

using abntpiano::HoldState;

struct FontCollection {
    TTF_Font* tiny   = nullptr;
    TTF_Font* small  = nullptr;
    TTF_Font* medium = nullptr;
    TTF_Font* large  = nullptr;
    TTF_Font* huge   = nullptr;

    bool load(const std::string& fontPath);
    void release();
};

// Funções utilitárias de renderização SDL
void renderText(SDL_Renderer* r, TTF_Font* font, const std::string& text,
                int x, int y, SDL_Color color, bool center = false);

void renderRoundRect(SDL_Renderer* r, SDL_Rect rect, int radius,
                     Uint8 R, Uint8 G, Uint8 B, Uint8 A);

void renderGradientRect(SDL_Renderer* r, SDL_Rect rect,
                        SDL_Color top, SDL_Color bottom);

} // namespace abntpiano::ui
