#!/usr/bin/env bash
set -e

# Garante que roda a partir do diretório do projeto (para achar assets/ e savegame)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Define DISPLAY caso não esteja configurado no terminal
export DISPLAY="${DISPLAY:-:0}"

# Se o binário não existir ou se CMakeLists.txt / src forem mais recentes, compila rapidamente
if [ ! -f "build/abntpiano_app" ]; then
    echo "⚙️ Compilando o jogo pela primeira vez..."
    cmake -B build -S .
    cmake --build build -j"$(nproc)"
fi

echo "🎹 Iniciando ABNT Piano..."
exec ./build/abntpiano_app "$@"
