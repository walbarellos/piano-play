#include "abntpiano/ui/MenuRenderer.hpp"
#include <algorithm>

namespace abntpiano::ui {

void MenuRenderer::render(SDL_Renderer* ren,
                          const FontCollection& fonts,
                          const SongCatalog& catalog,
                          size_t selectedSong,
                          int difficulty,
                          bool teacherMode,
                          int mouseX,
                          int mouseY) const {
    // 1. Fundo do Menu: gradiente escuro de cauda de piano
    SDL_Rect fullScreen{0, 0, kScreenWidth, kScreenHeight};
    renderGradientRect(ren, fullScreen, {10, 7, 15, 255}, {18, 14, 26, 255}); // #0A070F -> #120E1A

    // 2. Cabeçalho
    renderText(ren, fonts.large, "ABNT PIANO", 60, 24, {239, 230, 214, 255}); // #EFE6D6
    renderText(ren, fonts.small, "Simulador de Concerto e Treinador Musical para Teclado ABNT2 / QWERTY", 60, 68, {140, 164, 196, 220}); // #8CA4C4

    // Friso decorativo de latão e feltro
    SDL_Rect brassLine{60, 102, kScreenWidth - 120, 2};
    SDL_SetRenderDrawColor(ren, 199, 154, 74, 90); // #C79A4A
    SDL_RenderFillRect(ren, &brassLine);

    SDL_Rect feltAccent{60, 104, 160, 2};
    SDL_SetRenderDrawColor(ren, 179, 46, 66, 255); // #B32E42
    SDL_RenderFillRect(ren, &feltAccent);

    // ─── PAINEL ESQUERDO: SELEÇÃO DE OBRAS COMPLETAS ─────────────────────
    renderText(ren, fonts.small, "REPERTÓRIO DE CONCERTO (OBRAS COMPLETAS)", 60, 122, {199, 154, 74, 255});

    size_t songCount = catalog.songCount();
    int cardX = 60;
    int cardW = 710;
    int cardH = 88;
    int startY = 148;

    const std::string songDetails[] = {
        "3m 48s  •  1.041 notas  •  Estrutura A-B-A-C-A Integral",
        "1m 33s  •  604 notas  •  25 Compassos Integrais (Largo)",
        "3m 44s  •  2.819 notas  •  Allegretto Integral",
        "8m 05s  •  5.013 notas  •  Obra Integral de Concerto",
        "10m 41s • 10.711 notas  •  I. Presto Movimento Integral"
    };

    for (size_t i = 0; i < songCount && i < 5; i++) {
        const auto& s = catalog.getSong(i);
        int cardY = startY + static_cast<int>(i) * (cardH + 12);
        bool isSelected = (selectedSong == i);
        bool isHover = (mouseX >= cardX && mouseX <= cardX + cardW &&
                        mouseY >= cardY && mouseY <= cardY + cardH);

        SDL_Rect cardRect{cardX, cardY, cardW, cardH};

        // Fundo do card
        if (isSelected) {
            renderGradientRect(ren, cardRect, {38, 24, 48, 255}, {26, 17, 36, 255});
        } else if (isHover) {
            renderGradientRect(ren, cardRect, {26, 20, 36, 255}, {18, 14, 26, 255});
        } else {
            SDL_SetRenderDrawColor(ren, 16, 12, 24, 255);
            SDL_RenderFillRect(ren, &cardRect);
        }

        // Borda
        if (isSelected) {
            SDL_SetRenderDrawColor(ren, 199, 154, 74, 255); // Ouro
            SDL_RenderDrawRect(ren, &cardRect);
            SDL_Rect innerBorder{cardRect.x + 1, cardRect.y + 1, cardRect.w - 2, cardRect.h - 2};
            SDL_SetRenderDrawColor(ren, 199, 154, 74, 140);
            SDL_RenderDrawRect(ren, &innerBorder);
        } else if (isHover) {
            SDL_SetRenderDrawColor(ren, 140, 110, 170, 200);
            SDL_RenderDrawRect(ren, &cardRect);
        } else {
            SDL_SetRenderDrawColor(ren, 43, 36, 54, 255); // #2B2436
            SDL_RenderDrawRect(ren, &cardRect);
        }

        // Indicador numérico / atalho [F2..F6]
        std::string shortcutBadge = "[F" + std::to_string(i + 2) + "]";
        renderText(ren, fonts.tiny, shortcutBadge, cardX + 16, cardY + 16,
                   isSelected ? SDL_Color{199, 154, 74, 255} : SDL_Color{110, 104, 128, 200});

        // Título da peça
        renderText(ren, fonts.medium, s.title, cardX + 64, cardY + 12,
                   isSelected ? SDL_Color{255, 250, 240, 255} : SDL_Color{220, 215, 205, 255});

        // Compositor
        renderText(ren, fonts.small, s.composer, cardX + 64, cardY + 38,
                   isSelected ? SDL_Color{199, 154, 74, 255} : SDL_Color{140, 133, 152, 220});

        // Detalhes da obra
        std::string detail = (i < 5) ? songDetails[i] : "";
        renderText(ren, fonts.tiny, detail, cardX + 64, cardY + 62,
                   isSelected ? SDL_Color{220, 200, 240, 220} : SDL_Color{110, 104, 128, 200});

        // Selo de status selecionado
        if (isSelected) {
            renderCapsule(ren, static_cast<float>(cardX + cardW - 130), static_cast<float>(cardY + 30),
                          115.0f, 28.0f, {179, 46, 66, 255}, {140, 30, 50, 255});
            renderCapsuleOutline(ren, static_cast<float>(cardX + cardW - 130), static_cast<float>(cardY + 30),
                                 115.0f, 28.0f, {199, 154, 74, 255});
            renderText(ren, fonts.tiny, "SELECIONADA", cardX + cardW - 73, cardY + 44, {255, 255, 255, 255}, true);
        }
    }

    // ─── PAINEL DIREITO: CONFIGURAÇÃO DA PARTIDA & MODO TEACHER ─────────
    int rightX = 810;
    int rightW = 410;

    // 1. Dificuldade
    renderText(ren, fonts.small, "DIFICULDADE", rightX, 122, {199, 154, 74, 255});

    int diffY = 148;
    int diffH = 42;
    int diffW = 130;
    const char* diffLabels[] = {"FÁCIL [1]", "NORMAL [2]", "DIFÍCIL [3]"};

    for (int d = 1; d <= 3; d++) {
        int dx = rightX + (d - 1) * (diffW + 10);
        bool isDiffSel = (difficulty == d);
        bool isDiffHov = (mouseX >= dx && mouseX <= dx + diffW && mouseY >= diffY && mouseY <= diffY + diffH);

        SDL_Rect diffRect{dx, diffY, diffW, diffH};
        if (isDiffSel) {
            renderCapsule(ren, static_cast<float>(dx), static_cast<float>(diffY),
                          static_cast<float>(diffW), static_cast<float>(diffH),
                          {199, 154, 74, 255}, {160, 120, 50, 255});
            renderText(ren, fonts.small, diffLabels[d - 1], dx + diffW / 2, diffY + diffH / 2, {10, 7, 15, 255}, true);
        } else {
            SDL_Color bgCol = isDiffHov ? SDL_Color{30, 24, 42, 255} : SDL_Color{18, 14, 26, 255};
            renderCapsule(ren, static_cast<float>(dx), static_cast<float>(diffY),
                          static_cast<float>(diffW), static_cast<float>(diffH), bgCol, bgCol);
            renderCapsuleOutline(ren, static_cast<float>(dx), static_cast<float>(diffY),
                                 static_cast<float>(diffW), static_cast<float>(diffH),
                                 isDiffHov ? SDL_Color{140, 110, 170, 200} : SDL_Color{43, 36, 54, 255});
            renderText(ren, fonts.small, diffLabels[d - 1], dx + diffW / 2, diffY + diffH / 2, {185, 178, 196, 255}, true);
        }
    }

    // 2. MODO TEACHER (PROFESSOR)
    renderText(ren, fonts.small, "MODO PROFESSOR (TEACHER)", rightX, 222, {199, 154, 74, 255});

    int teachY = 248;
    int teachH = 64;
    bool isTeachHov = (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= teachY && mouseY <= teachY + teachH);

    if (teacherMode) {
        // Teacher LIGADO (ON): o professor joga e o jogador assiste
        renderCapsule(ren, static_cast<float>(rightX), static_cast<float>(teachY),
                      static_cast<float>(rightW), static_cast<float>(teachH),
                      {85, 24, 110, 255}, {55, 14, 75, 255});
        renderCapsuleOutline(ren, static_cast<float>(rightX), static_cast<float>(teachY),
                             static_cast<float>(rightW), static_cast<float>(teachH),
                             {199, 154, 74, 255});
        renderText(ren, fonts.medium, "PROFESSOR: LIGADO (ON)", rightX + rightW / 2, teachY + 22, {255, 255, 255, 255}, true);
        renderText(ren, fonts.tiny, "[Clique ou aperte T / F12 para alternar]", rightX + rightW / 2, teachY + 46, {255, 210, 240, 220}, true);
    } else {
        // Teacher DESLIGADO (OFF): o jogador toca
        SDL_Color bgCol = isTeachHov ? SDL_Color{28, 22, 38, 255} : SDL_Color{18, 14, 26, 255};
        renderCapsule(ren, static_cast<float>(rightX), static_cast<float>(teachY),
                      static_cast<float>(rightW), static_cast<float>(teachH), bgCol, bgCol);
        renderCapsuleOutline(ren, static_cast<float>(rightX), static_cast<float>(teachY),
                             static_cast<float>(rightW), static_cast<float>(teachH),
                             isTeachHov ? SDL_Color{140, 110, 170, 255} : SDL_Color{50, 42, 65, 255});
        renderText(ren, fonts.medium, "PROFESSOR: DESLIGADO (OFF)", rightX + rightW / 2, teachY + 22, {210, 204, 220, 255}, true);
        renderText(ren, fonts.tiny, "[Clique ou aperte T / F12 para alternar]", rightX + rightW / 2, teachY + 46, {110, 104, 128, 200}, true);
    }

    // Caixa de explicação do modo
    int expY = 324;
    int expH = 76;
    SDL_Rect expRect{rightX, expY, rightW, expH};
    SDL_SetRenderDrawColor(ren, teacherMode ? 32 : 14, teacherMode ? 14 : 10, teacherMode ? 42 : 20, 220);
    SDL_RenderFillRect(ren, &expRect);
    SDL_SetRenderDrawColor(ren, teacherMode ? 90 : 35, teacherMode ? 45 : 28, teacherMode ? 110 : 45, 200);
    SDL_RenderDrawRect(ren, &expRect);

    if (teacherMode) {
        renderText(ren, fonts.small, "Modo Assistir e Aprender:", rightX + 16, expY + 12, {255, 215, 0, 255});
        renderText(ren, fonts.tiny, "O professor toca a obra com precisao humana perfeita enquanto", rightX + 16, expY + 36, {230, 215, 245, 240});
        renderText(ren, fonts.tiny, "voce observa as teclas, o andamento, os acordes e o ritmo.", rightX + 16, expY + 54, {230, 215, 245, 240});
    } else {
        renderText(ren, fonts.small, "Modo Pratica Interativa:", rightX + 16, expY + 12, {140, 164, 196, 255});
        renderText(ren, fonts.tiny, "Voce assume o piano e toca as teclas no teclado do computador.", rightX + 16, expY + 36, {170, 164, 185, 220});
        renderText(ren, fonts.tiny, "A melodia soa no acerto; notas erradas ou perdidas ficam em mute.", rightX + 16, expY + 54, {170, 164, 185, 220});
    }

    // ─── 3. BOTÃO DE INICIAR (DESTAQUE MÁXIMO) ─────────────────────────
    int btnY = 435;
    int btnH = 82;
    bool isStartHov = (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= btnY && mouseY <= btnY + btnH);

    SDL_Color startTop = isStartHov ? SDL_Color{210, 52, 75, 255} : SDL_Color{179, 46, 66, 255}; // #B32E42
    SDL_Color startBot = isStartHov ? SDL_Color{150, 30, 50, 255} : SDL_Color{120, 22, 38, 255};

    renderCapsule(ren, static_cast<float>(rightX), static_cast<float>(btnY),
                  static_cast<float>(rightW), static_cast<float>(btnH), startTop, startBot);
    renderCapsuleOutline(ren, static_cast<float>(rightX), static_cast<float>(btnY),
                         static_cast<float>(rightW), static_cast<float>(btnH),
                         isStartHov ? SDL_Color{255, 225, 120, 255} : SDL_Color{199, 154, 74, 255});

    // Glow sutil no botão iniciar
    if (isStartHov) {
        renderGlowDisc(ren, static_cast<float>(rightX + rightW / 2), static_cast<float>(btnY + btnH / 2),
                       static_cast<float>(rightW / 2), {255, 100, 120, 80}, {255, 100, 120, 0});
    }

    // Ícone de Play vetorial estilizado
    int iconX = rightX + rightW / 2 - 62;
    int iconY = btnY + 28;
    for (int dy = -8; dy <= 8; dy++) {
        int w = 8 - std::abs(dy);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderDrawLine(ren, iconX, iconY + dy, iconX + w, iconY + dy);
    }

    renderText(ren, fonts.large, "INICIAR", rightX + rightW / 2 + 10, btnY + 24, {255, 255, 255, 255}, true);
    renderText(ren, fonts.tiny, "[Pressione ENTER ou clique]", rightX + rightW / 2, btnY + 58, {255, 210, 220, 230}, true);

    // 4. Botão Secundário: Tocar Livre (Free Play)
    int fpY = 538;
    int fpH = 52;
    bool isFpHov = (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= fpY && mouseY <= fpY + fpH);

    SDL_Color fpBg = isFpHov ? SDL_Color{35, 26, 48, 255} : SDL_Color{22, 16, 32, 255};
    renderCapsule(ren, static_cast<float>(rightX), static_cast<float>(fpY),
                  static_cast<float>(rightW), static_cast<float>(fpH), fpBg, fpBg);
    renderCapsuleOutline(ren, static_cast<float>(rightX), static_cast<float>(fpY),
                         static_cast<float>(rightW), static_cast<float>(fpH),
                         isFpHov ? SDL_Color{140, 164, 196, 255} : SDL_Color{55, 44, 70, 255});
    renderText(ren, fonts.small, "Tocar Livre (Free Play)", rightX + rightW / 2, fpY + 18, {210, 204, 225, 255}, true);
    renderText(ren, fonts.tiny, "[Tecla F1]", rightX + rightW / 2, fpY + 36, {110, 104, 128, 200}, true);

    // 5. Botão Atalhos (?)
    int atY = 604;
    int atH = 42;
    bool isAtHov = (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= atY && mouseY <= atY + atH);
    SDL_Color atBg = isAtHov ? SDL_Color{28, 22, 38, 255} : SDL_Color{14, 10, 20, 255};
    renderCapsule(ren, static_cast<float>(rightX), static_cast<float>(atY),
                  static_cast<float>(rightW), static_cast<float>(atH), atBg, atBg);
    renderCapsuleOutline(ren, static_cast<float>(rightX), static_cast<float>(atY),
                         static_cast<float>(rightW), static_cast<float>(atH),
                         isAtHov ? SDL_Color{140, 110, 170, 200} : SDL_Color{40, 32, 52, 255});
    renderText(ren, fonts.tiny, "?  Ver Guia de Atalhos do Teclado", rightX + rightW / 2, atY + 21, {140, 133, 152, 220}, true);

    // ─── RODAPÉ DO MENU ──────────────────────────────────────────────────
    renderText(ren, fonts.tiny,
               "[Cima / Baixo] Escolher Obra  |  [1/2/3] Dificuldade  |  [T / F12] Modo Professor  |  [ENTER] Iniciar  |  [ESC] Sair",
               kScreenWidth / 2, kScreenHeight - 24, {110, 104, 128, 220}, true);
}

MenuAction MenuRenderer::handleMouseClick(int mouseX, int mouseY,
                                          size_t& selectedSong,
                                          int& difficulty,
                                          bool& teacherMode,
                                          size_t songCount) const {
    // 1. Clique nas músicas do painel esquerdo
    int cardX = 60;
    int cardW = 710;
    int cardH = 88;
    int startY = 148;

    for (size_t i = 0; i < songCount && i < 5; i++) {
        int cardY = startY + static_cast<int>(i) * (cardH + 12);
        if (mouseX >= cardX && mouseX <= cardX + cardW &&
            mouseY >= cardY && mouseY <= cardY + cardH) {
            selectedSong = i;
            return MenuAction::SelectSong;
        }
    }

    // 2. Clique na dificuldade
    int rightX = 810;
    int diffY = 148;
    int diffH = 42;
    int diffW = 130;
    for (int d = 1; d <= 3; d++) {
        int dx = rightX + (d - 1) * (diffW + 10);
        if (mouseX >= dx && mouseX <= dx + diffW && mouseY >= diffY && mouseY <= diffY + diffH) {
            difficulty = d;
            return MenuAction::SelectDifficulty;
        }
    }

    // 3. Clique no botão de Modo Professor (Teacher)
    int teachY = 248;
    int teachH = 64;
    int rightW = 410;
    if (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= teachY && mouseY <= teachY + teachH) {
        teacherMode = !teacherMode;
        return MenuAction::ToggleTeacher;
    }

    // 4. Clique no botão INICIAR
    int btnY = 435;
    int btnH = 82;
    if (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= btnY && mouseY <= btnY + btnH) {
        return MenuAction::StartSong;
    }

    // 5. Clique no botão Free Play
    int fpY = 538;
    int fpH = 52;
    if (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= fpY && mouseY <= fpY + fpH) {
        return MenuAction::StartFreePlay;
    }

    // 6. Clique no botão Atalhos (?)
    int atY = 604;
    int atH = 42;
    if (mouseX >= rightX && mouseX <= rightX + rightW && mouseY >= atY && mouseY <= atY + atH) {
        return MenuAction::ToggleShortcuts;
    }

    return MenuAction::None;
}

} // namespace abntpiano::ui
