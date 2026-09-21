#pragma once

#include "abntpiano/AudioDevice.hpp"
#include "abntpiano/ChartGenerator.hpp"
#include "abntpiano/DemoPlayer.hpp"
#include "abntpiano/FreePlayController.hpp"
#include "abntpiano/SaveGameRepository.hpp"
#include "abntpiano/SongCatalog.hpp"
#include "abntpiano/SongModeController.hpp"
#include "abntpiano/SynthEngine.hpp"
#include "abntpiano/ui/HighwayRenderer.hpp"
#include "abntpiano/ui/HudRenderer.hpp"
#include "abntpiano/ui/KeyboardRenderer.hpp"
#include "abntpiano/ui/ParticleSystem.hpp"
#include "abntpiano/ui/RenderTypes.hpp"
#include "abntpiano/ui/ResultsOverlay.hpp"

#include <SDL.h>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace abntpiano {

enum class GameMode {
    FreePlay,
    SongMode
};

class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool init();
    void run();

private:
    void handleEvent(const SDL_Event& ev);
    void handleKeyDown(SDL_Keycode sym);
    void handleKeyUp(SDL_Keycode sym);

    void update(double rawDt);
    void render();

    void reloadChart();
    void resyncAudioClock(double playheadNow, double leadIn);
    int resolvePitchForKey(char key);
    const DifficultyConfig& getActiveDiff() const;
    const Song& getActiveSong() const;

    // Janela e renderizador
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    ui::FontCollection fonts_;

    // Camada de Áudio
    SynthEngine synth_;
    AudioDevice audioDevice_;

    // Camada de Dados e Conteúdo
    SongCatalog catalog_;
    SaveGameRepository saveRepo_;
    SaveGame currentGame_;
    std::string savePath_ = "abntpiano_save.json";

    // Camada de Jogo
    FreePlayController freePlay_;
    std::unique_ptr<SongModeController> songMode_;
    ChartGenerator chartGen_;
    DemoPlayer demoPlayer_;

    DifficultyConfig easyDiff_;
    DifficultyConfig normalDiff_;
    DifficultyConfig hardDiff_;

    // UI e Renderers
    ui::HighwayRenderer highwayRenderer_;
    ui::KeyboardRenderer keyboardRenderer_;
    ui::HudRenderer hudRenderer_;
    ui::ResultsOverlay resultsOverlay_;
    ui::ParticleSystem particles_;

    // Estado da Aplicação
    GameMode currentMode_ = GameMode::SongMode;
    bool running_ = true;
    size_t activeSongIndex_ = 0;
    int currentDifficulty_ = 1;
    double lookahead_ = 3.5;
    double playbackSpeed_ = 1.0;

    bool demoMode_ = false;
    double demoRestartTimer_ = 0.0;

    bool backingEnabled_ = true;
    bool autoMelody_ = true;
    size_t backingCursor_ = 0;
    size_t melodyCursor_ = 0;
    double songStartAudio_ = 0.0;
    double songSpeed_ = 1.0;

    bool hasFinished_ = false;
    ExecutionSummary finalSummary_;

    std::set<char> heldKeys_;
    std::map<char, HoldState> holdStates_;
    std::map<char, ui::KeyFeedback> keyFeedbacks_;
    std::map<char, int> soundingPitch_;

    std::string currentPhrase_;
    double phraseExpireTime_ = 0.0;
    size_t phraseIndex_ = 0;
};

} // namespace abntpiano
