#pragma once

#include "abntpiano/Song.hpp"
#include "abntpiano/JudgementEngine.hpp" // DifficultyConfig
#include <string>
#include <vector>

namespace abntpiano {

// Evento jogável contendo as teclas físicas mapeadas (RF12, RF13, dominio.md)
struct PlayableChordGroup {
    double onset = 0.0;
    std::vector<char> keys; // Subconjunto de 'A'-'Z' (RF02, RF13)
    std::vector<double> durations; // Duração em segundos de cada tecla (para notas sustentadas/hold)
    ChordGroup originalChordGroup;
};

// Estrutura do Chart jogável para uma determinada dificuldade (RF12)
struct Chart {
    std::string songId;
    DifficultyConfig difficulty;
    std::vector<PlayableChordGroup> playableEvents;
};

} // namespace abntpiano
