# Plano de Testes

## Níveis

1. **Unitário** — funções puras: `midiToFrequency`, `midiToName`,
   `KeyboardMapper`, agrupamento de `ChordGroup`, `ChartGenerator`,
   `JudgementEngine`, serialização de `SaveGame`. Sem I/O real, sem
   áudio, sem janela (RNF12).
2. **Integração** — `MidiImporter` com arquivos `.mid` reais (incluindo
   peças de domínio público, ex. Chopin); `SaveGameRepository` com
   filesystem real (tmp dir); `ChartGenerator` end-to-end por
   dificuldade.
3. **Performance** — latência input→áudio (RNF01), FPS em Song Mode com
   peça densa (RNF02), tempo de importação/geração de Chart para peças
   longas.
4. **Playtesting manual** — calibração de `hitWindow` (RNF04) e da
   heurística de redução de densidade (ADR-03); isso não é testável de
   forma puramente automatizada — precisa de sessões com jogador real.

## Ferramentas

- Fase atual (`tests/test_note.cpp`): assert-based, sem framework.
- Ao crescer o número de casos (Chart, Judgement, Save), migrar para
  Catch2 ou GoogleTest — decisão adiada até o volume justificar
  (evitar dependência prematura).

## Cobertura mínima antes de cada incremento

| Incremento | Cobertura obrigatória |
|---|---|
| Free Play | RF01–RF08 |
| Importação | RF09–RF11, RNF03, RNF05 |
| Chart/Dificuldade | RF12–RF15, ADR-03 |
| Song Mode | RF16–RF19, RNF04 |
| Pontuação | RF20–RF22 |
| Persistência | RF23–RF25, RNF06 |
