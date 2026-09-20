# Requisitos Não Funcionais (RNF)

## RNF01 — Latência de entrada→áudio

Latência entre pressionar uma tecla e ouvir o som correspondente deve
ser < 20 ms, com meta ideal < 10 ms.

## RNF02 — Taxa de atualização

A interface (incluindo visualização de notas caindo no Song Mode) deve
rodar a ≥ 60 FPS.

## RNF03 — Janela de simultaneidade (agrupamento de acordes)

Ao importar uma `Song`, notas cujo onset esteja a até **30 ms** uma da
outra devem ser agrupadas no mesmo `ChordGroup` (RF10). Valor
configurável por parâmetro de importação, não hardcoded.

## RNF04 — Janela de acerto (hit window) por dificuldade

A tolerância de tempo para julgar um input como `Perfect`/`Great`/
`Good`/`Miss` deve variar por dificuldade:

| Dificuldade | Perfect | Great | Good | Miss (acima de) |
|---|---|---|---|---|
| Easy | ±120 ms | ±180 ms | ±250 ms | 250 ms |
| Normal | ±80 ms | ±130 ms | ±180 ms | 180 ms |
| Hard | ±50 ms | ±90 ms | ±130 ms | 130 ms |
| Expert | ±30 ms | ±60 ms | ±90 ms | 90 ms |

Estes valores são parâmetros de configuração, não constantes de código
(ver `decisoes.md` ADR-04).

## RNF05 — Fidelidade de importação

A conversão MIDI → `Song` não deve perder eventos de nota (onset/offset/
velocity) além de arredondamento de ponto flutuante. Perda de fidelidade
só é aceitável na etapa `Song → Chart` (redução por dificuldade), nunca
na etapa `MIDI → Song`.

## RNF06 — Robustez de persistência

- Gravação do `SaveGame` deve ser atômica (escrever em arquivo
  temporário e renomear) para evitar corrupção em caso de crash durante
  o save.
- O formato do `SaveGame` deve incluir número de versão, para permitir
  migração futura sem perda de progresso.

## RNF07 — Portabilidade

Suporte a Linux e Windows no MVP; arquitetura preparada para Android
(RF de importação e geração de `Chart` não podem depender de I/O
específico de desktop).

## RNF08 — Confiabilidade

Falha ao reproduzir uma nota, importar uma peça ou carregar um save não
deve derrubar a aplicação (RF11, RF25).

## RNF09 — Usabilidade

O jogador deve conseguir iniciar o Free Play e tocar uma nota sem
consultar documentação. Seleção de peça/dificuldade deve ser navegável
apenas com teclado.

## RNF10 — Acessibilidade

Feedback visual E sonoro para todo `Judgement` (não depender só de cor,
ver `UX.md`); volume e buffer de áudio configuráveis.

## RNF11 — Manutenibilidade / separação de camadas

Nenhuma regra de jogo deve conhecer a biblioteca de áudio/gráficos
diretamente; nenhum código de áudio deve conhecer a UI; a `Song`/`Chart`
nunca deve depender do teclado físico (ver `arquitetura.md`).

## RNF12 — Testabilidade

As conversões `MIDI → Song`, `Song → Chart` e o cálculo de `Judgement`
devem ser testáveis sem inicializar áudio, janela ou input real.
