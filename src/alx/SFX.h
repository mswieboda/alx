#pragma once
#include <algorithm>
#include "core/Audio.h"
#include "alx/Random.h"

namespace alx {
namespace SFX {

    // --- COMBAT PRESETS ---

    // Player Sword Swipe: Bright metallic whip / air swoosh
    inline SfxrParams sword_swipe() {
        SfxrParams p;
        p.gain = 0.23f; // % baseline gain
        p.wave_type = NOISE;
        p.attack_time = 0.002f;
        p.sustain_time = 0.015f;
        p.decay_time = 0.060f;
        p.start_frequency = 0.750f;
        p.min_frequency = 0.350f;
        p.slide = -0.200f;
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
        p.gain = 0.05f; // % baseline gain
        p.wave_type = NOISE;
        p.attack_time = 0.01f;
        p.sustain_time = 0.30f;
        p.decay_time = 0.050f;
        p.start_frequency = 0.050f;
        p.min_frequency = 0.0100f;
        p.slide = 0.100f;
        return p;
    }

    // Refining Bubble: Dull low gurgling with randomized pitch variation
    inline SfxrParams refiner_bubble() {
        SfxrParams p;
        p.wave_type = SINE;
        p.gain = 0.05f; // % of baseline SFX volume
        p.attack_time = 0.010f;
        p.sustain_time = 0.030f;
        p.decay_time = 0.080f;
        float freq_var = Random::get_float(-0.0015f, 0.0015f);
        p.start_frequency = std::clamp(0.0035f + freq_var, 0.0020f, 0.0055f);
        p.min_frequency = 0.010f;
        p.slide = Random::get_float(0.0010f, 0.0020f);
        return p;
    }

    // Light Mana Conversion & Ejection: Very dull, low-pitch swell
    inline SfxrParams mana_converted() {
        SfxrParams p;
        p.wave_type = SINE;
        p.gain = 0.069f; // % of baseline SFX volume
        p.attack_time = 0.020f;
        p.sustain_time = 0.080f;
        p.decay_time = 0.160f;
        p.start_frequency = 0.005f;
        p.min_frequency = 0.0005f;
        p.slide = 0.001f;
        return p;
    }

    // Egg Hatch: Rising chirp ending in a shell pop
    // TODO: this might not work/no sound yet
    inline SfxrParams egg_hatch() {
        SfxrParams p;
        p.gain = 0.09f;
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
        p.gain = 1.95f;
        p.wave_type = SQUARE;
        p.attack_time = 0.001f;
        p.sustain_time = 0.035f;
        p.decay_time = 0.070f;
        p.start_frequency = 0.350f;
        p.min_frequency = 0.050f;
        p.slide = -0.250f;
        p.square_duty = 0.150f;
        return p;
    }

    // Grid Removal Snap: Ascending release click sound (inverted pitch sweep of build_snap)
    inline SfxrParams remove_snap() {
        SfxrParams p = build_snap();
        // p.gain = 0.5f;
        p.start_frequency = 0.0f;
        // p.slide = 0.150f;
        // p.square_duty = 0.250f;
        return p;
    }

    // Player Wall Bump: Hollow stone impact
    // TODO: this doesn't work/no sound yet
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

    // Hold Countdown Ticking: Soft, non-intrusive low/medium metronome click
    inline SfxrParams countdown_tick() {
        SfxrParams p;
        p.gain = 0.22f; // % baseline gain (soft, non-intrusive)
        p.wave_type = SINE;
        p.attack_time = 0.001f;
        p.sustain_time = 0.010f;
        p.decay_time = 0.040f;
        p.start_frequency = 0.240f;
        p.min_frequency = 0.080f;
        p.slide = -0.180f;
        return p;
    }

    // Victory Cleanse Chime: Bright, triumphant GBA-style victory chime
    inline SfxrParams victory_chime() {
        SfxrParams p;
        p.gain = 0.50f;
        p.wave_type = SINE;
        p.attack_time = 0.005f;
        p.sustain_time = 0.180f;
        p.decay_time = 0.550f;
        p.start_frequency = 0.520f;
        p.min_frequency = 0.120f;
        p.slide = 0.280f;
        p.vibrato_depth = 0.220f;
        p.vibrato_speed = 0.350f;
        return p;
    }

    // Prompt Toast Notification: Soft low/mid bass blip / tactile lock bump
    inline SfxrParams prompt_toast() {
        SfxrParams p;
        p.gain = 0.90f; // % baseline gain (soft, non-intrusive)
        p.wave_type = SAWTOOTH;
        p.attack_time = 0.001f;
        p.sustain_time = 0.033f;
        p.decay_time = 0.133f;
        p.start_frequency = 0.033f;
        p.min_frequency = 0.069f;
        p.slide = -0.069f;
        return p;
    }

    // Legacy presets
    inline SfxrParams phaser() { return sword_swipe(); }
    inline SfxrParams explosion() { return player_death(); }
    inline SfxrParams coin() { return refiner_bubble(); }

} // namespace SFX
} // namespace alx
