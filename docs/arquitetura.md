# Arquitetura

Diagrama correspondente: `01-diagramas/arquitetura.mmd`.

## Camadas

```
Application
  │
  ├── Input Layer        (KeyboardManager, KeyboardMapper)
  ├── Content Pipeline    (MidiImporter → Song)
  ├── Music Domain         (Song, ChordGroup, NoteEvent — sem I/O)
  ├── Chart Layer          (ChartGenerator: Song+Difficulty → Chart)
  ├── Game Layer            (FreePlayController, SongModeController,
  │                          ScoringEngine, JudgementEngine)
  ├── Audio Layer            (PianoEngine, SampleEngine/SynthEngine,
  │                           AudioDevice)
  ├── Persistence Layer       (SaveGameRepository, JSON serialization)
  └── UI Layer                (Renderer, HUD, menus)
```

Regra geral (RNF11): **dependências só apontam "para dentro/para baixo"**.
`Music Domain` e `Chart Layer` não conhecem áudio, input ou UI. `Game
Layer` orquestra os outros, mas não implementa parsing de MIDI nem
renderização de áudio diretamente.

## Módulos e responsabilidades

### Input Layer
- `KeyboardManager`: eventos brutos de keydown/keyup (SDL2), sem
  repeat, debounce por SO.
- `KeyboardMapper`: já implementado (`KeyboardMapper.hpp`) — tecla →
  `Note`, com offset de oitava.

### Content Pipeline
- `MidiImporter`: lê `.mid`, produz `Song` (RF09, RF10).
- Roda **offline ou em load time**, nunca no frame loop do jogo.

### Music Domain
- `Song`, `NoteEvent`, `ChordGroup` — modelos puros de dados (ver
  `dominio.md`).
- `Note.hpp`/`Note.cpp` (MIDI ↔ Hz ↔ nome) já pertence a este domínio,
  compartilhado com Input/Audio.

### Chart Layer
- `Difficulty`: parâmetros por nível (RNF04).
- `ChartGenerator`: função pura `(Song, Difficulty) → Chart` (RF12–15).
- `ChartCache`: evita regerar Chart repetidamente (ADR-06).

### Game Layer
- `FreePlayController`: consome Input Layer diretamente, sem Chart.
- `SongModeController`: avança o "playhead" no tempo, expõe quais
  `PlayableChordGroup` estão "chegando" para a UI renderizar.
- `JudgementEngine`: compara input (timestamp + teclas) contra o
  `PlayableChordGroup` esperado, usando a `hitWindow` da `Difficulty`
  (RF17, RF18).
- `ScoringEngine`: acumula `Judgement`s → pontuação, combo, grade
  (RF20, RF21).

### Audio Layer
- `PianoEngine`: recebe pedidos de note-on/note-off (de Free Play ou
  Song Mode), gerencia vozes ativas (polifonia).
- `SampleEngine` / `SynthEngine`: geração de som em si.
- `AudioDevice`: abstração sobre a lib de áudio (miniaudio/SDL Audio).

### Persistence Layer
- `SaveGameRepository`: load/save de `SaveGame`, escrita atômica
  (RNF06), versionamento.

### UI Layer
- `Renderer`: desenho de menus, HUD, notas caindo.
- Não contém regra de jogo — só lê estado exposto por Game Layer.

## Fluxos-chave (ver `01-diagramas/`)

- Nota simples (Free Play): `sequencia-nota-simples.mmd`
- Acorde/multitecla: `sequencia-acorde.mmd`
- Importação: `pipeline-importacao.mmd`
- Geração de Chart por dificuldade: `pipeline-dificuldade.mmd`
- Estados do jogo (menu → free play / song mode → resultado):
  `maquina-estados-jogo.mmd`

## O que muda em relação à v0.1 (proposta inicial)

- Adicionadas as camadas **Content Pipeline**, **Chart Layer** e
  **Persistence Layer**, ausentes na proposta original.
- `Game Layer` deixou de ser um bloco único — separado em
  `FreePlayController` / `SongModeController` / `JudgementEngine` /
  `ScoringEngine`, porque são responsabilidades e ciclos de vida
  diferentes.
