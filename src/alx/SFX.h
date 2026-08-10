#pragma once
#include "core/Audio.h"

namespace alx {
namespace SFX {

    // --- COMBAT PRESETS ---

    // Player Sword Swipe: Sharp metallic whip slash
    inline SfxrParams sword_swipe() {
        SfxrParams p;
        p.wave_type = SAWTOOTH;
        p.attack_time = 0.005f;
        p.sustain_time = 0.020f;
        p.decay_time = 0.080f;
        p.start_frequency = 0.450f;
        p.min_frequency = 0.050f;
        p.slide = -0.350f;
        return p;
    }

    // Enemy Hit: Punchy low thump
    inline SfxrParams enemy_hit() {
        SfxrParams p;
        p.wave_type = SQUARE;
        p.attack_time = 0.001f;
        p.sustain_time = 0.030f;
        p.decay_time = 0.090f;
        p.start_frequency = 0.120f;
        p.min_frequency = 0.020f;
        p.slide = -0.250f;
        p.square_duty = 0.400f;
        return p;
    }

    // Player Hit: Urgent high warning buzz
    inline SfxrParams player_hit() {
        SfxrParams p;
        p.wave_type = SQUARE;
        p.attack_time = 0.002f;
        p.sustain_time = 0.060f;
        p.decay_time = 0.140f;
        p.start_frequency = 0.550f;
        p.min_frequency = 0.100f;
        p.slide = -0.300f;
        p.vibrato_depth = 0.350f;
        p.vibrato_speed = 0.600f;
        p.square_duty = 0.250f;
        return p;
    }

    // Player Death: Heavy descending static fade
    inline SfxrParams player_death() {
        SfxrParams p;
        p.wave_type = NOISE;
        p.attack_time = 0.010f;
        p.sustain_time = 0.200f;
        p.decay_time = 0.450f;
        p.start_frequency = 0.300f;
        p.min_frequency = 0.010f;
        p.slide = -0.400f;
        return p;
    }

    // --- DARK TOWER, SPIRE & AUTOMATION PRESETS ---

    // Dark Tower Hit: Heavy resonant ominous rumble
    inline SfxrParams dark_tower_hit() {
        SfxrParams p;
        p.wave_type = SQUARE;
        p.attack_time = 0.010f;
        p.sustain_time = 0.100f;
        p.decay_time = 0.300f;
        p.start_frequency = 0.080f;
        p.min_frequency = 0.010f;
        p.slide = -0.100f;
        p.vibrato_depth = 0.400f;
        p.vibrato_speed = 0.250f;
        p.square_duty = 0.500f;
        return p;
    }

    // Dark Tower Spawn: Deep low-frequency rumble impact
    inline SfxrParams dark_tower_spawn() {
        SfxrParams p;
        p.wave_type = NOISE;
        p.attack_time = 0.020f;
        p.sustain_time = 0.150f;
        p.decay_time = 0.500f;
        p.start_frequency = 0.050f;
        p.min_frequency = 0.005f;
        p.slide = -0.200f;
        return p;
    }

    // Dark Tower Twilight Pulse: Low pulsating deep hum with slow vibrato
    inline SfxrParams twilight_pulse() {
        SfxrParams p;
        p.wave_type = SINE;
        p.attack_time = 0.080f;
        p.sustain_time = 0.250f;
        p.decay_time = 0.350f;
        p.start_frequency = 0.100f;
        p.min_frequency = 0.020f;
        p.vibrato_depth = 0.500f;
        p.vibrato_speed = 0.150f;
        return p;
    }

    // Spire Burn/Cackle: Electric high-voltage crackle
    inline SfxrParams spire_burn() {
        SfxrParams p;
        p.wave_type = NOISE;
        p.attack_time = 0.002f;
        p.sustain_time = 0.040f;
        p.decay_time = 0.080f;
        p.start_frequency = 0.650f;
        p.min_frequency = 0.100f;
        p.slide = 0.100f;
        return p;
    }

    // Refining Bubble: Gentle soft bubbling
    inline SfxrParams refiner_bubble() {
        SfxrParams p;
        p.wave_type = SINE;
        p.attack_time = 0.005f;
        p.sustain_time = 0.020f;
        p.decay_time = 0.050f;
        p.start_frequency = 0.420f;
        p.min_frequency = 0.100f;
        p.slide = 0.200f;
        return p;
    }

    // Egg Hatch: Rising chirp ending in a shell pop
    inline SfxrParams egg_hatch() {
        SfxrParams p;
        p.wave_type = SQUARE;
        p.attack_time = 0.002f;
        p.sustain_time = 0.050f;
        p.decay_time = 0.080f;
        p.start_frequency = 0.300f;
        p.min_frequency = 0.050f;
        p.slide = 0.500f;
        p.square_duty = 0.350f;
        return p;
    }

    // --- WORLD INTERACTION PRESETS ---

    // Grid Placement Snap: Heavy mechanical lock sound
    inline SfxrParams build_snap() {
        SfxrParams p;
        p.wave_type = SQUARE;
        p.attack_time = 0.001f;
        p.sustain_time = 0.030f;
        p.decay_time = 0.060f;
        p.start_frequency = 0.220f;
        p.min_frequency = 0.030f;
        p.slide = -0.180f;
        p.square_duty = 0.650f;
        return p;
    }

    // Player Wall Bump: Hollow stone impact
    inline SfxrParams wall_bump() {
        SfxrParams p;
        p.wave_type = SINE;
        p.attack_time = 0.001f;
        p.sustain_time = 0.020f;
        p.decay_time = 0.070f;
        p.start_frequency = 0.090f;
        p.min_frequency = 0.010f;
        p.slide = -0.200f;
        return p;
    }

    // Fixture Hit: Solid anvil strike
    inline SfxrParams fixture_hit() {
        SfxrParams p;
        p.wave_type = SAWTOOTH;
        p.attack_time = 0.001f;
        p.sustain_time = 0.040f;
        p.decay_time = 0.110f;
        p.start_frequency = 0.480f;
        p.min_frequency = 0.050f;
        p.slide = -0.350f;
        return p;
    }

    // Legacy presets
    inline SfxrParams phaser() { return sword_swipe(); }
    inline SfxrParams explosion() { return player_death(); }
    inline SfxrParams coin() { return refiner_bubble(); }

} // namespace SFX
} // namespace alx

