#include "abntpiano/FreePlayController.hpp"
#include "abntpiano/SongModeController.hpp"
#include "abntpiano/game/DemoPlayer.hpp"
#include "abntpiano/core/HoldState.hpp"
#include <cassert>
#include <iostream>

using namespace abntpiano;

int main() {
    std::cout << "[TEST] Validando FreePlayController (RF01, RF02, RF06, RF07)\n";
    {
        FreePlayController controller;
        int noteOnCount = 0;
        int noteOffCount = 0;
        std::string lastNote;

        controller.setNoteOnCallback([&](const Note& n) {
            noteOnCount++;
            lastNote = n.name;
        });
        controller.setNoteOffCallback([&](const Note&) {
            noteOffCount++;
        });

        // Pressiona 'Q' -> nota C4 (MIDI 60)
        bool handled = controller.onKeyDown('Q');
        assert(handled);
        assert(noteOnCount == 1);
        assert(lastNote == "C4");
        assert(controller.activeKeys().count('Q') == 1);

        // Solta 'Q'
        controller.onKeyUp('Q');
        assert(noteOffCount == 1);
        assert(controller.activeKeys().count('Q') == 0);

        // Oitava para cima (RF06)
        controller.shiftOctaveUp();
        controller.onKeyDown('q');
        assert(lastNote == "C5");

        // Tecla não mapeada (RF02)
        assert(!controller.onKeyDown('@'));
    }

    std::cout << "[TEST] Validando SongModeController (RF16, RF17, RF19, UC05)\n";
    {
        DifficultyConfig normalConfig{
            .name = "Normal",
            .hitWindow = {80.0, 130.0, 180.0, 180.0},
            .maxChordSize = 3,
            .noteDensityFactor = 1.0f,
            .allowPartialChord = false,
            .partialChordThreshold = 0.66f
        };

        Chart chart{
            .songId = "song_test",
            .difficulty = normalConfig,
            .playableEvents = {
                PlayableChordGroup{.onset = 1.0, .keys = {'Q'}, .originalChordGroup = {}},
                PlayableChordGroup{.onset = 2.0, .keys = {'W'}, .originalChordGroup = {}}
            }
        };

        SongModeController controller(chart);
        assert(controller.state() == SongState::Ready);

        int judgementsReceived = 0;
        Judgement lastJudgement;
        controller.setJudgementCallback([&](const Judgement& j, const PlayableChordGroup&) {
            judgementsReceived++;
            lastJudgement = j;
        });

        controller.start();
        assert(controller.state() == SongState::Playing);

        // Lookahead: a 0.0s, notas em 1.0s e 2.0s devem ser visíveis com lookahead de 2.5s
        auto visible = controller.getVisibleNotes(2.5);
        assert(visible.size() == 2);
        assert(visible[0].keys[0] == 'Q');

        // Avança até 1.0s e pressiona 'Q' exatamente no tempo
        controller.update(1.0);
        controller.onKeyDown('q');

        assert(judgementsReceived == 1);
        assert(lastJudgement.type == JudgementType::Perfect);

        // Pausa e retoma (RF19)
        controller.pause();
        assert(controller.state() == SongState::Paused);
        controller.update(0.5);
        assert(controller.playhead() == 1.0); // Não avança pausado
        controller.resume();
        assert(controller.state() == SongState::Playing);

        // Avança até 2.5s sem tocar a nota 'W' (que estava em 2.0s)
        // 2.5s > 2.0s + 0.180s -> deve expirar e disparar Miss automático (TC16)
        controller.update(1.5); // playhead vai para 2.5s
        assert(judgementsReceived == 2);
        assert(lastJudgement.type == JudgementType::Miss);

        // Finalização ao término da peça
        bool finishedCalled = false;
        controller.setFinishedCallback([&](const ExecutionSummary&) {
            finishedCalled = true;
        });
        controller.update(1.0); // playhead vai para 3.5s (> 2.0s + 1.0s)
        assert(controller.state() == SongState::Finished);
        assert(finishedCalled);
    }

    std::cout << "[TEST] Validando DemoPlayer (Zero Misses, suporte a acordes, sem colisao de teclas)\n";
    {
        DifficultyConfig normalConfig{
            .name = "Normal",
            .hitWindow = {80.0, 130.0, 180.0, 180.0},
            .maxChordSize = 3,
            .noteDensityFactor = 1.0f,
            .allowPartialChord = true,
            .partialChordThreshold = 0.50f
        };

        Chart chart{
            .songId = "demo_test_song",
            .difficulty = normalConfig,
            .playableEvents = {
                // Acorde com 2 teclas
                PlayableChordGroup{.onset = 0.5, .keys = {'Q', 'E'}, .durations = {0.4, 0.4}, .midiNotes = {60, 64}},
                // Nota individual rápida logo após
                PlayableChordGroup{.onset = 0.8, .keys = {'W'}, .durations = {0.2}, .midiNotes = {62}},
                // Acorde com 3 teclas
                PlayableChordGroup{.onset = 1.2, .keys = {'A', 'S', 'D'}, .durations = {0.5, 0.5, 0.5}, .midiNotes = {65, 67, 69}}
            }
        };

        SongModeController controller(chart);
        controller.start();

        DemoPlayer demo;
        demo.reset(0.0);

        std::set<char> heldKeys;
        std::map<char, HoldState> holdStates;

        int missCount = 0;
        int hitCount = 0;
        controller.setJudgementCallback([&](const Judgement& j, const PlayableChordGroup&) {
            if (j.type == JudgementType::Miss) {
                missCount++;
            } else {
                hitCount++;
            }
        });

        // Simula passagem de frames de 0 a 2.0s em passos de 0.016s (~60 FPS)
        for (double t = 0.0; t <= 2.0; t += 0.016) {
            double delta = t - controller.playhead();
            if (delta > 0.0) controller.update(delta);
            demo.update(controller.playhead(), controller, heldKeys, holdStates);
        }

        assert(missCount == 0);
        assert(hitCount == 3);
        assert(controller.scoringEngine().maxCombo() == 3);
        assert(controller.scoringEngine().missCount() == 0);
        std::cout << "  Hits registrados: " << hitCount << ", Misses: " << missCount << " (PERFEITO!)\n";
    }

    std::cout << "\n>>> TODOS OS TESTES DOS CONTROLLERS PASSARAM! <<<\n";
    return 0;
}
