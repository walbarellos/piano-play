#pragma once

#include "abntpiano/Song.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace abntpiano {

// Configuração do importador MIDI conforme RNF03
struct MidiImporterConfig {
    double simultaneityWindowMs = 30.0; // Janela RNF03 para agrupar ChordGroups
};

// Resultado da importação com mensagem de erro controlada (RF11)
struct ImportResult {
    bool success = false;
    std::string errorMessage;
    Song song;
};

// Pipeline de importação de MIDI para Song normalizada (RF09, RF10, RF11)
class MidiImporter {
public:
    explicit MidiImporter(MidiImporterConfig config = {}) : config_(config) {}

    // Importa a partir de um arquivo .mid no disco
    ImportResult importFromFile(const std::string& filepath) const;

    // Importa a partir de um buffer de bytes em memória
    ImportResult importFromBytes(
        const std::vector<uint8_t>& bytes,
        const std::string& songId = ""
    ) const;

    const MidiImporterConfig& config() const { return config_; }
    void setConfig(const MidiImporterConfig& config) { config_ = config; }

private:
    MidiImporterConfig config_;
};

} // namespace abntpiano
