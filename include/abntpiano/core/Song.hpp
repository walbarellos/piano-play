#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace abntpiano {

enum class SourceFormat {
    MIDI,
    MusicXML
};

// Evento musical bruto conforme modelo de domínio (dominio.md / RF09)
struct NoteEvent {
    double timestamp = 0.0;  // segundos, relativo ao início da Song
    int midiNote = 60;
    double duration = 0.0;   // segundos
    uint8_t velocity = 64;
    int voice = 0;           // canal ou track original (0 = indefinido)
};

// Conjunto de notas com onsets simultâneos (dentro da janela RNF03)
struct ChordGroup {
    double onset = 0.0;      // segundos do início do grupo
    std::vector<NoteEvent> noteEvents;
};

// Representação normalizada de uma peça musical (RF09)
struct Song {
    std::string id;
    std::string title;
    std::string composer;
    SourceFormat sourceFormat = SourceFormat::MIDI;
    double bpm = 120.0;
    double durationSeconds = 0.0;
    std::vector<ChordGroup> chordGroups;
};

} // namespace abntpiano
