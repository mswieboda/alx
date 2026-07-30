#pragma once
#include <algorithm>
#include <cmath>

namespace Collision {
    struct Circle {
        float cx = 0.0f;
        float cy = 0.0f;
        float radius = 0.0f;
    };

    struct AABB {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    inline bool aabb(float x1, float y1, float w1, float h1,
                     float x2, float y2, float w2, float h2) {
        return (x1 < x2 + w2 && x1 + w1 > x2 &&
                y1 < y2 + h2 && y1 + h1 > y2);
    }

    inline bool circle_vs_circle(const Circle& a, const Circle& b) {
        float dx = a.cx - b.cx;
        float dy = a.cy - b.cy;
        float min_dist = a.radius + b.radius;
        return (dx * dx + dy * dy) < (min_dist * min_dist);
    }

    inline bool circle_vs_circle(float cx1, float cy1, float r1, float cx2, float cy2, float r2) {
        float dx = cx1 - cx2;
        float dy = cy1 - cy2;
        float min_dist = r1 + r2;
        return (dx * dx + dy * dy) < (min_dist * min_dist);
    }

    inline bool circle_vs_aabb(const Circle& c, float rx, float ry, float rw, float rh) {
        float closest_x = std::clamp(c.cx, rx, rx + rw);
        float closest_y = std::clamp(c.cy, ry, ry + rh);
        float dx = c.cx - closest_x;
        float dy = c.cy - closest_y;
        return (dx * dx + dy * dy) < (c.radius * c.radius);
    }

    inline bool circle_vs_aabb(float cx, float cy, float r, float rx, float ry, float rw, float rh) {
        float closest_x = std::clamp(cx, rx, rx + rw);
        float closest_y = std::clamp(cy, ry, ry + rh);
        float dx = cx - closest_x;
        float dy = cy - closest_y;
        return (dx * dx + dy * dy) < (r * r);
    }

    inline bool circle_vs_aabb(const Circle& c, const AABB& b) {
        return circle_vs_aabb(c.cx, c.cy, c.radius, b.x, b.y, b.w, b.h);
    }


    inline bool resolve_soft_circle_overlap(float cx1, float cy1, float r1,
                                            float cx2, float cy2, float r2,
                                            float& push_x1, float& push_y1,
                                            float& push_x2, float& push_y2) {
        float dx = cx1 - cx2;
        float dy = cy1 - cy2;
        float dist_sq = dx * dx + dy * dy;
        float min_dist = r1 + r2;

        if (dist_sq < min_dist * min_dist && dist_sq > 0.0001f) {
            float dist = std::sqrt(dist_sq);
            float overlap = 0.5f * (min_dist - dist);
            float nx = dx / dist;
            float ny = dy / dist;

            push_x1 = nx * overlap;
            push_y1 = ny * overlap;
            push_x2 = -nx * overlap;
            push_y2 = -ny * overlap;
            return true;
        }
        push_x1 = push_y1 = push_x2 = push_y2 = 0.0f;
        return false;
    }
}
