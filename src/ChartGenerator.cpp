#include "abntpiano/ChartGenerator.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace abntpiano {

namespace {

// Tabela direta: posição de leitura (0..25) -> caractere 'A'-'Z'
// Q W E R T Y U I O P (0..9)
// A S D F G H J K L   (10..18)
// Z X C V B N M       (19..25)
constexpr const char* kReadingOrder = "QWERTYUIOPASDFGHJKLZXCVBNM";

// Encontra a melhor tecla para uma nota MIDI dentro do range das 26 teclas (RF13)
// minimizando a distância física em relação à última tecla tocada.
char mapMidiToBestKey(int midiNote, int baseMidi, int& lastReadingPos) {
    // Range das 26 teclas: baseMidi até baseMidi + 25
    int minRange = baseMidi;
    int maxRange = baseMidi + 25;

    // Preserva o tom e a oitava autênticos da partitura dentro do range das 26 teclas
    int candidateMidi = midiNote;
    while (candidateMidi < minRange) {
        candidateMidi += 12;
    }
    while (candidateMidi > maxRange) {
        candidateMidi -= 12;
    }

    int bestReadingPos = candidateMidi - minRange;
    if (bestReadingPos < 0 || bestReadingPos > 25) {
        bestReadingPos = (midiNote % 12 + 12) % 12;
    }

    lastReadingPos = bestReadingPos;
    return kReadingOrder[bestReadingPos];
}

} // namespace

Chart ChartGenerator::generateChart(
    const Song& song,
    const DifficultyConfig& difficulty
) const {
    Chart chart{
        .songId = song.id,
        .difficulty = difficulty,
        .playableEvents = {}
    };

    if (song.chordGroups.empty()) {
        return chart;
    }

    const auto& originalGroups = song.chordGroups;
    size_t totalOriginal = originalGroups.size();

    // Seleção de grupos conforme noteDensityFactor (RF14, ADR-03)
    std::vector<size_t> selectedIndices;
    if (difficulty.noteDensityFactor >= 1.0f || totalOriginal <= 1) {
        selectedIndices.resize(totalOriginal);
        for (size_t i = 0; i < totalOriginal; ++i) selectedIndices[i] = i;
    } else {
        size_t targetCount = std::max<size_t>(
            1,
            static_cast<size_t>(std::round(static_cast<float>(totalOriginal) * difficulty.noteDensityFactor))
        );
        targetCount = std::min(targetCount, totalOriginal);

        if (targetCount == totalOriginal) {
            for (size_t i = 0; i < totalOriginal; ++i) selectedIndices.push_back(i);
        } else if (targetCount == 1) {
            selectedIndices.push_back(0);
        } else {
            // Amostragem rítmica uniforme determinística ao longo da peça
            for (size_t k = 0; k < targetCount; ++k) {
                size_t idx = static_cast<size_t>(
                    std::round(static_cast<double>(k) * static_cast<double>(totalOriginal - 1) /
                               static_cast<double>(targetCount - 1))
                );
                if (selectedIndices.empty() || selectedIndices.back() != idx) {
                    selectedIndices.push_back(idx);
                }
            }
        }
    }

    int lastReadingPos = 12; // Posição inicial no centro do teclado (ex: 'D' / home row)
    chart.playableEvents.reserve(selectedIndices.size());

    int baseMidi = 60 + mapper_.octaveOffset();

    for (size_t idx : selectedIndices) {
        const auto& group = originalGroups[idx];
        auto notes = group.noteEvents;

        // Redução de tamanho de acorde se exceder maxChordSize (RF14)
        if (difficulty.maxChordSize > 0 && notes.size() > static_cast<size_t>(difficulty.maxChordSize)) {
            // Prioriza notas de maior duração e maior velocity (ADR-03)
            std::sort(notes.begin(), notes.end(), [](const NoteEvent& a, const NoteEvent& b) {
                double scoreA = a.duration * static_cast<double>(a.velocity);
                double scoreB = b.duration * static_cast<double>(b.velocity);
                if (std::abs(scoreA - scoreB) > 1e-4) {
                    return scoreA > scoreB;
                }
                return a.midiNote < b.midiNote;
            });
            notes.resize(difficulty.maxChordSize);
        }

        // Mapeia notas selecionadas para as teclas físicas e armazena suas durações
        std::vector<char> mappedKeys;
        std::vector<double> mappedDurations;
        for (const auto& note : notes) {
            char key = mapMidiToBestKey(note.midiNote, baseMidi, lastReadingPos);
            // Evita teclas duplicadas no mesmo acorde (ex.: oitavas dobradas)
            auto it = std::find(mappedKeys.begin(), mappedKeys.end(), key);
            if (it == mappedKeys.end()) {
                mappedKeys.push_back(key);
                mappedDurations.push_back(std::max(0.15, note.duration));
            } else {
                size_t existingIdx = static_cast<size_t>(std::distance(mappedKeys.begin(), it));
                mappedDurations[existingIdx] = std::max(mappedDurations[existingIdx], note.duration);
            }
        }

        chart.playableEvents.push_back(PlayableChordGroup{
            .onset = group.onset,
            .keys = std::move(mappedKeys),
            .durations = std::move(mappedDurations),
            .originalChordGroup = group
        });
    }

    return chart;
}

} // namespace abntpiano
