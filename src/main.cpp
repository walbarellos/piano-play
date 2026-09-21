#include "abntpiano/Note.hpp"
#include "abntpiano/KeyboardMapper.hpp"
#include "abntpiano/JudgementEngine.hpp"
#include "abntpiano/MidiImporter.hpp"
#include "abntpiano/ChartGenerator.hpp"
#include "abntpiano/ScoringEngine.hpp"
#include "abntpiano/FreePlayController.hpp"
#include "abntpiano/SongModeController.hpp"
#include "abntpiano/SynthEngine.hpp"
#include "abntpiano/SaveGameRepository.hpp"

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>
#include <random>
#include <algorithm>
#include <cmath>

using namespace abntpiano;

// ─── ÁUDIO GLOBAL ─────────────────────────────────────────────────────────────
static SynthEngine g_synth(44100.0);
static void audioCallback(void*, Uint8* stream, int len) {
    g_synth.render(reinterpret_cast<float*>(stream), len / sizeof(float));
}

// ─── LAYOUT DO TECLADO VISUAL ────────────────────────────────────────────────
struct KeyVisualPos { char key; int row, col; };
static const std::vector<KeyVisualPos> kVisualKeys = {
    {'Q',0,0},{'W',0,1},{'E',0,2},{'R',0,3},{'T',0,4},
    {'Y',0,5},{'U',0,6},{'I',0,7},{'O',0,8},{'P',0,9},
    {'A',1,0},{'S',1,1},{'D',1,2},{'F',1,3},{'G',1,4},
    {'H',1,5},{'J',1,6},{'K',1,7},{'L',1,8},
    {'Z',2,0},{'X',2,1},{'C',2,2},{'V',2,3},{'B',2,4},{'N',2,5},{'M',2,6}
};

static const std::vector<std::string> kComboPhrases = {
    "Mandou bem!", "Sequência incrível!", "Impecável!", "Não erra!", "Isso aí!", "Perfeito!"
};

// ─── PALETA DE CORES PREMIUM ─────────────────────────────────────────────────
// Fundo: azul noturno profundo
// Notas: azul intenso com borda ciano
// HIT line: ouro neon
// Teclas: slate escuro com brilho

// ─── SISTEMA DE PARTÍCULAS ───────────────────────────────────────────────────
struct Particle {
    float x=0,y=0,vx=0,vy=0,life=1.0f,decay=2.0f;
    SDL_Color color{255,215,0,255};
    int size=4;
};
struct FloatingJudgement {
    std::string text;
    float x=0,y=0,vy=-55.0f,life=1.0f;
    SDL_Color color{255,215,0,255};
};
struct LaneShockwave {
    int centerX=0; float life=1.0f; SDL_Color color{50,220,100,200};
};

// ─── ESTADO DE TECLAS LONGAS (HOLD) ─────────────────────────────────────────
enum class HoldState { Idle, Holding, Released, Missed };

// ─── RENDERIZAÇÃO DE TEXTO ───────────────────────────────────────────────────
void renderText(SDL_Renderer* r, TTF_Font* font, const std::string& text,
                int x, int y, SDL_Color color, bool center=false) {
    if (!font || text.empty()) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    if (tex) {
        int w=surf->w, h=surf->h;
        SDL_Rect dst{center?(x-w/2):x, center?(y-h/2):y, w, h};
        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

// Desenha um retângulo com bordas arredondadas via múltiplos rects (simulado)
void renderRoundRect(SDL_Renderer* r, SDL_Rect rect, int radius,
                     Uint8 R, Uint8 G, Uint8 B, Uint8 A) {
    SDL_SetRenderDrawColor(r, R, G, B, A);
    // corpo central
    SDL_Rect body{rect.x+radius, rect.y, rect.w-2*radius, rect.h};
    SDL_RenderFillRect(r, &body);
    SDL_Rect bodyV{rect.x, rect.y+radius, rect.w, rect.h-2*radius};
    SDL_RenderFillRect(r, &bodyV);
    // cantos arredondados (aproximação por círculo)
    int cx[4]={rect.x+radius, rect.x+rect.w-1-radius,
               rect.x+radius, rect.x+rect.w-1-radius};
    int cy[4]={rect.y+radius, rect.y+radius,
               rect.y+rect.h-1-radius, rect.y+rect.h-1-radius};
    for (int i=0;i<4;i++) {
        for (int dy=-radius;dy<=radius;dy++) {
            for (int dx=-radius;dx<=radius;dx++) {
                if (dx*dx+dy*dy<=radius*radius) {
                    SDL_RenderDrawPoint(r, cx[i]+dx, cy[i]+dy);
                }
            }
        }
    }
}

// Renderiza uma barra de gradiente horizontal simples (top color → bottom color)
void renderGradientRect(SDL_Renderer* r, SDL_Rect rect,
                        SDL_Color top, SDL_Color bottom) {
    for (int y=0; y<rect.h; y++) {
        float t = static_cast<float>(y) / static_cast<float>(rect.h-1);
        Uint8 rr = static_cast<Uint8>(top.r + t*(bottom.r-top.r));
        Uint8 gg = static_cast<Uint8>(top.g + t*(bottom.g-top.g));
        Uint8 bb = static_cast<Uint8>(top.b + t*(bottom.b-top.b));
        Uint8 aa = static_cast<Uint8>(top.a + t*(bottom.a-top.a));
        SDL_SetRenderDrawColor(r, rr, gg, bb, aa);
        SDL_RenderDrawLine(r, rect.x, rect.y+y, rect.x+rect.w-1, rect.y+y);
    }
}

// ─── MAIN ────────────────────────────────────────────────────────────────────
int main(int, char*[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Erro SDL: " << SDL_GetError() << "\n"; return 1;
    }
    if (TTF_Init() < 0) {
        std::cerr << "Aviso TTF: " << TTF_GetError() << "\n";
    }

    // Fontes — maior que o antigo para melhor legibilidade
    const char* FONT_PATH = "/usr/share/fonts/liberation/LiberationSans-Bold.ttf";
    TTF_Font* fontTiny   = TTF_OpenFont(FONT_PATH, 11);
    TTF_Font* fontSmall  = TTF_OpenFont(FONT_PATH, 14);
    TTF_Font* fontMedium = TTF_OpenFont(FONT_PATH, 20);
    TTF_Font* fontLarge  = TTF_OpenFont(FONT_PATH, 36);
    TTF_Font* fontHuge   = TTF_OpenFont(FONT_PATH, 56);

    // Áudio
    SDL_AudioSpec desired{}, obtained{};
    desired.freq = 44100; desired.format = AUDIO_F32SYS;
    desired.channels = 1; desired.samples = 512;
    desired.callback = audioCallback;
    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (audioDevice) { g_synth.setSampleRate(obtained.freq); SDL_PauseAudioDevice(audioDevice,0); }

    // ── JANELA 1280×800 ──
    const int SW = 1280, SH = 800;
    SDL_Window* window = SDL_CreateWindow(
        "ABNT Piano  ♪  [F2–F9] Músicas  |  [1–3] Dificuldade  |  [F1] Free Play  |  [F12] Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH,
        SDL_WINDOW_SHOWN
    );
    if (!window) { std::cerr << "Erro janela: " << SDL_GetError() << "\n"; SDL_Quit(); return 1; }

    SDL_Renderer* ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    SaveGameRepository saveRepo;
    std::string savePath = "abntpiano_save.json";
    auto loadResult = saveRepo.load(savePath);
    SaveGame currentGame = loadResult.saveGame;

    FreePlayController freePlay;
    freePlay.setNoteOnCallback([](const Note& n){ g_synth.noteOn(n.midi, 0.85f); });
    freePlay.setNoteOffCallback([](const Note& n){ g_synth.noteOff(n.midi); });

    // ── CATÁLOGO DE MÚSICAS ──
    std::vector<Song> songCatalog;
    MidiImporter midiImporter;
    struct SongEntry { std::string file, title, composer; };
    const std::vector<SongEntry> kSongs = {
        {"assets/songs/parabens_pra_voce.mid",              "Parabéns Pra Você",                        "Tradicional"},
        {"assets/songs/beethoven_ode_to_joy.mid",           "Ode to Joy",                               "L.v. Beethoven"},
        {"assets/songs/beethoven_fur_elise.mid",            "Für Elise (WoO 59)",                       "L.v. Beethoven"},
        {"assets/songs/chopin_prelude_op28_no4.mid",        "Prelude Op. 28 No. 4",                     "F. Chopin"},
        {"assets/songs/mozart_alla_turca.mid",              "Rondo Alla Turca (KV 331, III)",           "W.A. Mozart"},
        {"assets/songs/chopin_ballade_no1_op23.mid",        "Ballade No. 1 em Sol menor, Op. 23",       "F. Chopin"},
        {"assets/songs/beethoven_kreutzer_presto.mid",      "Sonata No. 9 \'Kreutzer\' - I. Presto",     "L.v. Beethoven"},
        {"assets/songs/beethoven_kreutzer_op47_mov1.mid",   "Sonata No. 9 \'Kreutzer\' Op. 47 - Completa","L.v. Beethoven"},
        {"assets/songs/beethoven_kreutzer_op47_mov3.mid",   "Sonata No. 9 \'Kreutzer\' Op. 47 - III",    "L.v. Beethoven"},
    };
    for (const auto& e : kSongs) {
        auto res = midiImporter.importFromFile(e.file);
        if (res.success && !res.song.chordGroups.empty()) {
            Song s = res.song; s.title = e.title; s.composer = e.composer;
            songCatalog.push_back(std::move(s));
        } else { std::cerr << "Aviso: falha ao carregar " << e.file << "\n"; }
    }
    if (songCatalog.empty()) { std::cerr << "Erro: sem músicas!\n"; return 1; }

    // ── DIFICULDADES ──
    DifficultyConfig easyDiff{
        .name="Easy", .hitWindow={140.0,200.0,280.0,280.0},
        .maxChordSize=1, .noteDensityFactor=1.0f,
        .allowPartialChord=true, .partialChordThreshold=0.33f
    };
    DifficultyConfig normalDiff{
        .name="Normal", .hitWindow={80.0,130.0,180.0,180.0},
        .maxChordSize=2, .noteDensityFactor=1.0f,
        .allowPartialChord=true, .partialChordThreshold=0.5f
    };
    DifficultyConfig hardDiff{
        .name="Hard", .hitWindow={50.0,90.0,130.0,130.0},
        .maxChordSize=4, .noteDensityFactor=1.0f,
        .allowPartialChord=false, .partialChordThreshold=1.0f
    };

    ChartGenerator chartGen;
    size_t activeSongIndex = 0;
    int currentDifficulty  = 1;
    double lookahead       = 3.5;
    double playbackSpeed   = 1.0;

    auto getActiveDiff = [&]() -> const DifficultyConfig& {
        if (currentDifficulty==1) return easyDiff;
        if (currentDifficulty==2) return normalDiff;
        return hardDiff;
    };
    auto getActiveSong = [&]() -> const Song& {
        return songCatalog[activeSongIndex % songCatalog.size()];
    };

    std::unique_ptr<SongModeController> songMode;

    // ─── LAYOUT GEOMÉTRICO ───────────────────────────────────────────────────
    // HUD top: 0..75
    // Play field: 75..600  (horizonte → hit line)
    // Keyboard zone: 600..800
    const int HUD_H      = 80;
    const int HORIZON_Y  = HUD_H + 2;
    const int HIT_Y      = 595;      // linha de impacto
    const int KBOARD_Y   = HIT_Y + 12;  // topo das teclas visuais

    // Teclas visuais: 3 linhas, bem espaçadas
    const int KEY_W      = 66;
    const int KEY_H      = 62;
    const int KEY_GAP    = 5;
    // Offset horizontal por fileira para alinhamento natural do QWERTY
    const int ROW_OFF[3] = {
        (SW - 10*(KEY_W+KEY_GAP) + KEY_GAP) / 2,  // Q-P
        (SW - 9*(KEY_W+KEY_GAP)  + KEY_GAP) / 2 + (KEY_W+KEY_GAP)/2,   // A-L
        (SW - 7*(KEY_W+KEY_GAP)  + KEY_GAP) / 2 + (KEY_W+KEY_GAP)      // Z-M
    };

    auto getKeyRect = [&](char key) -> SDL_Rect {
        char norm = toupper((unsigned char)key);
        for (const auto& vk : kVisualKeys) {
            if (vk.key == norm) {
                int x = ROW_OFF[vk.row] + vk.col*(KEY_W+KEY_GAP);
                int y = KBOARD_Y + vk.row*(KEY_H+KEY_GAP);
                return {x, y, KEY_W, KEY_H};
            }
        }
        return {0,0,0,0};
    };

    auto getKeyCenterX = [&](char key) -> int {
        SDL_Rect r = getKeyRect(key);
        return r.x + r.w/2;
    };

    // ─── ESTADO DE FEEDBACK ──────────────────────────────────────────────────
    struct KeyFeedback { JudgementType type; double expireTime; };
    std::map<char, KeyFeedback> keyFeedbacks;
    // Teclas pressionadas no momento (song mode)
    std::set<char> heldKeys;          // teclas fisicamente seguradas agora
    std::map<char, HoldState> holdStates; // estado visual da sustentação

    std::vector<Particle>         particles;
    std::vector<FloatingJudgement> floatingTexts;
    std::vector<LaneShockwave>     shockwaves;

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> randVX(-90.f, 90.f);
    std::uniform_real_distribution<float> randVY(-200.f, -70.f);
    std::uniform_real_distribution<float> randC(0.8f, 1.0f);

    std::string currentPhrase;
    double phraseExpireTime = 0.0;
    size_t phraseIndex = 0;

    ExecutionSummary finalSummary;
    bool hasFinished = false;

    // ─── MODO DEMO (AUTO-PLAY) ───────────────────────────────────────────────
    bool demoMode = false;
    size_t demoCursor = 0;
    double demoLastHitTime = -1.0;
    std::map<char,double> demoNoteOffTimes; // key → playhead de soltura (apenas visual)
    double demoRestartTimer = 0.0;

    // ─── SINCRONIA ÁUDIO↔VÍDEO ───────────────────────────────────────────────
    // O relógio mestre é o contador de amostras do device (g_synth.audioTime()).
    // O playhead é derivado dele, e as notas são agendadas em tempo absoluto de
    // áudio com precisão de amostra. Nada é disparado em tempo de frame.
    bool   backingEnabled   = true;  // acompanhamento (mão esquerda / vozes internas)
    bool   autoMelody       = true;  // guia melódico audível
    size_t backingCursor    = 0;
    size_t melodyCursor     = 0;
    double songStartAudio   = 0.0;   // audioTime que corresponde a playhead 0
    double songSpeed        = 1.0;   // snapshot de playbackSpeed em uso
    const double kScheduleAhead = 0.15; // antecedência de agendamento (s)
    const double kLeadIn        = 0.35; // silêncio inicial p/ encher o buffer
    std::map<char,int> soundingPitch;   // tecla física → midi real soando

    // Reancora o relógio da música no relógio de áudio e reposiciona os cursores
    // de agendamento. Chamado em troca de música, restart e mudança de velocidade.
    auto resyncAudioClock = [&](double playheadNow, double leadIn) {
        g_synth.clearSchedule();
        songSpeed = playbackSpeed;
        songStartAudio = g_synth.audioTime() + leadIn - playheadNow / songSpeed;
        backingCursor = 0;
        melodyCursor  = 0;
        demoCursor    = 0;
        demoLastHitTime = -1.0;
        if (songMode) {
            const auto& bn = songMode->chart().backingNotes;
            while (backingCursor < bn.size() && bn[backingCursor].onset < playheadNow) ++backingCursor;
            const auto& pe = songMode->chart().playableEvents;
            while (melodyCursor < pe.size() && pe[melodyCursor].onset < playheadNow) ++melodyCursor;
            while (demoCursor < pe.size() && pe[demoCursor].onset < playheadNow) ++demoCursor;
        }
    };

    // Resolve o pitch REAL que a tecla deve soar: procura a nota do chart mais
    // próxima da hit line para essa tecla; fora da janela, cai no mapa cromático.
    auto pitchForKey = [&](char key) -> int {
        char norm = (char)toupper((unsigned char)key);
        int best = -1; double bestDist = 1e9;
        if (songMode) {
            for (const auto& vn : songMode->getVisibleNotes(0.8)) {
                for (size_t k = 0; k < vn.keys.size() && k < vn.midiNotes.size(); ++k) {
                    if ((char)toupper((unsigned char)vn.keys[k]) != norm) continue;
                    double d = std::abs(vn.timeToHit);
                    if (d < bestDist) { bestDist = d; best = vn.midiNotes[k]; }
                }
            }
        }
        if (best >= 0 && bestDist <= 0.40) return best;
        auto note = freePlay.mapper().noteForKey(key);
        return note ? note->midi : -1;
    };

    // ─── RELOAD CHART ────────────────────────────────────────────────────────
    auto reloadChart = [&]() {
        g_synth.allNotesOff(); // Corta sons residuais da música anterior
        Chart chart = chartGen.generateChart(getActiveSong(), getActiveDiff());
        songMode = std::make_unique<SongModeController>(chart);
        hasFinished = false;
        demoRestartTimer = 0.0;
        keyFeedbacks.clear();
        holdStates.clear();
        heldKeys.clear();
        particles.clear();
        floatingTexts.clear();
        shockwaves.clear();
        demoCursor = 0;
        demoNoteOffTimes.clear();
        soundingPitch.clear();

        songMode->setJudgementCallback([&](const Judgement& j, const PlayableChordGroup& group) {
            for (char k : group.keys) {
                char norm = toupper((unsigned char)k);
                double expireT = songMode->playhead() + 0.42;
                keyFeedbacks[norm] = KeyFeedback{j.type, expireT};

                // Hold state: Miss → Missed (vermelho que some)
                if (j.type == JudgementType::Miss) {
                    holdStates[norm] = HoldState::Missed;
                }

                int cx = getKeyCenterX(norm);
                shockwaves.push_back({cx, 1.0f,
                    (j.type==JudgementType::Perfect) ? SDL_Color{255,215,50,230} :
                    (j.type==JudgementType::Great)   ? SDL_Color{50,230,100,220} :
                    (j.type==JudgementType::Good)    ? SDL_Color{70,180,255,200} :
                                                       SDL_Color{240,40,60,190}});

                // Partículas elegantes
                int pc = (j.type==JudgementType::Perfect)?12:
                         (j.type==JudgementType::Great)  ? 8:
                         (j.type==JudgementType::Good)   ? 4: 0;
                for (int p=0; p<pc; p++) {
                    SDL_Color pCol = (j.type==JudgementType::Perfect) ?
                        (p%3==0 ? SDL_Color{255,255,230,255} : SDL_Color{255,(Uint8)(210*randC(rng)),40,255}) :
                        (j.type==JudgementType::Great) ?
                        (p%2==0 ? SDL_Color{80,255,140,255} : SDL_Color{50,230,100,255}) :
                        SDL_Color{80,200,255,255};
                    particles.push_back({
                        (float)(cx + randVX(rng)*0.12f), (float)(HIT_Y-4),
                        randVX(rng)*0.75f, randVY(rng)*0.75f,
                        1.0f, 1.9f, pCol, (p%2==0)?5:3
                    });
                }
            }

            if (!group.keys.empty()) {
                int cx = getKeyCenterX(group.keys[0]);
                const char* label =
                    j.type==JudgementType::Perfect ? "PERFECT!" :
                    j.type==JudgementType::Great   ? "GREAT!"   :
                    j.type==JudgementType::Good    ? "GOOD"     : "MISS";
                SDL_Color lCol =
                    j.type==JudgementType::Perfect ? SDL_Color{255,215,0,255} :
                    j.type==JudgementType::Great   ? SDL_Color{60,255,100,255} :
                    j.type==JudgementType::Good    ? SDL_Color{80,200,255,255} :
                                                     SDL_Color{255,60,80,255};
                floatingTexts.push_back({label, (float)cx, (float)(HIT_Y-36), -65.f, 1.0f, lCol});
            }

            int combo = songMode->scoringEngine().currentCombo();
            if (combo>0 && combo%5==0) {
                currentPhrase = kComboPhrases[phraseIndex % kComboPhrases.size()];
                phraseIndex++;
                phraseExpireTime = songMode->playhead() + 1.4;
            }
        });

        songMode->setFinishedCallback([&](const ExecutionSummary& summary) {
            finalSummary = summary;
            hasFinished = true;
            g_synth.releaseAllNotes(); // Encerra suavemente notas remanescentes ao acabar
            heldKeys.clear();
            holdStates.clear();
            demoNoteOffTimes.clear();
            demoRestartTimer = 0.0;
            auto rec = summary.toScoreRecord(songMode->chart().songId+"_"+getActiveDiff().name,
                                             currentGame.profile.id);
            currentGame.scores.push_back(rec);
            saveRepo.save(currentGame, savePath);
        });

        songMode->start();
        resyncAudioClock(0.0, kLeadIn);
    };

    reloadChart();

    enum class GameMode { FreePlay, SongMode };
    GameMode currentMode = GameMode::SongMode;
    bool running = true;
    auto lastTime = std::chrono::high_resolution_clock::now();

    // ─────────────────────────────── LOOP PRINCIPAL ──────────────────────────
    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        double rawDt = std::chrono::duration<double>(now-lastTime).count();
        lastTime = now;
        rawDt = std::min(rawDt, 0.1);
        double dt = rawDt * playbackSpeed;

        // ── EVENTOS ────────────────────────────────────────────────────────────
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running=false; }
            else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                SDL_Keycode sym = ev.key.keysym.sym;
                if      (sym==SDLK_ESCAPE) { running=false; }
                else if (sym==SDLK_F1) {
                    currentMode=GameMode::FreePlay;
                    g_synth.allNotesOff();
                    heldKeys.clear();
                    holdStates.clear();
                    demoCursor = 0;
                    demoNoteOffTimes.clear();
                    soundingPitch.clear();
                }
                else if (sym==SDLK_F2) { currentMode=GameMode::SongMode; activeSongIndex=0; reloadChart(); }
                else if (sym==SDLK_F3) { currentMode=GameMode::SongMode; activeSongIndex=1; reloadChart(); }
                else if (sym==SDLK_F4) { currentMode=GameMode::SongMode; activeSongIndex=2; reloadChart(); }
                else if (sym==SDLK_F5) { currentMode=GameMode::SongMode; activeSongIndex=3; reloadChart(); }
                else if (sym==SDLK_F6) { currentMode=GameMode::SongMode; activeSongIndex=4; reloadChart(); }
                else if (sym==SDLK_F7) { currentMode=GameMode::SongMode; activeSongIndex=5; reloadChart(); }
                else if (sym==SDLK_F8) { currentMode=GameMode::SongMode; activeSongIndex=6; reloadChart(); }
                else if (sym==SDLK_F9) { currentMode=GameMode::SongMode; activeSongIndex=7; reloadChart(); }
                else if (sym==SDLK_F10) {
                    // Toggle guia melódico audível
                    autoMelody = !autoMelody;
                    g_synth.clearSchedule();
                    resyncAudioClock(songMode->playhead(), 0.0);
                }
                else if (sym==SDLK_F11) {
                    // Toggle acompanhamento (mão esquerda / vozes internas)
                    backingEnabled = !backingEnabled;
                    g_synth.clearSchedule();
                    resyncAudioClock(songMode->playhead(), 0.0);
                }
                else if (sym==SDLK_F12) {
                    // Toggle DEMO MODE — a máquina toca automaticamente
                    demoMode = !demoMode;
                    hasFinished = false;
                    g_synth.allNotesOff();
                    songMode->restart();
                    demoCursor = 0;
                    demoLastHitTime = -1.0;
                    demoNoteOffTimes.clear();
                    demoRestartTimer = 0.0;
                    heldKeys.clear();
                    holdStates.clear();
                    soundingPitch.clear();
                    resyncAudioClock(0.0, kLeadIn);
                }
                else if (sym==SDLK_TAB) {
                    currentMode=GameMode::SongMode;
                    activeSongIndex=(activeSongIndex+1)%songCatalog.size();
                    reloadChart();
                }
                else if (sym==SDLK_1) { currentDifficulty=1; reloadChart(); }
                else if (sym==SDLK_2) { currentDifficulty=2; reloadChart(); }
                else if (sym==SDLK_3) { currentDifficulty=3; reloadChart(); }
                else if (sym==SDLK_MINUS||sym==SDLK_KP_MINUS)
                    lookahead=std::min(6.0, lookahead+0.5);
                else if (sym==SDLK_EQUALS||sym==SDLK_PLUS||sym==SDLK_KP_PLUS)
                    lookahead=std::max(1.5, lookahead-0.5);
                else if (sym==SDLK_LEFTBRACKET) {
                    playbackSpeed=std::max(0.5,playbackSpeed-0.25);
                    resyncAudioClock(songMode->playhead(), 0.0);
                }
                else if (sym==SDLK_RIGHTBRACKET) {
                    playbackSpeed=std::min(1.5,playbackSpeed+0.25);
                    resyncAudioClock(songMode->playhead(), 0.0);
                }
                // REINICIAR: usa Enter (não usa letras — toda letra A-Z é tecla do piano!)
                else if (sym==SDLK_RETURN || sym==SDLK_KP_ENTER) {
                    hasFinished = false;
                    g_synth.allNotesOff();
                    heldKeys.clear();
                    holdStates.clear();
                    demoCursor = 0;
                    demoLastHitTime = -1.0;
                    demoNoteOffTimes.clear();
                    demoRestartTimer = 0.0;
                    songMode->restart();
                    soundingPitch.clear();
                    resyncAudioClock(0.0, kLeadIn);
                }
                else if (sym==SDLK_SPACE) { freePlay.setSustain(true); g_synth.setSustain(true); }
                else if (sym==SDLK_UP)   freePlay.shiftOctaveUp();
                else if (sym==SDLK_DOWN) freePlay.shiftOctaveDown();
                else if (sym>=SDLK_a && sym<=SDLK_z) {
                    char key = (char)('A'+(sym-SDLK_a));
                    if (currentMode==GameMode::FreePlay) {
                        freePlay.onKeyDown(key);
                    } else {
                        heldKeys.insert(key);
                        holdStates[key] = HoldState::Holding;
                        int pitch = pitchForKey(key);
                        songMode->onKeyDown(key);
                        // Com o guia melódico ligado a nota já está agendada no tom
                        // certo; disparar aqui de novo só duplicaria o ataque.
                        if (pitch >= 0 && !autoMelody) {
                            soundingPitch[key] = pitch;
                            g_synth.noteOn(pitch, 0.95f);
                        }
                    }
                }
            }
            else if (ev.type == SDL_KEYUP) {
                SDL_Keycode sym = ev.key.keysym.sym;
                if (sym==SDLK_SPACE) { freePlay.setSustain(false); g_synth.setSustain(false); }
                else if (sym>=SDLK_a && sym<=SDLK_z) {
                    char key = (char)('A'+(sym-SDLK_a));
                    if (currentMode==GameMode::FreePlay) {
                        freePlay.onKeyUp(key);
                    } else {
                        heldKeys.erase(key);
                        // Se era holding, muda para released (cinza)
                        auto it = holdStates.find(key);
                        if (it != holdStates.end() && it->second == HoldState::Holding) {
                            it->second = HoldState::Released;
                        }
                        auto sp = soundingPitch.find(key);
                        if (sp != soundingPitch.end()) { g_synth.noteOff(sp->second); soundingPitch.erase(sp); }
                    }
                }
            }
        }

        if (currentMode==GameMode::SongMode) {
            // ── RELÓGIO MESTRE: ÁUDIO ─────────────────────────────────────────
            // O playhead é DERIVADO do contador de amostras do device, nunca
            // acumulado a partir do delta de frame. Um engasgo de render não
            // dessincroniza mais nada: o vídeo pula, o áudio continua, e o
            // playhead segue o áudio.
            double target = (g_synth.audioTime() - songStartAudio) * songSpeed;

            if (demoMode && !hasFinished) {
                // ── DEMO AUTO-PLAYER HUMANO & MULTI-GRADE ─────────────────────
                // Em demo, avança o playhead passo a passo até cada nota e pressiona
                // com timing humano orgânico (72% Perfect, 20% Great, 8% Good, 0% Miss).
                const auto& pe = songMode->chart().playableEvents;
                const auto& hw = songMode->chart().difficulty.hitWindow;

                while (demoCursor < pe.size()) {
                    const auto& g = pe[demoCursor];
                    double nextGap = (demoCursor + 1 < pe.size()) ? (pe[demoCursor+1].onset - g.onset) : 1.0;
                    if (nextGap < 0.001) nextGap = 0.001;

                    int seed = (demoCursor * 37 + 13) % 100;
                    double humanOffset = 0.0;
                    if (seed < 72) {
                        double frac = ((seed % 19) - 9) / 9.0;
                        humanOffset = frac * (hw.perfect * 0.40) / 1000.0;
                    } else if (seed < 92) {
                        double sign = (seed % 2 == 0) ? 1.0 : -1.0;
                        double span = hw.great - hw.perfect;
                        humanOffset = sign * (hw.perfect + 0.15 * span + ((seed % 7) / 7.0) * (0.50 * span)) / 1000.0;
                    } else {
                        double sign = (seed % 2 == 0) ? 1.0 : -1.0;
                        double span = hw.good - hw.great;
                        humanOffset = sign * (hw.great + 0.15 * span + ((seed % 7) / 7.0) * (0.45 * span)) / 1000.0;
                    }

                    // Se a próxima nota for muito próxima (ex: semicolcheias rápidas),
                    // limita a variação para as notas não se atropelarem
                    if (nextGap < 0.25) {
                        double cap = nextGap * 0.35;
                        if (humanOffset > cap)  humanOffset = cap;
                        if (humanOffset < -cap) humanOffset = -cap;
                    }

                    double hitTime = g.onset + humanOffset;
                    if (hitTime < demoLastHitTime + 0.010) hitTime = demoLastHitTime + 0.010;
                    double maxSafe = g.onset + (hw.good - 25.0) / 1000.0;
                    if (hitTime > maxSafe) hitTime = maxSafe;

                    if (hitTime > target) {
                        break; // Próxima nota ainda não chegou
                    }

                    // Avança o playhead até o instante exato do hit humano
                    double step = hitTime - songMode->playhead();
                    if (step > 0.0) {
                        songMode->update(step);
                    }

                    if (!songMode->isGroupJudged(demoCursor)) {
                        for (size_t k = 0; k < g.keys.size(); ++k) {
                            char key = g.keys[k];
                            double dur = (k < g.durations.size()) ? g.durations[k] : 0.3;
                            songMode->onKeyDown(key);
                            heldKeys.insert(key);
                            holdStates[key] = HoldState::Holding;
                            demoNoteOffTimes[key] = songMode->playhead() + std::max(dur - 0.03, 0.06);
                        }
                    }
                    demoLastHitTime = songMode->playhead();
                    demoCursor++;
                }

                // Avança o restante até target
                double rem = target - songMode->playhead();
                if (rem > 0.0) songMode->update(rem);

                // Processa as solturas agendadas
                double phNow = songMode->playhead();
                for (auto it = demoNoteOffTimes.begin(); it != demoNoteOffTimes.end(); ) {
                    if (phNow >= it->second) {
                        char key = it->first;
                        heldKeys.erase(key);
                        auto hsIt = holdStates.find(key);
                        if (hsIt != holdStates.end() && hsIt->second == HoldState::Holding)
                            hsIt->second = HoldState::Idle;
                        it = demoNoteOffTimes.erase(it);
                    } else {
                        ++it;
                    }
                }
            } else {
                // Modo jogador normal: playhead segue o áudio continuamente
                double delta = target - songMode->playhead();
                if (delta > 0.0) songMode->update(delta);
            }

            const double ph      = songMode->playhead();
            const double horizon = ph + kScheduleAhead;

            auto audioTimeOf = [&](double onset) {
                return songStartAudio + onset / songSpeed;
            };

            // ── AGENDA O ACOMPANHAMENTO ──────────────────────────────────────
            if (backingEnabled) {
                const auto& bn = songMode->chart().backingNotes;
                while (backingCursor < bn.size() && bn[backingCursor].onset <= horizon) {
                    const auto& n = bn[backingCursor++];
                    float vel = std::clamp((float)n.velocity / 127.0f, 0.05f, 1.0f) * 0.55f;
                    g_synth.scheduleNoteOn (audioTimeOf(n.onset), n.midiNote, vel);
                    g_synth.scheduleNoteOff(audioTimeOf(n.onset + std::max(0.08, n.duration)), n.midiNote);
                }
            }

            // ── AGENDA O GUIA MELÓDICO ───────────────────────────────────────
            if (autoMelody) {
                const auto& pe = songMode->chart().playableEvents;
                while (melodyCursor < pe.size() && pe[melodyCursor].onset <= horizon) {
                    const size_t gi = melodyCursor++;
                    if (songMode->isGroupJudged(gi)) continue; // jogador já tocou
                    const auto& g = pe[gi];
                    for (size_t k = 0; k < g.midiNotes.size() && k < g.durations.size(); ++k) {
                        g_synth.scheduleNoteOn (audioTimeOf(g.onset), g.midiNotes[k], 0.92f);
                        g_synth.scheduleNoteOff(audioTimeOf(g.onset + std::max(0.12, g.durations[k])),
                                                g.midiNotes[k]);
                    }
                }
            }
        }

        // Se o modo demo terminou, aguarda 2.5s suavemente e reinicia limpo
        if (demoMode && currentMode == GameMode::SongMode && hasFinished) {
            demoRestartTimer += rawDt;
            if (demoRestartTimer >= 2.5) {
                demoRestartTimer = 0.0;
                hasFinished = false;
                g_synth.allNotesOff();
                songMode->restart();
                demoCursor = 0;
                demoLastHitTime = -1.0;
                demoNoteOffTimes.clear();
                heldKeys.clear();
                holdStates.clear();
                soundingPitch.clear();
                resyncAudioClock(0.0, kLeadIn);
            }
        }

        // ── PHYSICS FX ─────────────────────────────────────────────────────────
        for (auto& p : particles) {
            p.x += p.vx*(float)rawDt; p.y += p.vy*(float)rawDt;
            p.vy += 200.f*(float)rawDt;
            p.life -= p.decay*(float)rawDt;
        }
        particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Particle& p){return p.life<=0;}),particles.end());

        for (auto& ft : floatingTexts) {
            ft.y += ft.vy*(float)rawDt;
            ft.life -= 1.8f*(float)rawDt;
        }
        floatingTexts.erase(std::remove_if(floatingTexts.begin(),floatingTexts.end(),[](const FloatingJudgement& f){return f.life<=0;}),floatingTexts.end());

        for (auto& sw : shockwaves) sw.life -= 3.5f*(float)rawDt;
        shockwaves.erase(std::remove_if(shockwaves.begin(),shockwaves.end(),[](const LaneShockwave& s){return s.life<=0;}),shockwaves.end());

        // Limpeza do estado de hold: Missed e Released somem após cooldown curto (gerenciado por feedback)
        for (auto& [k,st] : holdStates) {
            auto it = keyFeedbacks.find(k);
            bool fbExpired = (it==keyFeedbacks.end() || it->second.expireTime <= songMode->playhead());
            if (st==HoldState::Missed && fbExpired) st=HoldState::Idle;
            if (st==HoldState::Released && fbExpired) st=HoldState::Idle;
        }

        // ══════════════════════ RENDERIZAÇÃO ═══════════════════════════════════
        // Fundo: azul noturno profundo com gradiente sutil
        SDL_SetRenderDrawColor(ren, 8, 12, 22, 255);
        SDL_RenderClear(ren);

        // Gradiente de fundo na play zone
        {
            SDL_Rect bgZone{0, HUD_H, SW, HIT_Y-HUD_H};
            renderGradientRect(ren, bgZone,
                {10, 14, 26, 255}, {6, 9, 18, 255});
        }

        // ─── NOTAS VISÍVEIS ─────────────────────────────────────────────────
        // Computar notas e estado de impacto ANTES do teclado para sensores
        std::vector<VisibleNote> visNotes;
        std::set<char> keysAtHitLine;
        if (currentMode==GameMode::SongMode && !hasFinished) {
            visNotes = songMode->getVisibleNotes(lookahead);
            for (const auto& vn : visNotes) {
                if (!vn.isJudged && std::abs(vn.timeToHit)<=0.14) {
                    for (char k : vn.keys)
                        keysAtHitLine.insert(toupper((unsigned char)k));
                }
            }
        }

        // ─── TRILHAS (LANES) ────────────────────────────────────────────────
        if (currentMode==GameMode::SongMode) {
            for (const auto& vk : kVisualKeys) {
                SDL_Rect r = getKeyRect(vk.key);
                int cx = r.x + r.w/2;
                bool atHit = keysAtHitLine.count(vk.key)>0;
                // Lane com leve brilho se nota próxima
                if (atHit) {
                    SDL_SetRenderDrawColor(ren, 40, 90, 40, 60);
                    SDL_Rect laneRect{cx-KEY_W/2, HORIZON_Y, KEY_W, HIT_Y-HORIZON_Y};
                    SDL_RenderFillRect(ren, &laneRect);
                }
                SDL_SetRenderDrawColor(ren, atHit?50:25, atHit?70:30, atHit?50:50, atHit?120:50);
                SDL_RenderDrawLine(ren, cx, HORIZON_Y, cx, HIT_Y);
            }
        }

        // ─── SHOCKWAVES ─────────────────────────────────────────────────────
        for (const auto& sw : shockwaves) {
            if (sw.life>0) {
                Uint8 a = (Uint8)std::clamp(sw.life*200.f, 0.f, 200.f);
                SDL_SetRenderDrawColor(ren, sw.color.r, sw.color.g, sw.color.b, a);
                int bw = (int)(10 + (1.f-sw.life)*35.f);
                SDL_Rect beam{sw.centerX-bw/2, HIT_Y-80, bw, 80};
                SDL_RenderFillRect(ren, &beam);
            }
        }

        // ─── NOTAS ─────────────────────────────────────────────────────────
        const int NOTE_HEAD_H = 28;
        if (currentMode==GameMode::SongMode && !hasFinished) {
            for (const auto& vn : visNotes) {
                double frac = vn.timeToHit / lookahead;
                int noteBottomY = HORIZON_Y + (int)((1.0-frac)*(HIT_Y-HORIZON_Y));
                int noteY = noteBottomY - NOTE_HEAD_H;
                bool isTouching = (std::abs(vn.timeToHit)<=0.13);

                // Barra de conexão em acordes (>1 tecla)
                if (vn.keys.size()>1 && !vn.isJudged) {
                    int minX=SW, maxX=0;
                    for (char k : vn.keys) {
                        SDL_Rect box = getKeyRect(k);
                        minX = std::min(minX, box.x+box.w/2);
                        maxX = std::max(maxX, box.x+box.w/2);
                    }
                    SDL_SetRenderDrawColor(ren, 80,200,255,120);
                    for (int d=-1;d<=1;d++) SDL_RenderDrawLine(ren,minX,noteY+NOTE_HEAD_H/2+d,maxX,noteY+NOTE_HEAD_H/2+d);
                }

                for (size_t k=0; k<vn.keys.size(); k++) {
                    char keyChar = vn.keys[k];
                    double dur = (k<vn.durations.size()) ? vn.durations[k] : 0.3;
                    SDL_Rect keyBox = getKeyRect(keyChar);
                    int nW = KEY_W - 6;
                    int nX = keyBox.x + 3;
                    int tailH = (int)((dur/lookahead)*(HIT_Y-HORIZON_Y));
                    bool isLongNote = (dur >= 0.5);

                    // ── CAUDA (HOLD RIBBON) ──
                    if (tailH > 12) {
                        int tailTopY  = noteBottomY - tailH;
                        int cTop      = std::max(HORIZON_Y, tailTopY);
                        int cBot      = std::min(HIT_Y+18, noteY + NOTE_HEAD_H);

                        if (cBot > cTop) {
                            // Corpo da cauda
                            int tw = nW - 14;
                            SDL_Rect tailR{nX+7, cTop, tw, cBot-cTop};

                            if (isTouching) {
                                // Verde pulsante quando no hit
                                SDL_SetRenderDrawColor(ren, 0, 180, 80, 190);
                                SDL_RenderFillRect(ren, &tailR);
                                SDL_SetRenderDrawColor(ren, 0, 255, 100, 240);
                            } else {
                                // Azul profundo com gradiente via múltiplas linhas
                                renderGradientRect(ren, tailR, {50,130,230,180}, {20,80,180,160});
                                SDL_SetRenderDrawColor(ren, 100, 200, 255, 180);
                            }
                            // Bordas laterais luminosas
                            SDL_RenderDrawLine(ren, tailR.x, tailR.y, tailR.x, tailR.y+tailR.h);
                            SDL_RenderDrawLine(ren, tailR.x+tailR.w-1, tailR.y, tailR.x+tailR.w-1, tailR.y+tailR.h);
                            // Topo da cauda
                            if (tailTopY >= HORIZON_Y) {
                                SDL_RenderDrawLine(ren, tailR.x, tailR.y, tailR.x+tailR.w, tailR.y);
                                SDL_RenderDrawLine(ren, tailR.x, tailR.y+1, tailR.x+tailR.w, tailR.y+1);
                            }
                        }
                    }

                    // ── CABEÇA DA NOTA ──
                    if (!vn.isJudged && noteY+NOTE_HEAD_H>=HORIZON_Y && noteY<=HIT_Y+28) {
                        SDL_Rect head{nX, noteY, nW, NOTE_HEAD_H};

                        if (isTouching) {
                            // Verde neon vivo — TOQUE AGORA!
                            renderGradientRect(ren, head, {30,255,110,255}, {0,200,80,255});
                            SDL_SetRenderDrawColor(ren, 220,255,230,255);
                            SDL_RenderDrawRect(ren, &head);
                            // Halo externo
                            SDL_Rect halo{head.x-2,head.y-2,head.w+4,head.h+4};
                            SDL_SetRenderDrawColor(ren, 0,255,100,80);
                            SDL_RenderDrawRect(ren, &halo);
                        } else if (isLongNote) {
                            // Nota longa: azul elétrico com borda dourada
                            renderGradientRect(ren, head, {40,150,240,255}, {20,100,200,255});
                            SDL_SetRenderDrawColor(ren, 255,200,80,255);
                            SDL_RenderDrawRect(ren, &head);
                        } else {
                            // Nota curta: azul ciano
                            renderGradientRect(ren, head, {50,160,255,245}, {30,120,220,245});
                            SDL_SetRenderDrawColor(ren, 120,220,255,255);
                            SDL_RenderDrawRect(ren, &head);
                        }

                        // Letra da tecla no centro
                        std::string letter(1, keyChar);
                        renderText(ren, fontSmall, letter,
                                   nX+nW/2, noteY+NOTE_HEAD_H/2,
                                   {240,248,255,255}, true);
                    }
                }
            }
        }

        // ─── LINHA DE HORIZONTE ─────────────────────────────────────────────
        {
            SDL_SetRenderDrawColor(ren, 40, 70, 130, 200);
            SDL_RenderDrawLine(ren, 0, HORIZON_Y, SW, HORIZON_Y);
            SDL_SetRenderDrawColor(ren, 60, 100, 160, 80);
            SDL_RenderDrawLine(ren, 0, HORIZON_Y+1, SW, HORIZON_Y+1);
        }

        // ─── LINHA DE IMPACTO (HIT LINE) — DOURADA NEON ─────────────────────
        {
            // Halo suave abaixo
            SDL_SetRenderDrawColor(ren, 255, 215, 0, 18);
            for (int d=1;d<=8;d++) SDL_RenderDrawLine(ren, 0, HIT_Y+d, SW, HIT_Y+d);
            // Halo suave acima
            for (int d=1;d<=8;d++) SDL_RenderDrawLine(ren, 0, HIT_Y-d, SW, HIT_Y-d);
            // Linha principal (3px)
            SDL_SetRenderDrawColor(ren, 255, 215, 0, 255);
            SDL_RenderDrawLine(ren, 0, HIT_Y-1, SW, HIT_Y-1);
            SDL_RenderDrawLine(ren, 0, HIT_Y,   SW, HIT_Y);
            SDL_RenderDrawLine(ren, 0, HIT_Y+1, SW, HIT_Y+1);
            // Inner white line
            SDL_SetRenderDrawColor(ren, 255, 248, 200, 180);
            SDL_RenderDrawLine(ren, 0, HIT_Y, SW, HIT_Y);
        }

        // Sensores na hit line — acendem verde quando nota chega
        if (currentMode==GameMode::SongMode) {
            for (const auto& vk : kVisualKeys) {
                SDL_Rect r = getKeyRect(vk.key);
                int cx = r.x + r.w/2;
                bool onHit = keysAtHitLine.count(vk.key)>0;
                if (onHit) {
                    // Sensor verde neon grande e brilhante
                    SDL_SetRenderDrawColor(ren, 0, 255, 100, 255);
                    SDL_Rect s{cx-14, HIT_Y-8, 28, 17};
                    SDL_RenderFillRect(ren, &s);
                    SDL_SetRenderDrawColor(ren, 220,255,230,255);
                    SDL_RenderDrawRect(ren, &s);
                    // Halo externo
                    SDL_SetRenderDrawColor(ren, 0,255,100,60);
                    SDL_Rect sHalo{s.x-3,s.y-3,s.w+6,s.h+6};
                    SDL_RenderDrawRect(ren, &sHalo);
                } else {
                    SDL_SetRenderDrawColor(ren, 200, 165, 30, 120);
                    SDL_Rect s{cx-9, HIT_Y-3, 18, 7};
                    SDL_RenderFillRect(ren, &s);
                }
            }
        }

        // ─── PARTÍCULAS ─────────────────────────────────────────────────────
        for (const auto& p : particles) {
            if (p.life<=0) continue;
            Uint8 a=(Uint8)std::clamp(p.life*255.f,0.f,255.f);
            SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b, a);
            SDL_Rect pr{(int)(p.x-p.size/2),(int)(p.y-p.size/2),p.size,p.size};
            SDL_RenderFillRect(ren, &pr);
        }

        // ─── TEXTOS FLUTUANTES ──────────────────────────────────────────────
        for (const auto& ft : floatingTexts) {
            if (ft.life<=0) continue;
            SDL_Color fc=ft.color; fc.a=(Uint8)std::clamp(ft.life*255.f,0.f,255.f);
            renderText(ren, fontMedium, ft.text, (int)ft.x, (int)ft.y, fc, true);
        }

        // ─── TECLADO VISUAL ─────────────────────────────────────────────────
        // Fundo da zona do teclado — mais escuro
        {
            SDL_Rect kbBg{0, KBOARD_Y-8, SW, SH-(KBOARD_Y-8)};
            SDL_SetRenderDrawColor(ren, 8, 10, 18, 255);
            SDL_RenderFillRect(ren, &kbBg);
            SDL_SetRenderDrawColor(ren, 30, 40, 65, 255);
            SDL_RenderDrawLine(ren, 0, KBOARD_Y-8, SW, KBOARD_Y-8);
        }

        for (const auto& vk : kVisualKeys) {
            SDL_Rect r = getKeyRect(vk.key);
            bool isPhysPressed = heldKeys.count(vk.key)>0;
            bool onHit = keysAtHitLine.count(vk.key)>0;
            auto itFb = keyFeedbacks.find(vk.key);
            bool hasFb = (itFb!=keyFeedbacks.end() &&
                          itFb->second.expireTime > songMode->playhead());
            auto itHold = holdStates.find(vk.key);
            HoldState hState = (itHold!=holdStates.end()) ? itHold->second : HoldState::Idle;

            // ── Determina cor de fundo da tecla ──
            Uint8 rr,gg,bb;
            if (hasFb) {
                switch(itFb->second.type) {
                    case JudgementType::Perfect: rr=255;gg=210;bb=30;  break; // ouro
                    case JudgementType::Great:   rr=40; gg=230;bb=100; break; // esmeralda
                    case JudgementType::Good:    rr=40; gg=160;bb=255; break; // ciano
                    case JudgementType::Miss:    rr=230;gg=30; bb=55;  break; // vermelho vivo
                }
            } else if (isPhysPressed || hState==HoldState::Holding) {
                // TECLA PRESSIONADA / SUSTENTANDO — VERDE VIVO
                rr=20; gg=200; bb=80;
            } else if (hState==HoldState::Released) {
                // SOLTOU A TECLA — CINZA MÉDIO
                rr=80; gg=85; bb=100;
            } else if (hState==HoldState::Missed) {
                // ERROU — VERMELHO QUE SOME
                rr=180; gg=20; bb=40;
            } else if (onHit) {
                // Nota chegando — verde escuro sutil como dica
                rr=18; gg=75; bb=40;
            } else {
                // Normal — slate azul escuro com sutil gradiente
                rr=22; gg=27; bb=44;
            }

            // Gradiente vertical na tecla (topo mais claro, base mais escura)
            renderGradientRect(ren, r,
                {(Uint8)std::min(255,rr+20),(Uint8)std::min(255,gg+20),(Uint8)std::min(255,bb+20),255},
                {rr,gg,bb,255});

            // Highlight sutil no topo da tecla (borda superior mais clara = efeito 3D)
            SDL_SetRenderDrawColor(ren,
                std::min(255,rr+50), std::min(255,gg+50), std::min(255,bb+60), 180);
            SDL_RenderDrawLine(ren, r.x+2, r.y+1, r.x+r.w-3, r.y+1);

            // Borda da tecla
            if (hasFb && itFb->second.type==JudgementType::Perfect) {
                SDL_SetRenderDrawColor(ren, 255,255,200,255);
            } else if (hasFb && itFb->second.type==JudgementType::Great) {
                SDL_SetRenderDrawColor(ren, 160,255,180,255);
            } else if (isPhysPressed || hState==HoldState::Holding) {
                SDL_SetRenderDrawColor(ren, 80,255,140,255); // borda verde neon
            } else if (hState==HoldState::Missed) {
                SDL_SetRenderDrawColor(ren, 255,60,80,255);
            } else if (onHit) {
                SDL_SetRenderDrawColor(ren, 50,255,120,255);
            } else {
                SDL_SetRenderDrawColor(ren, 50,62,95,255);
            }
            SDL_RenderDrawRect(ren, &r);
            // Borda interna extra para look profissional
            SDL_Rect innerBorder{r.x+1,r.y+1,r.w-2,r.h-2};
            SDL_SetRenderDrawColor(ren, 30,38,60,120);
            SDL_RenderDrawRect(ren, &innerBorder);

            // Letra grande da tecla
            SDL_Color keyLetterColor =
                (isPhysPressed||hState==HoldState::Holding) ? SDL_Color{220,255,230,255} :
                hasFb ? SDL_Color{255,255,255,255} :
                SDL_Color{200,210,230,255};
            std::string kStr(1, vk.key);
            renderText(ren, fontMedium, kStr, r.x+r.w/2, r.y+18, keyLetterColor, true);

            // Nome da nota abaixo (menor)
            auto note = freePlay.mapper().noteForKey(vk.key);
            if (note) {
                SDL_Color noteNameColor =
                    (isPhysPressed||hState==HoldState::Holding) ? SDL_Color{160,255,190,220} :
                    SDL_Color{100,120,160,200};
                renderText(ren, fontTiny, note->name, r.x+r.w/2, r.y+42, noteNameColor, true);
            }
        }

        // ─── HUD (TOPO) ─────────────────────────────────────────────────────
        // Fundo do HUD com gradiente
        {
            SDL_Rect hudBg{0,0,SW,HUD_H};
            renderGradientRect(ren, hudBg, {14,18,32,255}, {10,14,26,255});
            // Linha separadora luminosa
            SDL_SetRenderDrawColor(ren, 50,70,130,255);
            SDL_RenderDrawLine(ren, 0,HUD_H,SW,HUD_H);
            SDL_SetRenderDrawColor(ren, 70,100,180,100);
            SDL_RenderDrawLine(ren, 0,HUD_H-1,SW,HUD_H-1);
        }

        // HUD: título + compositor + badge de dificuldade (esquerda)
        if (currentMode==GameMode::FreePlay) {
            renderText(ren, fontMedium, "♪  FREE PLAY — Toque Livre", 18, 6, {100,200,255,255});
            renderText(ren, fontSmall,
                "[F2-F9] Músicas  [1/2/3] Dificuldade  [TAB] Próxima  [F1] Song Mode  [ESC] Sair",
                18, 34, {100,120,165,255});
        } else {
            const Song& song = getActiveSong();
            // Linha 1: ♪ Título
            renderText(ren, fontMedium, "♪  " + song.title, 18, 4, {255,215,0,255});
            // Linha 2: compositor
            renderText(ren, fontTiny, song.composer, 18, 30, {160,165,200,200});

            // Badge de dificuldade (colado ao título)
            const char* diffName = (currentDifficulty==1)?"EASY":(currentDifficulty==2)?"NORMAL":"HARD";
            SDL_Color diffBadgeCol =
                (currentDifficulty==1) ? SDL_Color{30,200,100,255} :
                (currentDifficulty==2) ? SDL_Color{70,180,255,255} :
                                         SDL_Color{255,140,40,255};
            int badgeX = 430;
            SDL_Rect badgeBg{badgeX, 6, (int)(strlen(diffName)*9+18), 22};
            SDL_SetRenderDrawColor(ren, diffBadgeCol.r/5, diffBadgeCol.g/5, diffBadgeCol.b/5, 220);
            SDL_RenderFillRect(ren, &badgeBg);
            SDL_SetRenderDrawColor(ren, diffBadgeCol.r, diffBadgeCol.g, diffBadgeCol.b, 255);
            SDL_RenderDrawRect(ren, &badgeBg);
            renderText(ren, fontSmall, diffName, badgeX+8, 10, diffBadgeCol);

            // Linha 3 (menor): Navegação compacta — abaixo do compositor, restrita à metade esquerda
            std::ostringstream nav;
            nav << "[F2-F9] Musica  [1/2/3] Dif  [TAB] Prox  [-/+] Queda:"
                << std::fixed << std::setprecision(1) << lookahead << "s  [Enter] Reiniciar  [F10] Guia  [F11] Acomp  [F12] DEMO";
            renderText(ren, fontTiny, nav.str(), 18, 48, {80,95,145,200});
        }

        // HUD: Score / Combo / Precisão / Grade (direita) — 4 colunas fixas, sem sobreposição
        if (currentMode==GameMode::SongMode) {
            const ScoringEngine& sc = songMode->scoringEngine();
            int64_t score = sc.currentScore();
            int combo     = sc.currentCombo();
            float acc     = sc.currentAccuracy()*100.f;
            std::string grade = gradeToString(sc.currentGrade());

            // Fundo semi-transparente nas 4 colunas para legibilidade
            SDL_SetRenderDrawColor(ren, 15,20,40,160);
            SDL_Rect statsBg{SW-470, 1, 468, HUD_H-2};
            SDL_RenderFillRect(ren, &statsBg);
            SDL_SetRenderDrawColor(ren, 40,55,95,180);
            SDL_RenderDrawLine(ren, SW-470, 1, SW-470, HUD_H-2);

            // 4 colunas: x fixo para cada stat
            const int COL_SCORE  = SW-455;
            const int COL_COMBO  = SW-330;
            const int COL_ACC    = SW-195;
            const int COL_GRADE  = SW-60;

            // SCORE
            renderText(ren, fontTiny, "SCORE", COL_SCORE, 5, {110,130,185,200});
            { std::ostringstream ss; ss << score;
              renderText(ren, fontLarge, ss.str(), COL_SCORE, 20, {240,245,255,255}); }

            // COMBO
            SDL_Color comboCol = combo>=10?SDL_Color{255,150,30,255}:SDL_Color{70,200,255,255};
            renderText(ren, fontTiny, "COMBO", COL_COMBO, 5, {110,130,185,200});
            { std::ostringstream ss; ss << "x" << combo;
              renderText(ren, fontLarge, ss.str(), COL_COMBO, 20, comboCol); }

            // PRECISAO
            SDL_Color accCol = acc>=95?SDL_Color{30,255,120,255}:acc>=70?SDL_Color{220,220,60,255}:SDL_Color{220,100,60,255};
            renderText(ren, fontTiny, "PRECISAO", COL_ACC, 5, {110,130,185,200});
            { std::ostringstream ss; ss << std::fixed << std::setprecision(1) << acc << "%";
              renderText(ren, fontLarge, ss.str(), COL_ACC, 20, accCol); }

            // GRADE
            SDL_Color gradeCol =
                grade=="S"?SDL_Color{255,200,0,255}:
                grade=="A"?SDL_Color{60,220,120,255}:
                grade=="B"?SDL_Color{70,180,255,255}:
                grade=="C"?SDL_Color{200,180,60,255}:
                           SDL_Color{200,80,80,255};
            renderText(ren, fontTiny, "GRADE", COL_GRADE, 5, {110,130,185,200});
            renderText(ren, fontLarge, grade, COL_GRADE, 20, gradeCol);

            // Combo phrase centralizada (se ativa)
            if (!currentPhrase.empty() && phraseExpireTime > songMode->playhead()) {
                float t = (float)std::min(1.0, phraseExpireTime - songMode->playhead());
                Uint8 a = (Uint8)(t * 255.f);
                renderText(ren, fontSmall, "★ " + currentPhrase + " ★", SW/2, 8, {255,215,0,a}, true);
            }

            // Badge DEMO piscante no centro do HUD
            if (demoMode) {
                // Pisca a ~2Hz usando o playhead
                bool blinkOn = (std::fmod(songMode->playhead(), 0.5) < 0.35);
                if (blinkOn) {
                    SDL_Rect demoBg{SW/2-48, 6, 96, 28};
                    SDL_SetRenderDrawColor(ren, 180,30,220,220);
                    SDL_RenderFillRect(ren, &demoBg);
                    SDL_SetRenderDrawColor(ren, 255,100,255,255);
                    SDL_RenderDrawRect(ren, &demoBg);
                    renderText(ren, fontSmall, "◉ DEMO", SW/2, 20, {255,255,255,255}, true);
                }
            }
        }

        // ─── BARRA DE PROGRESSO DA MÚSICA ───────────────────────────────────
        if (currentMode==GameMode::SongMode && !hasFinished) {
            const auto& events = songMode->chart().playableEvents;
            if (!events.empty()) {
                double totalTime = events.back().onset + 2.0;
                double progress  = std::clamp(songMode->playhead() / totalTime, 0.0, 1.0);
                int barX=18, barY=HUD_H-7, barW=SW-36, barH=4;
                // Fundo
                SDL_SetRenderDrawColor(ren, 30,40,65,255);
                SDL_Rect bgBar{barX,barY,barW,barH};
                SDL_RenderFillRect(ren, &bgBar);
                // Progresso
                SDL_SetRenderDrawColor(ren, 255,215,0,200);
                SDL_Rect progBar{barX, barY, (int)(barW*progress), barH};
                SDL_RenderFillRect(ren, &progBar);
            }
        }

        // ─── TELA DE RESULTADOS ──────────────────────────────────────────────
        if (hasFinished) {
            // Overlay principal
            SDL_Rect overlay{SW/2-370, SH/2-210, 740, 420};
            SDL_SetRenderDrawColor(ren, 5,8,20,230);
            SDL_RenderFillRect(ren, &overlay);

            // Borda colorida por grade
            SDL_Color borderCol =
                finalSummary.grade==Grade::S?SDL_Color{255,200,0,255}:
                finalSummary.grade==Grade::A?SDL_Color{60,220,120,255}:
                finalSummary.grade==Grade::B?SDL_Color{70,180,255,255}:
                finalSummary.grade==Grade::C?SDL_Color{200,180,60,255}:
                                             SDL_Color{200,80,80,255};
            SDL_SetRenderDrawColor(ren, borderCol.r, borderCol.g, borderCol.b, 255);
            SDL_RenderDrawRect(ren, &overlay);
            SDL_Rect ov2{overlay.x+1,overlay.y+1,overlay.w-2,overlay.h-2};
            SDL_SetRenderDrawColor(ren, borderCol.r/4, borderCol.g/4, borderCol.b/4, 180);
            SDL_RenderDrawRect(ren, &ov2);

            int cx = overlay.x + overlay.w/2; // centro horizontal do overlay

            // ── Título
            renderText(ren, fontLarge, "MUSICA CONCLUIDA!", cx, overlay.y+28, {255,215,0,255}, true);

            // ── Grade enorme
            std::string grStr = gradeToString(finalSummary.grade);
            renderText(ren, fontHuge, grStr, cx, overlay.y+75, borderCol, true);

            // ── Divisor
            SDL_SetRenderDrawColor(ren, 50,65,115,220);
            SDL_RenderDrawLine(ren, overlay.x+40, overlay.y+160, overlay.x+overlay.w-40, overlay.y+160);
            SDL_SetRenderDrawColor(ren, 35,48,85,150);
            SDL_RenderDrawLine(ren, overlay.x+40, overlay.y+161, overlay.x+overlay.w-40, overlay.y+161);

            // ── Linha de stats compacta
            {
                std::ostringstream ss;
                ss << "Pontuacao: " << finalSummary.totalScore
                   << "   Combo Max: " << finalSummary.maxCombo
                   << "   Precisao: " << std::fixed << std::setprecision(1)
                   << (finalSummary.accuracy*100.f) << "%";
                renderText(ren, fontSmall, ss.str(), cx, overlay.y+178, {210,218,245,255}, true);
            }

            // ── 4 colunas centralizadas (PERFECT / GREAT / GOOD / MISS)
            // Cada coluna: centro em cX, rótulo acima, número grande abaixo
            struct StatCol {
                int cx;
                const char* label;
                int count;
                SDL_Color color;
            };
            int colY_label = overlay.y + 212;
            int colY_num   = overlay.y + 234;
            int colSpacing  = overlay.w / 4; // 185px cada
            StatCol statCols[4] = {
                { overlay.x + colSpacing/2,        "PERFECT", finalSummary.perfectCount, {255,215,0,255}   },
                { overlay.x + colSpacing + colSpacing/2,  "GREAT",   finalSummary.greatCount,   {60,255,100,255}  },
                { overlay.x + 2*colSpacing + colSpacing/2,"GOOD",    finalSummary.goodCount,    {70,180,255,255}  },
                { overlay.x + 3*colSpacing + colSpacing/2,"MISS",    finalSummary.missCount,    {255,80,80,255}   },
            };

            for (const auto& col : statCols) {
                // Fundo sutil da coluna
                SDL_SetRenderDrawColor(ren, col.color.r/8, col.color.g/8, col.color.b/8, 150);
                SDL_Rect colBg{col.cx - colSpacing/2 + 8, colY_label - 6, colSpacing - 16, 105};
                SDL_RenderFillRect(ren, &colBg);
                SDL_SetRenderDrawColor(ren, col.color.r/3, col.color.g/3, col.color.b/3, 160);
                SDL_RenderDrawRect(ren, &colBg);

                // Rótulo centralizado
                renderText(ren, fontSmall, col.label, col.cx, colY_label, col.color, true);

                // Número grande centralizado logo abaixo
                renderText(ren, fontHuge, std::to_string(col.count), col.cx, colY_num + 12, col.color, true);
            }

            // ── Divisor inferior
            SDL_SetRenderDrawColor(ren, 50,65,115,180);
            SDL_RenderDrawLine(ren, overlay.x+40, overlay.y+355, overlay.x+overlay.w-40, overlay.y+355);

            // ── Instrução
            renderText(ren, fontSmall,
                "[Enter] Reiniciar  |  [TAB] Proxima Musica  |  [1/2/3] Dificuldade",
                cx, overlay.y+370, {100,140,210,210}, true);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(14);  // ~71 FPS cap
    }

    // ─── CLEANUP ──────────────────────────────────────────────────────────────
    if (fontTiny)   TTF_CloseFont(fontTiny);
    if (fontSmall)  TTF_CloseFont(fontSmall);
    if (fontMedium) TTF_CloseFont(fontMedium);
    if (fontLarge)  TTF_CloseFont(fontLarge);
    if (fontHuge)   TTF_CloseFont(fontHuge);
    TTF_Quit();
    if (audioDevice) SDL_CloseAudioDevice(audioDevice);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
