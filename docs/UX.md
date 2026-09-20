# UX

## Fluxo geral

```
Splash / Load SaveGame (UC07)
        │
        ▼
   Menu Principal
   ├── Free Play (UC01)
   ├── Song Select
   │      ├── selecionar peça
   │      ├── selecionar dificuldade (UC08, se desbloqueada)
   │      └── Jogar (UC05)
   └── Configurações (PlayerSettings)
```

## Zonas de tela (ADR-10)

A tela do Song Mode tem 3 zonas com responsabilidade exclusiva — um
efeito de uma zona nunca invade outra:

```
┌──────────────────────────────────────────┐
│ ZONA HUD (topo/cantos)                    │  ← score, combo, micro-frase
│                                            │
│ ZONA DE LEITURA (notas caindo)            │  ← NUNCA recebe efeito de
│                                            │    acerto/erro, só as notas
│                                            │    e a conexão de acordes
│──────────────────── hit line ─────────────│
│ ZONA DE RESPOSTA (teclado visual)         │  ← glow, flash, shake — todo
│  Q W E R T Y U I O P                      │    feedback de input mora
│   A S D F G H J K L                       │    aqui
│    Z X C V B N M                          │
└──────────────────────────────────────────┘
```

Regra: **a Zona de Leitura é somente-leitura.** Isso garante RF16 (o
jogador precisa conseguir ler o que vem a seguir) mesmo durante um
combo alto ou uma sequência de erros.

## Feedback de Judgement (RNF10 — não depender só de cor)

| Judgement | Cor (auxiliar) | Ícone/forma | Onde aparece | Som |
|---|---|---|---|---|
| Perfect | dourado | estrela cheia | glow forte na(s) tecla(s) do teclado visual | tick agudo curto |
| Great | verde | check | glow médio na(s) tecla(s) | tick médio |
| Good | azul | check tênue | glow fraco na(s) tecla(s) | tick grave |
| Miss | vermelho | X | flash vermelho rápido na(s) tecla(s) esperada(s) | nenhum som extra (silêncio é o feedback) |

Ícone/forma é o canal primário; cor é reforço — garante acessibilidade
para daltonismo. Todo esse feedback acontece **na Zona de Resposta**
(no teclado visual, igual ao glow do Synthesia na tecla do piano),
nunca sobreposto às notas caindo.

## Micro-frases e combo (Zona HUD)

- Combo normal (sem marco): só o número atualiza, sem texto extra —
  evitar ruído constante.
- Marcos de combo (x10, x25, x50, x100, x200...): dispara uma
  micro-frase curta em um pool rotativo (evita repetição), na Zona HUD,
  com fade-in/fade-out de ~800ms e leve scale-up. Nunca trava input,
  nunca ocupa a Zona de Leitura.
  - Exemplos de pool (PT-BR): "Mandou bem!", "Sequência incrível!",
    "Impecável!", "Não erra!", "Isso aí!".
- `Miss` que zera combo alto (>20): micro-frase neutra/encorajadora
  ("Recomeça!", "Segue o jogo!") — nunca punitiva/negativa, mesma
  lógica de `user_wellbeing`: não reforçar frustração.
- Todo esse sistema (frases + intensidade de glow) deve ser
  configurável/desativável nas `PlayerSettings` (RNF10) — jogador pode
  preferir uma tela mais "limpa".

## Song Mode — leitura da tela

- Notas descem em "lanes" verticais, uma por tecla ativa no `Chart`
  daquele trecho (não 26 lanes fixas — só as usadas, para não poluir).
  Posição X da lane = posição X da tecla correspondente no teclado
  visual (RF-novo, ver `RF.md` RF28) — a nota cai *literalmente em
  cima* de onde o jogador vai apertar, igual ao alinhamento
  nota-caindo→tecla-do-piano do Synthesia.
- Cada bloco de nota exibe a **letra da tecla impressa nele** (RF29) —
  redundante ao teclado visual embaixo, porque o olho do jogador fica
  na nota descendo, não no teclado.
- Acorde (RF18) = múltiplas notas alinhadas na mesma linha de tempo,
  visualmente conectadas (ex. barra horizontal ligando as lanes do
  grupo).
- HUD fixo (Zona HUD): pontuação, combo atual, precisão corrente,
  micro-frases.
- Ao errar (`Miss`), feedback fica contido na Zona de Resposta (flash
  na tecla) + combo zera no HUD — nunca um shake de tela inteira, isso
  distorceria a leitura da Zona de Leitura no momento em que o jogador
  mais precisa dela.

## Song Select

- Lista de peças com: título, compositor, dificuldades desbloqueadas
  (destacadas) vs. bloqueadas (com critério de desbloqueio visível, ex.
  "complete Prelude Op.28 No.4 em Normal com grade B+").
- Preview de densidade de notas por dificuldade (ex. mini-gráfico de
  barras) antes de confirmar — apoia UC08.

## Resultado (fim de UC05)

- Grade grande (S/A/B/C/D), acertos/erros por tipo de Judgement, maior
  combo, precisão.
- Ações: repetir, voltar ao Song Select, próxima peça desbloqueada (se
  houver).
