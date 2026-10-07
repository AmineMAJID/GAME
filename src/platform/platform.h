#pragma once
// Fabrique la plateforme adaptée à la cible de compilation.
#include "../engine/engine.h"

fe::IPlatform* createPlatform(int argc, char** argv);
