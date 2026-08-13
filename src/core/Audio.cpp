#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

extern "C" {
#include "pocketmod.h"
}

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <mutex>

#include "Audio.h"
#include "Log.h"

namespace Audio {

    struct SfxVoice {
        bool active{false};
        SfxrParams params{};
        float volume{DEFAULT_SFX_VOLUME};
        float pan{DEFAULT_SFX_PAN};
        uint32_t current_sample{0};
        uint32_t total_samples{0};
        float phase{0.0f};
        float f_freq{0.0f};
        float f_slide{0.0f};
        float vibrato_phase{0.0f};
        uint32_t noise_seed{0};
    };

    static ma_device g_audio_device;
    static bool g_initialized = false;
    static std::array<SfxVoice, AudioConfig::MAX_ACTIVE_VOICES> g_voices{};
    static std::mutex g_audio_mutex;

    // Music State
    static pocketmod_context g_pocketmod;
    static const uint8_t* g_music_data = nullptr;
    static size_t g_music_size = 0;
    static bool g_music_loaded = false;
    static bool g_music_playing = false;
    static bool g_music_paused = false;
    static bool g_music_loop = true;
    static float g_music_volume = DEFAULT_MUSIC_VOLUME;

    namespace {

        float calculate_envelope(const SfxrParams& params, float t) {
            if (t < params.attack_time) {
                return (params.attack_time > 0.0f) ? (t / params.attack_time) : 1.0f;
            }

            const float attack_and_sustain = params.attack_time + params.sustain_time;
            if (t < attack_and_sustain) {
                return 1.0f;
            }

            const float decay_elapsed = t - attack_and_sustain;
            if (params.decay_time > 0.0f) {
                return std::max(0.0f, 1.0f - (decay_elapsed / params.decay_time));
            }

            return 0.0f;
        }

        float generate_waveform_sample(WaveType type, float phase, float square_duty, uint32_t& noise_seed) {
            switch (type) {
                case SQUARE:
                    return (phase < square_duty) ? AudioConfig::SQUARE_WAVE_DUTY_GAIN : -AudioConfig::SQUARE_WAVE_DUTY_GAIN;
                case SAWTOOTH:
                    return 1.0f - 2.0f * phase;
                case SINE:
                    return std::sin(phase * 2.0f * AudioConfig::PI);
                case NOISE:
                    noise_seed = noise_seed * AudioConfig::NOISE_LCG_MULT + AudioConfig::NOISE_LCG_INC;
                    return (static_cast<float>(noise_seed) / AudioConfig::NOISE_NORMALIZE_DIV) * 2.0f - 1.0f;
            }
            return 0.0f;
        }

        float render_voice_sample(SfxVoice& voice) {
            const float t = static_cast<float>(voice.current_sample) / static_cast<float>(SAMPLE_RATE);
            const float env_vol = std::clamp(calculate_envelope(voice.params, t), 0.0f, 1.0f);

            // Frequency sweep / slide
            voice.f_freq += voice.f_slide;
            if (voice.f_freq < voice.params.min_frequency) {
                voice.f_freq = voice.params.min_frequency;
            }

            // Vibrato
            float current_freq = voice.f_freq;
            if (voice.params.vibrato_depth > 0.0f) {
                voice.vibrato_phase += voice.params.vibrato_speed * AudioConfig::VIBRATO_SPEED_SCALE;
                current_freq += std::sin(voice.vibrato_phase) * voice.params.vibrato_depth * AudioConfig::VIBRATO_DEPTH_SCALE;
            }

            // Phase accumulation
            voice.phase += current_freq / static_cast<float>(SAMPLE_RATE);
            while (voice.phase >= 1.0f) voice.phase -= 1.0f;
            while (voice.phase < 0.0f)  voice.phase += 1.0f;

            // Waveform generation
            const float raw_sample = generate_waveform_sample(
                voice.params.wave_type, voice.phase, voice.params.square_duty, voice.noise_seed
            );

            voice.current_sample++;
            return raw_sample * env_vol * AudioConfig::SFX_MASTER_GAIN;
        }

        void mix_music(float* output, ma_uint32 frame_count) {
            if (!g_music_loaded || !g_music_playing || g_music_paused) {
                return;
            }

            const int bytes_requested = static_cast<int>(frame_count * sizeof(float) * STEREO_CHANNELS);
            const int bytes_rendered = pocketmod_render(&g_pocketmod, output, bytes_requested);

            for (ma_uint32 i = 0; i < frame_count * STEREO_CHANNELS; ++i) {
                output[i] *= g_music_volume;
            }

            if (bytes_rendered == 0 && g_music_loop && g_music_data) {
                pocketmod_init(&g_pocketmod, g_music_data, static_cast<int>(g_music_size), SAMPLE_RATE);
            }
        }

        void mix_voices(float* output, ma_uint32 frame_count) {
            for (auto& voice : g_voices) {
                if (!voice.active) continue;

                const float left_gain  = voice.volume * std::clamp(1.0f - voice.pan, 0.0f, 1.0f);
                const float right_gain = voice.volume * std::clamp(1.0f + voice.pan, 0.0f, 1.0f);

                for (ma_uint32 i = 0; i < frame_count; ++i) {
                    if (voice.current_sample >= voice.total_samples) {
                        voice.active = false;
                        break;
                    }

                    const float final_sample = render_voice_sample(voice);
                    output[i * STEREO_CHANNELS]     += final_sample * left_gain;
                    output[i * STEREO_CHANNELS + 1] += final_sample * right_gain;
                }
            }
        }

        SfxVoice* allocate_voice_slot() {
            for (auto& v : g_voices) {
                if (!v.active) {
                    return &v;
                }
            }

            // Voice stealing: select active voice with minimum remaining samples
            SfxVoice* slot = nullptr;
            uint32_t min_remaining = UINT32_MAX;
            for (auto& v : g_voices) {
                const uint32_t remaining = (v.total_samples > v.current_sample) ? (v.total_samples - v.current_sample) : 0;
                if (remaining < min_remaining) {
                    min_remaining = remaining;
                    slot = &v;
                }
            }
            return slot;
        }

    } // namespace

    // --- MINIAUDIO STREAM CALLBACK ---
    static void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        (void)pDevice;
        (void)pInput;

        float* pOutputF32 = static_cast<float*>(pOutput);

        // Clear output buffer with silence
        std::fill_n(pOutputF32, frameCount * STEREO_CHANNELS, 0.0f);

        std::lock_guard<std::mutex> lock(g_audio_mutex);

        // Render music track and active SFX voice mix
        mix_music(pOutputF32, frameCount);
        mix_voices(pOutputF32, frameCount);
    }

    bool init() {
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = STEREO_CHANNELS; // Stereo stream
        config.sampleRate        = SAMPLE_RATE;
        config.dataCallback      = audio_data_callback;

        if (ma_device_init(NULL, &config, &g_audio_device) != MA_SUCCESS) {
            if constexpr (ALX_ENABLE_DEBUG) {
                Log::error("Failed to initialize miniaudio device!");
            }
            return false;
        }

        if (ma_device_start(&g_audio_device) != MA_SUCCESS) {
            if constexpr (ALX_ENABLE_DEBUG) {
                Log::error("Failed to start miniaudio device!");
            }
            ma_device_uninit(&g_audio_device);
            return false;
        }

        g_initialized = true;

        return true;
    }

    void cleanup() {
        if (g_initialized) {
            ma_device_uninit(&g_audio_device);
            g_initialized = false;
        }
    }

    void play_sfx(const SfxrParams& params, float volume_override, float pan) {
        if (!g_initialized) return;

        const float total_time = params.attack_time + params.sustain_time + params.decay_time;
        const uint32_t total_samples = static_cast<uint32_t>(total_time * static_cast<float>(SAMPLE_RATE));
        if (total_samples == 0) return;

        const float base_vol = (volume_override >= 0.0f) ? volume_override : DEFAULT_SFX_VOLUME;
        const float target_vol = base_vol * params.gain;

        const float clamped_vol = std::clamp(target_vol, MIN_VOLUME, MAX_SFX_VOLUME);
        const float clamped_pan = std::clamp(pan, MIN_SFX_PAN, MAX_SFX_PAN);

        std::lock_guard<std::mutex> lock(g_audio_mutex);

        SfxVoice* slot = allocate_voice_slot();
        if (!slot) return;

        static uint32_t s_seed_counter = 1u;
        s_seed_counter = s_seed_counter * AudioConfig::NOISE_LCG_MULT + AudioConfig::NOISE_LCG_INC;

        slot->params = params;
        slot->volume = clamped_vol;
        slot->pan = clamped_pan;
        slot->current_sample = 0;
        slot->total_samples = total_samples;
        slot->phase = 0.0f;
        slot->f_freq = params.start_frequency * params.start_frequency * AudioConfig::FREQ_SCALE_HZ;
        slot->f_slide = params.slide * AudioConfig::SLIDE_SCALE;
        slot->vibrato_phase = 0.0f;
        slot->noise_seed = s_seed_counter;
        slot->active = true;
    }

    bool load_music_from_memory(const uint8_t* data, size_t size) {
        std::lock_guard<std::mutex> lock(g_audio_mutex);

        g_music_data = data;
        g_music_size = size;

        if (!pocketmod_init(&g_pocketmod, data, static_cast<int>(size), SAMPLE_RATE)) {
            g_music_loaded = false;
            g_music_playing = false;
            g_music_paused = false;
            return false;
        }

        g_music_loaded = true;
        g_music_playing = false;
        g_music_paused = false;
        return true;
    }

    void play_music(bool loop) {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        if (!g_music_loaded) return;

        g_music_loop = loop;
        g_music_playing = true;
        g_music_paused = false;
    }

    void pause_music() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        if (g_music_playing) {
            g_music_paused = true;
        }
    }

    void resume_music() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        if (g_music_playing) {
            g_music_paused = false;
        }
    }

    void toggle_music() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        if (!g_music_playing) return;
        g_music_paused = !g_music_paused;
    }

    void stop_music() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        g_music_playing = false;
        g_music_paused = false;

        // Rewind track position back to pattern 0
        if (g_music_loaded && g_music_data) {
            pocketmod_init(&g_pocketmod, g_music_data, static_cast<int>(g_music_size), SAMPLE_RATE);
        }
    }

    bool is_music_loaded() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        return g_music_loaded;
    }

    bool is_music_playing() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        return g_music_loaded && g_music_playing && !g_music_paused;
    }

    bool is_music_paused() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        return g_music_loaded && g_music_playing && g_music_paused;
    }

    void set_music_volume(float volume) {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        g_music_volume = std::clamp(volume, MIN_VOLUME, MAX_MUSIC_VOLUME);
    }
} // namespace Audio
