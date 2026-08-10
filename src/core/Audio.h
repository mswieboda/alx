#pragma once
#include <vector>
#include <cstdint>
#include <cmath>

enum WaveType {
    SQUARE = 0,
    SAWTOOTH,
    SINE,
    NOISE
};

struct SfxrParams {
    WaveType wave_type = SQUARE;

    // Relative volume gain multiplier (1.0f = 100% baseline volume, 0.5f = 50%, etc.)
    float gain = 1.0f;

    // Envelopes (in seconds)
    float attack_time = 0.0f;
    float sustain_time = 0.1f;
    float decay_time = 0.2f;

    // Frequency / Pitch
    float start_frequency = 0.3f;
    float min_frequency = 0.0f;
    float slide = 0.0f;           // Pitch sweep rate

    // Vibrato
    float vibrato_depth = 0.0f;
    float vibrato_speed = 0.0f;

    // Square wave duty cycle
    float square_duty = 0.5f;
};

namespace Audio {
    // --- AUDIO SYSTEM CONSTANTS ---
    static constexpr float DEFAULT_MUSIC_VOLUME = 0.03f; // 0.05f;
    static constexpr float DEFAULT_SFX_VOLUME   = 0.09f;
    static constexpr float DEFAULT_SFX_PAN      = 0.0f;
    static constexpr float MIN_SFX_PAN          = -1.0f;
    static constexpr float MAX_SFX_PAN          = 1.0f;
    static constexpr float MIN_VOLUME           = 0.0f;
    static constexpr float MAX_SFX_VOLUME       = 0.75f;
    static constexpr float MAX_MUSIC_VOLUME     = 0.5f;
    static constexpr uint32_t SAMPLE_RATE       = 44100;
    static constexpr uint32_t STEREO_CHANNELS   = 2;

    bool init();
    void cleanup();

    // --- SFX ---

    // Triggers an SFXR sound effect with optional per-instance volume override and stereo panning (-1.0f left, 0.0f center, +1.0f right)
    void play_sfx(const SfxrParams& params, float volume_override = -1.0f, float pan = DEFAULT_SFX_PAN);

    // --- Music ---

    // Loading
    bool load_music_from_memory(const uint8_t* data, size_t size);

    // Playback state controls
    void play_music(bool loop = true);
    void pause_music();
    void resume_music();
    void toggle_music(); // Convenience method for pause/resume
    void stop_music();   // Stops and rewinds

    // State queries
    bool is_music_loaded();
    bool is_music_playing();
    bool is_music_paused();

    // Volume
    void set_music_volume(float volume); // 0.0f to 1.0f
} // namespace Audio
