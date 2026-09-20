#include "abntpiano/MidiImporter.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace abntpiano;

// Helper para montar cabeçalho MThd + MTrk de formato 0
std::vector<uint8_t> makeMidiFile(uint16_t division, const std::vector<uint8_t>& trackEvents) {
    std::vector<uint8_t> midi;

    // Header Chunk (14 bytes)
    midi.push_back('M'); midi.push_back('T'); midi.push_back('h'); midi.push_back('d');
    midi.push_back(0); midi.push_back(0); midi.push_back(0); midi.push_back(6); // header len = 6
    midi.push_back(0); midi.push_back(0); // format 0
    midi.push_back(0); midi.push_back(1); // 1 track
    midi.push_back(static_cast<uint8_t>(division >> 8));
    midi.push_back(static_cast<uint8_t>(division & 0xFF));

    // Track Chunk
    midi.push_back('M'); midi.push_back('T'); midi.push_back('r'); midi.push_back('k');
    uint32_t len = static_cast<uint32_t>(trackEvents.size() + 4); // + 4 para End of Track (00 FF 2F 00)
    midi.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
    midi.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
    midi.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    midi.push_back(static_cast<uint8_t>(len & 0xFF));

    // Events
    midi.insert(midi.end(), trackEvents.begin(), trackEvents.end());

    // End of Track meta-event
    midi.push_back(0x00); midi.push_back(0xFF); midi.push_back(0x2F); midi.push_back(0x00);

    return midi;
}

int main() {
    MidiImporter importer;

    std::cout << "[TEST] Executando TC05: MIDI monofônico -> Song com N ChordGroups de tamanho 1\n";
    {
        // Divisão: 480 ticks/beat. 120 BPM padrão -> 1 beat = 500 ms.
        // Nota 1 (C4 = 60): t=0 a t=240 (250 ms)
        // Nota 2 (D4 = 62): t=480 (500 ms) a t=720 (750 ms)
        std::vector<uint8_t> track = {
            // Delta=0: Note On (C4, vel 64)
            0x00, 0x90, 60, 64,
            // Delta=240 (VLQ: 0x81, 0x70): Note Off (C4)
            0x81, 0x70, 0x80, 60, 0,
            // Delta=240 (VLQ: 0x81, 0x70): Note On (D4, vel 64) -> tick 480
            0x81, 0x70, 0x90, 62, 64,
            // Delta=240: Note Off (D4) -> tick 720
            0x81, 0x70, 0x80, 62, 0
        };

        auto bytes = makeMidiFile(480, track);
        auto result = importer.importFromBytes(bytes, "mono_test");

        assert(result.success);
        assert(result.song.chordGroups.size() == 2);
        assert(result.song.chordGroups[0].noteEvents.size() == 1);
        assert(result.song.chordGroups[0].noteEvents[0].midiNote == 60);
        assert(result.song.chordGroups[1].noteEvents.size() == 1);
        assert(result.song.chordGroups[1].noteEvents[0].midiNote == 62);
        assert(result.song.chordGroups[0].onset < result.song.chordGroups[1].onset);
    }

    std::cout << "[TEST] Executando TC06: Acorde simultâneo (3 notas onset=0) -> 1 ChordGroup de tamanho 3\n";
    {
        // C4 (60), E4 (64), G4 (67) todos iniciam no tick 0 (delta=0)
        std::vector<uint8_t> track = {
            0x00, 0x90, 60, 64,
            0x00, 0x90, 64, 64,
            0x00, 0x90, 67, 64,
            // Delta=480: Note Off para as 3
            0x83, 0x60, 0x80, 60, 0,
            0x00, 0x80, 64, 0,
            0x00, 0x80, 67, 0
        };

        auto bytes = makeMidiFile(480, track);
        auto result = importer.importFromBytes(bytes, "chord_test");

        assert(result.success);
        assert(result.song.chordGroups.size() == 1);
        assert(result.song.chordGroups[0].noteEvents.size() == 3);
        assert(result.song.chordGroups[0].noteEvents[0].midiNote == 60);
        assert(result.song.chordGroups[0].noteEvents[1].midiNote == 64);
        assert(result.song.chordGroups[0].noteEvents[2].midiNote == 67);
    }

    std::cout << "[TEST] Executando TC07: Duas notas com onset a 40 ms (acima dos 30 ms) -> 2 ChordGroups\n";
    {
        // 480 ticks = 500 ms -> 1 tick = 1.04166 ms
        // 40 ticks = 41.66 ms > 30 ms da janela RNF03
        std::vector<uint8_t> track = {
            // Nota 1 no tick 0
            0x00, 0x90, 60, 64,
            // Nota 2 no tick 40 (delta=40)
            40, 0x90, 64, 64,
            // Note Off no tick 200 (delta=160 -> VLQ: 0x81, 0x20)
            0x81, 0x20, 0x80, 60, 0,
            0x00, 0x80, 64, 0
        };

        auto bytes = makeMidiFile(480, track);
        auto result = importer.importFromBytes(bytes, "gap_test");

        assert(result.success);
        assert(result.song.chordGroups.size() == 2);
        assert(result.song.chordGroups[0].noteEvents.size() == 1);
        assert(result.song.chordGroups[1].noteEvents.size() == 1);
    }

    std::cout << "[TEST] Executando TC08: Arquivo corrompido / truncado -> Erro reportado, sem crash (RF11)\n";
    {
        // 1. Buffer vazio
        auto res1 = importer.importFromBytes({}, "empty");
        assert(!res1.success);
        assert(!res1.errorMessage.empty());

        // 2. Assinatura inválida
        std::vector<uint8_t> invalidSig = {'B', 'A', 'D', '!', 0, 0, 0, 6, 0, 0, 0, 1, 1, 0xE0};
        auto res2 = importer.importFromBytes(invalidSig, "invalid_sig");
        assert(!res2.success);
        assert(!res2.errorMessage.empty());

        // 3. Arquivo inexistente no disco
        auto res3 = importer.importFromFile("/caminho/completamente/inexistente.mid");
        assert(!res3.success);
        assert(!res3.errorMessage.empty());
    }

    std::cout << "\n>>> TODOS OS CASOS DE TESTE DE IMPORTAÇÃO (TC05–TC08) PASSARAM! <<<\n";
    return 0;
}
