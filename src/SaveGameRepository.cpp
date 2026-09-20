#include "abntpiano/SaveGameRepository.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <optional>

namespace abntpiano {

namespace {

// Serialização simples e robusta em JSON (ADR-08)
std::string serializeSaveGame(const SaveGame& sg) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"version\": " << sg.version << ",\n";
    ss << "  \"profile\": {\n";
    ss << "    \"id\": \"" << sg.profile.id << "\",\n";
    ss << "    \"name\": \"" << sg.profile.name << "\",\n";
    
    // unlockedSongs
    ss << "    \"unlockedSongs\": [";
    for (size_t i = 0; i < sg.profile.unlockedSongs.size(); ++i) {
        ss << "\"" << sg.profile.unlockedSongs[i] << "\"";
        if (i + 1 < sg.profile.unlockedSongs.size()) ss << ", ";
    }
    ss << "],\n";

    // unlockedDifficulties
    ss << "    \"unlockedDifficulties\": {\n";
    size_t diffIdx = 0;
    for (const auto& [songId, diffs] : sg.profile.unlockedDifficulties) {
        ss << "      \"" << songId << "\": [";
        for (size_t i = 0; i < diffs.size(); ++i) {
            ss << "\"" << diffs[i] << "\"";
            if (i + 1 < diffs.size()) ss << ", ";
        }
        ss << "]";
        if (++diffIdx < sg.profile.unlockedDifficulties.size()) ss << ",";
        ss << "\n";
    }
    ss << "    },\n";

    // settings
    ss << "    \"settings\": {\n";
    ss << "      \"volume\": " << sg.profile.settings.volume << ",\n";
    ss << "      \"audioBufferFrames\": " << sg.profile.settings.audioBufferFrames << ",\n";
    ss << "      \"keyRepeatFilter\": " << (sg.profile.settings.keyRepeatFilter ? "true" : "false") << "\n";
    ss << "    }\n";
    ss << "  },\n";

    // scores
    ss << "  \"scores\": [\n";
    for (size_t i = 0; i < sg.scores.size(); ++i) {
        const auto& sc = sg.scores[i];
        ss << "    {\n";
        ss << "      \"chartId\": \"" << sc.chartId << "\",\n";
        ss << "      \"profileId\": \"" << sc.profileId << "\",\n";
        ss << "      \"accuracy\": " << sc.accuracy << ",\n";
        ss << "      \"maxCombo\": " << sc.maxCombo << ",\n";
        ss << "      \"grade\": \"" << gradeToString(sc.grade) << "\",\n";
        ss << "      \"playedAt\": " << sc.playedAt << "\n";
        ss << "    }";
        if (i + 1 < sg.scores.size()) ss << ",";
        ss << "\n";
    }
    ss << "  ]\n";
    ss << "}\n";
    return ss.str();
}

// Helpers simples de extração de campos em JSON
std::optional<std::string> extractString(const std::string& json, const std::string& key) {
    std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m;
    if (std::regex_search(json, m, re)) {
        return m[1].str();
    }
    return std::nullopt;
}

std::optional<long long> extractInt(const std::string& json, const std::string& key) {
    std::regex re("\"" + key + "\"\\s*:\\s*(-?[0-9]+)");
    std::smatch m;
    if (std::regex_search(json, m, re)) {
        return std::stoll(m[1].str());
    }
    return std::nullopt;
}

std::optional<double> extractDouble(const std::string& json, const std::string& key) {
    std::regex re("\"" + key + "\"\\s*:\\s*([0-9]*\\.?[0-9]+)");
    std::smatch m;
    if (std::regex_search(json, m, re)) {
        return std::stod(m[1].str());
    }
    return std::nullopt;
}

std::optional<bool> extractBool(const std::string& json, const std::string& key) {
    std::regex re("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (std::regex_search(json, m, re)) {
        return m[1].str() == "true";
    }
    return std::nullopt;
}

} // namespace

SaveResult SaveGameRepository::save(const SaveGame& saveGame, const std::string& filepath) const {
    std::filesystem::path finalPath(filepath);
    std::filesystem::path tmpPath(filepath + ".tmp");

    // Cria diretório pai se não existir
    if (finalPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(finalPath.parent_path(), ec);
    }

    // Escrita atômica: salva no .tmp e depois renomeia (RNF06)
    {
        std::ofstream tmpFile(tmpPath, std::ios::trunc);
        if (!tmpFile.is_open()) {
            return SaveResult{
                .success = false,
                .errorMessage = "Falha ao criar arquivo temporário: " + tmpPath.string()
            };
        }
        std::string json = serializeSaveGame(saveGame);
        tmpFile << json;
        tmpFile.flush();
        if (!tmpFile.good()) {
            return SaveResult{
                .success = false,
                .errorMessage = "Falha de I/O ao gravar arquivo temporário"
            };
        }
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, finalPath, ec);
    if (ec) {
        return SaveResult{
            .success = false,
            .errorMessage = "Falha ao renomear arquivo temporário para destino final: " + ec.message()
        };
    }

    return SaveResult{.success = true, .errorMessage = ""};
}

LoadResult SaveGameRepository::load(const std::string& filepath) const {
    // RF25 / TC22: Se não existir, cria novo sem crashar
    if (!std::filesystem::exists(filepath)) {
        return LoadResult{
            .success = true,
            .wasCreatedNew = true,
            .message = "Arquivo de save ausente. Novo SaveGame gerado.",
            .saveGame = SaveGame{}
        };
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        return LoadResult{
            .success = false,
            .wasCreatedNew = true,
            .message = "Falha ao abrir save existente. Novo perfil gerado.",
            .saveGame = SaveGame{}
        };
    }

    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto ver = extractInt(json, "version");
    if (!ver.has_value()) {
        // Arquivo corrompido -> cria novo sem crashar (RF25)
        return LoadResult{
            .success = false,
            .wasCreatedNew = true,
            .message = "Arquivo de save corrompido (versão ausente). Novo SaveGame gerado.",
            .saveGame = SaveGame{}
        };
    }

    // TC21: Versão futura desconhecida -> falha controlada, não sobrescreve
    if (*ver > kCurrentVersion) {
        return LoadResult{
            .success = false,
            .wasCreatedNew = false,
            .message = "Versão de save desconhecida ou mais recente que o executável.",
            .saveGame = {}
        };
    }

    SaveGame sg;
    sg.version = static_cast<int>(*ver);

    if (auto pId = extractString(json, "id")) sg.profile.id = *pId;
    if (auto pName = extractString(json, "name")) sg.profile.name = *pName;
    if (auto vol = extractDouble(json, "volume")) sg.profile.settings.volume = static_cast<float>(*vol);
    if (auto buf = extractInt(json, "audioBufferFrames")) sg.profile.settings.audioBufferFrames = static_cast<int>(*buf);
    if (auto kr = extractBool(json, "keyRepeatFilter")) sg.profile.settings.keyRepeatFilter = *kr;

    // Extrai unlockedSongs
    std::regex songsRe("\"unlockedSongs\"\\s*:\\s*\\[([^\\]]*)\\]");
    std::smatch sm;
    if (std::regex_search(json, sm, songsRe)) {
        std::string list = sm[1].str();
        std::regex itemRe("\"([^\"]+)\"");
        auto begin = std::sregex_iterator(list.begin(), list.end(), itemRe);
        auto end = std::sregex_iterator();
        std::vector<std::string> songs;
        for (auto it = begin; it != end; ++it) {
            songs.push_back((*it)[1].str());
        }
        if (!songs.empty()) {
            sg.profile.unlockedSongs = std::move(songs);
        }
    }

    // Extrai scores
    std::regex scoreBlockRe("\\{\\s*\"chartId\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"profileId\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"accuracy\"\\s*:\\s*([0-9.]+)\\s*,\\s*\"maxCombo\"\\s*:\\s*([0-9]+)\\s*,\\s*\"grade\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"playedAt\"\\s*:\\s*([0-9]+)\\s*\\}");
    auto scoreBegin = std::sregex_iterator(json.begin(), json.end(), scoreBlockRe);
    auto scoreEnd = std::sregex_iterator();
    for (auto it = scoreBegin; it != scoreEnd; ++it) {
        ScoreRecord sc;
        sc.chartId = (*it)[1].str();
        sc.profileId = (*it)[2].str();
        sc.accuracy = std::stof((*it)[3].str());
        sc.maxCombo = std::stoi((*it)[4].str());
        sc.grade = stringToGrade((*it)[5].str());
        sc.playedAt = std::stoll((*it)[6].str());
        sg.scores.push_back(sc);
    }

    return LoadResult{
        .success = true,
        .wasCreatedNew = false,
        .message = "Save carregado com sucesso.",
        .saveGame = std::move(sg)
    };
}

} // namespace abntpiano
