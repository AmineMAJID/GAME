#pragma once
// Générateur pseudo-aléatoire déterministe (xorshift64*) — même seed = même monde.
#include <cstdint>

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed = 1) : s(seed ? seed : 0x9E3779B97F4A7C15ull) {}
    uint64_t nextU64() {
        s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
        return s * 0x2545F4914F6CDD1Dull;
    }
    uint32_t next() { return (uint32_t)(nextU64() >> 32); }
    int range(int a, int b) { return a + (int)(next() % (uint32_t)(b - a + 1)); } // [a..b]
    float uf() { return (float)((nextU64() >> 11) * (1.0 / 9007199254740992.0)); } // [0..1)
    bool chance(float p) { return uf() < p; }
};
