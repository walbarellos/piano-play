#pragma once
#include "abntpiano/Chart.hpp"   // PlayableChordGroup, HitWindow, Difficulty
                                  // (assumindo os campos exatos de dominio.md;
                                  // ajustar o include se o header real tiver
                                  // outro nome/caminho)
#include <vector>

namespace abntpiano {

// ============================================================
// Judgement (valor) — ver dominio.md
// ============================================================

enum class JudgementType { Perfect, Great, Good, Miss };

struct Judgement {
    JudgementType type;
    double deltaMs;   // ver regra de composição em judgeChordGroup()
};

// ============================================================
// Entrada bruta: uma tecla pressionada em um instante
// ============================================================

struct KeyHit {
    char key;
    double timestampSec;
};

// ============================================================
// Núcleo puro (RNF12) — sem I/O, sem tempo real, sem estado.
// Testável diretamente por TC13–TC16 sem instanciar nada.
// ============================================================

// Classifica um delta (ms; sinal = adiantado/atrasado, magnitude decide
// a faixa) contra a hitWindow da Difficulty. Limites são INCLUSIVOS na
// borda de cima de cada faixa (ex.: exatamente 180ms em Normal ainda é
// Good; RNF04 diz "Miss ACIMA de 180ms") — isso resolve a ambiguidade
// que TC14 deixa em aberto ("validar limite exato").
JudgementType classifyDelta(double deltaMs, const HitWindow& hitWindow);

// Decide o Judgement de um ChordGroup inteiro (RF17/RF18/ADR-11).
//
// Contrato:
// - `hits` pode conter teclas fora de `expected.keys`; são ignoradas.
// - Um hit só conta como válido para uma key se |delta| <= hitWindow.missAbove.
// - Se HÁ hit válido para TODAS as keys de `expected.keys`:
//     resultado = pior JudgementType entre elas, via classifyDelta
//     (nunca Miss nesse ramo, por definição de "válido" acima).
//     Ex.: 2 Perfect + 1 Great -> Great.
//     deltaMs do resultado = deltaMs da key que definiu o pior caso.
// - Se FALTA hit válido para alguma key (chord parcial):
//     - allowPartialChord == false  -> Miss (Hard/Expert).
//     - allowPartialChord == true:
//         - >=1 hit válido entre as keys esperadas -> Good.
//         - 0 hits válidos                          -> Miss.
//     deltaMs do resultado, nesse ramo, = hitWindow.missAbove (sentinela
//     — não há "um" delta que represente ausência de tecla).
// - Determinístico: mesma entrada -> mesma saída (pré-requisito de
//   TC13–TC16 e do princípio de determinismo do SRS).
// - Se `hits` tiver mais de um hit válido para a MESMA key, usa o de
//   menor |delta| (mais próximo do onset). Isso é uma escolha de
//   projeto, não algo que os docs especificam — confirmar antes de
//   implementar; teclado ABNT sem N-key rollover pode gerar repique.
Judgement judgeChordGroup(
    const PlayableChordGroup& expected,
    const std::vector<KeyHit>& hits,
    const HitWindow& hitWindow,
    bool allowPartialChord
);

// ============================================================
// Casca com estado — o que SongModeController de fato chama
// (ver 01-diagramas/sequencia-acorde.mmd: registerInput por tecla,
// resolve() quando o grupo fecha ou expira).
//
// Só acumula KeyHits e delega a decisão a judgeChordGroup(); a lógica
// de julgamento em si não depende desta classe existir, então
// TC13–TC16 podem testar judgeChordGroup() isoladamente (RNF12).
// ============================================================

class JudgementSession {
public:
    JudgementSession(PlayableChordGroup expected,
                      HitWindow hitWindow,
                      bool allowPartialChord);

    // SM->>JE: registerInput(key, t). Ignora key fora de expected.keys
    // (não é input inválido — pode pertencer a outro grupo / Free Play
    // concorrente). Quem decide QUAIS sessões estão ativas em um dado
    // instante é o SongModeController, não esta classe (RNF11).
    void registerInput(char key, double timestampSec);

    // true assim que o grupo pode ser fechado: todas as keys esperadas
    // já têm hit válido registrado, OU nowSec já passou de
    // expected.onset + hitWindow.missAbove (expiração, TC16).
    bool isResolvable(double nowSec) const;

    // Só deve ser chamada com isResolvable(nowSec) == true. Delega para
    // judgeChordGroup() com os hits acumulados até o momento.
    Judgement resolve(double nowSec) const;

private:
    PlayableChordGroup expected_;
    HitWindow hitWindow_;
    bool allowPartialChord_;
    std::vector<KeyHit> hits_;
};

} // namespace abntpiano
