#include "abntpiano/MidiImporter.hpp"
#include <fstream>
#include <algorithm>
#include <map>
#include <cmath>

namespace abntpiano {

namespace {

// Leitura segura de inteiros big-endian
bool readUint16(const uint8_t*& ptr, const uint8_t* end, uint16_t& out) {
    if (ptr + 2 > end) return false;
    out = static_cast<uint16_t>((ptr[0] << 8) | ptr[1]);
    ptr += 2;
    return true;
}

bool readUint32(const uint8_t*& ptr, const uint8_t* end, uint32_t& out) {
    if (ptr + 4 > end) return false;
    out = (static_cast<uint32_t>(ptr[0]) << 24) |
          (static_cast<uint32_t>(ptr[1]) << 16) |
          (static_cast<uint32_t>(ptr[2]) << 8)  |
          static_cast<uint32_t>(ptr[3]);
    ptr += 4;
    return true;
}

// Leitura de Variable Length Quantity (VLQ)
bool readVLQ(const uint8_t*& ptr, const uint8_t* end, uint32_t& value) {
    value = 0;
    for (int i = 0; i < 4; ++i) {
        if (ptr >= end) return false;
        uint8_t byte = *ptr++;
        value = (value << 7) | (byte & 0x7F);
        if (!(byte & 0x80)) return true;
    }
    return false;
}

struct TempoPoint {
    uint32_t tick = 0;
    uint32_t usPerQuarter = 500000; // 120 BPM padrão
    double timeSeconds = 0.0;
};

struct RawNoteOn {
    uint32_t startTick = 0;
    uint8_t velocity = 64;
    int trackIndex = 0;
};

} // namespace

ImportResult MidiImporter::importFromFile(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return ImportResult{
            .success = false,
            .errorMessage = "Não foi possível abrir o arquivo: " + filepath,
            .song = {}
        };
    }

    std::vector<uint8_t> buffer(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    // Extrai nome base do arquivo para ID se necessário
    std::string filename = filepath;
    auto pos = filename.find_last_of("/\\");
    if (pos != std::string::npos) {
        filename = filename.substr(pos + 1);
    }
    auto dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        filename = filename.substr(0, dot);
    }

    return importFromBytes(buffer, filename);
}

ImportResult MidiImporter::importFromBytes(
    const std::vector<uint8_t>& bytes,
    const std::string& songId
) const {
    const uint8_t* ptr = bytes.data();
    const uint8_t* end = ptr + bytes.size();

    // Validação mínima de cabeçalho
    if (bytes.size() < 14) {
        return ImportResult{
            .success = false,
            .errorMessage = "Arquivo MIDI truncado ou inválido (menos de 14 bytes)",
            .song = {}
        };
    }

    // Checa 'MThd'
    if (ptr[0] != 'M' || ptr[1] != 'T' || ptr[2] != 'h' || ptr[3] != 'd') {
        return ImportResult{
            .success = false,
            .errorMessage = "Assinatura 'MThd' não encontrada",
            .song = {}
        };
    }
    ptr += 4;

    uint32_t headerLength = 0;
    if (!readUint32(ptr, end, headerLength) || headerLength < 6) {
        return ImportResult{
            .success = false,
            .errorMessage = "Tamanho de cabeçalho MThd inválido",
            .song = {}
        };
    }

    uint16_t format = 0;
    uint16_t numTracks = 0;
    uint16_t divisionRaw = 0;
    if (!readUint16(ptr, end, format) ||
        !readUint16(ptr, end, numTracks) ||
        !readUint16(ptr, end, divisionRaw)) {
        return ImportResult{
            .success = false,
            .errorMessage = "Falha ao ler parâmetros do cabeçalho MIDI",
            .song = {}
        };
    }

    // Se o header tiver mais de 6 bytes (extensões), pula o restante
    if (headerLength > 6) {
        ptr += (headerLength - 6);
    }

    if (numTracks == 0) {
        return ImportResult{
            .success = false,
            .errorMessage = "Arquivo MIDI não contém tracks (numTracks = 0)",
            .song = {}
        };
    }

    double ticksPerQuarter = 0.0;
    if (divisionRaw & 0x8000) {
        // SMPTE - formato raro em SMF para jogos, aproximação
        int fps = -(static_cast<int8_t>((divisionRaw >> 8) & 0xFF));
        int subframes = divisionRaw & 0xFF;
        ticksPerQuarter = static_cast<double>(fps * subframes);
    } else {
        ticksPerQuarter = static_cast<double>(divisionRaw & 0x7FFF);
    }

    if (ticksPerQuarter <= 0.0) {
        ticksPerQuarter = 480.0; // fallback seguro
    }

    // Coletor de eventos intermediários
    struct RawNoteEvent {
        uint32_t startTick;
        uint32_t endTick;
        int midiNote;
        uint8_t velocity;
        int voice;
    };
    std::vector<RawNoteEvent> rawNotes;
    std::vector<TempoPoint> tempoPoints;
    std::string songTitle;
    std::string songComposer;

    // Parser das Tracks
    for (int t = 0; t < numTracks && ptr < end; ++t) {
        if (ptr + 8 > end) break;

        // Procura 'MTrk'
        if (ptr[0] != 'M' || ptr[1] != 'T' || ptr[2] != 'r' || ptr[3] != 'k') {
            // Se não for MTrk, pode ser chunk desconhecido, lê comprimento e pula
            ptr += 4;
            uint32_t skipLen = 0;
            if (readUint32(ptr, end, skipLen)) {
                ptr += skipLen;
                continue;
            }
            break;
        }
        ptr += 4;

        uint32_t trackLength = 0;
        if (!readUint32(ptr, end, trackLength)) break;

        const uint8_t* trackEnd = ptr + trackLength;
        if (trackEnd > end) {
            trackEnd = end; // Truncado suavemente sem crash (RF11)
        }

        uint32_t currentTick = 0;
        uint8_t runningStatus = 0;

        // Notas ativas na track: chave = (channel << 8) | midiNote
        std::map<uint16_t, RawNoteOn> activeNotes;

        while (ptr < trackEnd) {
            uint32_t deltaTicks = 0;
            if (!readVLQ(ptr, trackEnd, deltaTicks)) break;
            currentTick += deltaTicks;

            if (ptr >= trackEnd) break;

            uint8_t status = *ptr;
            if (status & 0x80) {
                ptr++;
                runningStatus = status;
            } else {
                status = runningStatus;
                if (!(status & 0x80)) {
                    // Running status inválido, aborta track
                    break;
                }
            }

            uint8_t eventType = status & 0xF0;
            uint8_t channel = status & 0x0F;

            if (status == 0xFF) {
                // Meta Event
                runningStatus = 0;
                if (ptr >= trackEnd) break;
                uint8_t metaType = *ptr++;
                uint32_t metaLen = 0;
                if (!readVLQ(ptr, trackEnd, metaLen)) break;

                const uint8_t* metaData = ptr;
                ptr += metaLen;
                if (ptr > trackEnd) ptr = trackEnd;

                if (metaType == 0x01) { // Text / Comentário
                    std::string text(reinterpret_cast<const char*>(metaData), metaLen);
                    if (songComposer.empty()) songComposer = text;
                } else if (metaType == 0x03) { // Track Name / Título
                    std::string text(reinterpret_cast<const char*>(metaData), metaLen);
                    if (songTitle.empty() && !text.empty()) songTitle = text;
                } else if (metaType == 0x51 && metaLen == 3) { // Set Tempo
                    uint32_t us = (static_cast<uint32_t>(metaData[0]) << 16) |
                                  (static_cast<uint32_t>(metaData[1]) << 8)  |
                                  static_cast<uint32_t>(metaData[2]);
                    tempoPoints.push_back(TempoPoint{currentTick, us, 0.0});
                }
            } else if (status == 0xF0 || status == 0xF7) {
                // Sysex
                runningStatus = 0;
                uint32_t sysexLen = 0;
                if (!readVLQ(ptr, trackEnd, sysexLen)) break;
                ptr += sysexLen;
                if (ptr > trackEnd) ptr = trackEnd;
            } else if (eventType == 0x80 || (eventType == 0x90 && ptr < trackEnd && ptr[1] == 0)) {
                // Note Off (ou Note On com velocity 0)
                if (ptr + 2 > trackEnd) break;
                uint8_t key = ptr[0];
                ptr += 2; // consome key e velocity

                uint16_t noteKey = (static_cast<uint16_t>(channel) << 8) | key;
                auto it = activeNotes.find(noteKey);
                if (it != activeNotes.end()) {
                    rawNotes.push_back(RawNoteEvent{
                        .startTick = it->second.startTick,
                        .endTick = currentTick,
                        .midiNote = key,
                        .velocity = it->second.velocity,
                        .voice = it->second.trackIndex
                    });
                    activeNotes.erase(it);
                }
            } else if (eventType == 0x90) {
                // Note On (velocity > 0)
                if (ptr + 2 > trackEnd) break;
                uint8_t key = ptr[0];
                uint8_t vel = ptr[1];
                ptr += 2;

                uint16_t noteKey = (static_cast<uint16_t>(channel) << 8) | key;
                // Se já havia uma nota aberta na mesma tecla, fecha antes de abrir nova
                auto it = activeNotes.find(noteKey);
                if (it != activeNotes.end()) {
                    rawNotes.push_back(RawNoteEvent{
                        .startTick = it->second.startTick,
                        .endTick = currentTick,
                        .midiNote = key,
                        .velocity = it->second.velocity,
                        .voice = it->second.trackIndex
                    });
                }
                activeNotes[noteKey] = RawNoteOn{
                    .startTick = currentTick,
                    .velocity = vel,
                    .trackIndex = t
                };
            } else if (eventType == 0xC0 || eventType == 0xD0) {
                // Eventos de 1 byte de dados
                if (ptr < trackEnd) ptr++;
            } else {
                // Eventos de 2 bytes de dados (0xA0, 0xB0, 0xE0)
                if (ptr + 2 <= trackEnd) ptr += 2;
                else ptr = trackEnd;
            }
        }

        // Fecha notas que ficaram abertas no final da track
        for (const auto& [noteKey, onInfo] : activeNotes) {
            uint8_t key = noteKey & 0xFF;
            rawNotes.push_back(RawNoteEvent{
                .startTick = onInfo.startTick,
                .endTick = currentTick,
                .midiNote = key,
                .velocity = onInfo.velocity,
                .voice = onInfo.trackIndex
            });
        }

        ptr = trackEnd;
    }

    // Constrói o Mapa de Tempo ordenado
    std::sort(tempoPoints.begin(), tempoPoints.end(), [](const TempoPoint& a, const TempoPoint& b) {
        return a.tick < b.tick;
    });

    if (tempoPoints.empty() || tempoPoints.front().tick != 0) {
        tempoPoints.insert(tempoPoints.begin(), TempoPoint{0, 500000, 0.0});
    }

    // Calcula tempo acumulado em segundos para cada ponto de tempo
    for (size_t i = 1; i < tempoPoints.size(); ++i) {
        uint32_t deltaTicks = tempoPoints[i].tick - tempoPoints[i - 1].tick;
        double seconds = static_cast<double>(deltaTicks) *
            (static_cast<double>(tempoPoints[i - 1].usPerQuarter) / (1000000.0 * ticksPerQuarter));
        tempoPoints[i].timeSeconds = tempoPoints[i - 1].timeSeconds + seconds;
    }

    auto tickToSeconds = [&](uint32_t tick) -> double {
        // Encontra o último ponto de tempo com ponto.tick <= tick
        size_t idx = 0;
        for (size_t i = 1; i < tempoPoints.size(); ++i) {
            if (tempoPoints[i].tick <= tick) {
                idx = i;
            } else {
                break;
            }
        }
        uint32_t deltaTicks = tick - tempoPoints[idx].tick;
        double seconds = static_cast<double>(deltaTicks) *
            (static_cast<double>(tempoPoints[idx].usPerQuarter) / (1000000.0 * ticksPerQuarter));
        return tempoPoints[idx].timeSeconds + seconds;
    };

    // Converte rawNotes para NoteEvent normalizados
    std::vector<NoteEvent> noteEvents;
    noteEvents.reserve(rawNotes.size());
    for (const auto& r : rawNotes) {
        double startSec = tickToSeconds(r.startTick);
        double endSec = tickToSeconds(r.endTick);
        double duration = std::max(0.02, endSec - startSec);

        noteEvents.push_back(NoteEvent{
            .timestamp = startSec,
            .midiNote = r.midiNote,
            .duration = duration,
            .velocity = r.velocity,
            .voice = r.voice
        });
    }

    // Ordena noteEvents por timestamp crescente
    std::sort(noteEvents.begin(), noteEvents.end(), [](const NoteEvent& a, const NoteEvent& b) {
        if (std::abs(a.timestamp - b.timestamp) < 1e-5) {
            return a.midiNote < b.midiNote;
        }
        return a.timestamp < b.timestamp;
    });

    // Agrupamento em ChordGroups conforme RNF03 (janela de simultaneidade)
    std::vector<ChordGroup> chordGroups;
    for (const auto& note : noteEvents) {
        if (chordGroups.empty()) {
            chordGroups.push_back(ChordGroup{
                .onset = note.timestamp,
                .noteEvents = {note}
            });
        } else {
            double deltaMs = (note.timestamp - chordGroups.back().onset) * 1000.0;
            if (deltaMs <= config_.simultaneityWindowMs) {
                chordGroups.back().noteEvents.push_back(note);
            } else {
                chordGroups.push_back(ChordGroup{
                    .onset = note.timestamp,
                    .noteEvents = {note}
                });
            }
        }
    }

    // Duração total da música
    double maxEndTime = 0.0;
    for (const auto& n : noteEvents) {
        maxEndTime = std::max(maxEndTime, n.timestamp + n.duration);
    }

    double initialBpm = 60.0 / (static_cast<double>(tempoPoints.front().usPerQuarter) / 1000000.0);

    Song song{
        .id = songId.empty() ? "song_01" : songId,
        .title = songTitle.empty() ? (songId.empty() ? "Untitled" : songId) : songTitle,
        .composer = songComposer.empty() ? "Unknown" : songComposer,
        .sourceFormat = SourceFormat::MIDI,
        .bpm = initialBpm,
        .durationSeconds = maxEndTime,
        .chordGroups = std::move(chordGroups)
    };

    return ImportResult{
        .success = true,
        .errorMessage = "",
        .song = std::move(song)
    };
}

} // namespace abntpiano
