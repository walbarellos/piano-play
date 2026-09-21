# 🎹 ABNT Piano (Piano Play)

Um jogo de ritmo e simulador de piano interativo para teclado de computador (layout ABNT2/QWERTY), construído em **C++20** com **SDL2** e **SDL2_ttf**.

Desenvolvido com síntese de áudio em tempo real (polifonia, ADSR com decaimento natural, harmônicos), sistema de julgamento de timing (Perfect, Great, Good, Miss), visualizador de notas em cascata vertical com suporte a notas longas (*hold ribbons*), e catálogo de peças clássicas em partituras MIDI autênticas.

---

## 🚀 Como Executar

### Pré-requisitos (Debian/Ubuntu/Fedora/Arch)
- Compilador C++20 (GCC ou Clang)
- CMake 3.20+
- `libsdl2-dev`
- `libsdl2-ttf-dev`

### Execução Rápida
Basta executar o script de inicialização:
```bash
./jogar.sh
# ou
./run.sh
```

### Compilação Manual
```bash
cmake -B build -S .
cmake --build build -j$(nproc)
./build/abntpiano_app
```

### Executar Testes
```bash
ctest --test-dir build --output-on-failure
```

---

## 🎮 Controles

### Músicas e Modos
- **F1**: Modo *Free Play* (toque livre no teclado)
- **F2**: *Parabéns Pra Você* (Tutorial)
- **F3**: *Beethoven - Ode to Joy (Hino à Alegria)*
- **F4**: *Beethoven - Für Elise (WoO 59)*
- **F5**: *Chopin - Prelude Op. 28 No. 4*
- **F6**: *Mozart - Rondo Alla Turca (KV 331, III)*
- **F7**: *Chopin - Ballade No. 1 em Sol menor, Op. 23 (Completa - 5.013 notas)*
- **F8**: *Beethoven - Sonata No. 9 'Kreutzer' - I. Presto (Shigatsu wa Kimi no Uso ver. - 10.711 notas)*
- **TAB**: Alternar para a próxima canção do catálogo
- **F10**: Alternar Guia Melódico Audível
- **F11**: Alternar Acompanhamento (mão esquerda)
- **F12**: **Modo DEMO Automático** (a máquina demonstra com timing humano autêntico: Perfect/Great/Good)

### Dificuldade e Ajustes
- **1**: Fácil (Easy)
- **2**: Normal
- **3**: Difícil (Hard)
- **- / +**: Ajustar velocidade de queda das notas (lookahead)
- **[ / ]**: Ajustar velocidade de reprodução
- **Enter**: Reiniciar canção atual
- **Barra de Espaço**: Pedal de Sustain
- **ESC**: Sair do jogo

### Teclas do Piano
O teclado do computador mapeia 26 semitons cromáticos em três fileiras:
- Fileira 1: `Q` a `P`
- Fileira 2: `A` a `L`
- Fileira 3: `Z` a `M`

---

## 📂 Arquitetura Modular e Estrutura do Projeto

O projeto segue rigorosamente a arquitetura em camadas descrita em `docs/arquitetura.md`:

```
Application (App, main)
  │
  ├── Input Layer          (KeyboardMapper, FreePlayController)
  ├── Content Pipeline      (MidiImporter, SongCatalog)
  ├── Music Domain          (Song, ChordGroup, NoteEvent, Note, HoldState)
  ├── Chart Layer           (ChartGenerator, Chart)
  ├── Game Layer            (SongModeController, JudgementEngine, ScoringEngine, DemoPlayer)
  ├── Audio Layer           (SynthEngine, AudioDevice)
  ├── Persistence Layer     (SaveGameRepository, SaveGame)
  └── UI Layer              (HighwayRenderer, KeyboardRenderer, HudRenderer, ResultsOverlay, ParticleSystem)
```

- `assets/songs/`: Partituras MIDI curadas e autênticas, sem duplicatas ou arquivos corrompidos.
- `include/abntpiano/`: Interfaces públicas dos módulos do domínio, áudio, game e UI.
- `src/`: Implementações modulares e de responsabilidade única (SRP), sem monólitos.
- `tests/`: Bateria de testes unitários automatizados com CTest.
- `docs/`: Documentação detalhada dos requisitos, casos de uso, arquitetura e decisões de design.
