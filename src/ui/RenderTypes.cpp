#include "abntpiano/ui/RenderTypes.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace abntpiano::ui {

static std::vector<PianoKeyVisual> createPianoLayout() {
    const std::string kOrder = "QWERTYUIOPASDFGHJKLZXCVBNM";
    const std::string kNames[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    std::vector<PianoKeyVisual> keys;
    keys.reserve(26);

    // Identificar teclas e contar naturais
    int naturalCount = 0;
    for (int i = 0; i < 26; i++) {
        int midi = 60 + i;
        int pc = midi % 12;
        bool sharp = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        if (!sharp) naturalCount++;
    }

    float wW = (static_cast<float>(kScreenWidth) - kPadX * 2.0f) / static_cast<float>(naturalCount);
    float bW = wW * 0.60f;

    int ni = 0;
    for (int i = 0; i < 26; i++) {
        int midi = 60 + i;
        int pc = midi % 12;
        bool sharp = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        int octave = (midi / 12) - 1;
        std::string name = kNames[pc] + std::to_string(octave);

        PianoKeyVisual k;
        k.ch = kOrder[i];
        k.midi = midi;
        k.pc = pc;
        k.isSharp = sharp;
        k.name = name;

        if (!sharp) {
            k.x = kPadX + static_cast<float>(ni) * wW + wW / 2.0f;
            k.w = wW - 3.0f;
            ni++;
        } else {
            k.x = kPadX + static_cast<float>(ni) * wW;
            k.w = bW;
        }
        keys.push_back(k);
    }
    return keys;
}

const std::vector<PianoKeyVisual>& getPianoLayout() {
    static const std::vector<PianoKeyVisual> kKeys = createPianoLayout();
    return kKeys;
}

const PianoKeyVisual* getPianoKey(char key) {
    char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
    const auto& keys = getPianoLayout();
    for (const auto& k : keys) {
        if (k.ch == norm) return &k;
    }
    return nullptr;
}

const PianoKeyVisual* getPianoKeyByMidi(int midi) {
    const auto& keys = getPianoLayout();
    for (const auto& k : keys) {
        if (k.midi == midi) return &k;
    }
    return nullptr;
}

float pitchHue(int pc) {
    static const int kFifths[12] = {0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5};
    int idx = 0;
    for (int i = 0; i < 12; i++) {
        if (kFifths[i] == pc) { idx = i; break; }
    }
    return 352.0f - (static_cast<float>(idx) / 11.0f) * 190.0f;
}

SDL_Color hslToRgb(float h, float s, float l, Uint8 a) {
    while (h < 0.0f) h += 360.0f;
    while (h >= 360.0f) h -= 360.0f;
    s = std::clamp(s / 100.0f, 0.0f, 1.0f);
    l = std::clamp(l / 100.0f, 0.0f, 1.0f);

    float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
    float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float r1 = 0, g1 = 0, b1 = 0;
    if (h < 60.0f)       { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120.0f) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180.0f) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240.0f) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300.0f) { r1 = x; g1 = 0; b1 = c; }
    else                 { r1 = c; g1 = 0; b1 = x; }

    return SDL_Color{
        static_cast<Uint8>(std::clamp((r1 + m) * 255.0f, 0.0f, 255.0f)),
        static_cast<Uint8>(std::clamp((g1 + m) * 255.0f, 0.0f, 255.0f)),
        static_cast<Uint8>(std::clamp((b1 + m) * 255.0f, 0.0f, 255.0f)),
        a
    };
}

bool FontCollection::load(const std::string& fontPath) {
    release();
    tiny   = TTF_OpenFont(fontPath.c_str(), 11);
    small  = TTF_OpenFont(fontPath.c_str(), 14);
    medium = TTF_OpenFont(fontPath.c_str(), 20);
    large  = TTF_OpenFont(fontPath.c_str(), 26);
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

void renderTextRight(SDL_Renderer* r, TTF_Font* font, const std::string& text,
                     int x, int y, SDL_Color color) {
    if (!font || text.empty()) return;
    int w = 0, h = 0;
    TTF_SizeUTF8(font, text.c_str(), &w, &h);
    renderText(r, font, text, x - w, y, color, false);
}

void renderGradientRect(SDL_Renderer* r, SDL_Rect rect,
                        SDL_Color top, SDL_Color bottom) {
    if (rect.h <= 0 || rect.w <= 0) return;
    SDL_Vertex verts[4];
    verts[0].position = {static_cast<float>(rect.x), static_cast<float>(rect.y)};
    verts[0].color = top;
    verts[0].tex_coord = {0, 0};

    verts[1].position = {static_cast<float>(rect.x + rect.w), static_cast<float>(rect.y)};
    verts[1].color = top;
    verts[1].tex_coord = {0, 0};

    verts[2].position = {static_cast<float>(rect.x + rect.w), static_cast<float>(rect.y + rect.h)};
    verts[2].color = bottom;
    verts[2].tex_coord = {0, 0};

    verts[3].position = {static_cast<float>(rect.x), static_cast<float>(rect.y + rect.h)};
    verts[3].color = bottom;
    verts[3].tex_coord = {0, 0};

    int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(r, nullptr, verts, 4, indices, 6);
}

void renderTrapezoid(SDL_Renderer* ren,
                     float topX, float topW, float topY,
                     float botX, float botW, float botY,
                     SDL_Color topColor, SDL_Color botColor) {
    SDL_Vertex verts[4];
    verts[0].position = {topX - topW / 2.0f, topY};
    verts[0].color = topColor;
    verts[0].tex_coord = {0, 0};

    verts[1].position = {topX + topW / 2.0f, topY};
    verts[1].color = topColor;
    verts[1].tex_coord = {0, 0};

    verts[2].position = {botX + botW / 2.0f, botY};
    verts[2].color = botColor;
    verts[2].tex_coord = {0, 0};

    verts[3].position = {botX - botW / 2.0f, botY};
    verts[3].color = botColor;
    verts[3].tex_coord = {0, 0};

    int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(ren, nullptr, verts, 4, indices, 6);
}

void renderTrapezoidOutline(SDL_Renderer* ren,
                            float topX, float topW, float topY,
                            float botX, float botW, float botY,
                            SDL_Color color) {
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    SDL_FPoint pts[5] = {
        {topX - topW / 2.0f, topY},
        {topX + topW / 2.0f, topY},
        {botX + botW / 2.0f, botY},
        {botX - botW / 2.0f, botY},
        {topX - topW / 2.0f, topY}
    };
    SDL_RenderDrawLinesF(ren, pts, 5);
}

static SDL_Color lerpColor(SDL_Color c1, SDL_Color c2, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return SDL_Color{
        static_cast<Uint8>(c1.r + t * (c2.r - c1.r)),
        static_cast<Uint8>(c1.g + t * (c2.g - c1.g)),
        static_cast<Uint8>(c1.b + t * (c2.b - c1.b)),
        static_cast<Uint8>(c1.a + t * (c2.a - c1.a))
    };
}

void renderCapsule(SDL_Renderer* ren, float x, float y, float w, float h,
                   SDL_Color topColor, SDL_Color botColor) {
    if (w <= 0.0f || h <= 0.0f) return;

    float rad = std::min(w, h) / 2.0f;
    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;

    // Centro do polígono
    SDL_FPoint center{x + w / 2.0f, y + h / 2.0f};
    SDL_Color midCol = lerpColor(topColor, botColor, 0.5f);
    verts.push_back({center, midCol, {0, 0}});

    auto addPerimeterPt = [&](float px, float py) {
        float t = (h > 0.001f) ? ((py - y) / h) : 0.5f;
        SDL_Color col = lerpColor(topColor, botColor, t);
        verts.push_back({{px, py}, col, {0, 0}});
    };

    if (h >= w) {
        // Cápsula vertical
        float cx = x + rad;
        float cyTop = y + rad;
        float cyBot = y + h - rad;
        constexpr int kSegments = 8;

        // Top arc (PI até 2*PI)
        for (int i = 0; i <= kSegments; i++) {
            float ang = 3.14159265f + (3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments));
            addPerimeterPt(cx + rad * std::cos(ang), cyTop + rad * std::sin(ang));
        }
        // Bottom arc (0 até PI)
        for (int i = 0; i <= kSegments; i++) {
            float ang = 3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments);
            addPerimeterPt(cx + rad * std::cos(ang), cyBot + rad * std::sin(ang));
        }
    } else {
        // Cápsula horizontal
        float cy = y + rad;
        float cxLeft = x + rad;
        float cxRight = x + w - rad;
        constexpr int kSegments = 8;

        // Right arc (-PI/2 até PI/2)
        for (int i = 0; i <= kSegments; i++) {
            float ang = -1.5707963f + (3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments));
            addPerimeterPt(cxRight + rad * std::cos(ang), cy + rad * std::sin(ang));
        }
        // Left arc (PI/2 até 3*PI/2)
        for (int i = 0; i <= kSegments; i++) {
            float ang = 1.5707963f + (3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments));
            addPerimeterPt(cxLeft + rad * std::cos(ang), cy + rad * std::sin(ang));
        }
    }

    int n = static_cast<int>(verts.size());
    for (int i = 1; i < n - 1; i++) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }
    indices.push_back(0);
    indices.push_back(n - 1);
    indices.push_back(1);

    SDL_RenderGeometry(ren, nullptr, verts.data(), static_cast<int>(verts.size()),
                       indices.data(), static_cast<int>(indices.size()));
}

void renderCapsuleOutline(SDL_Renderer* ren, float x, float y, float w, float h,
                          SDL_Color color) {
    if (w <= 0.0f || h <= 0.0f) return;
    float rad = std::min(w, h) / 2.0f;
    std::vector<SDL_FPoint> pts;

    if (h >= w) {
        float cx = x + rad;
        float cyTop = y + rad;
        float cyBot = y + h - rad;
        constexpr int kSegments = 8;

        for (int i = 0; i <= kSegments; i++) {
            float ang = 3.14159265f + (3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments));
            pts.push_back({cx + rad * std::cos(ang), cyTop + rad * std::sin(ang)});
        }
        for (int i = 0; i <= kSegments; i++) {
            float ang = 3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments);
            pts.push_back({cx + rad * std::cos(ang), cyBot + rad * std::sin(ang)});
        }
    } else {
        float cy = y + rad;
        float cxLeft = x + rad;
        float cxRight = x + w - rad;
        constexpr int kSegments = 8;

        for (int i = 0; i <= kSegments; i++) {
            float ang = -1.5707963f + (3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments));
            pts.push_back({cxRight + rad * std::cos(ang), cy + rad * std::sin(ang)});
        }
        for (int i = 0; i <= kSegments; i++) {
            float ang = 1.5707963f + (3.14159265f * static_cast<float>(i) / static_cast<float>(kSegments));
            pts.push_back({cxLeft + rad * std::cos(ang), cy + rad * std::sin(ang)});
        }
    }

    if (!pts.empty()) {
        pts.push_back(pts.front());
        SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLinesF(ren, pts.data(), static_cast<int>(pts.size()));
    }
}

void renderRoundRectSelective(SDL_Renderer* ren,
                              float x, float y, float w, float h,
                              float rTop, float rBot,
                              SDL_Color fillTop, SDL_Color fillBot) {
    if (w <= 0.0f || h <= 0.0f) return;
    rTop = std::clamp(rTop, 0.0f, std::min(w / 2.0f, h / 2.0f));
    rBot = std::clamp(rBot, 0.0f, std::min(w / 2.0f, h / 2.0f));

    std::vector<SDL_Vertex> verts;
    std::vector<int> indices;

    SDL_FPoint center{x + w / 2.0f, y + h / 2.0f};
    SDL_Color midCol = lerpColor(fillTop, fillBot, 0.5f);
    verts.push_back({center, midCol, {0, 0}});

    auto addPt = [&](float px, float py) {
        float t = (h > 0.001f) ? ((py - y) / h) : 0.5f;
        SDL_Color col = lerpColor(fillTop, fillBot, t);
        verts.push_back({{px, py}, col, {0, 0}});
    };

    constexpr int kArcSegs = 4;
    // Canto Superior Esquerdo
    if (rTop > 0.0f) {
        float cx = x + rTop, cy = y + rTop;
        for (int i = 0; i <= kArcSegs; i++) {
            float a = 3.14159265f + (1.5707963f * static_cast<float>(i) / static_cast<float>(kArcSegs));
            addPt(cx + rTop * std::cos(a), cy + rTop * std::sin(a));
        }
    } else {
        addPt(x, y);
    }

    // Canto Superior Direito
    if (rTop > 0.0f) {
        float cx = x + w - rTop, cy = y + rTop;
        for (int i = 0; i <= kArcSegs; i++) {
            float a = 4.71238898f + (1.5707963f * static_cast<float>(i) / static_cast<float>(kArcSegs));
            addPt(cx + rTop * std::cos(a), cy + rTop * std::sin(a));
        }
    } else {
        addPt(x + w, y);
    }

    // Canto Inferior Direito
    if (rBot > 0.0f) {
        float cx = x + w - rBot, cy = y + h - rBot;
        for (int i = 0; i <= kArcSegs; i++) {
            float a = 0.0f + (1.5707963f * static_cast<float>(i) / static_cast<float>(kArcSegs));
            addPt(cx + rBot * std::cos(a), cy + rBot * std::sin(a));
        }
    } else {
        addPt(x + w, y + h);
    }

    // Canto Inferior Esquerdo
    if (rBot > 0.0f) {
        float cx = x + rBot, cy = y + h - rBot;
        for (int i = 0; i <= kArcSegs; i++) {
            float a = 1.5707963f + (1.5707963f * static_cast<float>(i) / static_cast<float>(kArcSegs));
            addPt(cx + rBot * std::cos(a), cy + rBot * std::sin(a));
        }
    } else {
        addPt(x, y + h);
    }

    int n = static_cast<int>(verts.size());
    for (int i = 1; i < n - 1; i++) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }
    indices.push_back(0);
    indices.push_back(n - 1);
    indices.push_back(1);

    SDL_RenderGeometry(ren, nullptr, verts.data(), static_cast<int>(verts.size()),
                       indices.data(), static_cast<int>(indices.size()));
}

void renderGlowDisc(SDL_Renderer* ren, float cx, float cy, float radius,
                    SDL_Color centerCol, SDL_Color edgeCol) {
    if (radius <= 0.0f) return;
    constexpr int kSegments = 16;
    std::vector<SDL_Vertex> verts;
    verts.reserve(kSegments + 2);

    verts.push_back({{cx, cy}, centerCol, {0, 0}});
    for (int i = 0; i <= kSegments; i++) {
        float a = 6.2831853f * static_cast<float>(i) / static_cast<float>(kSegments);
        verts.push_back({{cx + radius * std::cos(a), cy + radius * std::sin(a)}, edgeCol, {0, 0}});
    }

    std::vector<int> indices;
    indices.reserve(kSegments * 3);
    for (int i = 1; i <= kSegments; i++) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    SDL_RenderGeometry(ren, nullptr, verts.data(), static_cast<int>(verts.size()),
                       indices.data(), static_cast<int>(indices.size()));
}

} // namespace abntpiano::ui
