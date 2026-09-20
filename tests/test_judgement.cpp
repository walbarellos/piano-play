#include "abntpiano/JudgementEngine.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace abntpiano;

static bool approxEqual(double a, double b, double eps = 0.001) {
    return std::fabs(a - b) < eps;
}

int main() {
    // Configurações de teste conforme RNF04
    DifficultyConfig normalConfig{
        .name = "Normal",
        .hitWindow = {
            .perfect = 80.0,
            .great = 130.0,
            .good = 180.0,
            .missAbove = 180.0
        },
        .maxChordSize = 3,
        .noteDensityFactor = 1.0f,
        .allowPartialChord = false,
        .partialChordThreshold = 0.66f
    };

    DifficultyConfig easyConfig{
        .name = "Easy",
        .hitWindow = {
            .perfect = 120.0,
            .great = 180.0,
            .good = 250.0,
            .missAbove = 250.0
        },
        .maxChordSize = 2,
        .noteDensityFactor = 0.7f,
        .allowPartialChord = true,
        .partialChordThreshold = 0.33f // basta 1 nota de um acorde de 2 ou 3
    };

    DifficultyConfig hardConfig{
        .name = "Hard",
        .hitWindow = {
            .perfect = 50.0,
            .great = 90.0,
            .good = 130.0,
            .missAbove = 130.0
        },
        .maxChordSize = 4,
        .noteDensityFactor = 1.0f,
        .allowPartialChord = false,
        .partialChordThreshold = 1.0f
    };

    std::cout << "[TEST] Executando TC13: Input exatamente no onset esperado -> Perfect\n";
    {
        double onset = 10.0;
        std::vector<char> targets = {'Q'};
        std::vector<KeyInputEvent> inputs = {
            {.key = 'Q', .timestamp = 10.0}
        };

        auto res = judgeChordGroup(targets, onset, inputs, normalConfig);
        assert(res.type == JudgementType::Perfect);
        assert(res.deltaMs.has_value());
        assert(approxEqual(*res.deltaMs, 0.0));
        assert(res.keysHit == 1);
        assert(res.keysTotal == 1);
    }

    std::cout << "[TEST] Executando TC14: Validar fronteiras exatas da HitWindow (Normal)\n";
    {
        double onset = 5.0;
        std::vector<char> targets = {'A'};

        // 80 ms cravados -> Perfect
        auto res80 = judgeChordGroup(targets, onset, {{'A', 5.080}}, normalConfig);
        assert(res80.type == JudgementType::Perfect);

        // 100 ms -> Great (entre 80ms e 130ms)
        auto res100 = judgeChordGroup(targets, onset, {{'A', 5.100}}, normalConfig);
        assert(res100.type == JudgementType::Great);

        // 130 ms cravados -> Great
        auto res130 = judgeChordGroup(targets, onset, {{'A', 5.130}}, normalConfig);
        assert(res130.type == JudgementType::Great);

        // 150 ms -> Good (entre 130ms e 180ms)
        auto res150 = judgeChordGroup(targets, onset, {{'A', 5.150}}, normalConfig);
        assert(res150.type == JudgementType::Good);

        // 180 ms cravados -> Good (fronteira inclusiva ADR-12)
        auto res180 = judgeChordGroup(targets, onset, {{'A', 5.180}}, normalConfig);
        assert(res180.type == JudgementType::Good);

        // 181 ms -> Miss (acima de 180ms)
        auto res181 = judgeChordGroup(targets, onset, {{'A', 5.181}}, normalConfig);
        assert(res181.type == JudgementType::Miss);

        // Adiantado (Early): -75 ms -> Perfect
        auto resEarly = judgeChordGroup(targets, onset, {{'A', 4.925}}, normalConfig);
        assert(resEarly.type == JudgementType::Perfect);
        assert(approxEqual(*resEarly.deltaMs, -75.0));
    }

    std::cout << "[TEST] Executando TC15a: ChordGroup de 3 teclas, 2 pressionadas em Hard/Expert -> Miss\n";
    {
        double onset = 12.0;
        std::vector<char> targets = {'S', 'D', 'G'};
        // Jogador pressiona apenas 'S' e 'D' dentro da janela
        std::vector<KeyInputEvent> inputs = {
            {.key = 'S', .timestamp = 12.010}, // +10ms
            {.key = 'D', .timestamp = 12.020}  // +20ms
        };

        auto res = judgeChordGroup(targets, onset, inputs, hardConfig);
        assert(res.type == JudgementType::Miss);
        assert(res.keysHit == 2);
        assert(res.keysTotal == 3);
    }

    std::cout << "[TEST] Executando TC15b: ChordGroup de 3 teclas, 2 pressionadas em Easy -> Good\n";
    {
        double onset = 12.0;
        std::vector<char> targets = {'S', 'D', 'G'};
        std::vector<KeyInputEvent> inputs = {
            {.key = 'S', .timestamp = 12.010},
            {.key = 'D', .timestamp = 12.020}
        };

        auto res = judgeChordGroup(targets, onset, inputs, easyConfig);
        assert(res.type == JudgementType::Good);
        assert(res.keysHit == 2);
        assert(res.keysTotal == 3);
        assert(res.deltaMs.has_value());
    }

    std::cout << "[TEST] Executando TC16: Nenhum input na janela -> Miss com delta nulo (std::nullopt)\n";
    {
        double onset = 20.0;
        std::vector<char> targets = {'W'};
        std::vector<KeyInputEvent> inputs = {}; // nada

        auto res = judgeChordGroup(targets, onset, inputs, normalConfig);
        assert(res.type == JudgementType::Miss);
        assert(!res.deltaMs.has_value());
        assert(res.keysHit == 0);
        assert(res.keysTotal == 1);
    }

    std::cout << "[TEST] Executando Regra RF18: Acerto pleno = pior caso individual\n";
    {
        double onset = 8.0;
        std::vector<char> targets = {'Q', 'W', 'E'};
        std::vector<KeyInputEvent> inputs = {
            {.key = 'Q', .timestamp = 8.010}, // +10ms -> Perfect
            {.key = 'W', .timestamp = 8.020}, // +20ms -> Perfect
            {.key = 'E', .timestamp = 8.100}  // +100ms -> Great em Normal
        };

        auto res = judgeChordGroup(targets, onset, inputs, normalConfig);
        assert(res.type == JudgementType::Great); // Pior caso entre Perfect, Perfect e Great
        assert(res.keysHit == 3);
        assert(res.keysTotal == 3);
        assert(approxEqual(*res.deltaMs, 100.0));
    }

    std::cout << "[TEST] Executando ADR-12 (Repique/Chatter): Seleciona menor |delta|\n";
    {
        double onset = 15.0;
        std::vector<char> targets = {'K'};
        // Primeiro hit afobado a -150ms (Good), segundo hit no tempo a +5ms (Perfect)
        std::vector<KeyInputEvent> inputs = {
            {.key = 'K', .timestamp = 14.850}, // -150ms
            {.key = 'K', .timestamp = 15.005}  // +5ms
        };

        auto res = judgeChordGroup(targets, onset, inputs, normalConfig);
        assert(res.type == JudgementType::Perfect);
        assert(approxEqual(*res.deltaMs, 5.0));
    }

    std::cout << "[TEST] Executando Case-insensitivity ('a' vs 'A')\n";
    {
        double onset = 30.0;
        std::vector<char> targets = {'p'};
        std::vector<KeyInputEvent> inputs = {
            {.key = 'P', .timestamp = 30.002}
        };

        auto res = judgeChordGroup(targets, onset, inputs, normalConfig);
        assert(res.type == JudgementType::Perfect);
    }

    std::cout << "\n>>> TODOS OS CASOS DE TESTE (TC13–TC16b + ADR-12) PASSARAM COM SUCESSO! <<<\n";
    return 0;
}
