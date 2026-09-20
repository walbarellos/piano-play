# Mapeamento de Teclado

## ADR-09 — Mapeamento por posição física, não alfabético
**Status:** aceito, substitui o mapeamento alfabético usado até a v0.2
do código (`A→C4, B→C#4...`).

**Problema:** ordem alfabética não corresponde à posição física das
teclas no layout ABNT. Semitons vizinhos (`A→B→C→D`) ficam em fileiras
diferentes do teclado, exigindo que a mão pule de linha a cada nota —
impraticável para tocar uma melodia e sem nenhuma lógica espacial pro
jogador aprender.

**Decisão:** mapear pela posição de leitura do teclado físico
(esquerda→direita, cima→baixo), preservando ainda as 26 teclas `A–Z`
(RF01/RF02) como universo válido de entrada.

```
Fileira 1 (superior):  Q  W  E  R  T  Y  U  I  O  P
Fileira 2 (home row):  A  S  D  F  G  H  J  K  L
Fileira 3 (inferior):  Z  X  C  V  B  N  M
```

Dentro de cada fileira, esquerda→direita = grave→agudo. Ao final de uma
fileira, a próxima fileira continua a escala cromática — **não** volta
pro grave; é uma leitura contínua tipo texto (boustrophedon simples:
fileira 1, depois fileira 2, depois fileira 3, sempre esquerda→direita).

## Tabela canônica (posição base, sem offset de oitava)

| Pos | Tecla | MIDI | Nota | Pos | Tecla | MIDI | Nota |
|---|---|---|---|---|---|---|---|
| 0 | Q | 60 | C4  | 13 | F | 73 | C#5 |
| 1 | W | 61 | C#4 | 14 | G | 74 | D5  |
| 2 | E | 62 | D4  | 15 | H | 75 | D#5 |
| 3 | R | 63 | D#4 | 16 | J | 76 | E5  |
| 4 | T | 64 | E4  | 17 | K | 77 | F5  |
| 5 | Y | 65 | F4  | 18 | L | 78 | F#5 |
| 6 | U | 66 | F#4 | 19 | Z | 79 | G5  |
| 7 | I | 67 | G4  | 20 | X | 80 | G#5 |
| 8 | O | 68 | G#4 | 21 | C | 81 | A5  |
| 9 | P | 69 | A4  | 22 | V | 82 | A#5 |
| 10 | A | 70 | A#4 | 23 | B | 83 | B5  |
| 11 | S | 71 | B4  | 24 | N | 84 | C6  |
| 12 | D | 72 | C5  | 25 | M | 85 | C#6 |

Range total sem offset: **C4 a C#6** (26 semitons = 2 oitavas + 1
semitom). Com `shiftOctaveUp/Down` (RF06), esse range inteiro desloca
em blocos de 12 semitons.

## Teclado visual (substitui o piano de 88 teclas do Synthesia)

A referência do Synthesia desenha um piano real porque o instrumento
dela é um piano. O nosso instrumento é o teclado — então a barra
inferior da UI é um **teclado QWERTY estilizado de 3 fileiras**, não um
piano:

```
┌────────────────────────────────────────────┐
│  Q  W  E  R  T  Y  U  I  O  P               │
│   A  S  D  F  G  H  J  K  L                 │
│    Z  X  C  V  B  N  M                      │
└────────────────────────────────────────────┘
```

Cada tecla é destacada (cor + glow, ver `UX.md`) exatamente como o
Synthesia destaca a tecla do piano ao tocar — mesma linguagem visual,
instrumento diferente. Isso resolve o problema de "como enumerar":
**o jogador não memoriza uma ordem alfabética abstrata; ele lê a
posição, igual já faz com o teclado físico o dia todo.**

## Consequência para o Chart (RF13)

Ao gerar um `Chart`, o `ChartGenerator` deve preferir, entre teclas
enarmonicamente equivalentes por transposição de oitava, a que fica
fisicamente mais próxima da última tecla tocada (usar
`KeyboardMapper::readingPositionForKey` como métrica de distância) —
isso reduz saltos de mão desnecessários em peças com movimento
melódico contínuo. Ver `pipeline-dificuldade.mmd` (atualizar em
implementação futura com este critério).
