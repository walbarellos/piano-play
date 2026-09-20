# Requisitos Funcionais (RF)

Organizados por módulo. IDs não reaproveitam numeração antiga — esta é a
lista canônica a partir da v0.2.

## Módulo: Entrada / Teclado

- **RF01** — O sistema deve reproduzir a nota correspondente à tecla
  alfabética (`A–Z`) pressionada.
- **RF02** — O sistema deve reconhecer as 26 teclas `A–Z` do layout
  ABNT, mapeando cada uma a um semitom conforme a **posição física de
  leitura** (não ordem alfabética — ver ADR-09 e
  `mapeamento-teclado.md`), e ignorar teclas não mapeadas.
- **RF03** — Ao soltar uma tecla, o sistema deve iniciar o *release* da
  nota correspondente (envelope de amplitude, não corte abrupto).
- **RF04** — O sistema deve suportar polifonia: múltiplas teclas
  pressionadas simultaneamente produzem múltiplas notas simultâneas.
- **RF05** — O sistema deve suportar sustain (tecla dedicada, ex. barra
  de espaço) que prolonga o *release* das notas ativas.
- **RF06** — O sistema deve permitir deslocar a oitava ativa (para cima
  e para baixo) por meio de teclas dedicadas.

## Módulo: Piano Livre (Free Play)

- **RF07** — O jogador deve poder tocar livremente, sem objetivo,
  usando o mapeamento de RF01–RF06.
- **RF08** — A interface do Free Play deve indicar, para cada tecla
  ativa, a nota correspondente (ex. `Q → C4`), usando o teclado visual
  (RF28).

## Módulo: Importação de Peças

- **RF09** — O sistema deve importar arquivos MIDI (`.mid`) e convertê-los
  em uma `Song` normalizada (lista de `NoteEvent`/`ChordGroup` + metadados:
  título, compositor, tempo/BPM, duração total).
- **RF10** — O sistema deve agrupar `NoteEvent`s cujo onset esteja dentro
  da janela de simultaneidade (ver RNF de timing) em um `ChordGroup`.
- **RF11** — Uma importação malformada ou parcialmente inválida não deve
  derrubar a aplicação; deve gerar erro reportável ao usuário.

## Módulo: Geração de Chart / Dificuldade

- **RF12** — O sistema deve gerar, a partir de uma `Song`, um `Chart`
  jogável para cada `Difficulty` suportada (`Easy`, `Normal`, `Hard`,
  `Expert`).
- **RF13** — A geração de `Chart` deve mapear eventos musicais às 26
  teclas disponíveis, aplicando transposição de oitava quando a nota
  original estiver fora do range mapeado no momento.
- **RF14** — Em dificuldades mais baixas, a geração de `Chart` deve
  reduzir densidade de notas (ex.: remover vozes secundárias, simplificar
  acordes grandes em subconjuntos) — ver `decisoes.md` ADR-03.
- **RF15** — Em dificuldades mais altas, a geração de `Chart` deve
  preservar mais fielmente polifonia e timing original, com janela de
  acerto mais estreita.

## Módulo: Song Mode (execução de peças)

- **RF16** — O sistema deve apresentar o `Chart` da peça ao jogador com
  visualização de notas se aproximando no tempo (estilo *falling notes*),
  sincronizada ao áudio/tempo da peça.
- **RF17** — O sistema deve capturar o input do jogador (RF01–RF04) e
  compará-lo ao `Chart`, gerando um `Judgement` (`Perfect`/`Great`/
  `Good`/`Miss`) por nota ou `ChordGroup`.
- **RF18** — Para um `ChordGroup`, quando todas as teclas acertam
  dentro da janela de acerto definida para a dificuldade, o julgamento
  do grupo é o **pior caso individual** entre elas (ex.: 2 `Perfect` +
  1 `Great` → grupo julgado `Great`). Em caso de acerto parcial (nem
  todas as teclas do grupo pressionadas na janela):
  - Em dificuldades estritas (`Hard`, `Expert`), o grupo é julgado como
    `Miss` total (zera combo).
  - Em dificuldades com tolerância configurada (`Easy`, parametrizável
    em `Normal` via `allowPartialChord`), se ao menos uma tecla válida do
    grupo for acertada na janela, o grupo é julgado como `Good` (mantém o
    combo com pontuação reduzida), mitigando limitações de hardware
    (*key rollover* / *ghosting* de teclados comuns ABNT) e suavizando a
    curva motora inicial. Caso nenhuma tecla seja acertada, o resultado é
    `Miss`.
- **RF19** — O sistema deve permitir pausar, reiniciar e abandonar uma
  execução de peça em andamento.

## Módulo: Pontuação e Gamificação

- **RF20** — O sistema deve calcular pontuação com base nos `Judgement`s
  (peso diferente por tipo) e manter um contador de combo consecutivo.
- **RF21** — Ao final de uma execução, o sistema deve apresentar um
  resumo: acertos, erros, maior combo, precisão (%) e uma nota/grade
  final (ex. `S/A/B/C/D`).
- **RF22** — O sistema deve desbloquear conteúdo (peças, dificuldades)
  com base em critérios de progresso (ex.: completar peça anterior com
  grade mínima).

## Módulo: Persistência

- **RF23** — O sistema deve salvar localmente o progresso do jogador:
  perfil, scores por `Chart`, peças/dificuldades desbloqueadas e
  configurações (volume, buffer de áudio, mapeamento de teclas).
- **RF24** — O sistema deve carregar o `SaveGame` na inicialização e
  aplicar as configurações salvas antes de qualquer entrada do jogador.
- **RF25** — Uma falha ao carregar o `SaveGame` (arquivo corrompido/
  ausente) não deve impedir o uso do sistema — deve criar um `SaveGame`
  novo e notificar o jogador.

## Módulo: Feedback e Interface de Jogo

- **RF26** — Todo feedback visual de acerto/erro (glow, flash) deve ser
  renderizado exclusivamente na Zona de Resposta (teclado visual /
  hit line), nunca sobreposto à área onde as notas descem (ADR-10).
- **RF27** — Ao atingir marcos de combo (x10, x25, x50, x100...), o
  sistema deve exibir uma micro-frase de incentivo em zona de HUD fixa,
  sorteada de um pool para evitar repetição, sem bloquear o jogo. Esse
  comportamento deve ser configurável/desativável (RNF10).
- **RF28** — O sistema deve exibir um teclado visual de 3 fileiras
  (layout QWERTY) na parte inferior da tela, com teclas destacadas por
  cor conforme a voz/mão do `Chart` — substitui o piano de 88 teclas
  usado em referências como o Synthesia, já que o instrumento aqui é o
  teclado físico.
- **RF29** — Cada bloco de nota caindo no Song Mode deve exibir a letra
  da tecla correspondente impressa nele, e sua posição horizontal deve
  corresponder à posição da tecla no teclado visual (RF28).
