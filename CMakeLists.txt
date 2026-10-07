# ============================================================
# FarmVale — build CMake (Windows/MSVC ou MinGW, Linux/X11)
# Usage :  cmake -B build -S . && cmake --build build --config Release
# Pour Steam : ajoute -DFARMVALE_STEAM=ON et le chemin du SDK.
# ============================================================
cmake_minimum_required(VERSION 3.16)
project(FarmVale CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CORE_SRC
    src/core/items.cpp src/core/world.cpp src/core/sim.cpp
    src/core/sim_time.cpp src/core/sim_actions.cpp src/core/sim_animals.cpp
    src/core/sim_economy.cpp src/core/sim_quests.cpp src/core/serialize.cpp)
set(ENGINE_SRC
    src/engine/draw.cpp src/engine/audio.cpp src/engine/icons.cpp
    src/engine/image.cpp src/engine/font_data.cpp)
set(GAME_SRC
    src/game/game.cpp src/game/game_update.cpp src/game/game_render_world.cpp
    src/game/game_render_ui.cpp src/game/game_menus.cpp)

if(WIN32)
    add_executable(FarmVale WIN32 ${CORE_SRC} ${ENGINE_SRC} ${GAME_SRC}
        src/platform/platform_win32.cpp src/main.cpp)
    target_link_libraries(FarmVale PRIVATE gdi32 user32 shell32 winmm ole32)
    if(FARMVALE_STEAM)
        target_compile_definitions(FarmVale PRIVATE FARMVALE_STEAM)
        # Adapte ce chemin vers ton Steamworks SDK :
        target_include_directories(FarmVale PRIVATE "C:/steamworks_sdk/sdk/public/steam")
        target_link_libraries(FarmVale PRIVATE "C:/steamworks_sdk/sdk/redistributable_bin/win64/steam_api64.lib")
    endif()
else()
    find_package(X11)
    if(NOT X11_FOUND)
        message(STATUS "libX11 non trouvée : exécute tools/fetch_x11_headers.sh (en-têtes vendored)")
        include_directories(third_party/x11/include)
        add_executable(farmvale ${CORE_SRC} ${ENGINE_SRC} ${GAME_SRC}
            src/platform/platform_x11.cpp src/main.cpp)
        target_link_libraries(farmvale PRIVATE /usr/lib/x86_64-linux-gnu/libX11.so.6)
    else()
        include_directories(${X11_INCLUDE_DIR})
        add_executable(farmvale ${CORE_SRC} ${ENGINE_SRC} ${GAME_SRC}
            src/platform/platform_x11.cpp src/main.cpp)
        target_link_libraries(farmvale PRIVATE ${X11_LIBRARIES})
    endif()
endif()
