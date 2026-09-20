#include "abntpiano/KeyboardMapper.hpp"
#include "abntpiano/Note.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace abntpiano;

static bool approxEqual(double a, double b, double eps = 0.01) {
    return std::fabs(a - b) < eps;
}

int main() {
    // Frequências de referência
    assert(approxEqual(midiToFrequency(69), 440.0));       // A4
    assert(approxEqual(midiToFrequency(60), 261.6256));    // C4
    assert(approxEqual(midiToFrequency(108), 4186.009, 0.05)); // C8
    assert(approxEqual(midiToFrequency(21), 27.5, 0.01));  // A0

    // Nomes
    assert(midiToName(60) == "C4");
    assert(midiToName(69) == "A4");
    assert(midiToName(61) == "C#4");

    // KeyboardMapper: mapeamento por POSIÇÃO FÍSICA (ADR-09), não alfabeto.
    // Q é a primeira tecla da fileira superior -> nota base (C4).
    KeyboardMapper mapper; // base = C4 (MIDI 60)
    auto q = mapper.noteForKey('Q');
    assert(q.has_value());
    assert(q->name == "C4");
    assert(q->midi == 60);

    // P é a última tecla da fileira superior (posição 9) -> A4.
    auto p = mapper.noteForKey('P');
    assert(p.has_value());
    assert(p->midi == 60 + 9);

    // A é a primeira tecla da home row (posição 10) -> A#4.
    auto a = mapper.noteForKey('A');
    assert(a.has_value());
    assert(a->midi == 60 + 10);

    // M é a última tecla da fileira inferior (posição 25).
    auto m = mapper.noteForKey('M');
    assert(m.has_value());
    assert(m->midi == 60 + 25);

    // Case-insensitive
    auto q_lower = mapper.noteForKey('q');
    assert(q_lower.has_value());
    assert(q_lower->midi == q->midi);

    // Duas teclas fisicamente vizinhas na mesma fileira (Q, W) devem ser
    // semitons vizinhos.
    auto w = mapper.noteForKey('W');
    assert(w.has_value());
    assert(w->midi == q->midi + 1);

    // Oitava (RF06)
    mapper.shiftOctaveUp();
    auto q2 = mapper.noteForKey('Q');
    assert(q2->name == "C5");

    mapper.shiftOctaveDown();
    mapper.shiftOctaveDown();
    auto q3 = mapper.noteForKey('Q');
    assert(q3->name == "C3");

    // Teclas não mapeadas (RF02)
    KeyboardMapper mapper2;
    assert(!mapper2.noteForKey('1').has_value());
    assert(!mapper2.noteForKey('@').has_value());

    // readingPositionForKey
    assert(KeyboardMapper::readingPositionForKey('Q') == 0);
    assert(KeyboardMapper::readingPositionForKey('P') == 9);
    assert(KeyboardMapper::readingPositionForKey('A') == 10);
    assert(KeyboardMapper::readingPositionForKey('M') == 25);
    assert(KeyboardMapper::readingPositionForKey('1') == -1);

    std::cout << "Todos os testes passaram.\n";
    return 0;
}
