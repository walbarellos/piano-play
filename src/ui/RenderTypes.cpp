#include "abntpiano/ui/RenderTypes.hpp"
#include <iostream>

namespace abntpiano::ui {

const std::vector<KeyVisualPos> kVisualKeys = {
    {'Q',0,0},{'W',0,1},{'E',0,2},{'R',0,3},{'T',0,4},
    {'Y',0,5},{'U',0,6},{'I',0,7},{'O',0,8},{'P',0,9},
    {'A',1,0},{'S',1,1},{'D',1,2},{'F',1,3},{'G',1,4},
    {'H',1,5},{'J',1,6},{'K',1,7},{'L',1,8},
    {'Z',2,0},{'X',2,1},{'C',2,2},{'V',2,3},{'B',2,4},{'N',2,5},{'M',2,6}
};

bool FontCollection::load(const std::string& fontPath) {
    release();
    tiny   = TTF_OpenFont(fontPath.c_str(), 11);
    small  = TTF_OpenFont(fontPath.c_str(), 14);
    medium = TTF_OpenFont(fontPath.c_str(), 20);
    large  = TTF_OpenFont(fontPath.c_str(), 36);
    huge   = TTF_OpenFont(fontPath.c_str(), 56);

    if (!small) {
        std::cerr << "[FontCollection] Falha ao carregar fonte em: " << fontPath << "\n";
        return false;
    }
    return true;
}

void FontCollection::release() {
    if (tiny)   { TTF_CloseFont(tiny);   tiny   = nullptr; }
    if (small)  { TTF_CloseFont(small);  small  = nullptr; }
    if (medium) { TTF_CloseFont(medium); medium = nullptr; }
    if (large)  { TTF_CloseFont(large);  large  = nullptr; }
    if (huge)   { TTF_CloseFont(huge);   huge   = nullptr; }
}

void renderText(SDL_Renderer* r, TTF_Font* font, const std::string& text,
                int x, int y, SDL_Color color, bool center) {
    if (!font || text.empty()) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    if (tex) {
        int w = surf->w, h = surf->h;
        SDL_Rect dst{center ? (x - w / 2) : x, center ? (y - h / 2) : y, w, h};
        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

void renderRoundRect(SDL_Renderer* r, SDL_Rect rect, int radius,
                     Uint8 R, Uint8 G, Uint8 B, Uint8 A) {
    SDL_SetRenderDrawColor(r, R, G, B, A);
    SDL_Rect body{rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
    SDL_RenderFillRect(r, &body);
    SDL_Rect bodyV{rect.x, rect.y + radius, rect.w, rect.h - 2 * radius};
    SDL_RenderFillRect(r, &bodyV);

    int cx[4] = {rect.x + radius, rect.x + rect.w - 1 - radius,
                 rect.x + radius, rect.x + rect.w - 1 - radius};
    int cy[4] = {rect.y + radius, rect.y + radius,
                 rect.y + rect.h - 1 - radius, rect.y + rect.h - 1 - radius};
    for (int i = 0; i < 4; i++) {
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx * dx + dy * dy <= radius * radius) {
                    SDL_RenderDrawPoint(r, cx[i] + dx, cy[i] + dy);
                }
            }
        }
    }
}

void renderGradientRect(SDL_Renderer* r, SDL_Rect rect,
                        SDL_Color top, SDL_Color bottom) {
    if (rect.h <= 0) return;
    for (int y = 0; y < rect.h; y++) {
        float t = static_cast<float>(y) / static_cast<float>(std::max(1, rect.h - 1));
        Uint8 rr = static_cast<Uint8>(top.r + t * (bottom.r - top.r));
        Uint8 gg = static_cast<Uint8>(top.g + t * (bottom.g - top.g));
        Uint8 bb = static_cast<Uint8>(top.b + t * (bottom.b - top.b));
        Uint8 aa = static_cast<Uint8>(top.a + t * (bottom.a - top.a));
        SDL_SetRenderDrawColor(r, rr, gg, bb, aa);
        SDL_RenderDrawLine(r, rect.x, rect.y + y, rect.x + rect.w - 1, rect.y + y);
    }
}

} // namespace abntpiano::ui
