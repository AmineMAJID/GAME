#pragma once
// ============================================================
// FarmVale — audio procédural (aucun asset requis)
// SFX synthétisés + musique générative (chords doux).
// ============================================================
#include "engine.h"
#include <vector>
#include <cstdint>

namespace fe {

enum class Sfx {
    Till, Water, Plant, Harvest, Chop, Scythe, Forage,
    Feed, Pet, Milk, Catch, Coin, Quest, LevelUp,
    Door, Open, Swing, Birth, Breed, Sleep, Wakeup,
    Select, Error, Bell, Button
};

struct Audio {
    bool enabled = true;
    bool musicOn = true;
    float master = 0.85f;
    IPlatform* platform = nullptr;

    void init(IPlatform* p) { platform = p; }
    void sfx(Sfx s);
    void beep(int hz, int ms);

    void startMusic() { musicOn = true; nextNoteMs = 0; }
    void stopMusic() { musicOn = false; }
    void update(double nowMs);

private:
    void play(const std::vector<int16_t>& samples);
    std::vector<int16_t> synth(int type, float freq, float dur, float vol);
    double nextNoteMs = 0;
    int chordStep = 0;
    int arpStep = 0;
};

} // namespace fe
