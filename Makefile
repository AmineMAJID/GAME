# ============================================================
# FarmVale — Makefile (Linux / macOS)
#   make            -> build le jeu (backend X11)
#   make headless   -> build la version headless (tests/screenshots)
#   make test       -> compile et lance le test de simulation
#   make windows    -> cross-compile build/FarmVale.exe (via Zig)
#   make run        -> lance le jeu
#   make clean
# Sur Windows : voir build_windows.bat (aucune dépendance requise)
# ============================================================

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
INCLUDES  = -Isrc -Isrc/core -Isrc/engine -Isrc/game -Isrc/platform

CORE_SRC  = $(wildcard src/core/*.cpp)
ENGINE_SRC = $(wildcard src/engine/*.cpp)
GAME_SRC  = $(wildcard src/game/*.cpp)

# --- Détection des headers X11 (système ou vendored) ---
X11_SYS := $(wildcard /usr/include/X11/Xlib.h)
ifneq ($(X11_SYS),)
X11_FLAGS = -lX11
X11_INC =
else ifneq ($(wildcard third_party/x11/include/X11/Xlib.h),)
X11_FLAGS = /usr/lib/x86_64-linux-gnu/libX11.so.6
X11_INC = -Ithird_party/x11/include
else
# Ni système ni vendored : on télécharge les en-têtes officiels (GitHub)
X11_FETCH := $(shell sh tools/fetch_x11_headers.sh 2>&1)
X11_FLAGS = /usr/lib/x86_64-linux-gnu/libX11.so.6
X11_INC = -Ithird_party/x11/include
endif

.PHONY: all headless test windows run clean

all: build/farmvale

build/farmvale: $(CORE_SRC) $(ENGINE_SRC) $(GAME_SRC) src/platform/platform_x11.cpp src/main.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(X11_INC) -o $@ $^ $(X11_FLAGS) -ldl -lpthread

headless: build/farmvale_headless

build/farmvale_headless: $(CORE_SRC) $(ENGINE_SRC) $(GAME_SRC) src/platform/platform_headless.cpp src/main.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -DFARMVALE_HEADLESS -o $@ $^ -lpthread

test: build/sim_test
	./build/sim_test

build/sim_test: $(CORE_SRC) tests/sim_test.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# --- Cross-compilation Windows (Zig embarque les en-têtes MinGW) ---
windows:
	@bash tools/build_windows.sh

run: build/farmvale
	./build/farmvale

clean:
	rm -rf build
