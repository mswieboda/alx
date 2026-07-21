#pragma once
#include "core/Audio.h"

namespace SFX {
    inline SfxrParams phaser() {
        SfxrParams p;
        p.wave_type = SQUARE;
        p.start_frequency = 0.25f;
        p.slide = 0.3f; // Pitch sweeps UP
        p.sustain_time = 0.05f;
        p.decay_time = 0.12f;
        return p;
    }

    inline SfxrParams explosion() {
        SfxrParams p;
        p.wave_type = NOISE;
        p.start_frequency = 0.2f;
        p.slide = -0.15f; // Pitch sweeps DOWN
        p.sustain_time = 0.1f;
        p.decay_time = 0.35f;
        return p;
    }

    inline SfxrParams coin() {
        SfxrParams p;
        p.wave_type = SQUARE;
        p.start_frequency = 0.03f;
        p.slide = 0.03f;
        p.sustain_time = 0.03f;
        p.decay_time = 0.03f;
        return p;
    }
}
