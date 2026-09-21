#include "abntpiano/ui/ParticleSystem.hpp"
#include <algorithm>

namespace abntpiano::ui {

ParticleSystem::ParticleSystem()
    : rng_(42) {}

void ParticleSystem::spawnHit(JudgementType type, int keyCenterX, int hitY) {
    SDL_Color waveColor =
        (type == JudgementType::Perfect) ? SDL_Color{255, 215, 50, 230} :
        (type == JudgementType::Great)   ? SDL_Color{50, 230, 100, 220} :
        (type == JudgementType::Good)    ? SDL_Color{70, 180, 255, 200} :
                                           SDL_Color{240, 40, 60, 190};
    spawnShockwave(keyCenterX, waveColor);

    int count = (type == JudgementType::Perfect) ? 12 :
                (type == JudgementType::Great)   ? 8 :
                (type == JudgementType::Good)    ? 4 : 0;

    for (int p = 0; p < count; p++) {
        SDL_Color pCol = (type == JudgementType::Perfect) ?
            (p % 3 == 0 ? SDL_Color{255, 255, 230, 255} : SDL_Color{255, static_cast<Uint8>(210 * randC_(rng_)), 40, 255}) :
            (type == JudgementType::Great) ?
            (p % 2 == 0 ? SDL_Color{80, 255, 140, 255} : SDL_Color{50, 230, 100, 255}) :
            SDL_Color{80, 200, 255, 255};

        particles_.push_back({
            static_cast<float>(keyCenterX + randVX_(rng_) * 0.12f),
            static_cast<float>(hitY - 4),
            randVX_(rng_) * 0.75f,
            randVY_(rng_) * 0.75f,
            1.0f,
            1.9f,
            pCol,
            (p % 2 == 0) ? 5 : 3
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
            Uint8 a = static_cast<Uint8>(std::clamp(sw.life * 200.0f, 0.0f, 200.0f));
            SDL_SetRenderDrawColor(ren, sw.color.r, sw.color.g, sw.color.b, a);
            int bw = static_cast<int>(10 + (1.0f - sw.life) * 35.0f);
            SDL_Rect beam{sw.centerX - bw / 2, hitY - 80, bw, 80};
            SDL_RenderFillRect(ren, &beam);
        }
    }
}

void ParticleSystem::renderParticles(SDL_Renderer* ren) const {
    for (const auto& p : particles_) {
        if (p.life <= 0.0f) continue;
        Uint8 a = static_cast<Uint8>(std::clamp(p.life * 255.0f, 0.0f, 255.0f));
        SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b, a);
        SDL_Rect pr{static_cast<int>(p.x - p.size / 2), static_cast<int>(p.y - p.size / 2), p.size, p.size};
        SDL_RenderFillRect(ren, &pr);
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
