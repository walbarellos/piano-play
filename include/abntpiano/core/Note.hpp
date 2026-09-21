#pragma once

#include <string>
#include <array>
#include <cmath>

namespace abntpiano {

struct Note {
    int midi = 60;
    double frequency = 261.6256;
    std::string name = "C4";
};

inline double midiToFrequency(int midi) {
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

inline std::string midiToName(int midi) {
    static constexpr std::array<const char*, 12> kNoteNames = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    int noteIndex = (midi % 12 + 12) % 12;
    int octave = (midi / 12) - 1;
    return std::string(kNoteNames[noteIndex]) + std::to_string(octave);
}

inline Note makeNote(int midi) {
    return Note{
        .midi = midi,
        .frequency = midiToFrequency(midi),
        .name = midiToName(midi)
    };
}

} // namespace abntpiano
