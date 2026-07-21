#pragma once
#include <cstdint>

inline uint32_t blend_pixel(uint32_t dest, uint32_t src) {
    uint32_t src_alpha = (src >> 24) & 0xFF;
    if (src_alpha == 0xFF) return src;
    if (src_alpha == 0x00) return dest;

    uint32_t src_r = (src >> 16) & 0xFF;
    uint32_t src_g = (src >> 8) & 0xFF;
    uint32_t src_b = src & 0xFF;

    uint32_t dest_alpha = (dest >> 24) & 0xFF;
    uint32_t dest_r = (dest >> 16) & 0xFF;
    uint32_t dest_g = (dest >> 8) & 0xFF;
    uint32_t dest_b = dest & 0xFF;

    uint32_t out_r = (src_r * src_alpha + dest_r * (255 - src_alpha)) / 255;
    uint32_t out_g = (src_g * src_alpha + dest_g * (255 - src_alpha)) / 255;
    uint32_t out_b = (src_b * src_alpha + dest_b * (255 - src_alpha)) / 255;
    uint32_t out_a = src_alpha + (dest_alpha * (255 - src_alpha)) / 255;

    return (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
}
