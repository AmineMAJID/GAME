// ============================================================
// FarmVale — plateforme headless (sans fenêtre)
// Sert aux tests automatisés et aux captures d'écran (bot).
// Usage: farmvale_headless [--bot jours] [--shots prefixe] [--frames N]
// ============================================================
#include "platform.h"
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstring>
#include <string>

namespace fe {

class HeadlessPlatform : public IPlatform {
public:
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    long frames = 0;
    long maxFrames = 100000;
    bool quit = false;

    bool poll() override {
        if (quit || frames >= maxFrames) return false;
        return true;
    }
    void present(const Framebuffer&) override { frames++; }
    void requestQuit() override { quit = true; }
    double timeMs() override {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }
    void sleepMs(int ms) override {
        if (ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    void setTitle(const std::string& t) override { printf("[titre] %s\n", t.c_str()); }
};

} // namespace fe

fe::IPlatform* createPlatform(int argc, char** argv) {
    auto* p = new fe::HeadlessPlatform();
    p->windowScale = 2;
    p->saveDir = "build";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) p->maxFrames = atol(argv[++i]);
    }
    return p;
}

// Point d'entrée (build headless / tests)
extern int farmvale_main(int argc, char** argv);
int main(int argc, char** argv) {
    return farmvale_main(argc, argv);
}
