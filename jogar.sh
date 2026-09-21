#!/usr/bin/env bash
set -e

# Garante que roda a partir do diretório do projeto (para achar assets/ e savegame)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Define DISPLAY caso não esteja configurado no terminal
export DISPLAY="${DISPLAY:-:0}"

# Garante build configurado e compilação incremental atualizada
if [ ! -d "build" ]; then
    echo "⚙️ Configurando CMake..."
    cmake -B build -S .
fi
cmake --build build -j"$(nproc)"

echo "🎹 Iniciando ABNT Piano..."
exec ./build/abntpiano_app "$@"
