#pragma once

#include "abntpiano/KeyboardMapper.hpp"
#include <functional>
#include <set>

namespace abntpiano {

// Controlador do modo Piano Livre (Free Play) (RF07, RF08, UC01)
class FreePlayController {
public:
    using NoteOnCallback = std::function<void(const Note& note)>;
    using NoteOffCallback = std::function<void(const Note& note)>;

    explicit FreePlayController(KeyboardMapper mapper = KeyboardMapper())
        : mapper_(mapper) {}

    void setNoteOnCallback(NoteOnCallback cb) { noteOnCb_ = std::move(cb); }
    void setNoteOffCallback(NoteOffCallback cb) { noteOffCb_ = std::move(cb); }

    // Eventos de teclado
    bool onKeyDown(char key) {
        auto noteOpt = mapper_.noteForKey(key);
        if (!noteOpt.has_value()) return false;

        char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
        activeKeys_.insert(norm);

        if (noteOnCb_) {
            noteOnCb_(*noteOpt);
        }
        return true;
    }

    bool onKeyUp(char key) {
        auto noteOpt = mapper_.noteForKey(key);
        if (!noteOpt.has_value()) return false;

        char norm = static_cast<char>(std::toupper(static_cast<unsigned char>(key)));
        activeKeys_.erase(norm);

        if (!sustainActive_ && noteOffCb_) {
            noteOffCb_(*noteOpt);
        }
        return true;
    }

    // Controle de Sustain (RF05)
    void setSustain(bool active) {
        sustainActive_ = active;
        if (!sustainActive_ && noteOffCb_) {
            // Ao soltar o pedal/barra de sustain, dispara release das notas que não estão fisicamente pressionadas
            // (gerenciado pelo motor de áudio)
        }
    }
    bool isSustainActive() const { return sustainActive_; }

    // Controle de Oitavas (RF06)
    void shiftOctaveUp()   { mapper_.shiftOctaveUp(); }
    void shiftOctaveDown() { mapper_.shiftOctaveDown(); }
    int octaveOffset() const { return mapper_.octaveOffset(); }

    const std::set<char>& activeKeys() const { return activeKeys_; }
    const KeyboardMapper& mapper() const { return mapper_; }

private:
    KeyboardMapper mapper_;
    bool sustainActive_ = false;
    std::set<char> activeKeys_;

    NoteOnCallback noteOnCb_;
    NoteOffCallback noteOffCb_;
};

} // namespace abntpiano
