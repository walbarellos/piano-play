# Documentação — ABNT Piano

Metodologia: Waterfall (Sommerville) — cada fase em sua pasta numerada
dentro de `00-cascata/`. Diagramas ficam à parte em `01-diagramas/`
para não misturar texto normativo com artefato visual.

## 00-cascata/

- `01-requisitos/` — SRS.md, RF.md, RNF.md
- `02-analise/` — dominio.md, casos-de-uso.md, mapeamento-teclado.md
- `03-arquitetura/` — arquitetura.md, decisoes.md (ADRs)
- `04-design/` — UX.md, UI.md
- `05-testes/` — plano-testes.md, casos-testes.md

## 01-diagramas/

Todos em Mermaid (`.mmd`), renderizáveis em qualquer viewer compatível
(GitHub, VS Code com extensão, mermaid.live):

- `arquitetura.mmd` — camadas do sistema
- `dominio.mmd` — diagrama de classes do domínio
- `casos-de-uso.mmd` — casos de uso
- `sequencia-nota-simples.mmd` — fluxo de uma nota no Free Play
- `sequencia-acorde.mmd` — julgamento de acorde/multitecla no Song Mode
- `maquina-estados-jogo.mmd` — estados do jogo (menu → play → resultado)
- `pipeline-importacao.mmd` — MIDI → Song
- `pipeline-dificuldade.mmd` — Song → Chart por dificuldade

## Ordem de leitura sugerida

1. `01-requisitos/SRS.md` (visão geral + glossário)
2. `01-requisitos/RF.md` + `RNF.md`
3. `02-analise/dominio.md` (entender Song/Chart/ChordGroup antes do resto)
4. `02-analise/mapeamento-teclado.md` (ADR-09 — por que não é alfabético)
5. `02-analise/casos-de-uso.md`
5. `03-arquitetura/arquitetura.md` + `decisoes.md`
6. `04-design/UX.md` + `UI.md`
7. `05-testes/plano-testes.md` + `casos-testes.md`
