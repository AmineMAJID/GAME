#include "audio.h"
#include <cmath>

namespace fe {

static const int RATE = 22050;

std::vector<int16_t> Audio::synth(int type, float freq, float dur, float vol) {
    int n = (int)(dur * RATE);
    std::vector<int16_t> out(n);
    float phase = 0;
    float inc = freq * 2.0f * 3.14159265f / RATE;
    for (int i = 0; i < n; i++) {
        float t = (float)i / RATE;
        // enveloppe ADSR douce
        float env = 1.0f;
        float atk = 0.01f, rel = 0.08f;
        if (t < atk) env = t / atk;
        if (t > dur - rel) env = std::max(0.0f, (dur - t) / rel);
        float s = 0;
        switch (type) {
            case 0: s = std::sin(phase); break;                       // sinus (doux)
            case 1: s = std::sin(phase) + 0.3f * std::sin(2 * phase); break; // riche
            case 2: s = phase < 3.14159f ? 1.0f : -1.0f; break;        // carré
            case 3: s = (float)((int)(phase / 3.14159265f) % 2 == 0 ? 1 : -1) * 0.5f; break;
            default: s = std::sin(phase); break;
        }
        // bruit blanc pour les percussions
        if (type == 9) s = ((rand() % 2000) - 1000) / 1000.0f;
        out[i] = (int16_t)(s * env * vol * master * 32767.0f * 0.5f);
        phase += inc;
        if (phase > 6.2831f * 64) phase = std::fmod(phase, 6.2831f);
    }
    return out;
}

void Audio::play(const std::vector<int16_t>& samples) {
    if (!enabled || !platform || samples.empty()) return;
    platform->playPcm(samples.data(), (int)samples.size(), RATE);
}

void Audio::beep(int hz, int ms) {
    if (!enabled || !platform) return;
    platform->beep(hz, ms);
}

void Audio::sfx(Sfx s) {
    if (!enabled) return;
    switch (s) {
        case Sfx::Till:    play(synth(9, 90, 0.07f, 0.5f)); break;
        case Sfx::Water:   play(synth(9, 500, 0.10f, 0.25f)); play(synth(0, 700, 0.06f, 0.2f)); break;
        case Sfx::Plant:   play(synth(1, 520, 0.08f, 0.35f)); play(synth(1, 780, 0.06f, 0.3f)); break;
        case Sfx::Harvest: play(synth(1, 660, 0.07f, 0.4f)); play(synth(1, 990, 0.09f, 0.35f)); break;
        case Sfx::Chop:    play(synth(9, 140, 0.09f, 0.5f)); play(synth(1, 220, 0.05f, 0.3f)); break;
        case Sfx::Scythe:  play(synth(9, 300, 0.08f, 0.3f)); break;
        case Sfx::Forage:  play(synth(0, 880, 0.06f, 0.3f)); break;
        case Sfx::Feed:    play(synth(1, 440, 0.05f, 0.3f)); play(synth(1, 660, 0.05f, 0.3f)); break;
        case Sfx::Pet:     play(synth(0, 1046, 0.07f, 0.3f)); play(synth(0, 1318, 0.09f, 0.25f)); break;
        case Sfx::Milk:    play(synth(1, 330, 0.12f, 0.3f)); break;
        case Sfx::Catch:   play(synth(1, 740, 0.06f, 0.4f)); play(synth(1, 1100, 0.08f, 0.3f)); break;
        case Sfx::Coin:    play(synth(1, 1318, 0.05f, 0.4f)); play(synth(1, 1760, 0.10f, 0.45f)); break;
        case Sfx::Quest:   play(synth(0, 523, 0.08f, 0.35f)); play(synth(0, 659, 0.08f, 0.35f)); play(synth(0, 784, 0.14f, 0.4f)); break;
        case Sfx::LevelUp: play(synth(0, 523, 0.09f, 0.4f)); play(synth(0, 784, 0.09f, 0.4f)); play(synth(0, 1046, 0.16f, 0.45f)); break;
        case Sfx::Door:    play(synth(1, 180, 0.14f, 0.35f)); break;
        case Sfx::Open:    play(synth(1, 600, 0.05f, 0.3f)); play(synth(1, 900, 0.07f, 0.3f)); break;
        case Sfx::Swing:   play(synth(9, 250, 0.05f, 0.25f)); break;
        case Sfx::Birth:   play(synth(0, 880, 0.08f, 0.35f)); play(synth(0, 1320, 0.12f, 0.35f)); break;
        case Sfx::Breed:   play(synth(0, 698, 0.08f, 0.3f)); play(synth(0, 880, 0.12f, 0.3f)); break;
        case Sfx::Sleep:   play(synth(0, 220, 0.30f, 0.25f)); break;
        case Sfx::Wakeup:  play(synth(0, 440, 0.10f, 0.3f)); play(synth(0, 554, 0.10f, 0.3f)); break;
        case Sfx::Select:  play(synth(0, 1200, 0.03f, 0.25f)); break;
        case Sfx::Error:   play(synth(2, 160, 0.10f, 0.35f)); break;
        case Sfx::Bell:    play(synth(1, 1568, 0.20f, 0.3f)); break;
        case Sfx::Button:  play(synth(0, 900, 0.04f, 0.3f)); break;
    }
}

// ------------------------------------------------------------
// Musique générative : arpèges d'accords doux (La majeur)
// ------------------------------------------------------------
void Audio::update(double nowMs) {
    if (!musicOn || !enabled) return;
    if (nextNoteMs == 0) nextNoteMs = nowMs + 500;
    if (nowMs < nextNoteMs) return;
    // Progression : La / Mi / Fa#m / Ré  (A - E - F#m - D)
    static const float chords[4][3] = {
        {220.00f, 261.63f, 329.63f},  // La majeur
        {164.81f, 246.94f, 329.63f},  // Mi majeur
        {185.00f, 220.00f, 277.18f},  // Fa# mineur
        {146.83f, 220.00f, 293.66f},  // Ré majeur
    };
    const float* ch = chords[chordStep % 4];
    float f = ch[arpStep % 3] * (arpStep >= 3 ? 2.0f : 1.0f);
    play(synth(0, f, 0.45f, 0.16f));
    arpStep++;
    if (arpStep >= 6) { arpStep = 0; chordStep++; }
    nextNoteMs = nowMs + 280;
}

} // namespace fe
