#include "abntpiano/ChartGenerator.hpp"
#include <cassert>
#include <iostream>

using namespace abntpiano;

int main() {
    // Monta uma Song sintética para testes com 10 ChordGroups
    // O primeiro acorde tem 4 notas simultâneas (C4, E4, G4, B4)
    Song testSong{
        .id = "nocturne_op9_no2",
        .title = "Nocturne Op. 9 No. 2",
        .composer = "Chopin",
        .sourceFormat = SourceFormat::MIDI,
        .bpm = 120.0,
        .durationSeconds = 10.0,
        .chordGroups = {}
    };

    // Grupo 0: Acorde de 4 notas
    testSong.chordGroups.push_back(ChordGroup{
        .onset = 0.0,
        .noteEvents = {
            {.timestamp = 0.0, .midiNote = 60, .duration = 1.0, .velocity = 80},
            {.timestamp = 0.0, .midiNote = 64, .duration = 1.0, .velocity = 70},
            {.timestamp = 0.0, .midiNote = 67, .duration = 1.0, .velocity = 60},
            {.timestamp = 0.0, .midiNote = 71, .duration = 1.0, .velocity = 50}
        }
    });

    // Grupos 1 a 9: notas individuais
    for (int i = 1; i < 10; ++i) {
        testSong.chordGroups.push_back(ChordGroup{
            .onset = static_cast<double>(i),
            .noteEvents = {
                {.timestamp = static_cast<double>(i), .midiNote = 60 + (i % 12), .duration = 0.5, .velocity = 64}
            }
        });
    }

    ChartGenerator generator;

    DifficultyConfig expertConfig{
        .name = "Expert",
        .hitWindow = {30.0, 60.0, 90.0, 90.0},
        .maxChordSize = 4,
        .noteDensityFactor = 1.0f,
        .allowPartialChord = false,
        .partialChordThreshold = 1.0f
    };

    DifficultyConfig easyConfig{
        .name = "Easy",
        .hitWindow = {120.0, 180.0, 250.0, 250.0},
        .maxChordSize = 2,
        .noteDensityFactor = 0.5f,
        .allowPartialChord = true,
        .partialChordThreshold = 0.5f
    };

    std::cout << "[TEST] Executando TC09: Expert preserva 100% dos ChordGroups originais\n";
    {
        Chart chart = generator.generateChart(testSong, expertConfig);
        assert(chart.playableEvents.size() == testSong.chordGroups.size());
        assert(chart.playableEvents.size() == 10);
    }

    std::cout << "[TEST] Executando TC10: Easy tem menos PlayableChordGroups que a Song original\n";
    {
        Chart chart = generator.generateChart(testSong, easyConfig);
        assert(chart.playableEvents.size() < testSong.chordGroups.size());
        assert(chart.playableEvents.size() == 5); // 50% de 10
    }

    std::cout << "[TEST] Executando TC11: ChordGroup maior que maxChordSize reduz para <= maxChordSize\n";
    {
        // No Easy, maxChordSize = 2. O grupo 0 original tinha 4 notas.
        Chart chart = generator.generateChart(testSong, easyConfig);
        assert(!chart.playableEvents.empty());
        assert(chart.playableEvents[0].keys.size() <= 2);
    }

    std::cout << "[TEST] Executando TC12: Determinismo (duas gerações idênticas para mesmo input)\n";
    {
        Chart chart1 = generator.generateChart(testSong, easyConfig);
        Chart chart2 = generator.generateChart(testSong, easyConfig);

        assert(chart1.playableEvents.size() == chart2.playableEvents.size());
        for (size_t i = 0; i < chart1.playableEvents.size(); ++i) {
            assert(chart1.playableEvents[i].onset == chart2.playableEvents[i].onset);
            assert(chart1.playableEvents[i].keys == chart2.playableEvents[i].keys);
        }
    }

    std::cout << "[TEST] Validando Mapeamento Posicional RF13 (teclas válidas 'A'-'Z')\n";
    {
        Chart chart = generator.generateChart(testSong, expertConfig);
        for (const auto& ev : chart.playableEvents) {
            for (char k : ev.keys) {
                assert(k >= 'A' && k <= 'Z');
            }
        }
    }

    std::cout << "\n>>> TODOS OS CASOS DE TESTE DO CHARTGENERATOR (TC09–TC12) PASSARAM! <<<\n";
    return 0;
}
