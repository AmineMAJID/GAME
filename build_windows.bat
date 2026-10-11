@echo off
REM ============================================================
REM FarmVale — build Windows (pour Steam)
REM Produit build\FarmVale.exe (autonome, aucune DLL requise).
REM
REM Prérequis : MinGW-w64 (g++) dans le PATH
REM   https://www.mingw-w64.org/downloads/
REM   ou via MSYS2 : pacman -S mingw-w64-x86_64-gcc
REM
REM Pour activer Steam : décommente la section STEAM ci-dessous
REM (nécessite le Steamworks SDK, voir steam\README_STEAM.md)
REM ============================================================
setlocal
cd /d "%~dp0"
if not exist build mkdir build

set SRC=src\core\items.cpp src\core\world.cpp src\core\sim.cpp src\core\sim_time.cpp src\core\sim_actions.cpp src\core\sim_animals.cpp src\core\sim_economy.cpp src\core\sim_quests.cpp src\core\serialize.cpp src\engine\draw.cpp src\engine\audio.cpp src\engine\icons.cpp src\engine\image.cpp src\engine\font_data.cpp src\game\game.cpp src\game\game_update.cpp src\game\game_render_world.cpp src\game\game_render_ui.cpp src\game\game_menus.cpp src\platform\platform_win32.cpp src\main.cpp

set FLAGS=-std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -mwindows
set LIBS=-lgdi32 -luser32 -lshell32 -lwinmm -lole32 -static -static-libgcc -static-libstdc++

echo Compilation de FarmVale.exe ...
g++ %FLAGS% %SRC% -o build\FarmVale.exe %LIBS%
if errorlevel 1 (
    echo ECHEC de la compilation.
    exit /b 1
)

REM --- Section STEAM (optionnel) ---------------------------------
REM 1. Télécharge le Steamworks SDK, copie steam_api64.lib dans build\
REM 2. Remplace FLAGS par : -DFARMVALE_STEAM -IC:\chemin\steamworks_sdk\sdk\public\steam
REM 3. Remplace LIBS par : build\steam_api64.lib -Lbuild (et garde les autres)
REM 4. Copie steam_api64.dll (sdk\redistributable_bin\win64) à côté de l'exe
REM ----------------------------------------------------------------

echo.
echo OK -> build\FarmVale.exe
echo Teste-le : build\FarmVale.exe
endlocal
