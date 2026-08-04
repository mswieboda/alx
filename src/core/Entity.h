#pragma once
#include <vector>
#include <string>
#include <variant>
#include <cstdint>
#include "Transform.h"
#include "Font.h"
#include "assets/Fonts.h"
#include "assets/Images.h"

// Renderable variants
struct SpriteRender {
    const uint8_t* pixels;
    uint32_t pixels_size;
    int width;
    int height;
    bool is_flip_h = false;
    bool is_flip_v = false;
};

// Animated Sprite related concepts, for AnimatedSpriteRender
// Defines a single frame's source rectangle inside the master spritesheet texture
struct SpriteFrame {
    int x = 0;             // Pixel X coordinate on 2D sheet (or 0)
    int y = 0;             // Pixel Y coordinate on 2D sheet (or 0)
    int width = 0;         // Width of this specific frame
    int height = 0;        // Height of this specific frame
    int duration_ms = 100; // How long to hold this frame (per-frame custom timing!)
    size_t offset = 0;     // RLE slice byte offset in sheet_pixels
    size_t len = 0;        // RLE slice byte length
};

// A named sequence of frames (e.g., "idle", "run", "jump")
struct SpriteAnimation {
    std::string name;
    std::vector<int> frame_indices; // Crucial: Reuses/reorders frames (e.g., {0, 1, 2, 1, 0})
    bool loop = true;
};

struct AnimatedSpriteRender {
    const uint8_t* sheet_pixels = nullptr;       // Pointer to the raw texture file data
    uint32_t sheet_pixels_size = 0;
    int sheet_width = 0;
    int sheet_height = 0;

    // The frame pool (slice your texture once into this vector during setup)
    std::vector<SpriteFrame> master_frames;

    // Playback state
    SpriteAnimation current_anim;      // The current sequence definition
    int current_sequence_index = 0;    // Where we are in the frame_indices loop
    float elapsed_time_ms = 0.0f;      // Ticker that accumulates delta time (dt)
    bool is_playing = true;
    bool is_flip_h = false;
    bool is_flip_v = false;

    void set_frame(int frame_index) {
        if (frame_index >= 0 && static_cast<size_t>(frame_index) < master_frames.size()) {
            current_anim.frame_indices = { frame_index };
            current_sequence_index = 0;
            elapsed_time_ms = 0.0f;
            is_playing = false;
        }
    }

    void play(const std::string& name = "") {
        if (!name.empty() && current_anim.name != name) {
            current_anim.name = name;
            current_sequence_index = 0;
            elapsed_time_ms = 0.0f;
        }
        is_playing = true;
    }

    void pause() {
        is_playing = false;
    }

    void resume() {
        is_playing = true;
    }

    template <size_t N_FRAMES, size_t N_ANIMS>
    static AnimatedSpriteRender create(
        const uint8_t* pixels,
        size_t pixels_size,
        const Assets::Images::FrameDescriptor (&frames)[N_FRAMES],
        const Assets::Images::TagDescriptor (&anims)[N_ANIMS]
    ) {
        AnimatedSpriteRender anim;
        anim.sheet_pixels = pixels;
        anim.sheet_pixels_size = static_cast<uint32_t>(pixels_size);
        anim.sheet_width = frames[0].width;
        anim.sheet_height = frames[0].height;

        for (size_t i = 0; i < N_FRAMES; ++i) {
            anim.master_frames.push_back(SpriteFrame{
                0, 0,
                static_cast<int>(frames[i].width),
                static_cast<int>(frames[i].height),
                static_cast<int>(frames[i].duration_ms),
                frames[i].offset,
                frames[i].len
            });
        }

        if (N_ANIMS > 0) {
            anim.current_anim.name = anims[0].name;
            anim.current_anim.loop = anims[0].loop;
            for (uint16_t f = anims[0].from_frame; f <= anims[0].to_frame; ++f) {
                anim.current_anim.frame_indices.push_back(static_cast<int>(f));
            }
        }

        return anim;
    }
};

struct RectangleRender {
    uint32_t color;
    bool fill = true;
    int thickness = 1;
};

struct TextRender {
    std::string text;
    uint32_t color = 0xFFFFFFFF;
    int scale = 1;
    const FontData* font = &Font::DEFAULT_BLANK;
};

using RenderComponent = std::variant<SpriteRender, AnimatedSpriteRender, RectangleRender, TextRender>;

struct Entity {
    Transform transform;
    Transform transform_prev;
    RenderComponent visual;
    bool active = true;

    // Simple optional custom tag/ID to identify types (e.g. "player", "coin")
    std::string tag;

    Entity(Transform t, RenderComponent v, bool act = true, std::string tg = "")
        : transform(t), transform_prev(t), visual(std::move(v)), active(act), tag(std::move(tg)) {}
};
