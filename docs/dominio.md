# Modelo de Domínio

Diagrama de classes correspondente: `01-diagramas/dominio.mmd`.

## Entidades principais

### Note (valor, não entidade persistida)
```
midi: int
frequency: double
name: string   // "C4", "C#4"...
```
Já implementado em `Note.hpp`. Puramente derivado do número MIDI —
nunca é armazenado além de cache.

### NoteEvent
```
timestamp: double     // segundos, relativo ao início da Song
midiNote: int
duration: double
velocity: uint8
voice: int             // opcional: qual voz/mão original (0 = indefinido)
```
Representa um evento bruto de nota, como veio do MIDI.

### ChordGroup
```
onset: double
noteEvents: NoteEvent[]   // todos com onset dentro da janela RNF03
```
Agrupamento derivado de `NoteEvent`s simultâneos. Uma nota "solo" é um
`ChordGroup` de tamanho 1 — isso unifica o modelo: **o jogo sempre lida
com `ChordGroup`s, nunca com `NoteEvent`s soltos** (RF17, RF18).

### Song
```
id: string
title: string
composer: string
sourceFormat: enum { MIDI }   // MusicXML fica como extensão futura
bpm: double (ou tempo map, se peça tiver rubato/mudança de andamento)
durationSeconds: double
chordGroups: ChordGroup[]     // ordenados por onset
```
Resultado da importação (RF09). Representa a peça musical **fiel ao
original**, sem qualquer adaptação de dificuldade ou de teclado.

### Difficulty
```
name: enum { Easy, Normal, Hard, Expert }
hitWindow: { perfect, great, good, missAbove }   // ms, ver RNF04
maxChordSize: int          // acordes maiores são reduzidos
noteDensityFactor: float   // 0.0–1.0, fração de ChordGroups mantidos
allowPartialChord: bool    // se true (ex.: Easy), acerto parcial na janela pontua 'Good' e preserva combo (RF18)
```

### Chart
```
songId: string
difficulty: Difficulty
playableEvents: PlayableChordGroup[]
```

### PlayableChordGroup
```
onset: double
keys: char[]           // subconjunto de 'A'-'Z', já mapeado
originalChordGroup: ChordGroup   // referência para exibição/áudio
```
É o que o Song Mode efetivamente usa para renderizar notas caindo e
julgar input (RF16, RF17). Gerado por `ChartGenerator` a partir de
`Song` + `Difficulty` (RF12–RF15).

### Judgement (valor)
```
type: enum { Perfect, Great, Good, Miss }
deltaMs: double
```

### ScoreRecord
```
chartId: string          // songId + difficulty
profileId: string
accuracy: float
maxCombo: int
grade: enum { S, A, B, C, D }
playedAt: datetime
```

### PlayerProfile
```
id: string
name: string
unlockedSongs: string[]
unlockedDifficulties: map<songId, Difficulty[]>
settings: PlayerSettings
```

### PlayerSettings
```
volume: float
audioBufferSize: int
keyBindings: map<action, char>   // sustain, oitava+, oitava- etc.
```

### SaveGame
```
version: int
profile: PlayerProfile
scores: ScoreRecord[]
```

## Regras de domínio importantes

1. **`Song` é imutável após importação.** Qualquer adaptação de
   dificuldade gera um novo `Chart`, nunca modifica a `Song`.
2. **Todo julgamento de input opera sobre `PlayableChordGroup`**, não
   sobre notas individuais — isso é o que torna acordes (multitecla)
   consistentes com notas solo (RF18).
3. **`ChartGenerator` é uma função pura**: `(Song, Difficulty) → Chart`.
   Sem estado, sem I/O — testável isoladamente (RNF12).
4. **Nenhuma entidade de domínio conhece teclado físico real** — o
   mapeamento de `PlayableChordGroup.keys` usa o mesmo `KeyboardMapper`
   do Free Play, mas a entidade em si só guarda `char[]`.
