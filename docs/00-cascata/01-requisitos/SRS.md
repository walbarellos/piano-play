# SRS — Software Requirements Specification
## ABNT Piano

Versão: 0.2 (revisão pós-conceito Synthesia)

---

## 1. Introdução

Jogo de piano que usa o teclado físico (apenas `A–Z`) como instrumento,
com dois modos centrais:

- **Free Play** — o jogador toca livremente, sem objetivo, como um piano
  cromático de 26 teclas.
- **Song Mode** — o jogador executa peças reais (ex.: Chopin), importadas
  de arquivos MIDI/MusicXML, com notas apresentadas visualmente no tempo
  (estilo Synthesia), níveis de dificuldade, pontuação e progresso salvo.

## 2. Objetivos

- Dar a sensação de "aprender/tocar piano" sem exigir um piano físico.
- Permitir importar peças reais e jogá-las, não só melodias autorais.
- Ter uma curva de dificuldade real (Easy → Expert) que reduz/adapta a
  peça original, não apenas acelera o tempo.
- Ter progresso persistente (scores, peças desbloqueadas, configurações).

## 3. Escopo

Dentro do escopo (MVP + incrementos definidos):
- Piano cromático de 26 teclas com oitavas.
- Polifonia e acordes (multitecla) com janela de tempo definida.
- Importação de MIDI (`.mid`) — MusicXML fica como extensão futura,
  ver `decisoes.md` (ADR-05).
- Geração de `Chart` jogável a partir de uma `Song`, por dificuldade.
- Modo Song com visualização de notas caindo + julgamento de acerto.
- Sistema de pontuação, combo e grade final.
- Persistência local de progresso (save file).

Fora do escopo (nesta fase):
- Multiplayer/duelo (mencionado no conceito original, fica pós-MVP).
- Suporte a pedal de sustain físico (MIDI controller externo).
- Detecção de dedilhado / mãos via visão computacional.

## 4. Stakeholders

- Jogador (usuário final).
- Desenvolvedor (Ohr).

## 5. Requisitos

Ver `RF.md` (funcionais) e `RNF.md` (não funcionais).

## 6. Glossário

| Termo | Definição |
|---|---|
| **Song** | Representação normalizada de uma peça musical importada (metadados + eventos de nota/acorde brutos). |
| **Chart** | Versão jogável de uma `Song`, derivada para uma `Difficulty` específica (notas mapeadas às 26 teclas, reduzidas/adaptadas). |
| **ChordGroup** | Conjunto de `NoteEvent`s cujo onset cai dentro da janela de simultaneidade e que devem ser pressionados juntos. |
| **Judgement** | Resultado da comparação entre o tempo de input do jogador e o tempo esperado de uma nota/acorde: `Perfect`, `Great`, `Good`, `Miss`. |
| **Janela de acerto (hit window)** | Intervalo de tempo, em ms, no qual um input é aceito para uma nota/acorde específico. |
| **SaveGame** | Estado persistido do jogador: perfil, scores, peças desbloqueadas, configurações. |

## 7. Critérios de aceitação (nível SRS)

- Toda peça MIDI válida de até N vozes simultâneas deve ser importável
  sem erro fatal (pode gerar `Chart` degradado, mas não deve crashar).
- Em qualquer dificuldade, o `Chart` gerado deve ser jogável apenas com
  as 26 teclas + modificadores de oitava definidos.
- O julgamento de uma nota/acorde deve ser determinístico dado o mesmo
  input e o mesmo `Chart`.
