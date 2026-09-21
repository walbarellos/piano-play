#pragma once

#include "abntpiano/HoldState.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <array>
#include <string>
#include <vector>

namespace abntpiano::ui {

// Dimensões do jogo
constexpr int kScreenWidth  = 1280;
constexpr int kScreenHeight = 800;

// Layout vertical refinado do piano
constexpr int kHudHeight    = 78;
constexpr int kHorizonY     = 92;   // kHudHeight + 14
constexpr int kHitY         = 592;
constexpr int kKeyboardY    = 608;  // kHitY + 16
constexpr int kSpanY        = kHitY - kHorizonY; // 500
constexpr float kPadX       = 26.0f;

// Estrutura de tecla física do piano visual
struct PianoKeyVisual {
    char ch = 0;          // Letra ABNT / QWERTY ('Q'..'M')
    int midi = 0;         // Nota MIDI (60..85)
    int pc = 0;           // Pitch Class (0..11)
    bool isSharp = false; // Tecla sustenida (preta)
    std::string name;     // "C4", "C#4", etc.
    float x = 0.0f;       // Centro X exato
    float w = 0.0f;       // Largura da tecla
};

// Funções de acesso ao layout do piano (15 naturais + 11 sustenidas)
const std::vector<PianoKeyVisual>& getPianoLayout();
const PianoKeyVisual* getPianoKey(char key);
const PianoKeyVisual* getPianoKeyByMidi(int midi);

// Perspectiva de escala (0.62 no horizonte até 1.00 na linha de impacto)
inline float persp(float f) {
    return 0.62f + 0.38f * (1.0f - f);
}

// Círculo de Quintas para Matiz de Altura (352° -> 162°)
float pitchHue(int pc);

// Conversão HSL -> RGB
SDL_Color hslToRgb(float h, float s, float l, Uint8 a = 255);

// Compatibilidade de fontes
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

// Primitivas de renderização otimizadas via SDL_RenderGeometry / Hardware
void renderText(SDL_Renderer* r, TTF_Font* font, const std::string& text,
                int x, int y, SDL_Color color, bool center = false);

void renderTextRight(SDL_Renderer* r, TTF_Font* font, const std::string& text,
                     int x, int y, SDL_Color color);

void renderGradientRect(SDL_Renderer* r, SDL_Rect rect,
                        SDL_Color top, SDL_Color bottom);

void renderTrapezoid(SDL_Renderer* ren,
                     float topX, float topW, float topY,
                     float botX, float botW, float botY,
                     SDL_Color topColor, SDL_Color botColor);

void renderTrapezoidOutline(SDL_Renderer* ren,
                            float topX, float topW, float topY,
                            float botX, float botW, float botY,
                            SDL_Color color);

void renderCapsule(SDL_Renderer* ren, float x, float y, float w, float h,
                   SDL_Color topColor, SDL_Color botColor);

void renderCapsuleOutline(SDL_Renderer* ren, float x, float y, float w, float h,
                          SDL_Color color);

void renderRoundRectSelective(SDL_Renderer* ren,
                              float x, float y, float w, float h,
                              float rTop, float rBot,
                              SDL_Color fillTop, SDL_Color fillBot);

void renderGlowDisc(SDL_Renderer* ren, float cx, float cy, float radius,
                    SDL_Color centerCol, SDL_Color edgeCol);

} // namespace abntpiano::ui
