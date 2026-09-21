#include "abntpiano/SongModeController.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace abntpiano {

SongModeController::SongModeController(Chart chart)
    : chart_(std::move(chart)) {
    isGroupJudged_.resize(chart_.playableEvents.size(), false);
    groupJudgements_.resize(chart_.playableEvents.size(), JudgementType::Miss);
    groupInputs_.resize(chart_.playableEvents.size());
    unjudgedCount_ = chart_.playableEvents.size();
    firstUnjudgedIdx_ = 0;
}

void SongModeController::start() {
    state_ = SongState::Playing;
    playhead_ = 0.0;
    scoring_.reset();
    std::fill(isGroupJudged_.begin(), isGroupJudged_.end(), false);
    std::fill(groupJudgements_.begin(), groupJudgements_.end(), JudgementType::Miss);
    for (auto& inputs : groupInputs_) {
        inputs.clear();
    }
    unjudgedCount_ = chart_.playableEvents.size();
    firstUnjudgedIdx_ = 0;
}

void SongModeController::pause() {
    if (state_ == SongState::Playing) {
        state_ = SongState::Paused;
    }
}

void SongModeController::resume() {
    if (state_ == SongState::Paused) {
        state_ = SongState::Playing;
    }
}

void SongModeController::restart() {
    start();
}

void SongModeController::abandon() {
    state_ = SongState::Abandoned;
}

void SongModeController::update(double deltaTimeSeconds) {
    if (state_ != SongState::Playing) return;

    playhead_ += deltaTimeSeconds;
    checkExpiredNotes();

    // Verifica se todas as notas foram julgadas em O(1)
    if (unjudgedCount_ == 0) {
        double lastTime = 0.0;
        if (!chart_.playableEvents.empty()) {
            lastTime = chart_.playableEvents.back().onset;
        }
        if (playhead_ >= lastTime + 1.0) {
            state_ = SongState::Finished;
            if (finishedCb_) {
                finishedCb_(scoring_.finalize());
            }
        }
    }
}

void SongModeController::checkExpiredNotes() {
    double missWindowSec = (chart_.difficulty.hitWindow.missAbove + 1e-4) / 1000.0;

    for (size_t i = firstUnjudgedIdx_; i < chart_.playableEvents.size(); ++i) {
        if (!isGroupJudged_[i]) {
            if (chart_.playableEvents[i].onset + missWindowSec >= playhead_) {
                // Como playableEvents está estritamente ordenado por onset,
                // nenhuma nota posterior a esta pode ter expirado!
                break;
            }

            // Nota/acorde expirou por timeout (TC16)
            auto judgement = judgeChordGroup(
                chart_.playableEvents[i].keys,
                chart_.playableEvents[i].onset,
                groupInputs_[i],
                chart_.difficulty
            );

            isGroupJudged_[i] = true;
            groupJudgements_[i] = judgement.type;
            if (unjudgedCount_ > 0) --unjudgedCount_;
            scoring_.registerJudgement(judgement);

            if (judgementCb_) {
                judgementCb_(judgement, chart_.playableEvents[i]);
            }
        }
    }
    while (firstUnjudgedIdx_ < chart_.playableEvents.size() && isGroupJudged_[firstUnjudgedIdx_]) {
        ++firstUnjudgedIdx_;
    }
}

void SongModeController::onKeyDown(char key) {
    if (state_ != SongState::Playing) return;

    char normKey = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
    double missWindowSec = (chart_.difficulty.hitWindow.missAbove + 1e-4) / 1000.0;

    // Encontra o grupo pendente mais próximo que espera essa tecla
    int bestIdx = -1;
    double minDistance = 1e9;

    for (size_t i = firstUnjudgedIdx_; i < chart_.playableEvents.size(); ++i) {
        if (!isGroupJudged_[i]) {
            double onset = chart_.playableEvents[i].onset;
            if (onset > playhead_ + missWindowSec) {
                // Notas futuras além da janela de acerto não podem corresponder a este input
                break;
            }
            double dist = std::abs(playhead_ - onset);

            if (dist <= missWindowSec) {
                const auto& expectedKeys = chart_.playableEvents[i].keys;
                bool expectsThisKey = false;
                for (char k : expectedKeys) {
                    if (static_cast<char>(std::toupper(static_cast<unsigned char>(k))) == normKey) {
                        expectsThisKey = true;
                        break;
                    }
                }

                if (expectsThisKey && dist < minDistance) {
                    minDistance = dist;
                    bestIdx = static_cast<int>(i);
                }
            }
        }
    }

    if (bestIdx >= 0) {
        size_t idx = static_cast<size_t>(bestIdx);
        groupInputs_[idx].push_back(KeyInputEvent{normKey, playhead_});

        auto judgement = judgeChordGroup(
            chart_.playableEvents[idx].keys,
            chart_.playableEvents[idx].onset,
            groupInputs_[idx],
            chart_.difficulty
        );

        // Se o grupo foi acertado integralmente ou atingiu critério parcial (Easy), finaliza o grupo
        if (judgement.type != JudgementType::Miss) {
            isGroupJudged_[idx] = true;
            groupJudgements_[idx] = judgement.type;
            if (unjudgedCount_ > 0) --unjudgedCount_;
            scoring_.registerJudgement(judgement);
            if (judgementCb_) {
                judgementCb_(judgement, chart_.playableEvents[idx]);
            }
            while (firstUnjudgedIdx_ < chart_.playableEvents.size() && isGroupJudged_[firstUnjudgedIdx_]) {
                ++firstUnjudgedIdx_;
            }
        }
    }
}

void SongModeController::triggerDemoHit(size_t groupIndex, JudgementType type, double hitTimestamp) {
    if (state_ != SongState::Playing) return;
    if (groupIndex >= chart_.playableEvents.size()) return;
    if (isGroupJudged_[groupIndex]) return;

    const auto& group = chart_.playableEvents[groupIndex];
    size_t keysTotal = group.keys.size();

    for (char k : group.keys) {
        char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(k)));
        groupInputs_[groupIndex].push_back(KeyInputEvent{norm, hitTimestamp});
    }

    double deltaMs = (hitTimestamp - group.onset) * 1000.0;
    Judgement judgement{
        .type = type,
        .deltaMs = deltaMs,
        .keysHit = keysTotal,
        .keysTotal = keysTotal
    };

    isGroupJudged_[groupIndex] = true;
    groupJudgements_[groupIndex] = type;
    if (unjudgedCount_ > 0) --unjudgedCount_;
    scoring_.registerJudgement(judgement);

    if (judgementCb_) {
        judgementCb_(judgement, group);
    }

    while (firstUnjudgedIdx_ < chart_.playableEvents.size() && isGroupJudged_[firstUnjudgedIdx_]) {
        ++firstUnjudgedIdx_;
    }
}

std::vector<VisibleNote> SongModeController::getVisibleNotes(double lookaheadSeconds) const {
    std::vector<VisibleNote> visible;
    for (size_t i = 0; i < chart_.playableEvents.size(); ++i) {
        const auto& ev = chart_.playableEvents[i];
        double timeToHit = ev.onset - playhead_;

        // Encontra maior duração no grupo para manter a cauda visível até concluir o hold
        double maxDur = 0.2;
        for (double d : ev.durations) {
            maxDur = std::max(maxDur, d);
        }

        if (timeToHit <= lookaheadSeconds && timeToHit >= -maxDur - 0.3) {
            visible.push_back(VisibleNote{
                .groupIndex = i,
                .timeToHit = timeToHit,
                .keys = ev.keys,
                .durations = ev.durations,
                .midiNotes = ev.midiNotes,
                .isJudged = isGroupJudged_[i],
                .judgement = groupJudgements_[i]
            });
        }
    }
    return visible;
}

} // namespace abntpiano
