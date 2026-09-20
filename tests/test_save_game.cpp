#include "abntpiano/SaveGameRepository.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace abntpiano;

int main() {
    SaveGameRepository repo;
    std::string testPath = "/tmp/abntpiano_test_save.json";

    // Limpa qualquer arquivo anterior
    std::filesystem::remove(testPath);
    std::filesystem::remove(testPath + ".tmp");

    std::cout << "[TEST] Executando TC22: Ausência de arquivo de save -> cria SaveGame novo sem crashar (RF25)\n";
    {
        auto loadRes = repo.load(testPath);
        assert(loadRes.success);
        assert(loadRes.wasCreatedNew);
        assert(loadRes.saveGame.version == 1);
        assert(loadRes.saveGame.profile.id == "default_player");
    }

    std::cout << "[TEST] Executando TC19: Round-trip salvar e carregar SaveGame\n";
    {
        SaveGame sg;
        sg.version = 1;
        sg.profile.id = "chopin_fan";
        sg.profile.name = "Frederic";
        sg.profile.settings.volume = 0.95f;
        sg.profile.settings.audioBufferFrames = 512;
        sg.profile.settings.keyRepeatFilter = true;
        sg.profile.unlockedSongs = {"nocturne_op9_no2", "prelude_op28_no4"};

        ScoreRecord score{
            .chartId = "nocturne_op9_no2_Hard",
            .profileId = "chopin_fan",
            .accuracy = 0.985f,
            .maxCombo = 142,
            .grade = Grade::S,
            .playedAt = 1711002200
        };
        sg.scores.push_back(score);

        auto saveRes = repo.save(sg, testPath);
        assert(saveRes.success);
        assert(std::filesystem::exists(testPath));

        auto loadRes = repo.load(testPath);
        assert(loadRes.success);
        assert(!loadRes.wasCreatedNew);
        assert(loadRes.saveGame.version == 1);
        assert(loadRes.saveGame.profile.id == "chopin_fan");
        assert(loadRes.saveGame.profile.name == "Frederic");
        assert(loadRes.saveGame.profile.settings.audioBufferFrames == 512);
        assert(loadRes.saveGame.scores.size() == 1);
        assert(loadRes.saveGame.scores[0].grade == Grade::S);
        assert(loadRes.saveGame.scores[0].maxCombo == 142);
    }

    std::cout << "[TEST] Executando TC20: Simular crash com arquivo .tmp incompleto -> Save anterior intacto (RNF06)\n";
    {
        // Cria um arquivo temporário incompleto
        std::ofstream tmpFile(testPath + ".tmp");
        tmpFile << "{ \"corrompido\": true ";
        tmpFile.close();

        // O save principal deve continuar íntegro
        auto loadRes = repo.load(testPath);
        assert(loadRes.success);
        assert(!loadRes.wasCreatedNew);
        assert(loadRes.saveGame.profile.id == "chopin_fan");

        // Limpa o .tmp de teste
        std::filesystem::remove(testPath + ".tmp");
    }

    std::cout << "[TEST] Executando TC21: Carregar SaveGame com versão futura -> falha controlada sem corromper\n";
    {
        std::string futurePath = "/tmp/abntpiano_future_save.json";
        {
            std::ofstream f(futurePath);
            f << "{\n  \"version\": 999,\n  \"profile\": { \"name\": \"TimeTraveler\" }\n}\n";
        }

        auto loadRes = repo.load(futurePath);
        assert(!loadRes.success);
        assert(!loadRes.wasCreatedNew); // Não sobrescreve/recria sobre versão desconhecida
        assert(std::filesystem::exists(futurePath)); // Arquivo original permanece intocado

        std::filesystem::remove(futurePath);
    }

    // Limpeza final
    std::filesystem::remove(testPath);

    std::cout << "\n>>> TODOS OS CASOS DE TESTE DE PERSISTÊNCIA (TC19–TC22) PASSARAM! <<<\n";
    return 0;
}
