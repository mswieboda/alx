#pragma once
#include <cstdint>
#include "core/Draw.h"
#include "alx/Layer.h"

namespace alx {
namespace DrawFX {

inline void shadow(
    float entity_x, float entity_y, float entity_w, float entity_h,
    int z_index, int sort_y_override,
    float rx_ratio = 0.8f, float ry_ratio_of_rx = 0.45f,
    uint32_t color = 0x60000000
) {
    float shadow_cx = entity_x + (entity_w / 2.0f);
    float shadow_cy = entity_y + entity_h;
    float shadow_rx = entity_w * rx_ratio;
    float shadow_ry = shadow_rx * ry_ratio_of_rx;
    Draw::oval(
        shadow_cx,
        shadow_cy,
        shadow_rx,
        shadow_ry,
        color,
        true, // fill
        1,    // thickness
        z_index,
        sort_y_override
    );
}

} // namespace DrawFX
} // namespace alx
