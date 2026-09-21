#include "abntpiano/ui/HighwayRenderer.hpp"
#include <algorithm>
#include <cmath>

namespace abntpiano::ui {

void HighwayRenderer::render(SDL_Renderer* ren,
                             const FontCollection& fonts,
                             const KeyboardRenderer& /*keyboard*/,
                             const std::vector<VisibleNote>& visNotes,
                             const std::set<char>& keysAtHitLine,
                             const std::map<char, KeyFeedback>& feedbacks,
                             const std::set<char>& heldKeys,
                             const std::map<char, HoldState>& holdStates,
                             double lookahead,
                             double playhead) const {
    renderPlayfieldBackground(ren, visNotes);
    renderLanes(ren);
    renderNotes(ren, fonts, visNotes, heldKeys, holdStates, lookahead, playhead);
    renderFeltRail(ren, keysAtHitLine, feedbacks, heldKeys, holdStates, playhead);
}

void HighwayRenderer::renderPlayfieldBackground(SDL_Renderer* ren, const std::vector<VisibleNote>& visNotes) const {
    // 1. Fundo em gradiente suave: #07050B -> #100C18 -> #191221
    int midY = kHudHeight + static_cast<int>(kSpanY * 0.62f);
    SDL_Rect bgTop{0, kHudHeight, kScreenWidth, midY - kHudHeight};
    renderGradientRect(ren, bgTop, {7, 5, 11, 255}, {16, 12, 24, 255});

    SDL_Rect bgBot{0, midY, kScreenWidth, kHitY - midY};
    renderGradientRect(ren, bgBot, {16, 12, 24, 255}, {25, 18, 33, 255});

    // 2. Respiração ambiente: tom médio das notas soando agora (~5% opacidade)
    float hueSum = 0.0f;
    int soundingCount = 0;
    for (const auto& vn : visNotes) {
        if (vn.timeToHit <= 0.0) {
            for (size_t i = 0; i < vn.keys.size(); i++) {
                double dur = (i < vn.durations.size()) ? vn.durations[i] : 0.3;
                if (vn.timeToHit + dur >= 0.0) {
                    const auto* k = getPianoKey(vn.keys[i]);
                    if (k) {
                        hueSum += pitchHue(k->pc);
                        soundingCount++;
                    }
                }
            }
        }
    }
    if (soundingCount > 0) {
        float avgHue = hueSum / static_cast<float>(soundingCount);
        SDL_Color breathCol = hslToRgb(avgHue, 55.0f, 46.0f, 13);
        SDL_Rect breathRect{0, kHudHeight, kScreenWidth, kSpanY};
        SDL_SetRenderDrawColor(ren, breathCol.r, breathCol.g, breathCol.b, breathCol.a);
        SDL_RenderFillRect(ren, &breathRect);
    }

    // 3. Brilho suave no horizonte: tampa do piano aberta (latão #C79A4A com fade)
    SDL_Rect horizonGlow{0, kHorizonY - 24, kScreenWidth, 140};
    renderGradientRect(ren, horizonGlow, {199, 154, 74, 38}, {199, 154, 74, 0});
}

void HighwayRenderer::renderLanes(SDL_Renderer* ren) const {
    const auto& keys = getPianoLayout();

    // Pistas em duas passagens: Naturais primeiro, Sustenidas por cima
    for (bool sharpPass : {false, true}) {
        for (const auto& k : keys) {
            if (k.isSharp != sharpPass) continue;

            float wTop = k.w * persp(1.0f);
            float wBot = k.w * persp(0.0f);

            SDL_Color fillCol = k.isSharp ?
                SDL_Color{6, 4, 10, 168} :      // rgba(6,4,10,.66)
                SDL_Color{237, 230, 214, 8};    // rgba(237,230,214,.030)

            SDL_Color outlineCol = k.isSharp ?
                SDL_Color{140, 164, 196, 23} :  // rgba(140,164,196,.09)
                SDL_Color{237, 230, 214, 14};   // rgba(237,230,214,.055)

            renderTrapezoid(ren, k.x, wTop, static_cast<float>(kHorizonY),
                            k.x, wBot, static_cast<float>(kHitY),
                            fillCol, fillCol);

            renderTrapezoidOutline(ren, k.x, wTop, static_cast<float>(kHorizonY),
                                   k.x, wBot, static_cast<float>(kHitY),
                                   outlineCol);
        }
    }
}

void HighwayRenderer::renderNotes(SDL_Renderer* ren, const FontCollection& fonts,
                                  const std::vector<VisibleNote>& visNotes,
                                  const std::set<char>& heldKeys,
                                  const std::map<char, HoldState>& holdStates,
                                  double lookahead, double playhead) const {
    (void)playhead;
    for (const auto& vn : visNotes) {
        double ttl = vn.timeToHit;
        bool isHit = vn.isJudged && (vn.judgement != JudgementType::Miss);
        bool isMissed = vn.isJudged && (vn.judgement == JudgementType::Miss);

        // Barra de conexão de acordes simultâneos (apenas enquanto o acorde não foi acertado)
        if (vn.keys.size() > 1 && !isHit && ttl >= 0.0) {
            float minX = static_cast<float>(kScreenWidth), maxX = 0.0f;
            float chordY = 0.0f;
            for (char kChar : vn.keys) {
                const auto* pk = getPianoKey(kChar);
                if (pk) {
                    minX = std::min(minX, pk->x);
                    maxX = std::max(maxX, pk->x);
                    float f = static_cast<float>(ttl / lookahead);
                    chordY = static_cast<float>(kHorizonY) + (1.0f - f) * static_cast<float>(kSpanY);
                }
            }
            if (maxX > minX) {
                SDL_SetRenderDrawColor(ren, 199, 154, 74, 90);
                SDL_RenderDrawLineF(ren, minX, chordY, maxX, chordY);
                SDL_RenderDrawLineF(ren, minX, chordY - 1.0f, maxX, chordY - 1.0f);
            }
        }

        for (size_t idx = 0; idx < vn.keys.size(); idx++) {
            char keyChar = vn.keys[idx];
            const auto* k = getPianoKey(keyChar);
            if (!k) continue;

            double dur = (idx < vn.durations.size()) ? vn.durations[idx] : 0.3;
            bool isLong = (dur > 0.32);
            char normKey = static_cast<char>(std::toupper(static_cast<unsigned char>(keyChar)));

            auto itHold = holdStates.find(normKey);
            bool isHolding = (heldKeys.count(normKey) > 0 ||
                             (itHold != holdStates.end() && itHold->second == HoldState::Holding));

            float hue = pitchHue(k->pc);

            // ── NOTAS CURTAS (dur <= 0.32) ──────────────────────────
            if (!isLong) {
                // Se foi acertada e pressionada:
                if (isHit) {
                    // Flash imediato no momento do acerto (dentro dos primeiros 80ms)
                    if (ttl < -0.08) {
                        // DESAPARECE DA PISTA! Fim da corrida após ser pressionada.
                        continue;
                    }

                    // Efeito de impacto no momento exato em que foi pressionada: muda de cor para brilho dourado/branco
                    float flashW = k->w * 0.90f;
                    float flashX = k->x - flashW / 2.0f;
                    float flashY = static_cast<float>(kHitY) - 14.0f;
                    SDL_Color flashTop{255, 255, 235, 255};
                    SDL_Color flashBot = hslToRgb(hue, 95.0f, 75.0f, 255);
                    renderCapsule(ren, flashX, flashY, flashW, 16.0f, flashTop, flashBot);
                    renderGlowDisc(ren, k->x, static_cast<float>(kHitY), flashW * 1.2f,
                                   {255, 255, 255, 220}, hslToRgb(hue, 90.0f, 70.0f, 0));
                    continue;
                }

                // Se passou da linha sem ser pressionada (Miss):
                if (isMissed) {
                    if (ttl < -0.22) continue; // Desaparece rapidamente após o erro
                }

                if (ttl > lookahead) continue;

                // Nota curta descendo normalmente antes do impacto
                float f = static_cast<float>(ttl / lookahead);
                float y = static_cast<float>(kHorizonY) + (1.0f - f) * static_cast<float>(kSpanY);
                float s = persp(std::max(0.0f, f));
                float w = k->w * s * 0.86f;
                float h = 18.0f;
                float x = k->x - w / 2.0f;
                float top = y - h;
                float near = std::max(0.0f, 1.0f - std::abs(static_cast<float>(ttl)) / 0.28f);

                // Rastro fantasma sutil
                for (int gg = 3; gg >= 1; gg--) {
                    double gttl = ttl + static_cast<double>(gg) * 0.055;
                    if (gttl > lookahead || gttl < 0.0) continue;
                    float gf = static_cast<float>(gttl / lookahead);
                    float gy = static_cast<float>(kHorizonY) + (1.0f - gf) * static_cast<float>(kSpanY);
                    float gs = persp(gf);
                    float gw = k->w * gs * 0.86f * 0.70f;
                    Uint8 trailAlpha = static_cast<Uint8>(255.0f * 0.04f * static_cast<float>(4 - gg));
                    SDL_Color trailCol = hslToRgb(hue, 55.0f, k->isSharp ? 32.0f : 70.0f, trailAlpha);
                    renderCapsule(ren, k->x - gw / 2.0f, gy - 6.0f, gw, 12.0f, trailCol, trailCol);
                }

                // Glow de aproximação
                if (near > 0.0f) {
                    Uint8 glowAlpha = static_cast<Uint8>(near * 85.0f);
                    renderGlowDisc(ren, k->x, y - h * 0.5f, std::max(w, 24.0f) * 1.25f,
                                   hslToRgb(hue, 70.0f, 68.0f, glowAlpha),
                                   hslToRgb(hue, 70.0f, 68.0f, 0));
                }

                // Cor da nota
                SDL_Color topCol, botCol;
                if (isMissed) {
                    topCol = SDL_Color{50, 25, 30, 110};
                    botCol = SDL_Color{80, 30, 40, 130};
                } else {
                    topCol = k->isSharp ? hslToRgb(hue, 42.0f, 20.0f) : hslToRgb(hue, 46.0f, 46.0f);
                    botCol = k->isSharp ? hslToRgb(hue, 52.0f, 58.0f) : hslToRgb(hue, 72.0f, 86.0f);
                }

                renderCapsule(ren, x, top, w, h, topCol, botCol);

                // Highlight radial de impacto na base da cápsula
                renderGlowDisc(ren, k->x, y - 6.0f, std::max(w * 0.8f, 20.0f),
                               hslToRgb(hue, 20.0f, 96.0f, 130),
                               hslToRgb(hue, 20.0f, 96.0f, 0));

                // Letra
                if (w > 20.0f) {
                    SDL_Color letterCol = k->isSharp ? hslToRgb(hue, 30.0f, 94.0f, 235) : hslToRgb(hue, 55.0f, 16.0f, 184);
                    std::string chStr(1, k->ch);
                    renderText(ren, (s > 0.8f) ? fonts.small : fonts.tiny,
                               chStr, static_cast<int>(k->x), static_cast<int>(y - 11.0f),
                               letterCol, true);
                }
            }
            // ── NOTAS LONGAS (dur > 0.32) ───────────────────────────
            else {
                // Se o hold já foi completado no tempo (passou da duração total):
                if (ttl + dur <= 0.0) {
                    // Nota longa concluída e consumida, desaparece da pista
                    continue;
                }

                if (ttl > lookahead) continue;

                // Geometria da nota longa:
                // Quando a cabeça atinge a linha de impacto (ttl <= 0.0), a base FICA PRESA em kHitY
                // e não avança tela abaixo; a cauda continua descendo em direção à linha.
                float y = (ttl <= 0.0) ? static_cast<float>(kHitY) :
                                         static_cast<float>(kHorizonY) + (1.0f - static_cast<float>(ttl / lookahead)) * static_cast<float>(kSpanY);

                float f2 = static_cast<float>(std::min(1.0, (ttl + dur) / lookahead));
                float yT = static_cast<float>(kHorizonY) + (1.0f - f2) * static_cast<float>(kSpanY);

                float s = persp(std::max(0.0f, static_cast<float>(std::max(0.0, ttl) / lookahead)));
                float w = k->w * s * 0.86f;
                float h = std::max(20.0f, y - yT);
                float x = k->x - w / 2.0f;
                float top = y - h;

                // Fração de sustentação em tempo real
                float elapsed = (ttl <= 0.0) ? static_cast<float>(-ttl) : 0.0f;
                float holdProgress = std::clamp(elapsed / static_cast<float>(dur), 0.0f, 1.0f);

                // MUDANÇA DE COR CONFORME PRESSIONAMENTO:
                // Se o jogador acertou e está segurando ativamente: cor vibrante e energética (luminous hold)
                SDL_Color topCol, botCol;
                if (isHolding && ttl <= 0.0) {
                    topCol = hslToRgb(hue, 90.0f, 65.0f);
                    botCol = hslToRgb(hue, 100.0f, 85.0f);
                } else if (isMissed || (!isHolding && ttl <= -0.15)) {
                    // Soltou prematuramente ou errou: escurece para indicar interrupção
                    topCol = SDL_Color{50, 25, 30, 120};
                    botCol = SDL_Color{85, 40, 50, 140};
                } else {
                    // Descendo antes do impacto
                    topCol = k->isSharp ? hslToRgb(hue, 42.0f, 20.0f) : hslToRgb(hue, 46.0f, 46.0f);
                    botCol = k->isSharp ? hslToRgb(hue, 52.0f, 58.0f) : hslToRgb(hue, 72.0f, 86.0f);
                }

                // Corpo da cápsula longa
                renderCapsule(ren, x, top, w, h, topCol, botCol);

                // Faixa central e textura de sustentação
                if (h > 30.0f) {
                    float cx = x + w / 2.0f;
                    float stripeW = std::max(2.0f, w * 0.10f);
                    renderCapsule(ren, cx - stripeW / 2.0f, top + 8.0f, stripeW, h - 16.0f,
                                  {255, 255, 255, 36}, {255, 255, 255, 36});

                    SDL_Color hatchCol = isHolding ? hslToRgb(hue, 80.0f, 85.0f, 80) :
                                                     (k->isSharp ? SDL_Color{10, 14, 22, 56} : hslToRgb(hue, 60.0f, 30.0f, 36));
                    SDL_SetRenderDrawColor(ren, hatchCol.r, hatchCol.g, hatchCol.b, hatchCol.a);
                    for (float hy = top + 14.0f; hy < y - 14.0f; hy += 9.0f) {
                        SDL_RenderDrawLineF(ren, x + 3.0f, hy, x + w - 3.0f, hy + 6.0f);
                    }
                }

                // ── PREENCHIMENTO EM TEMPO REAL CONFORME O JOGADOR APERTA ──
                if (isHolding && ttl <= 0.0 && holdProgress > 0.01f) {
                    float fillHeight = h * holdProgress;
                    float fillTop = y - fillHeight;
                    float fillW = w * 0.76f;
                    float fillX = k->x - fillW / 2.0f;

                    // Núcleo líquido elétrico de preenchimento
                    SDL_Color coreTop{255, 255, 255, 240};
                    SDL_Color coreBot = hslToRgb(hue, 100.0f, 90.0f, 255);
                    renderCapsule(ren, fillX, fillTop, fillW, fillHeight, coreTop, coreBot);

                    // Crista brilhante indicando a frente do preenchimento
                    renderGlowDisc(ren, k->x, fillTop + 4.0f, fillW * 0.9f,
                                   {255, 255, 255, 250}, hslToRgb(hue, 95.0f, 80.0f, 0));

                    // Aura de sustentação contínua
                    renderGlowDisc(ren, k->x, static_cast<float>(kHitY), w * 1.4f,
                                   hslToRgb(hue, 100.0f, 80.0f, 140), hslToRgb(hue, 100.0f, 80.0f, 0));
                }

                // Highlight radial de impacto na base da cápsula
                renderGlowDisc(ren, k->x, y - std::min(h, 20.0f) * 0.4f, std::max(w * 0.8f, 22.0f),
                               hslToRgb(hue, 20.0f, 96.0f, 130),
                               hslToRgb(hue, 20.0f, 96.0f, 0));

                // Letra
                if (w > 20.0f && h > 20.0f) {
                    SDL_Color letterCol = isHolding ? SDL_Color{255, 255, 255, 255} :
                                          (k->isSharp ? hslToRgb(hue, 30.0f, 94.0f, 235) : hslToRgb(hue, 55.0f, 16.0f, 184));
                    std::string chStr(1, k->ch);
                    renderText(ren, (s > 0.8f) ? fonts.small : fonts.tiny,
                               chStr, static_cast<int>(k->x), static_cast<int>(y - std::min(h, 26.0f) / 2.0f - 2),
                               letterCol, true);
                }
            }
        }
    }
}

void HighwayRenderer::renderFeltRail(SDL_Renderer* ren,
                                     const std::set<char>& keysAtHitLine,
                                     const std::map<char, KeyFeedback>& feedbacks,
                                     const std::set<char>& heldKeys,
                                     const std::map<char, HoldState>& holdStates,
                                     double currentPlayhead) const {
    // 1. Trilho de feltro acústico
    SDL_Rect railBack{0, kHitY - 5, kScreenWidth, 10};
    SDL_SetRenderDrawColor(ren, 179, 46, 66, 33); // rgba(179,46,66,.13)
    SDL_RenderFillRect(ren, &railBack);

    SDL_Rect railCore{0, kHitY - 1, kScreenWidth, 3};
    SDL_SetRenderDrawColor(ren, 179, 46, 66, 255); // #B32E42
    SDL_RenderFillRect(ren, &railCore);

    const auto& keys = getPianoLayout();

    // 2. Receptores em cada pista
    for (const auto& k : keys) {
        float hue = pitchHue(k.pc);
        float rw = k.w * 0.70f;
        float rx = k.x - rw / 2.0f;
        float ry = static_cast<float>(kHitY) - 3.5f;

        float hitFactor = 0.0f;

        // Se a tecla está sendo ativamente segurada (hold progress):
        char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(k.ch)));
        auto itHold = holdStates.find(norm);
        bool isHolding = (heldKeys.count(norm) > 0 || (itHold != holdStates.end() && itHold->second == HoldState::Holding));
        if (isHolding) {
            hitFactor = 1.0f;
        } else {
            auto itFb = feedbacks.find(k.ch);
            if (itFb != feedbacks.end() && itFb->second.type != JudgementType::Miss) {
                double dt = currentPlayhead - (itFb->second.expireTime - 0.20);
                if (dt >= 0.0 && dt < 0.22) {
                    hitFactor = 1.0f - static_cast<float>(dt / 0.22);
                }
            }
            if (hitFactor <= 0.0f && keysAtHitLine.count(k.ch) > 0) {
                hitFactor = 0.55f;
            }
        }

        if (hitFactor > 0.0f) {
            Uint8 a = static_cast<Uint8>((0.25f + 0.75f * hitFactor) * 255.0f);
            SDL_Color hitCol = hslToRgb(hue, 70.0f, 88.0f, a);
            renderCapsule(ren, rx, ry, rw, 7.0f, hitCol, hitCol);

            // Glow radial
            renderGlowDisc(ren, k.x, static_cast<float>(kHitY),
                           70.0f * hitFactor + 18.0f,
                           hslToRgb(hue, 75.0f, 72.0f, static_cast<Uint8>(hitFactor * 130.0f)),
                           hslToRgb(hue, 75.0f, 72.0f, 0));

            // Burst de partículas em órbita radial
            if (hitFactor > 0.25f) {
                for (int p = 0; p < 6; p++) {
                    float ang = static_cast<float>((k.midi * 97 + p * 61) % 360) * 0.0174533f;
                    float dist = (1.0f - hitFactor) * 34.0f + 6.0f;
                    float px = k.x + std::cos(ang) * dist;
                    float py = static_cast<float>(kHitY) + std::sin(ang) * dist * 0.6f;
                    SDL_Color spCol = hslToRgb(hue, 70.0f, 82.0f, static_cast<Uint8>((hitFactor - 0.25f) * 1.3f * 255.0f));
                    SDL_SetRenderDrawColor(ren, spCol.r, spCol.g, spCol.b, spCol.a);
                    SDL_Rect spRect{static_cast<int>(px - 1), static_cast<int>(py - 1), 3, 3};
                    SDL_RenderFillRect(ren, &spRect);
                }
            }
        } else {
            SDL_Color dimCol{199, 154, 74, 56}; // rgba(199,154,74,.22)
            renderCapsule(ren, rx, ry, rw, 7.0f, dimCol, dimCol);
        }
    }
}

} // namespace abntpiano::ui
