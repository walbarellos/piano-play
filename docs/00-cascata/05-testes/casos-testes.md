# Casos de Teste

## Note / KeyboardMapper (já implementado — ver tests/test_note.cpp)
- `TC01` — MIDI 69 → 440.0 Hz.
- `TC02` — MIDI 60 → "C4", 261.6256 Hz.
- `TC03` — Tecla `'1'`, `'@'` → `KeyboardMapper` retorna `nullopt`.
- `TC04` — Offset de oitava (+12/-12) reflete corretamente no nome da
  nota.

## Importação (MidiImporter)
- `TC05` — Arquivo `.mid` válido de peça monofônica → `Song` com N
  `ChordGroup`s de tamanho 1, onsets em ordem crescente.
- `TC06` — Arquivo `.mid` com acorde (3 notas onset ~0 ms) → 1
  `ChordGroup` de tamanho 3 (RNF03).
- `TC07` — Duas notas com onset a 40 ms de diferença (acima da janela
  de 30 ms) → 2 `ChordGroup`s distintos.
- `TC08` — Arquivo corrompido → erro reportado, sem crash, catálogo
  inalterado (RF11).

## ChartGenerator
- `TC09` — Mesma `Song` + `Difficulty=Expert` → `Chart` preserva 100%
  dos `ChordGroup`s originais (dentro de `maxChordSize`).
- `TC10` — Mesma `Song` + `Difficulty=Easy` → `Chart` tem menos
  `PlayableChordGroup`s que a `Song` original (redução aplicada).
- `TC11` — `ChordGroup` maior que `maxChordSize` da dificuldade →
  `PlayableChordGroup` correspondente tem `keys.length <= maxChordSize`.
- `TC12` — Determinismo: gerar o `Chart` duas vezes para o mesmo input
  produz resultado idêntico (ADR-03/ADR-06).

## JudgementEngine
- `TC13` — Input exatamente no onset esperado → `Perfect`.
- `TC14` — Input a 100 ms de atraso em dificuldade `Normal` → `Good`
  (dentro de ±180 ms) ou `Miss` conforme tabela RNF04 — validar limite
  exato.
- `TC15a` — `ChordGroup` de 3 teclas, jogador pressiona só 2 dentro da
  janela em dificuldade `Hard`/`Expert` → resultado do grupo é `Miss` (RF18).
- `TC15b` — `ChordGroup` de 2 ou 3 teclas, jogador pressiona 1 ou 2 dentro
  da janela em dificuldade com `allowPartialChord` (`Easy`) → resultado do
  grupo é `Good` e o combo é mantido (RF18, ADR-11).
- `TC16` — Nenhum input dentro da `hitWindow` → `Miss` automático
  quando o tempo da nota expira.

## ScoringEngine
- `TC17` — Sequência conhecida de `Judgement`s → pontuação e combo
  calculados batem com valor esperado (fixture determinístico).
- `TC18` — Um `Miss` no meio de uma sequência zera o combo mas não a
  pontuação acumulada.

## Persistência
- `TC19` — Salvar e carregar `SaveGame` produz objeto equivalente
  (round-trip).
- `TC20` — Simular crash durante save (arquivo temporário incompleto)
  → `SaveGame` anterior no disco permanece íntegro (RNF06).
- `TC21` — Carregar `SaveGame` com versão desconhecida/futura → falha
  controlada, não corrompe o arquivo existente.
- `TC22` — Ausência de arquivo de save → cria `SaveGame` novo sem
  crashar (RF25).
