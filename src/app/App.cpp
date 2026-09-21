#include "abntpiano/App.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

namespace abntpiano {

static const std::vector<std::string> kComboPhrases = {
    "Mandou bem!", "Sequência incrível!", "Impecável!", "Não erra!", "Isso aí!", "Perfeito!"
};

App::App()
    : synth_(44100.0),
      audioDevice_(synth_) {
    easyDiff_ = DifficultyConfig{
        .name = "Easy", .hitWindow = {140.0, 200.0, 280.0, 280.0},
        .maxChordSize = 1, .noteDensityFactor = 1.0f,
        .allowPartialChord = true, .partialChordThreshold = 0.33f
    };
    normalDiff_ = DifficultyConfig{
        .name = "Normal", .hitWindow = {80.0, 130.0, 180.0, 180.0},
        .maxChordSize = 2, .noteDensityFactor = 1.0f,
        .allowPartialChord = true, .partialChordThreshold = 0.5f
    };
    hardDiff_ = DifficultyConfig{
        .name = "Hard", .hitWindow = {50.0, 90.0, 130.0, 130.0},
        .maxChordSize = 4, .noteDensityFactor = 1.0f,
        .allowPartialChord = false, .partialChordThreshold = 1.0f
    };

    freePlay_.setNoteOnCallback([this](const Note& n) { synth_.noteOn(n.midi, 0.85f); });
    freePlay_.setNoteOffCallback([this](const Note& n) { synth_.noteOff(n.midi); });
}

App::~App() {
    fonts_.release();
    TTF_Quit();
    audioDevice_.close();
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool App::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Erro SDL: " << SDL_GetError() << "\n";
        return false;
    }
    if (TTF_Init() < 0) {
        std::cerr << "Aviso TTF: " << TTF_GetError() << "\n";
    }

    const char* fontPath = "/usr/share/fonts/liberation/LiberationSans-Bold.ttf";
    fonts_.load(fontPath);

    if (!audioDevice_.open(44100, 512)) {
        std::cerr << "Aviso: Áudio rodando sem dispositivo de saída físico.\n";
    }

    window_ = SDL_CreateWindow(
        "ABNT Piano - Simulador de Concerto e Treinador Musical",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        ui::kScreenWidth, ui::kScreenHeight,
        SDL_WINDOW_SHOWN
    );
    if (!window_) {
        std::cerr << "Erro ao criar janela: " << SDL_GetError() << "\n";
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    auto loadResult = saveRepo_.load(savePath_);
    currentGame_ = loadResult.saveGame;

    if (!catalog_.loadDefaultCatalog()) {
        std::cerr << "Erro: Nenhuma música encontrada no catálogo!\n";
        return false;
    }

    reloadChart();
    synth_.allNotesOff();
    synth_.clearSchedule();
    return true;
}

const DifficultyConfig& App::getActiveDiff() const {
    if (currentDifficulty_ == 1) return easyDiff_;
    if (currentDifficulty_ == 2) return normalDiff_;
    return hardDiff_;
}

const Song& App::getActiveSong() const {
    return catalog_.getSong(activeSongIndex_);
}

void App::resyncAudioClock(double playheadNow, double leadIn) {
    synth_.clearSchedule();
    songSpeed_ = playbackSpeed_;
    songStartAudio_ = synth_.audioTime() + leadIn - playheadNow / songSpeed_;
    backingCursor_ = 0;
    melodyCursor_ = 0;
    demoPlayer_.reset(playheadNow);

    if (songMode_) {
        const auto& bn = songMode_->chart().backingNotes;
        while (backingCursor_ < bn.size() && bn[backingCursor_].onset < playheadNow) ++backingCursor_;
        const auto& pe = songMode_->chart().playableEvents;
        while (melodyCursor_ < pe.size() && pe[melodyCursor_].onset < playheadNow) ++melodyCursor_;
    }
}

int App::resolvePitchForKey(char key) {
    char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
    int best = -1;
    double bestDist = 1e9;
    if (songMode_) {
        for (const auto& vn : songMode_->getVisibleNotes(0.8)) {
            for (size_t k = 0; k < vn.keys.size() && k < vn.midiNotes.size(); ++k) {
                if (static_cast<char>(std::toupper(static_cast<unsigned char>(vn.keys[k]))) != norm) continue;
                double d = std::abs(vn.timeToHit);
                if (d < bestDist) { bestDist = d; best = vn.midiNotes[k]; }
            }
        }
    }
    if (best >= 0 && bestDist <= 0.40) return best;
    auto note = freePlay_.mapper().noteForKey(key);
    return note ? note->midi : -1;
}

void App::reloadChart() {
    synth_.allNotesOff();
    Chart chart = chartGen_.generateChart(getActiveSong(), getActiveDiff());
    songMode_ = std::make_unique<SongModeController>(chart);
    hasFinished_ = false;
    teacherRestartTimer_ = 0.0;
    keyFeedbacks_.clear();
    holdStates_.clear();
    heldKeys_.clear();
    particles_.clear();
    soundingPitch_.clear();
    demoPlayer_.reset(0.0);

    songMode_->setJudgementCallback([this](const Judgement& j, const PlayableChordGroup& group) {
        if (j.type != JudgementType::Miss) {
            // Nota foi acertada: reproduz áudio no sintetizador!
            for (size_t k = 0; k < group.midiNotes.size() && k < group.keys.size(); ++k) {
                int midi = group.midiNotes[k];
                char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(group.keys[k])));
                double dur = (k < group.durations.size()) ? group.durations[k] : 0.25;

                soundingPitch_[norm] = midi;
                synth_.noteOn(midi, 0.95f);

                // Notas curtas ou em modo professor agendam o desligamento
                // Notas longas sustentadas pelo jogador serão desligadas ao soltar a tecla (handleKeyUp)
                if (dur <= 0.32 || teacherMode_) {
                    synth_.scheduleNoteOff(synth_.audioTime() + std::max(0.12, dur), midi);
                }
            }
        } else {
            // Nota perdida (Miss): MUTE! Silêncio na melodia
        }

        for (char k : group.keys) {
            char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(k)));
            double expireT = songMode_->playhead() + 0.20;
            keyFeedbacks_[norm] = ui::KeyFeedback{j.type, expireT};

            if (j.type == JudgementType::Miss) {
                holdStates_[norm] = HoldState::Missed;
            }

            int cx = keyboardRenderer_.getKeyCenterX(norm);
            particles_.spawnHit(j.type, cx, ui::kHitY);
        }

        if (!group.keys.empty()) {
            int cx = keyboardRenderer_.getKeyCenterX(group.keys[0]);
            const char* label =
                j.type == JudgementType::Perfect ? "PERFECT!" :
                j.type == JudgementType::Great   ? "GREAT!"   :
                j.type == JudgementType::Good    ? "GOOD"     : "MISS";
            SDL_Color lCol =
                j.type == JudgementType::Perfect ? SDL_Color{255, 215, 0, 255} :
                j.type == JudgementType::Great   ? SDL_Color{60, 255, 100, 255} :
                j.type == JudgementType::Good    ? SDL_Color{80, 200, 255, 255} :
                                                   SDL_Color{255, 60, 80, 255};
            particles_.spawnFloatingText(label, cx, ui::kHitY, lCol);
        }

        int combo = songMode_->scoringEngine().currentCombo();
        if (combo > 0 && combo % 5 == 0) {
            currentPhrase_ = kComboPhrases[phraseIndex_ % kComboPhrases.size()];
            phraseIndex_++;
            phraseExpireTime_ = songMode_->playhead() + 1.4;
        }
    });

    songMode_->setFinishedCallback([this](const ExecutionSummary& summary) {
        finalSummary_ = summary;
        hasFinished_ = true;
        synth_.releaseAllNotes();
        heldKeys_.clear();
        holdStates_.clear();
        teacherRestartTimer_ = 0.0;
        auto rec = summary.toScoreRecord(songMode_->chart().songId + "_" + getActiveDiff().name,
                                         currentGame_.profile.id);
        currentGame_.scores.push_back(rec);
        saveRepo_.save(currentGame_, savePath_);
    });

    songMode_->start();
    resyncAudioClock(0.0, 0.35);
}

void App::handleKeyDown(SDL_Keycode sym) {
    if (showShortcutsOverlay_) {
        if (sym == SDLK_SLASH || sym == SDLK_QUESTION || sym == SDLK_ESCAPE) {
            showShortcutsOverlay_ = false;
            return;
        }
    } else if (sym == SDLK_SLASH || sym == SDLK_QUESTION) {
        showShortcutsOverlay_ = true;
        return;
    }

    // ─── 1. MODO MENU PRINCIPAL ──────────────────────────────────────────
    if (currentMode_ == GameMode::MainMenu) {
        if (sym == SDLK_ESCAPE) {
            running_ = false;
            return;
        }
        if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
            currentMode_ = GameMode::SongMode;
            reloadChart();
            return;
        }
        if (sym == SDLK_UP) {
            if (activeSongIndex_ > 0) {
                activeSongIndex_--;
            } else if (catalog_.songCount() > 0) {
                activeSongIndex_ = catalog_.songCount() - 1;
            }
            return;
        }
        if (sym == SDLK_DOWN) {
            if (catalog_.songCount() > 0) {
                activeSongIndex_ = (activeSongIndex_ + 1) % catalog_.songCount();
            }
            return;
        }
        if (sym == SDLK_1) {
            currentDifficulty_ = 1;
            return;
        }
        if (sym == SDLK_2) {
            currentDifficulty_ = 2;
            return;
        }
        if (sym == SDLK_3) {
            currentDifficulty_ = 3;
            return;
        }
        if (sym == SDLK_t || sym == SDLK_F12) {
            teacherMode_ = !teacherMode_;
            return;
        }
        if (sym >= SDLK_F2 && sym <= SDLK_F6) {
            size_t idx = static_cast<size_t>(sym - SDLK_F2);
            if (idx < catalog_.songCount()) {
                activeSongIndex_ = idx;
            }
            return;
        }
        if (sym == SDLK_F1) {
            currentMode_ = GameMode::FreePlay;
            synth_.allNotesOff();
            heldKeys_.clear();
            holdStates_.clear();
            soundingPitch_.clear();
            return;
        }
        return;
    }

    // ─── 2. EM JOGO (SongMode ou FreePlay) ────────────────────────────────
    if (sym == SDLK_ESCAPE) {
        // Tecla ESC retorna ao Menu Principal
        currentMode_ = GameMode::MainMenu;
        synth_.allNotesOff();
        synth_.clearSchedule();
        heldKeys_.clear();
        holdStates_.clear();
        soundingPitch_.clear();
        return;
    }

    if (sym == SDLK_F1) {
        currentMode_ = GameMode::FreePlay;
        synth_.allNotesOff();
        heldKeys_.clear();
        holdStates_.clear();
        soundingPitch_.clear();
    } else if (sym >= SDLK_F2 && sym <= SDLK_F6) {
        currentMode_ = GameMode::SongMode;
        activeSongIndex_ = static_cast<size_t>(sym - SDLK_F2);
        reloadChart();
    } else if (sym == SDLK_F10) {
        autoMelody_ = !autoMelody_;
        synth_.clearSchedule();
        if (songMode_) resyncAudioClock(songMode_->playhead(), 0.0);
    } else if (sym == SDLK_F11) {
        backingEnabled_ = !backingEnabled_;
        synth_.clearSchedule();
        if (songMode_) resyncAudioClock(songMode_->playhead(), 0.0);
    } else if (sym == SDLK_F12 || sym == SDLK_t) {
        teacherMode_ = !teacherMode_;
        hasFinished_ = false;
        synth_.allNotesOff();
        if (songMode_) {
            songMode_->restart();
            heldKeys_.clear();
            holdStates_.clear();
            soundingPitch_.clear();
            resyncAudioClock(0.0, 0.35);
        }
    } else if (sym == SDLK_TAB) {
        currentMode_ = GameMode::SongMode;
        activeSongIndex_ = catalog_.nextIndex(activeSongIndex_);
        reloadChart();
    } else if (sym == SDLK_1) {
        currentDifficulty_ = 1; reloadChart();
    } else if (sym == SDLK_2) {
        currentDifficulty_ = 2; reloadChart();
    } else if (sym == SDLK_3) {
        currentDifficulty_ = 3; reloadChart();
    } else if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS) {
        lookahead_ = std::min(6.0, lookahead_ + 0.5);
    } else if (sym == SDLK_EQUALS || sym == SDLK_PLUS || sym == SDLK_KP_PLUS) {
        lookahead_ = std::max(1.5, lookahead_ - 0.5);
    } else if (sym == SDLK_LEFTBRACKET) {
        playbackSpeed_ = std::max(0.5, playbackSpeed_ - 0.25);
        if (songMode_) resyncAudioClock(songMode_->playhead(), 0.0);
    } else if (sym == SDLK_RIGHTBRACKET) {
        playbackSpeed_ = std::min(1.5, playbackSpeed_ + 0.25);
        if (songMode_) resyncAudioClock(songMode_->playhead(), 0.0);
    } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
        hasFinished_ = false;
        synth_.allNotesOff();
        heldKeys_.clear();
        holdStates_.clear();
        if (songMode_) songMode_->restart();
        soundingPitch_.clear();
        resyncAudioClock(0.0, 0.35);
    } else if (sym == SDLK_SPACE) {
        freePlay_.setSustain(true);
        synth_.setSustain(true);
    } else if (sym == SDLK_UP) {
        freePlay_.shiftOctaveUp();
    } else if (sym == SDLK_DOWN) {
        freePlay_.shiftOctaveDown();
    } else if (sym >= SDLK_a && sym <= SDLK_z) {
        char key = static_cast<char>('A' + (sym - SDLK_a));
        if (currentMode_ == GameMode::FreePlay) {
            freePlay_.onKeyDown(key);
        } else if (currentMode_ == GameMode::SongMode && songMode_) {
            heldKeys_.insert(key);
            holdStates_[key] = HoldState::Holding;
            songMode_->onKeyDown(key);
        }
    }
}

void App::handleKeyUp(SDL_Keycode sym) {
    if (sym == SDLK_SPACE) {
        freePlay_.setSustain(false);
        synth_.setSustain(false);
    } else if (sym >= SDLK_a && sym <= SDLK_z) {
        char key = static_cast<char>('A' + (sym - SDLK_a));
        if (currentMode_ == GameMode::FreePlay) {
            freePlay_.onKeyUp(key);
        } else {
            heldKeys_.erase(key);
            auto it = holdStates_.find(key);
            if (it != holdStates_.end() && it->second == HoldState::Holding) {
                it->second = HoldState::Released;
            }
            auto sp = soundingPitch_.find(key);
            if (sp != soundingPitch_.end()) {
                synth_.noteOff(sp->second);
                soundingPitch_.erase(sp);
            }
        }
    }
}

void App::handleEvent(const SDL_Event& ev) {
    if (ev.type == SDL_QUIT) {
        running_ = false;
    } else if (ev.type == SDL_MOUSEMOTION) {
        mouseX_ = ev.motion.x;
        mouseY_ = ev.motion.y;
    } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
        if (ev.button.button == SDL_BUTTON_LEFT) {
            if (showShortcutsOverlay_) {
                showShortcutsOverlay_ = false;
                return;
            }

            if (currentMode_ == GameMode::MainMenu) {
                ui::MenuAction act = menuRenderer_.handleMouseClick(
                    ev.button.x, ev.button.y,
                    activeSongIndex_, currentDifficulty_, teacherMode_,
                    catalog_.songCount()
                );
                if (act == ui::MenuAction::StartSong) {
                    currentMode_ = GameMode::SongMode;
                    reloadChart();
                } else if (act == ui::MenuAction::StartFreePlay) {
                    currentMode_ = GameMode::FreePlay;
                    synth_.allNotesOff();
                    heldKeys_.clear();
                    holdStates_.clear();
                    soundingPitch_.clear();
                } else if (act == ui::MenuAction::ToggleShortcuts) {
                    showShortcutsOverlay_ = true;
                }
            } else {
                if (ev.button.x <= 130 && ev.button.y <= ui::kHudHeight) {
                    showShortcutsOverlay_ = !showShortcutsOverlay_;
                }
            }
        }
    } else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
        handleKeyDown(ev.key.keysym.sym);
    } else if (ev.type == SDL_KEYUP) {
        handleKeyUp(ev.key.keysym.sym);
    }
}

void App::update(double rawDt) {
    if (currentMode_ == GameMode::MainMenu) {
        particles_.update(rawDt);
        return;
    }

    if (currentMode_ == GameMode::SongMode && songMode_) {
        double target = (synth_.audioTime() - songStartAudio_) * songSpeed_;
        double delta = target - songMode_->playhead();
        if (delta > 0.0) songMode_->update(delta);

        if (teacherMode_ && !hasFinished_) {
            demoPlayer_.update(songMode_->playhead(), *songMode_, heldKeys_, holdStates_);
        }

        const double ph = songMode_->playhead();
        const double horizon = ph + 0.15;
        auto audioTimeOf = [this](double onset) {
            return songStartAudio_ + onset / songSpeed_;
        };

        if (backingEnabled_) {
            const auto& bn = songMode_->chart().backingNotes;
            while (backingCursor_ < bn.size() && bn[backingCursor_].onset <= horizon) {
                const auto& n = bn[backingCursor_++];
                float vel = std::clamp(static_cast<float>(n.velocity) / 127.0f, 0.05f, 1.0f) * 0.55f;
                synth_.scheduleNoteOn(audioTimeOf(n.onset), n.midiNote, vel);
                synth_.scheduleNoteOff(audioTimeOf(n.onset + std::max(0.08, n.duration)), n.midiNote);
            }
        }

        if (autoMelody_ && !teacherMode_) {
            const auto& pe = songMode_->chart().playableEvents;
            while (melodyCursor_ < pe.size() && pe[melodyCursor_].onset <= horizon) {
                const size_t gi = melodyCursor_++;
                const auto& g = pe[gi];
                for (size_t k = 0; k < g.midiNotes.size() && k < g.durations.size(); ++k) {
                    synth_.scheduleNoteOn(audioTimeOf(g.onset), g.midiNotes[k], 0.92f);
                    synth_.scheduleNoteOff(audioTimeOf(g.onset + std::max(0.12, g.durations[k])), g.midiNotes[k]);
                }
            }
        }
    }

    if (teacherMode_ && currentMode_ == GameMode::SongMode && hasFinished_) {
        teacherRestartTimer_ += rawDt;
        if (teacherRestartTimer_ >= 2.5) {
            teacherRestartTimer_ = 0.0;
            hasFinished_ = false;
            synth_.allNotesOff();
            songMode_->restart();
            heldKeys_.clear();
            holdStates_.clear();
            soundingPitch_.clear();
            resyncAudioClock(0.0, 0.35);
        }
    }

    // Gera faíscas e confetes de sustentação contínua estilo Guitar Hero
    if (songMode_ && currentMode_ == GameMode::SongMode && !hasFinished_) {
        for (const auto& vn : songMode_->getVisibleNotes(0.05)) {
            if (vn.timeToHit <= 0.0) {
                for (size_t kIdx = 0; kIdx < vn.keys.size(); ++kIdx) {
                    double dur = (kIdx < vn.durations.size()) ? vn.durations[kIdx] : 0.25;
                    if (dur > 0.32 && (vn.timeToHit + dur > 0.0)) {
                        char normKey = static_cast<char>(std::toupper(static_cast<unsigned char>(vn.keys[kIdx])));
                        auto itH = holdStates_.find(normKey);
                        bool isHolding = (heldKeys_.count(normKey) > 0 ||
                                         (itH != holdStates_.end() && itH->second == HoldState::Holding));
                        if (isHolding) {
                            const auto* pk = ui::getPianoKey(normKey);
                            if (pk) {
                                float hue = ui::pitchHue(pk->pc);
                                particles_.spawnSustainEmbers(static_cast<int>(pk->x), ui::kHitY, hue, 2);
                            }
                        }
                    }
                }
            }
        }
    }

    particles_.update(rawDt);

    if (songMode_) {
        for (auto& [k, st] : holdStates_) {
            auto it = keyFeedbacks_.find(k);
            bool fbExpired = (it == keyFeedbacks_.end() || it->second.expireTime <= songMode_->playhead());
            if (st == HoldState::Missed && fbExpired) st = HoldState::Idle;
            if (st == HoldState::Released && fbExpired) st = HoldState::Idle;
        }
    }
}

void App::render() {
    SDL_SetRenderDrawColor(renderer_, 7, 5, 11, 255); // #07050B
    SDL_RenderClear(renderer_);

    if (currentMode_ == GameMode::MainMenu) {
        menuRenderer_.render(renderer_, fonts_, catalog_, activeSongIndex_,
                             currentDifficulty_, teacherMode_, mouseX_, mouseY_);
        if (showShortcutsOverlay_) {
            hudRenderer_.renderShortcutsOverlay(renderer_, fonts_);
        }
        SDL_RenderPresent(renderer_);
        return;
    }

    std::vector<VisibleNote> visNotes;
    std::set<char> keysAtHitLine;
    if (currentMode_ == GameMode::SongMode && songMode_ && !hasFinished_) {
        visNotes = songMode_->getVisibleNotes(lookahead_);
        for (const auto& vn : visNotes) {
            if (!vn.isJudged && std::abs(vn.timeToHit) <= 0.14) {
                for (char k : vn.keys) {
                    keysAtHitLine.insert(static_cast<char>(std::toupper(static_cast<unsigned char>(k))));
                }
            }
        }
    }

    double playhead = songMode_ ? songMode_->playhead() : 0.0;

    if (currentMode_ == GameMode::SongMode) {
        highwayRenderer_.render(renderer_, fonts_, keyboardRenderer_, visNotes, keysAtHitLine,
                                keyFeedbacks_, heldKeys_, holdStates_, lookahead_, playhead);
    }

    particles_.renderShockwaves(renderer_, ui::kHitY);
    particles_.renderParticles(renderer_);
    particles_.renderFloatingTexts(renderer_, fonts_.medium);

    keyboardRenderer_.render(renderer_, fonts_, freePlay_.mapper(),
                             heldKeys_, keysAtHitLine, keyFeedbacks_, holdStates_, playhead);

    double totalDuration = 0.0;
    if (songMode_ && !songMode_->chart().playableEvents.empty()) {
        totalDuration = songMode_->chart().playableEvents.back().onset + 2.0;
    }

    ScoringEngine dummyScoring;
    const ScoringEngine& sc = songMode_ ? songMode_->scoringEngine() : dummyScoring;
    hudRenderer_.render(renderer_, fonts_, getActiveSong(), currentDifficulty_, lookahead_,
                        sc, playhead, totalDuration, currentMode_ == GameMode::FreePlay,
                        teacherMode_, currentPhrase_, phraseExpireTime_, showShortcutsOverlay_);

    if (hasFinished_) {
        resultsOverlay_.render(renderer_, fonts_, finalSummary_);
    }

    SDL_RenderPresent(renderer_);
}

void App::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (running_) {
        auto now = std::chrono::high_resolution_clock::now();
        double rawDt = std::chrono::duration<double>(now - lastTime).count();
        lastTime = now;
        rawDt = std::min(rawDt, 0.1);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            handleEvent(ev);
        }

        update(rawDt);
        render();

        SDL_Delay(14);
    }
}

} // namespace abntpiano
