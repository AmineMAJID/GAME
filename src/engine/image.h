#pragma once
// ============================================================
// FarmVale — export d'images (BMP 24 bits, sans dépendance)
// ============================================================
#include "engine.h"
#include <string>

namespace fe {
bool saveBMP(const Framebuffer& fb, const std::string& path);
}
