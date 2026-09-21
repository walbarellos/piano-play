#pragma once

#include "abntpiano/SaveGame.hpp"
#include <string>

namespace abntpiano {

struct SaveResult {
    bool success = false;
    std::string errorMessage;
};

struct LoadResult {
    bool success = false;
    bool wasCreatedNew = false;
    std::string message;
    SaveGame saveGame;
};

// Repositório de persistência local com gravação atômica e versionamento (RF23–RF25, RNF06, ADR-08)
class SaveGameRepository {
public:
    static constexpr int kCurrentVersion = 1;

    explicit SaveGameRepository() = default;

    // Salva o SaveGame atomicamente via arquivo temporário + renomeação (RNF06)
    SaveResult save(const SaveGame& saveGame, const std::string& filepath) const;

    // Carrega o SaveGame do disco. Se ausente ou corrompido, gera novo sem crashar (RF25)
    LoadResult load(const std::string& filepath) const;
};

} // namespace abntpiano
