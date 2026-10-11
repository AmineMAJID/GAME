#include "image.h"
#include <fstream>
#include <cstdint>

namespace fe {

bool saveBMP(const Framebuffer& fb, const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    int w = fb.w, h = fb.h;
    int rowSize = (w * 3 + 3) & ~3; // aligné sur 4 octets
    uint32_t dataSize = rowSize * h;
    uint32_t fileSize = 54 + dataSize;

    auto put32 = [&](uint32_t v) { f.put(v & 255); f.put((v >> 8) & 255); f.put((v >> 16) & 255); f.put((v >> 24) & 255); };
    auto put16 = [&](uint16_t v) { f.put(v & 255); f.put((v >> 8) & 255); };

    // En-tête BMP
    f.put('B'); f.put('M');
    put32(fileSize);
    put32(0);
    put32(54);
    put32(40);          // taille du DIB header
    put32(w);
    put32(h);
    put16(1);           // plans
    put16(24);          // bits par pixel
    put32(0);           // compression
    put32(dataSize);
    put32(2835); put32(2835);
    put32(0); put32(0);

    // Pixels (bottom-up, BGR)
    std::vector<uint8_t> row(rowSize, 0);
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            uint32_t p = fb.px[(size_t)y * w + x];
            row[x * 3 + 0] = (uint8_t)(p & 255);        // B
            row[x * 3 + 1] = (uint8_t)((p >> 8) & 255); // G
            row[x * 3 + 2] = (uint8_t)((p >> 16) & 255);// R
        }
        f.write((const char*)row.data(), rowSize);
    }
    return (bool)f;
}

} // namespace fe
