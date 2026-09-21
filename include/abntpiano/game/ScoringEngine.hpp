#pragma once

#include "abntpiano/JudgementEngine.hpp"
#include "abntpiano/SaveGame.hpp"
#include <cstdint>
#include <chrono>

namespace abntpiano {

// Resumo final de uma execução de peça (RF21)
struct ExecutionSummary {
    int64_t totalScore = 0;
    int maxCombo = 0;
    float accuracy = 0.0f; // 0.0 a 1.0 (ex: 0.965 = 96.5%)
    Grade grade = Grade::D;
    
    int perfectCount = 0;
    int greatCount = 0;
    int goodCount = 0;
    int missCount = 0;

    int totalNotes() const {
        return perfectCount + greatCount + goodCount + missCount;
    }

    // Converte para ScoreRecord pronto para persistência no SaveGameRepository
    ScoreRecord toScoreRecord(const std::string& chartId, const std::string& profileId) const {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        return ScoreRecord{
            .chartId = chartId,
            .profileId = profileId,
            .accuracy = accuracy,
            .maxCombo = maxCombo,
            .grade = grade,
            .playedAt = now
        };
    }
};

// Motor de pontuação, combo e cálculo de grade (RF20, RF21)
class ScoringEngine {
public:
    ScoringEngine() = default;

    // Reseta estado para nova partida
    void reset();

    // Registra um julgamento de acerto/erro (RF20)
    void registerJudgement(const Judgement& judgement);

    // Getters de estado em tempo real
    int64_t currentScore() const { return score_; }
    int currentCombo() const { return currentCombo_; }
    int maxCombo() const { return maxCombo_; }
    float currentAccuracy() const;
    Grade currentGrade() const;

    int perfectCount() const { return perfectCount_; }
    int greatCount() const { return greatCount_; }
    int goodCount() const { return goodCount_; }
    int missCount() const { return missCount_; }

    // Gera o resumo consolidado ao final da execução (RF21)
    ExecutionSummary finalize() const;

private:
    int64_t score_ = 0;
    int currentCombo_ = 0;
    int maxCombo_ = 0;

    int perfectCount_ = 0;
    int greatCount_ = 0;
    int goodCount_ = 0;
    int missCount_ = 0;

    static float calculateComboMultiplier(int combo);
    static Grade gradeFromAccuracy(float acc);
};

} // namespace abntpiano
