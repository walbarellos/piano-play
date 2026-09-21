#include "abntpiano/JudgementEngine.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>

namespace abntpiano {

namespace {

char normalizeKey(char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

constexpr double kEps = 1e-4;

JudgementType evaluateSingleKey(double absDeltaMs, const HitWindow& window) {
    if (absDeltaMs <= window.perfect + kEps) {
        return JudgementType::Perfect;
    }
    if (absDeltaMs <= window.great + kEps) {
        return JudgementType::Great;
    }
    if (absDeltaMs <= window.good + kEps) {
        return JudgementType::Good;
    }
    return JudgementType::Miss;
}

// Retorna verdadeiro se 'a' for pior (mais severo) que 'b'.
// Severidade: Miss > Good > Great > Perfect.
bool isWorse(JudgementType a, JudgementType b) {
    auto rank = [](JudgementType t) {
        switch (t) {
            case JudgementType::Perfect: return 3;
            case JudgementType::Great:   return 2;
            case JudgementType::Good:    return 1;
            case JudgementType::Miss:    return 0;
        }
        return 0;
    };
    return rank(a) < rank(b);
}

} // namespace

Judgement judgeChordGroup(
    const std::vector<char>& targetKeys,
    double targetOnset,
    const std::vector<KeyInputEvent>& inputs,
    const DifficultyConfig& config
) {
    if (targetKeys.empty()) {
        return Judgement{
            .type = JudgementType::Miss,
            .deltaMs = std::nullopt,
            .keysHit = 0,
            .keysTotal = 0
        };
    }

    // Normaliza e desduplica target keys para garantir contagem exata
    std::vector<char> uniqueTargets;
    uniqueTargets.reserve(targetKeys.size());
    for (char k : targetKeys) {
        char norm = normalizeKey(k);
        if (std::find(uniqueTargets.begin(), uniqueTargets.end(), norm) == uniqueTargets.end()) {
            uniqueTargets.push_back(norm);
        }
    }

    size_t keysTotal = uniqueTargets.size();
    size_t keysHit = 0;
    JudgementType worstHitType = JudgementType::Perfect;
    double deltaOfWorstHit = 0.0;

    for (char targetKey : uniqueTargets) {
        std::optional<double> bestDeltaForThisKey;
        double minAbsDelta = std::numeric_limits<double>::infinity();

        for (const auto& input : inputs) {
            if (normalizeKey(input.key) == targetKey) {
                double deltaMs = (input.timestamp - targetOnset) * 1000.0;
                double absDelta = std::abs(deltaMs);

                // Janela máxima inclusiva (ADR-12 regra 3)
                if (absDelta <= config.hitWindow.missAbove + kEps) {
                    // Repique: seleciona o de menor |deltaMs| (ADR-12 regra 1)
                    if (absDelta < minAbsDelta) {
                        minAbsDelta = absDelta;
                        bestDeltaForThisKey = deltaMs;
                    }
                }
            }
        }

        if (bestDeltaForThisKey.has_value()) {
            JudgementType keyType = evaluateSingleKey(minAbsDelta, config.hitWindow);
            if (keyType != JudgementType::Miss) {
                keysHit++;
                if (isWorse(keyType, worstHitType) || keysHit == 1) {
                    worstHitType = keyType;
                    deltaOfWorstHit = *bestDeltaForThisKey;
                }
            }
        }
    }

    // 1. Ausência de input válido na janela (timeout) -> Miss sem delta (ADR-12 regra 2)
    if (keysHit == 0) {
        return Judgement{
            .type = JudgementType::Miss,
            .deltaMs = std::nullopt,
            .keysHit = 0,
            .keysTotal = keysTotal
        };
    }

    // 2. Acerto pleno -> pior caso individual (RF18)
    if (keysHit == keysTotal) {
        return Judgement{
            .type = worstHitType,
            .deltaMs = deltaOfWorstHit,
            .keysHit = keysHit,
            .keysTotal = keysTotal
        };
    }

    // 3. Acerto parcial
    float hitRatio = static_cast<float>(keysHit) / static_cast<float>(keysTotal);
    if (config.allowPartialChord && hitRatio >= config.partialChordThreshold) {
        // Tolerância em Easy / Normal configurado (ADR-11)
        return Judgement{
            .type = JudgementType::Good,
            .deltaMs = deltaOfWorstHit,
            .keysHit = keysHit,
            .keysTotal = keysTotal
        };
    }

    // Acerto parcial em dificuldade estrita (Hard/Expert) -> Miss total (RF18)
    return Judgement{
        .type = JudgementType::Miss,
        .deltaMs = deltaOfWorstHit,
        .keysHit = keysHit,
        .keysTotal = keysTotal
    };
}

} // namespace abntpiano
