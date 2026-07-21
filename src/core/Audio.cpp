#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

extern "C" {
#include "pocketmod.h"
}

#include <algorithm>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <random>
#include <thread>

#include "Audio.h"
#include "Log.h"

namespace Audio {

    struct ActiveVoice {
        std::vector<float> samples;
        size_t cursor = 0;
        bool finished = false;
    };

    static ma_device g_audio_device;
    static bool g_initialized = false;
    static std::vector<ActiveVoice> g_active_voices;
    static std::mutex g_audio_mutex;

    // Music State
    static pocketmod_context g_pocketmod;
    static const uint8_t* g_music_data = nullptr;
    static size_t g_music_size = 0;
    static bool g_music_loaded = false;
    static bool g_music_playing = false;
    static bool g_music_paused = false;
    static bool g_music_loop = true;
    static float g_music_volume = 0.5f;

    // --- SFXR PCM SYNTHESIZER GENERATOR ---
    static std::vector<float> generate_sfx_buffer(const SfxrParams& p, uint32_t sample_rate = 44100) {
        float total_time = p.attack_time + p.sustain_time + p.decay_time;
        size_t total_samples = static_cast<size_t>(total_time * sample_rate);
        if (total_samples == 0) return {};

        std::vector<float> buffer(total_samples, 0.0f);

        float phase = 0.0f;
        float f_freq = p.start_frequency * p.start_frequency * 8000.0f; // Map float to Hz range
        float f_slide = p.slide * 100.0f;
        float vibrato_phase = 0.0f;

        for (size_t i = 0; i < total_samples; ++i) {
            float t = static_cast<float>(i) / sample_rate;

            // Envelopes
            float env_vol = 0.0f;
            if (t < p.attack_time) {
                env_vol = t / p.attack_time;
            } else if (t < p.attack_time + p.sustain_time) {
                env_vol = 1.0f;
            } else {
                float decay_elapsed = t - (p.attack_time + p.sustain_time);
                env_vol = 1.0f - (decay_elapsed / p.decay_time);
            }
            env_vol = std::clamp(env_vol, 0.0f, 1.0f);

            // Frequency sweep/slide
            f_freq += f_slide;
            if (f_freq < p.min_frequency) f_freq = p.min_frequency;

            // Vibrato
            float current_freq = f_freq;
            if (p.vibrato_depth > 0.0f) {
                vibrato_phase += p.vibrato_speed * 0.05f;
                current_freq += std::sin(vibrato_phase) * p.vibrato_depth * 100.0f;
            }

            // Phase accumulation
            phase += current_freq / sample_rate;
            while (phase >= 1.0f) phase -= 1.0f;

            // Waveform generation
            float sample = 0.0f;
            switch (p.wave_type) {
                case SQUARE:
                    sample = (phase < p.square_duty) ? 0.5f : -0.5f;
                    break;
                case SAWTOOTH:
                    sample = 1.0f - 2.0f * phase;
                    break;
                case SINE:
                    sample = std::sin(phase * 2.0f * 3.14159265f);
                    break;
                case NOISE:
                    {
                        thread_local std::mt19937 generator(std::random_device{}());
                        std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
                        sample = distribution(generator);
                    }
                    break;
            }

            buffer[i] = sample * env_vol * 0.25f; // Master volume scaling
        }

        return buffer;
    }

    // --- MINIAUDIO STREAM CALLBACK ---
    static void audio_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        float* pOutputF32 = static_cast<float*>(pOutput);

        // Clear output buffer with silence
        std::fill_n(pOutputF32, frameCount, 0.0f);

        std::lock_guard<std::mutex> lock(g_audio_mutex);

        // Render Music via pocketmod if playing
        if (g_music_loaded && g_music_playing && !g_music_paused) {
            int bytes_requested = frameCount * sizeof(float) * 2;
            int bytes_rendered = pocketmod_render(&g_pocketmod, pOutputF32, bytes_requested);

            // Apply music volume scaling
            for (ma_uint32 i = 0; i < frameCount * 2; ++i) {
                pOutputF32[i] *= g_music_volume;
            }

            // Loop music if finished
            if (bytes_rendered == 0 && g_music_loop && g_music_data) {
                pocketmod_init(&g_pocketmod, g_music_data, static_cast<int>(g_music_size), 44100);
            }
        }

        // Mix active SFXR voices on top of music
        for (auto& voice : g_active_voices) {
            if (voice.finished) continue;

            for (ma_uint32 i = 0; i < frameCount; ++i) {
                if (voice.cursor < voice.samples.size()) {
                    float s = voice.samples[voice.cursor++];
                    pOutputF32[i * 2]     += s;
                    pOutputF32[i * 2 + 1] += s;
                } else {
                    voice.finished = true;
                    break;
                }
            }
        }

        // Clean up completed SFX voices
        g_active_voices.erase(
            std::remove_if(g_active_voices.begin(), g_active_voices.end(),
                           [](const ActiveVoice& v) { return v.finished; }),
            g_active_voices.end()
        );
    }

    bool init() {
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format   = ma_format_f32;
        config.playback.channels = 2; // Stereo stream
        config.sampleRate        = 44100;
        config.dataCallback      = audio_data_callback;

        if (ma_device_init(NULL, &config, &g_audio_device) != MA_SUCCESS) {
            Log::error("Failed to initialize miniaudio device!");
            return false;
        }

        if (ma_device_start(&g_audio_device) != MA_SUCCESS) {
            Log::error("Failed to start miniaudio device!");
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

    void play_sfx(const SfxrParams& params) {
        if (!g_initialized) return;

        std::thread([params]() {
            std::vector<float> samples = generate_sfx_buffer(params, 44100);
            if (samples.empty()) return;

            std::lock_guard<std::mutex> lock(g_audio_mutex);
            if (g_initialized) {
                g_active_voices.push_back({ std::move(samples), 0, false });
            }
        }).detach();
    }

    bool load_music_from_memory(const uint8_t* data, size_t size) {
        std::lock_guard<std::mutex> lock(g_audio_mutex);

        g_music_data = data;
        g_music_size = size;

        if (!pocketmod_init(&g_pocketmod, data, static_cast<int>(size), 44100)) {
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
            pocketmod_init(&g_pocketmod, g_music_data, static_cast<int>(g_music_size), 44100);
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
        g_music_volume = std::clamp(volume, 0.0f, 1.0f);
    }
}
