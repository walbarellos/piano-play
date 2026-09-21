#pragma once

#include "abntpiano/JudgementEngine.hpp"
#include "abntpiano/ui/RenderTypes.hpp"
#include <random>
#include <string>
#include <vector>

namespace abntpiano::ui {

struct Particle {
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    float life = 1.0f;
    float decay = 2.0f;
    SDL_Color color{255, 215, 0, 255};
    int size = 4;
};

struct FloatingJudgement {
    std::string text;
    float x = 0;
    float y = 0;
    float vy = -55.0f;
    float life = 1.0f;
    SDL_Color color{255, 215, 0, 255};
};

struct LaneShockwave {
    int centerX = 0;
    float life = 1.0f;
    SDL_Color color{50, 220, 100, 200};
};

class ParticleSystem {
public:
    ParticleSystem();

    void spawnHit(JudgementType type, int keyCenterX, int hitY);
    void spawnFloatingText(const std::string& text, int centerX, int hitY, SDL_Color color);
    void spawnShockwave(int centerX, SDL_Color color);

    void update(double dt);
    void renderShockwaves(SDL_Renderer* ren, int hitY) const;
    void renderParticles(SDL_Renderer* ren) const;
    void renderFloatingTexts(SDL_Renderer* ren, TTF_Font* font) const;

    void clear();

private:
    std::vector<Particle> particles_;
    std::vector<FloatingJudgement> floatingTexts_;
    std::vector<LaneShockwave> shockwaves_;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> randVX_{-90.0f, 90.0f};
    std::uniform_real_distribution<float> randVY_{-200.0f, -70.0f};
    std::uniform_real_distribution<float> randC_{0.8f, 1.0f};
};

} // namespace abntpiano::ui
