#include "abntpiano/ScoringEngine.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace abntpiano;

static bool approxEqual(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

int main() {
    ScoringEngine engine;

    std::cout << "[TEST] Executando TC17: Fixture determinístico de pontuação e combo\n";
    {
        engine.reset();
        // 5 Perfects consecutivos (combo 1 a 5, mult = 1.0)
        // 5 * 1000 = 5000 pontos
        for (int i = 0; i < 5; ++i) {
            engine.registerJudgement({JudgementType::Perfect, 0.0, 1, 1});
        }
        assert(engine.currentCombo() == 5);
        assert(engine.maxCombo() == 5);
        assert(engine.currentScore() == 5000);
        assert(approxEqual(engine.currentAccuracy(), 1.0f));
        assert(engine.currentGrade() == Grade::S);

        // 1 Great (combo 6, mult = 1.0) -> +700 = 5700
        engine.registerJudgement({JudgementType::Great, 85.0, 1, 1});
        assert(engine.currentCombo() == 6);
        assert(engine.currentScore() == 5700);

        // 1 Good (combo 7, mult = 1.0) -> +400 = 6100
        engine.registerJudgement({JudgementType::Good, 140.0, 1, 1});
        assert(engine.currentCombo() == 7);
        assert(engine.currentScore() == 6100);
    }

    std::cout << "[TEST] Executando TC18: Miss zera o combo mas preserva pontuação acumulada\n";
    {
        int64_t scoreBeforeMiss = engine.currentScore(); // 6100
        int maxComboBeforeMiss = engine.maxCombo();       // 7

        // Registra um Miss
        engine.registerJudgement({JudgementType::Miss, std::nullopt, 0, 1});

        assert(engine.currentCombo() == 0); // Zera combo atual!
        assert(engine.maxCombo() == maxComboBeforeMiss); // Maior combo é mantido
        assert(engine.currentScore() == scoreBeforeMiss); // Pontuação intacta!
        assert(engine.missCount() == 1);

        // Próximo acerto reconstrói o combo a partir de 1
        engine.registerJudgement({JudgementType::Perfect, 10.0, 1, 1});
        assert(engine.currentCombo() == 1);
        assert(engine.currentScore() == scoreBeforeMiss + 1000);
    }

    std::cout << "[TEST] Validando Cálculo de Grade e Finalização (RF21)\n";
    {
        engine.reset();
        // 10 notas: 9 Perfect, 1 Miss -> acc = 9.0 / 10 = 0.90 -> Grade A
        for (int i = 0; i < 9; ++i) {
            engine.registerJudgement({JudgementType::Perfect, 0.0, 1, 1});
        }
        engine.registerJudgement({JudgementType::Miss, std::nullopt, 0, 1});

        auto summary = engine.finalize();
        assert(summary.perfectCount == 9);
        assert(summary.missCount == 1);
        assert(approxEqual(summary.accuracy, 0.90f));
        assert(summary.grade == Grade::A);

        auto scoreRec = summary.toScoreRecord("prelude_op28_no4_Normal", "player1");
        assert(scoreRec.chartId == "prelude_op28_no4_Normal");
        assert(scoreRec.profileId == "player1");
        assert(scoreRec.grade == Grade::A);
        assert(scoreRec.playedAt > 0);
    }

    std::cout << "\n>>> TODOS OS CASOS DE TESTE DO SCORINGENGINE (TC17–TC18) PASSARAM! <<<\n";
    return 0;
}
