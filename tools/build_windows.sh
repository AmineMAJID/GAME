#!/bin/sh
# ============================================================
# FarmVale — cross-compilation Windows depuis Linux (Zig)
# Produit build/FarmVale.exe sans installer MinGW.
# Prérequis : pip install ziglang
# ============================================================
set -e
cd "$(dirname "$0")/.."
mkdir -p build

ZIG="python3 -m ziglang"

command -v python3 >/dev/null || { echo "python3 requis"; exit 1; }
$ZIG version >/dev/null 2>&1 || {
    echo "Zig non trouvé. Installe-le : pip install ziglang"
    exit 1
}

echo "Compilation Windows (x86_64-windows-gnu) ..."
$ZIG c++ -target x86_64-windows-gnu -std=c++17 -O2 \
    src/core/*.cpp src/engine/*.cpp src/game/*.cpp \
    src/platform/platform_win32.cpp src/main.cpp \
    -o build/FarmVale.exe \
    -lwinmm -lgdi32 -luser32 -lshell32

echo "OK -> build/FarmVale.exe (copie-le sur ta machine Windows pour tester)"
