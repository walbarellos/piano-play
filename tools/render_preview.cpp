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

struct Ev { double t; int midi; float vel; bool on; };

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

    std::vector<Ev> evs;
    for (const auto& g : chart.playableEvents) {
        for (size_t i = 0; i < g.midiNotes.size(); ++i) {
            evs.push_back({g.onset, g.midiNotes[i], 0.95f, true});
            evs.push_back({g.onset + std::max(0.10, g.durations[i]), g.midiNotes[i], 0.0f, false});
        }
    }
    for (const auto& b : chart.backingNotes) {
        evs.push_back({b.onset, b.midiNote, std::clamp(b.velocity / 127.0f, 0.05f, 1.0f) * 0.60f, true});
        evs.push_back({b.onset + std::max(0.08, b.duration), b.midiNote, 0.0f, false});
    }
    std::sort(evs.begin(), evs.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });

    SynthEngine synth(sr);
    std::vector<float> out;
    out.reserve(static_cast<size_t>(length * sr));

    const size_t block = 64;
    std::vector<float> buf(block);
    size_t cursor = 0;
    while (cursor < evs.size() && evs[cursor].t < offset) ++cursor;

    double t = offset;
    const double dt = static_cast<double>(block) / sr;
    while (t < offset + length) {
        while (cursor < evs.size() && evs[cursor].t <= t) {
            const auto& e = evs[cursor++];
            if (e.on) synth.noteOn(e.midi, e.vel); else synth.noteOff(e.midi);
        }
        synth.render(buf.data(), block);
        out.insert(out.end(), buf.begin(), buf.end());
        t += dt;
    }

    writeWav(argv[2], out, sr);
    std::printf("%s -> %s  (%.1fs, %zu eventos, melodia=%zu, backing=%zu)\n",
                argv[1], argv[2], length, evs.size(),
                chart.playableEvents.size(), chart.backingNotes.size());
    return 0;
}
