#include "abntpiano/ui/ParticleSystem.hpp"
#include <algorithm>

namespace abntpiano::ui {

ParticleSystem::ParticleSystem()
    : rng_(42) {}

void ParticleSystem::spawnHit(JudgementType type, int keyCenterX, int hitY) {
    if (type == JudgementType::Miss) {
        // No Miss: SEM feixe, sem caixa dura, sem quadrado!
        // Apenas uma sutil faísca vermelha de erro
        for (int p = 0; p < 3; p++) {
            particles_.push_back({
                static_cast<float>(keyCenterX + randVX_(rng_) * 0.05f),
                static_cast<float>(hitY - 2),
                randVX_(rng_) * 0.25f,
                -35.0f - randC_(rng_) * 30.0f,
                1.0f,
                3.5f,
                SDL_Color{220, 40, 50, 180},
                2
            });
        }
        return;
    }

    SDL_Color waveColor =
        (type == JudgementType::Perfect) ? SDL_Color{255, 220, 60, 240} :
        (type == JudgementType::Great)   ? SDL_Color{60, 240, 110, 230} :
                                           SDL_Color{80, 200, 255, 210};
    spawnShockwave(keyCenterX, waveColor);

    int count = (type == JudgementType::Perfect) ? 14 :
                (type == JudgementType::Great)   ? 9 : 5;

    for (int p = 0; p < count; p++) {
        SDL_Color pCol;
        if (type == JudgementType::Perfect) {
            int cPick = p % 4;
            if (cPick == 0) pCol = SDL_Color{255, 255, 240, 255}; // Branco incandescente
            else if (cPick == 1) pCol = SDL_Color{255, 215, 0, 255}; // Ouro
            else if (cPick == 2) pCol = SDL_Color{255, 140, 30, 255}; // Fogo âmbar
            else pCol = SDL_Color{255, 80, 150, 255}; // Confete magenta
        } else if (type == JudgementType::Great) {
            pCol = (p % 2 == 0) ? SDL_Color{100, 255, 160, 255} : SDL_Color{40, 230, 120, 255};
        } else {
            pCol = (p % 2 == 0) ? SDL_Color{100, 220, 255, 255} : SDL_Color{60, 170, 255, 255};
        }

        particles_.push_back({
            static_cast<float>(keyCenterX + randVX_(rng_) * 0.10f),
            static_cast<float>(hitY - 2),
            randVX_(rng_) * 0.85f,
            randVY_(rng_) * 0.85f,
            1.0f,
            2.2f,
            pCol,
            (p % 3 == 0) ? 4 : 3
        });
    }
}

void ParticleSystem::spawnSustainEmbers(int centerX, int hitY, float hue, int count) {
    if (particles_.size() > 60) return; // Limite rigoroso: zero custo de FPS!

    for (int i = 0; i < count; i++) {
        float vx = (randC_(rng_) - 0.5f) * 60.0f;
        float vy = -90.0f - randC_(rng_) * 130.0f;

        SDL_Color col;
        int colorType = static_cast<int>(randC_(rng_) * 100.0f) % 4;
        if (colorType == 0) col = SDL_Color{255, 255, 220, 255};
        else if (colorType == 1) col = SDL_Color{255, 120, 20, 255};
        else if (colorType == 2) col = SDL_Color{255, 215, 0, 255};
        else col = hslToRgb(hue, 95.0f, 75.0f);

        particles_.push_back({
            static_cast<float>(centerX) + (randC_(rng_) - 0.5f) * 22.0f,
            static_cast<float>(hitY) - 3.0f - randC_(rng_) * 6.0f,
            vx,
            vy,
            1.0f,
            2.8f,
            col,
            (i % 2 == 0) ? 4 : 2
        });
    }
}

void ParticleSystem::spawnFloatingText(const std::string& text, int centerX, int hitY, SDL_Color color) {
    floatingTexts_.push_back({text, static_cast<float>(centerX), static_cast<float>(hitY - 36), -65.0f, 1.0f, color});
}

void ParticleSystem::spawnShockwave(int centerX, SDL_Color color) {
    shockwaves_.push_back({centerX, 1.0f, color});
}

void ParticleSystem::update(double dt) {
    for (auto& p : particles_) {
        p.x += p.vx * static_cast<float>(dt);
        p.y += p.vy * static_cast<float>(dt);
        p.vy += 200.0f * static_cast<float>(dt);
        p.life -= p.decay * static_cast<float>(dt);
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
        [](const Particle& p) { return p.life <= 0.0f; }), particles_.end());

    for (auto& ft : floatingTexts_) {
        ft.y += ft.vy * static_cast<float>(dt);
        ft.life -= 1.8f * static_cast<float>(dt);
    }
    floatingTexts_.erase(std::remove_if(floatingTexts_.begin(), floatingTexts_.end(),
        [](const FloatingJudgement& f) { return f.life <= 0.0f; }), floatingTexts_.end());

    for (auto& sw : shockwaves_) {
        sw.life -= 3.5f * static_cast<float>(dt);
    }
    shockwaves_.erase(std::remove_if(shockwaves_.begin(), shockwaves_.end(),
        [](const LaneShockwave& s) { return s.life <= 0.0f; }), shockwaves_.end());
}

void ParticleSystem::renderShockwaves(SDL_Renderer* ren, int hitY) const {
    for (const auto& sw : shockwaves_) {
        if (sw.life > 0.0f) {
            float t = 1.0f - sw.life;
            Uint8 a = static_cast<Uint8>(std::clamp(sw.life * 220.0f, 0.0f, 220.0f));

            // Lens-flare radial suave de impacto (SEM nenhum retângulo!)
            float radius = 18.0f + t * 40.0f;
            SDL_Color coreCol{sw.color.r, sw.color.g, sw.color.b, a};
            SDL_Color fadeCol{sw.color.r, sw.color.g, sw.color.b, 0};
            renderGlowDisc(ren, static_cast<float>(sw.centerX), static_cast<float>(hitY), radius, coreCol, fadeCol);

            // Anel elíptico de choque em expansão (Guitar Hero strike ring)
            float ringR = 14.0f + t * 32.0f;
            Uint8 ringA = static_cast<Uint8>(std::clamp(sw.life * 255.0f, 0.0f, 255.0f));
            SDL_SetRenderDrawColor(ren, 255, 255, 255, ringA);
            for (int angle = 0; angle < 360; angle += 20) {
                float rad = static_cast<float>(angle) * 0.0174533f;
                float px = static_cast<float>(sw.centerX) + std::cos(rad) * ringR;
                float py = static_cast<float>(hitY) + std::sin(rad) * (ringR * 0.45f);
                SDL_RenderDrawPointF(ren, px, py);
                SDL_RenderDrawPointF(ren, px + 0.5f, py);
            }
        }
    }
}

void ParticleSystem::renderParticles(SDL_Renderer* ren) const {
    for (const auto& p : particles_) {
        if (p.life <= 0.0f) continue;
        Uint8 a = static_cast<Uint8>(std::clamp(p.life * 255.0f, 0.0f, 255.0f));
        SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b, a);

        int s = p.size;
        int px = static_cast<int>(p.x);
        int py = static_cast<int>(p.y);

        // Confete / faísca estelar em diamante (estilo Guitar Hero)
        SDL_RenderDrawLine(ren, px - s, py, px + s, py);
        SDL_RenderDrawLine(ren, px, py - s, px, py + s);
        if (s > 2) {
            SDL_RenderDrawLine(ren, px - 1, py - 1, px + 1, py + 1);
            SDL_RenderDrawLine(ren, px - 1, py + 1, px + 1, py - 1);
        }

        // Núcleo incandescente nas partículas mais jovens
        if (p.life > 0.45f) {
            SDL_SetRenderDrawColor(ren, 255, 255, 255, a);
            SDL_RenderDrawPoint(ren, px, py);
        }
    }
}

void ParticleSystem::renderFloatingTexts(SDL_Renderer* ren, TTF_Font* font) const {
    for (const auto& ft : floatingTexts_) {
        if (ft.life <= 0.0f) continue;
        SDL_Color fc = ft.color;
        fc.a = static_cast<Uint8>(std::clamp(ft.life * 255.0f, 0.0f, 255.0f));
        renderText(ren, font, ft.text, static_cast<int>(ft.x), static_cast<int>(ft.y), fc, true);
    }
}

void ParticleSystem::clear() {
    particles_.clear();
    floatingTexts_.clear();
    shockwaves_.clear();
}

} // namespace abntpiano::ui
