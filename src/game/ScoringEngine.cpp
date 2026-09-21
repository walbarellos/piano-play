#include "abntpiano/ScoringEngine.hpp"
#include <cmath>
#include <algorithm>

namespace abntpiano {

void ScoringEngine::reset() {
    score_ = 0;
    currentCombo_ = 0;
    maxCombo_ = 0;
    perfectCount_ = 0;
    greatCount_ = 0;
    goodCount_ = 0;
    missCount_ = 0;
}

float ScoringEngine::calculateComboMultiplier(int combo) {
    if (combo >= 100) return 2.0f;
    if (combo >= 50)  return 1.5f;
    if (combo >= 25)  return 1.2f;
    if (combo >= 10)  return 1.1f;
    return 1.0f;
}

Grade ScoringEngine::gradeFromAccuracy(float acc) {
    if (acc >= 0.95f) return Grade::S;
    if (acc >= 0.85f) return Grade::A;
    if (acc >= 0.75f) return Grade::B;
    if (acc >= 0.60f) return Grade::C;
    return Grade::D;
}

void ScoringEngine::registerJudgement(const Judgement& judgement) {
    switch (judgement.type) {
        case JudgementType::Perfect: {
            currentCombo_++;
            maxCombo_ = std::max(maxCombo_, currentCombo_);
            perfectCount_++;
            float mult = calculateComboMultiplier(currentCombo_);
            score_ += static_cast<int64_t>(std::round(1000.0f * mult));
            break;
        }
        case JudgementType::Great: {
            currentCombo_++;
            maxCombo_ = std::max(maxCombo_, currentCombo_);
            greatCount_++;
            float mult = calculateComboMultiplier(currentCombo_);
            score_ += static_cast<int64_t>(std::round(700.0f * mult));
            break;
        }
        case JudgementType::Good: {
            currentCombo_++;
            maxCombo_ = std::max(maxCombo_, currentCombo_);
            goodCount_++;
            float mult = calculateComboMultiplier(currentCombo_);
            score_ += static_cast<int64_t>(std::round(400.0f * mult));
            break;
        }
        case JudgementType::Miss: {
            // RF18 / TC18: Miss zera o combo atual, preserva pontuação acumulada
            currentCombo_ = 0;
            missCount_++;
            break;
        }
    }
}

float ScoringEngine::currentAccuracy() const {
    int total = perfectCount_ + greatCount_ + goodCount_ + missCount_;
    if (total == 0) return 1.0f;
    
    double weightedPoints = 1.0 * perfectCount_ + 0.7 * greatCount_ + 0.4 * goodCount_;
    return static_cast<float>(weightedPoints / static_cast<double>(total));
}

Grade ScoringEngine::currentGrade() const {
    return gradeFromAccuracy(currentAccuracy());
}

ExecutionSummary ScoringEngine::finalize() const {
    return ExecutionSummary{
        .totalScore = score_,
        .maxCombo = maxCombo_,
        .accuracy = currentAccuracy(),
        .grade = currentGrade(),
        .perfectCount = perfectCount_,
        .greatCount = greatCount_,
        .goodCount = goodCount_,
        .missCount = missCount_
    };
}

} // namespace abntpiano
