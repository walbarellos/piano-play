#include "abntpiano/ChartGenerator.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace abntpiano {

namespace {

// Tabela direta: posição de leitura (0..25) -> caractere 'A'-'Z'
// Q W E R T Y U I O P (0..9) / A S D F G H J K L (10..18) / Z X C V B N M (19..25)
constexpr const char* kReadingOrder = "QWERTYUIOPASDFGHJKLZXCVBNM";

// Dobra o pitch para dentro do range das 26 teclas apenas para escolher a TECLA.
// O pitch original é preservado separadamente em PlayableChordGroup::midiNotes.
char mapMidiToKey(int midiNote, int baseMidi) {
    int folded = midiNote;
    while (folded < baseMidi)      folded += 12;
    while (folded > baseMidi + 25) folded -= 12;
    int pos = folded - baseMidi;
    if (pos < 0 || pos > 25) pos = ((midiNote % 12) + 12) % 12;
    return kReadingOrder[pos];
}

// Detecta a voz melódica: entre as vozes (tracks) com participação relevante,
// a de maior pitch médio. Funciona tanto para piano ("Piano right" vs "Piano left")
// quanto para sonatas violino+piano (violino = voz mais aguda).
int detectMelodyVoice(const Song& song) {
    std::map<int, std::pair<long long, long long>> stats; // voice -> {somaPitch, count}
    long long total = 0;
    for (const auto& g : song.chordGroups) {
        for (const auto& n : g.noteEvents) {
            auto& s = stats[n.voice];
            s.first  += n.midiNote;
            s.second += 1;
            ++total;
        }
    }
    if (stats.size() <= 1 || total == 0) return -1;

    int best = -1;
    double bestMean = -1.0;
    for (const auto& [voice, s] : stats) {
        if (s.second * 10 < total) continue; // ignora vozes marginais (<10% das notas)
        double mean = static_cast<double>(s.first) / static_cast<double>(s.second);
        if (mean > bestMean) { bestMean = mean; best = voice; }
    }
    return best;
}

} // namespace

Chart ChartGenerator::generateChart(
    const Song& song,
    const DifficultyConfig& difficulty
) const {
    Chart chart{
        .songId = song.id,
        .difficulty = difficulty,
        .playableEvents = {},
        .backingNotes = {},
        .melodyVoice = -1
    };

    if (song.chordGroups.empty()) return chart;

    const int melodyVoice = detectMelodyVoice(song);
    chart.melodyVoice = melodyVoice;
    const int baseMidi = 60 + mapper_.octaveOffset();

    // Amostragem por dificuldade: grupos NÃO selecionados não somem da música,
    // eles caem inteiros no acompanhamento. A peça sempre soa completa.
    const size_t totalGroups = song.chordGroups.size();
    std::vector<bool> isPlayable(totalGroups, true);
    if (difficulty.noteDensityFactor < 1.0f && totalGroups > 1) {
        const double keep = std::clamp(static_cast<double>(difficulty.noteDensityFactor), 0.05, 1.0);
        double acc = 0.0;
        for (size_t i = 0; i < totalGroups; ++i) {
            acc += keep;
            if (acc >= 1.0) { acc -= 1.0; isPlayable[i] = true; }
            else            { isPlayable[i] = false; }
        }
    }

    chart.playableEvents.reserve(totalGroups);

    auto pushBacking = [&](const NoteEvent& n) {
        chart.backingNotes.push_back(BackingNote{
            .onset = n.timestamp,
            .duration = std::max(0.05, n.duration),
            .midiNote = n.midiNote,
            .velocity = n.velocity,
            .voice = n.voice
        });
    };

    for (size_t gi = 0; gi < totalGroups; ++gi) {
        const auto& group = song.chordGroups[gi];

        // 1) Separa melodia x acompanhamento pela voz detectada.
        std::vector<NoteEvent> melody;
        std::vector<NoteEvent> backing;
        for (const auto& n : group.noteEvents) {
            if (melodyVoice < 0 || n.voice == melodyVoice) melody.push_back(n);
            else                                           backing.push_back(n);
        }

        // 2) Grupo não selecionado pela dificuldade → tudo vira acompanhamento.
        if (!isPlayable[gi] || melody.empty()) {
            for (const auto& n : group.noteEvents) pushBacking(n);
            continue;
        }

        // 3) Dentro da melodia, skyline: voz superior primeiro (é a linha cantável).
        std::sort(melody.begin(), melody.end(), [](const NoteEvent& a, const NoteEvent& b) {
            if (a.midiNote != b.midiNote) return a.midiNote > b.midiNote;
            return a.duration > b.duration;
        });

        size_t maxKeep = melody.size();
        if (difficulty.maxChordSize > 0) {
            maxKeep = std::min(maxKeep, static_cast<size_t>(difficulty.maxChordSize));
        }

        // Notas melódicas excedentes NÃO são apagadas: vão para o acompanhamento.
        for (size_t i = maxKeep; i < melody.size(); ++i) backing.push_back(melody[i]);
        melody.resize(maxKeep);

        std::vector<char>   mappedKeys;
        std::vector<double> mappedDurations;
        std::vector<int>    mappedMidi;
        for (const auto& note : melody) {
            char key = mapMidiToKey(note.midiNote, baseMidi);
            auto it = std::find(mappedKeys.begin(), mappedKeys.end(), key);
            if (it == mappedKeys.end()) {
                mappedKeys.push_back(key);
                mappedDurations.push_back(std::max(0.12, note.duration));
                mappedMidi.push_back(note.midiNote);
            } else {
                // Colisão de tecla (mesma classe de altura em oitavas diferentes):
                // mantém a mais longa como alvo e manda a outra para o acompanhamento.
                size_t idx = static_cast<size_t>(std::distance(mappedKeys.begin(), it));
                if (note.duration > mappedDurations[idx]) {
                    backing.push_back(NoteEvent{
                        .timestamp = group.onset,
                        .midiNote = mappedMidi[idx],
                        .duration = mappedDurations[idx],
                        .velocity = note.velocity,
                        .voice = note.voice
                    });
                    mappedDurations[idx] = note.duration;
                    mappedMidi[idx] = note.midiNote;
                } else {
                    backing.push_back(note);
                }
            }
        }

        for (const auto& n : backing) pushBacking(n);

        chart.playableEvents.push_back(PlayableChordGroup{
            .onset = group.onset,
            .keys = std::move(mappedKeys),
            .durations = std::move(mappedDurations),
            .midiNotes = std::move(mappedMidi),
            .originalChordGroup = group
        });
    }

    std::sort(chart.backingNotes.begin(), chart.backingNotes.end(),
              [](const BackingNote& a, const BackingNote& b) { return a.onset < b.onset; });

    return chart;
}

} // namespace abntpiano
