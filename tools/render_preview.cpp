// Renderiza offline um trecho do Chart (melodia + acompanhamento) para .wav.
// Uso: render_preview <arquivo.mid> <saida.wav> [segundos] [offsetSegundos]
#include "abntpiano/MidiImporter.hpp"
#include "abntpiano/ChartGenerator.hpp"
#include "abntpiano/SynthEngine.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

using namespace abntpiano;

namespace {

void writeWav(const std::string& path, const std::vector<float>& mono, int sampleRate) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    auto u16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
    uint32_t dataBytes = static_cast<uint32_t>(mono.size() * 2);
    f.write("RIFF", 4); u32(36 + dataBytes); f.write("WAVE", 4);
    f.write("fmt ", 4); u32(16); u16(1); u16(1);
    u32(static_cast<uint32_t>(sampleRate));
    u32(static_cast<uint32_t>(sampleRate * 2));
    u16(2); u16(16);
    f.write("data", 4); u32(dataBytes);
    for (float s : mono) {
        int v = static_cast<int>(std::clamp(s, -1.0f, 1.0f) * 32767.0f);
        u16(static_cast<uint16_t>(static_cast<int16_t>(v)));
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "uso: render_preview <mid> <wav> [seg] [offset]\n"); return 1; }
    const double length = (argc > 3) ? std::atof(argv[3]) : 45.0;
    const double offset = (argc > 4) ? std::atof(argv[4]) : 0.0;
    const int sr = 44100;

    MidiImporter imp;
    auto res = imp.importFromFile(argv[1]);
    if (!res.success) { std::fprintf(stderr, "erro: %s\n", res.errorMessage.c_str()); return 1; }

    DifficultyConfig diff{
        .name = "Easy", .hitWindow = {140.0, 200.0, 280.0, 280.0},
        .maxChordSize = 1, .noteDensityFactor = 1.0f,
        .allowPartialChord = true, .partialChordThreshold = 0.33f
    };
    Chart chart = ChartGenerator().generateChart(res.song, diff);

    // Agenda tudo em tempo absoluto de áudio: mesmo caminho do jogo.
    SynthEngine synth(sr);
    for (const auto& g : chart.playableEvents) {
        if (g.onset < offset || g.onset > offset + length) continue;
        for (size_t i = 0; i < g.midiNotes.size(); ++i) {
            synth.scheduleNoteOn (g.onset - offset, g.midiNotes[i], 0.92f);
            synth.scheduleNoteOff(g.onset - offset + std::max(0.12, g.durations[i]), g.midiNotes[i]);
        }
    }
    for (const auto& b : chart.backingNotes) {
        if (b.onset < offset || b.onset > offset + length) continue;
        float vel = std::clamp(b.velocity / 127.0f, 0.05f, 1.0f) * 0.55f;
        synth.scheduleNoteOn (b.onset - offset, b.midiNote, vel);
        synth.scheduleNoteOff(b.onset - offset + std::max(0.08, b.duration), b.midiNote);
    }

    const size_t block = 512;
    std::vector<float> buf(block);
    std::vector<float> out;
    out.reserve(static_cast<size_t>(length * sr));
    for (size_t n = 0; n < static_cast<size_t>(length * sr); n += block) {
        synth.render(buf.data(), block);
        out.insert(out.end(), buf.begin(), buf.end());
    }

    writeWav(argv[2], out, sr);
    std::printf("%s -> %s  (%.1fs, melodia=%zu, backing=%zu)\n",
                argv[1], argv[2], length,
                chart.playableEvents.size(), chart.backingNotes.size());
    return 0;
}
