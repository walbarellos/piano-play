#pragma once

#include "abntpiano/Chart.hpp"
#include "abntpiano/JudgementEngine.hpp"
#include "abntpiano/ScoringEngine.hpp"
#include <functional>
#include <vector>
#include <optional>

namespace abntpiano {

enum class SongState {
    Ready,
    Playing,
    Paused,
    Finished,
    Abandoned
};

// Item visível para o renderizador da Zona de Leitura (notas caindo)
struct VisibleNote {
    size_t groupIndex = 0;
    double timeToHit = 0.0; // segundos até cruzar a hit line (<= 0 significa cruzando/passou)
    std::vector<char> keys;
    std::vector<double> durations; // Duração das notas correspondentes
    std::vector<int> midiNotes;    // Pitch real para o synth (paralelo a keys)
    bool isJudged = false;
};

// Orquestrador do Song Mode (RF16, RF17, RF18, RF19, UC05)
class SongModeController {
public:
    using JudgementCallback = std::function<void(const Judgement& judgement, const PlayableChordGroup& group)>;
    using FinishedCallback = std::function<void(const ExecutionSummary& summary)>;

    explicit SongModeController(Chart chart);

    void setJudgementCallback(JudgementCallback cb) { judgementCb_ = std::move(cb); }
    void setFinishedCallback(FinishedCallback cb) { finishedCb_ = std::move(cb); }

    // Controles de fluxo (RF19)
    void start();
    void pause();
    void resume();
    void restart();
    void abandon();

    // Avanço do playhead no frame loop
    void update(double deltaTimeSeconds);

    // Eventos de entrada do jogador
    void onKeyDown(char key);

    // Lookahead para a UI Layer (RF16, ADR-10)
    // Retorna notas dentro da janela de visão (ex.: próximos 2.5 segundos de descida)
    std::vector<VisibleNote> getVisibleNotes(double lookaheadSeconds) const;

    // Getters de estado
    SongState state() const { return state_; }
    double playhead() const { return playhead_; }

    // Consulta se um grupo já foi julgado (usado pelo guia melódico automático)
    bool isGroupJudged(size_t index) const {
        return index < isGroupJudged_.size() && isGroupJudged_[index];
    }
    const Chart& chart() const { return chart_; }
    const ScoringEngine& scoringEngine() const { return scoring_; }

private:
    Chart chart_;
    SongState state_ = SongState::Ready;
    double playhead_ = 0.0;

    ScoringEngine scoring_;

    // Rastreamento por grupo
    std::vector<bool> isGroupJudged_;
    std::vector<std::vector<KeyInputEvent>> groupInputs_;
    size_t unjudgedCount_ = 0;
    size_t firstUnjudgedIdx_ = 0;

    JudgementCallback judgementCb_;
    FinishedCallback finishedCb_;

    void checkExpiredNotes();
};

} // namespace abntpiano
