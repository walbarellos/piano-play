#include "abntpiano/SynthEngine.hpp"
#include <cassert>
#include <iostream>
#include <vector>

using namespace abntpiano;

int main() {
    std::cout << "[TEST] Validando SynthEngine: Polifonia (RF04)\n";
    {
        SynthEngine synth(44100.0);
        assert(synth.activeVoiceCount() == 0);

        // Toca acorde de 3 notas (C4, E4, G4)
        synth.noteOn(60);
        synth.noteOn(64);
        synth.noteOn(67);
        assert(synth.activeVoiceCount() == 3);

        // Renderiza 1000 amostras
        std::vector<float> buffer(1000, 0.0f);
        synth.render(buffer.data(), buffer.size());

        // Confirma que gerou sinal de áudio não nulo
        float maxAmp = 0.0f;
        for (float s : buffer) {
            maxAmp = std::max(maxAmp, std::abs(s));
            assert(s >= -1.0f && s <= 1.0f); // Sem clipping
        }
        assert(maxAmp > 0.01f);
    }

    std::cout << "[TEST] Validando Envelope e Release (RF03)\n";
    {
        SynthEngine synth(44100.0);
        synth.noteOn(60);
        assert(synth.activeVoiceCount() == 1);

        // Solta a nota -> entra em Release
        synth.noteOff(60);

        // Renderiza ~0.5s de áudio para dar tempo do envelope de release zerar
        std::vector<float> buffer(44100 / 2, 0.0f);
        synth.render(buffer.data(), buffer.size());

        // Após release completo, a voz deve estar inativa
        assert(synth.activeVoiceCount() == 0);
    }

    std::cout << "[TEST] Validando Pedal de Sustain (RF05)\n";
    {
        SynthEngine synth(44100.0);
        synth.setSustain(true);
        synth.noteOn(60);

        // Solta a tecla fisicamente, mas sustain está ativado
        synth.noteOff(60);

        // Renderiza áudio por 0.2s: a voz deve continuar ativa por causa do sustain
        std::vector<float> buffer(44100 / 5, 0.0f);
        synth.render(buffer.data(), buffer.size());
        assert(synth.activeVoiceCount() == 1);

        // Solta o pedal de sustain
        synth.setSustain(false);

        // Renderiza tempo suficiente para o release
        std::vector<float> releaseBuffer(44100, 0.0f);
        synth.render(releaseBuffer.data(), releaseBuffer.size());
        assert(synth.activeVoiceCount() == 0);
    }

    std::cout << "\n>>> TODOS OS TESTES DO SYNTHENGINE PASSARAM! <<<\n";
    return 0;
}
