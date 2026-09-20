#pragma once
#include "abntpiano/Note.hpp"
#include <array>
#include <cctype>
#include <optional>

namespace abntpiano {

// Mapeia A-Z (case-insensitive) para 26 semitons cromáticos consecutivos,
// seguindo a POSIÇÃO FÍSICA das teclas no teclado ABNT (ordem de leitura),
// não a ordem alfabética — ver ADR-09. Layout (esquerda->direita,
// cima->baixo):
//
//   Q W E R T Y U I O P   -> posições 0..9   (agudo)
//   A S D F G H J K L     -> posições 10..18
//   Z X C V B N M         -> posições 19..25 (grave)
//
// Padrão: Q -> C4 (MIDI 60) ... M -> C#6 (MIDI 85).
// Teclas vizinhas na fileira = semitons vizinhos: tocar uma linha reta
// da esquerda pra direita numa fileira é uma escala cromática.
class KeyboardMapper {
public:
    explicit KeyboardMapper(int baseMidi = 60) : baseMidi_(baseMidi) {}

    void setOctaveOffset(int semitoneOffset) { octaveOffset_ = semitoneOffset; }
    int octaveOffset() const { return octaveOffset_; }

    void shiftOctaveUp()   { octaveOffset_ += 12; }
    void shiftOctaveDown() { octaveOffset_ -= 12; }

    // Retorna o Note para uma tecla 'A'-'Z' (case-insensitive), ou
    // std::nullopt se a tecla não estiver mapeada (RF02).
    std::optional<Note> noteForKey(char key) const {
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
        if (c < 'A' || c > 'Z') return std::nullopt;

        int index = kAlphaToReadingPosition[static_cast<size_t>(c - 'A')];
        int midi = baseMidi_ + index + octaveOffset_;
        return makeNote(midi);
    }

    // Posição de leitura (0..25) de uma tecla 'A'-'Z', ou -1 se inválida.
    // Usada pela UI para posicionar o teclado visual (docs UX.md /
    // mapeamento-teclado.md) e pelo ChartGenerator para escolher teclas
    // fisicamente próximas ao gerar um Chart (RF13).
    static int readingPositionForKey(char key) {
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
        if (c < 'A' || c > 'Z') return -1;
        return kAlphaToReadingPosition[static_cast<size_t>(c - 'A')];
    }

private:
    int baseMidi_;
    int octaveOffset_ = 0;

    // Índice = letra - 'A' (ordem alfabética); valor = posição de leitura
    // no layout QWERTY (ver comentário da classe). Gerado a partir de
    // "QWERTYUIOPASDFGHJKLZXCVBNM".
    static constexpr std::array<int, 26> kAlphaToReadingPosition = {
        10, 23, 21, 12, 2, 13, 14, 15, 7, 16, 17, 18, 25,
        24, 8, 9, 0, 3, 11, 4, 6, 22, 1, 20, 5, 19
    };
};

} // namespace abntpiano
