# Casos de Uso

Diagrama correspondente: `01-diagramas/casos-de-uso.mmd`.

## UC01 — Tocar piano livre (Free Play)

**Ator:** Jogador
**Pré-condição:** aplicação iniciada, `SaveGame` carregado (ou criado).
**Fluxo principal:**
1. Jogador seleciona "Free Play" no menu.
2. Jogador pressiona teclas `A–Z`; sistema toca as notas correspondentes
   (RF01–RF04).
3. Jogador solta teclas; sistema aplica release (RF03).
**Fluxo alternativo:** Jogador altera oitava (RF06) a qualquer momento.

## UC02 — Alterar oitava

**Ator:** Jogador
**Fluxo:** Jogador pressiona tecla de oitava+/oitava-; `KeyboardMapper`
aplica offset; próximas notas tocadas refletem o novo offset.

## UC03 — Importar peça

**Ator:** Jogador (ou processo de build/conteúdo, offline)
**Fluxo principal:**
1. Sistema recebe um arquivo `.mid`.
2. `MidiImporter` faz parse e gera `Song` normalizada (RF09, RF10).
3. `Song` é adicionada ao catálogo de peças disponíveis.
**Fluxo de exceção:** arquivo malformado → erro reportado, catálogo
não é alterado (RF11).

## UC04 — Gerar Chart para uma dificuldade

**Ator:** Sistema (disparado ao entrar no Song Mode ou como pré-processo)
**Fluxo principal:**
1. Jogador seleciona peça + dificuldade.
2. `ChartGenerator` recebe `Song` + `Difficulty`.
3. Gera `Chart` (RF12–RF15): mapeia eventos a teclas, reduz densidade
   conforme dificuldade.
4. `Chart` é cacheado (evita regerar a cada partida, ver `decisoes.md`
   ADR-06).

## UC05 — Jogar uma peça (Song Mode)

**Ator:** Jogador
**Pré-condição:** `Chart` disponível (UC04).
**Fluxo principal:**
1. Sistema inicia contagem regressiva e começa a "descer" as notas do
   `Chart` sincronizadas ao tempo (RF16).
2. Jogador pressiona teclas correspondentes.
3. Para cada `PlayableChordGroup`, sistema compara timing do input com
   o onset esperado e gera `Judgement` (RF17, RF18).
4. Sistema atualiza pontuação e combo em tempo real (RF20).
5. Ao final, sistema exibe resumo (RF21) e atualiza `ScoreRecord`.
**Fluxo alternativo:** Jogador pausa (RF19) → sistema congela tempo e
input até retomar.
**Fluxo alternativo:** Jogador abandona → nenhum `ScoreRecord` é salvo.

## UC06 — Salvar progresso

**Ator:** Sistema (automático ao final de UC05, ou explícito no menu)
**Fluxo:** `SaveGame` atual é serializado e gravado atomicamente
(RF23, RNF06).

## UC07 — Carregar progresso

**Ator:** Sistema (na inicialização)
**Fluxo principal:** Sistema lê `SaveGame` do disco, valida versão,
aplica `PlayerSettings` (RF24).
**Fluxo de exceção:** arquivo ausente/corrompido → cria `SaveGame` novo,
notifica jogador (RF25).

## UC08 — Selecionar dificuldade

**Ator:** Jogador
**Pré-condição:** dificuldade desbloqueada para a peça (RF22).
**Fluxo:** Jogador navega entre `Easy/Normal/Hard/Expert` disponíveis
para a peça selecionada; sistema mostra preview (ex. densidade de
notas) antes de confirmar.
