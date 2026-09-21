#pragma once

#include "abntpiano/SongCatalog.hpp"
#include "abntpiano/ui/RenderTypes.hpp"
#include <SDL.h>

namespace abntpiano::ui {

enum class MenuAction {
    None,
    StartSong,
    StartFreePlay,
    ToggleTeacher,
    SelectDifficulty,
    SelectSong,
    ToggleShortcuts,
    Quit
};

class MenuRenderer {
public:
    MenuRenderer() = default;

    void render(SDL_Renderer* ren,
                const FontCollection& fonts,
                const SongCatalog& catalog,
                size_t selectedSong,
                int difficulty,
                bool teacherMode,
                int mouseX,
                int mouseY) const;

    MenuAction handleMouseClick(int mouseX, int mouseY,
                                size_t& selectedSong,
                                int& difficulty,
                                bool& teacherMode,
                                size_t songCount) const;
};

} // namespace abntpiano::ui
