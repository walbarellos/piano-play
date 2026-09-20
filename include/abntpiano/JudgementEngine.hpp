#pragma once

#include <vector>
#include <string>
#include <optional>
#include <cstdint>
#include <cmath>

namespace abntpiano {

// Tipos de julgamento de acerto conforme RF17 / RNF04
enum class JudgementType {
    Perfect,
    Great,
    Good,
    Miss
};

// Hit window por dificuldade em milissegundos conforme RNF04 / ADR-04 / ADR-12.
// Limites superiores são inclusivos (<= value).
struct HitWindow {
    double perfect = 80.0;    // ex: Normal +-80ms
    double great   = 130.0;   // ex: Normal +-130ms
    double good    = 180.0;   // ex: Normal +-180ms
    double missAbove = 180.0; // > missAbove -> Miss
};

// Parâmetros de dificuldade conforme dominio.md / ADR-11 / ADR-12
struct DifficultyConfig {
    std::string name;
    HitWindow hitWindow;
    int maxChordSize = 3;
    float noteDensityFactor = 1.0f;
    bool allowPartialChord = false;       // Easy = true, Hard/Expert = false
    float partialChordThreshold = 0.5f;   // Fração mínima de teclas (ex: 0.5f = >=50%; Normal = 0.66f)
};

// Evento de input capturado do jogador
struct KeyInputEvent {
    char key;           // 'A'-'Z' (case-insensitive)
    double timestamp;   // segundos relativo ao início da música
};

// Resultado de avaliação de uma nota ou grupo de notas (RF17, RF18, ADR-12)
struct Judgement {
    JudgementType type = JudgementType::Miss;
    
    // Diferença em ms entre o onset esperado e o input do jogador (input - onset).
    // Negativo = adiantado (early), Positivo = atrasado (late).
    // std::nullopt caso o Miss ocorra por expiração de tempo (nenhum toque) ou ausência de input.
    std::optional<double> deltaMs = std::nullopt;

    // Quantidade de teclas do grupo que foram acertadas dentro da janela
    size_t keysHit = 0;
    size_t keysTotal = 0;
};

// Função pura que julga um acorde/nota sem manter estado (RNF11, RNF12).
// - targetKeys: lista de teclas esperadas ('A'-'Z')
// - targetOnset: timestamp esperado da nota em segundos
// - inputs: inputs do jogador capturados na vizinhança da janela
// - config: parâmetros da dificuldade ativa
//
// Regras aplicadas (ADR-11, ADR-12):
// 1. Repique: se houver múltiplos inputs para a mesma tecla, adota o de menor |deltaMs|.
// 2. Acerto pleno: todas as teclas acertam -> resultado é o pior caso individual (RF18).
// 3. Acerto parcial:
//    - Se allowPartialChord == true e keysHit/keysTotal >= threshold -> JudgementType::Good.
//    - Caso contrário -> JudgementType::Miss.
// 4. Sem input dentro da janela -> JudgementType::Miss (deltaMs = std::nullopt).
Judgement judgeChordGroup(
    const std::vector<char>& targetKeys,
    double targetOnset,
    const std::vector<KeyInputEvent>& inputs,
    const DifficultyConfig& config
);

} // namespace abntpiano
