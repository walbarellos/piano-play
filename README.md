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
- **F4**: *Beethoven - Für Elise*
- **F5**: *Mozart - Rondo Alla Turca*
- **F6**: *Chopin - Prelude Op. 28 No. 4*
- **F7**: *Chopin - Ballade No. 1 in G Minor, Op. 23*
- **F8**: *Beethoven - Sonata 'Kreutzer', Op. 47*
- **TAB**: Alternar para a próxima canção
- **F12**: **Modo DEMO Automático** (a máquina demonstra a música com execução perfeita)

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

## 📂 Estrutura do Projeto

- `assets/songs/`: Canções em formato MIDI.
- `include/abntpiano/`: Cabeçalhos da arquitetura (SynthEngine, ChartGenerator, JudgementEngine, MidiImporter, ScoringEngine, etc.).
- `src/`: Implementações dos subsistemas e interface gráfica principal em SDL2 (`main.cpp`).
- `tests/`: Bateria de testes unitários automatizados com CTest.
- `docs/`: Documentação detalhada dos requisitos, casos de uso, arquitetura e decisões de design.
