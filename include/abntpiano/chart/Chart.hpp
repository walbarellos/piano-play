#pragma once

#include "abntpiano/Song.hpp"
#include "abntpiano/JudgementEngine.hpp" // DifficultyConfig
#include <cstdint>
#include <string>
#include <vector>

namespace abntpiano {

// Evento jogável contendo as teclas físicas mapeadas (RF12, RF13, dominio.md)
struct PlayableChordGroup {
    double onset = 0.0;
    std::vector<char> keys;        // Subconjunto de 'A'-'Z' (RF02, RF13)
    std::vector<double> durations; // Duração em segundos de cada tecla
    std::vector<int> midiNotes;    // Pitch REAL da partitura, paralelo a keys/durations.
                                   // A tecla é só o alvo de input; o áudio usa este pitch.
    ChordGroup originalChordGroup;
};

// Nota de acompanhamento: não é jogável, é renderizada pelo synth para que a peça
// soe completa (mão esquerda, vozes internas, notas descartadas pela dificuldade).
struct BackingNote {
    double onset = 0.0;
    double duration = 0.0;
    int midiNote = 60;
    uint8_t velocity = 64;
    int voice = 0;
};

// Estrutura do Chart jogável para uma determinada dificuldade (RF12)
struct Chart {
    std::string songId;
    DifficultyConfig difficulty;
    std::vector<PlayableChordGroup> playableEvents;
    std::vector<BackingNote> backingNotes; // ordenado por onset
    int melodyVoice = -1;
};

} // namespace abntpiano
