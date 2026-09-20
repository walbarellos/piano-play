# Decisões de Arquitetura (ADR)

## ADR-01 — Linguagem: C++20
**Status:** aceito.
Baixa latência de áudio, controle de memória, engine própria,
portabilidade Linux/Windows/Android futuro.

## ADR-02 — Áudio: samples + síntese opcional
**Status:** aceito.
`SampleEngine` para som realista de piano (MVP); `SynthEngine` como
modo experimental futuro. Ambos implementam a mesma interface consumida
por `PianoEngine`, trocáveis sem afetar Game Layer.

## ADR-03 — Redução de densidade em dificuldades baixas
**Status:** aceito, algoritmo a refinar em implementação.
Abordagem inicial: para `Easy`/`Normal`, `ChartGenerator` prioriza a
nota de maior duração/velocity dentro de cada `ChordGroup` grande
(`maxChordSize`), descarta vozes secundárias quando o número de
`ChordGroup`s por segundo excede `noteDensityFactor`. Critério exato
(qual nota "importa mais") é heurística, não fórmula fechada — validar
com playtesting (ver `plano-testes.md`).

## ADR-04 — Hit window como configuração, não constante
**Status:** aceito.
Valores de RNF04 vivem em dados (`Difficulty` carregado de config),
não em código, para permitir balanceamento sem recompilar.

## ADR-05 — MusicXML fora do MVP
**Status:** aceito.
MIDI cobre o caso de uso principal (peças de domínio público, ex.
Chopin, disponíveis como `.mid`). MusicXML traz complexidade adicional
(notação, dedilhado sugerido) sem benefício direto para o gameplay
inicial. Reavaliar quando houver necessidade de dedilhado/mãos.

## ADR-06 — Chart é cacheado, não persistido como parte do SaveGame
**Status:** aceito.
`Chart` é derivado deterministicamente de `Song` + `Difficulty`
(ADR-03), então pode ser recalculado. Cache em memória/disco por
performance, mas `SaveGame` só guarda `ScoreRecord`s e referências
(`songId`, `difficulty`), nunca o `Chart` em si — evita inflar o save e
evita invalidação manual quando o algoritmo de geração mudar.

## ADR-07 — Song Mode não depende de Free Play, mas reusa Audio/Input
**Status:** aceito.
`SongModeController` e `FreePlayController` são independentes entre si
(RNF11), mas ambos chamam `PianoEngine`/`KeyboardMapper` — não há
duplicação de lógica de áudio ou de mapeamento de tecla, só de
orquestração de estado de jogo.

## ADR-08 — Persistência em JSON local
**Status:** aceito.
Formato legível, fácil de versionar/migrar (RNF06), suficiente para
volume de dados esperado (perfil + scores). Reavaliar apenas se volume
de peças/scores crescer muito (não esperado no escopo atual).

## ADR-09 — Mapeamento de teclado por posição física
**Status:** aceito, substitui mapeamento alfabético da v0.2.
Detalhes e tabela completa em `02-analise/mapeamento-teclado.md`.
Resumo: `A→C4` alfabético foi trocado por mapeamento em ordem de
leitura do teclado físico (`Q→C4`, fileira por fileira), porque
semitons vizinhos precisam estar fisicamente vizinhos para a peça ser
tocável. Consequência direta: a UI usa um teclado QWERTY estilizado
como "instrumento visual", não um piano (ver `UX.md` — Teclado Visual).

## ADR-10 — Feedback de acerto/erro fica fora da área de notas caindo
**Status:** aceito.
Toda reação a input (glow, micro-frase, combo) é renderizada na faixa
inferior (hit line / teclado visual) ou em zona de HUD fixa — nunca
dentro da área onde as notas descem. Motivo: a área de notas é a única
fonte de informação sobre o que vem a seguir; qualquer efeito ali
atrapalha a leitura da peça (mesmo princípio que o Synthesia já usa:
o glow acontece na tecla, não no trilho da nota). Ver `UX.md` —
Sistema de Feedback.

## ADR-11 — Tolerância a acordes parciais em dificuldades introdutórias (Easy)
**Status:** aceito.
Em dificuldades altas (`Hard`, `Expert`), um acorde (`ChordGroup`) com
teclas faltantes resulta em `Miss` total (RF18 estrito), exigindo precisão
polifônica completa.
Em contrapartida, no `Easy` (e opcionalmente `Normal` via `allowPartialChord`),
o acerto de ao menos uma tecla válida do grupo dentro da janela gera
julgamento `Good` em vez de `Miss`, preservando a contagem de combo.
Motivos:
1. **Limitação de hardware:** teclados de membrana comuns ABNT sem *N-key rollover*
   frequentemente sofrem *ghosting* ou bloqueio elétrico ao pressionar 2–3
   teclas simultâneas, punindo o jogador iniciante por hardware.
2. **Curva de aprendizado motora:** a coordenação de acordes polifônicos em
   teclas alfanuméricas é assimétrica; o modo introdutório deve acolher a
   tentativa em vez de quebrar imediatamente o combo.

## ADR-12 — Resolução de Repique e Ausência de Input no JudgementEngine
**Status:** aceito.
1. **Repique de tecla (Debounce/Chatter):** Caso múltiplos inputs da mesma tecla sejam
   recebidos dentro da janela de um mesmo `ChordGroup`, o `JudgementEngine` adota o evento
   de menor `|deltaMs|` (o mais próximo do onset ideal), evitando punição por *key bounce*.
2. **Delta temporal em Miss sem input:** Quando um grupo expira por ausência de toque
   (timeout), o delta temporal é nulo (`std::nullopt`), diferenciando semanticamente um erro
   por omissão de um erro de sincronia/timing.
3. **Fronteiras de tolerância inclusivas:** Os limites superiores da `HitWindow` (RNF04)
   são inclusivos (`<=`), de modo que um atraso exato de 180 ms em `Normal` é avaliado como
   `Good`, e não `Miss`.
4. **Isolamento de camada:** A orquestração de tempo e o lookahead de grupos ativos pertencem
   ao `SongModeController`; a avaliação rítmica é uma função pura em `JudgementEngine` (RNF11, RNF12).
