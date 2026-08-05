#pragma once
#include <cstdint>

struct FacingVector {
    float dx = 0.0f;
    float dy = 1.0f;
};

struct Facing {
    // TODO: maybe switch order to N, NE, E, SE, S, SW, W, NW
    //   UNLESS it breaks something, then DO NOT change
    enum Type : uint8_t {
        East,
        NorthEast,
        North,
        NorthWest,
        West,
        SouthWest,
        South,
        SouthEast
    };

    static Type from_vector(float dx, float dy) {
        if (dx > 0.5f && dy > 0.5f) return SouthEast;
        if (dx > 0.5f && dy < -0.5f) return NorthEast;
        if (dx < -0.5f && dy > 0.5f) return SouthWest;
        if (dx < -0.5f && dy < -0.5f) return NorthWest;
        if (dx > 0.5f) return East;
        if (dx < -0.5f) return West;
        if (dy < -0.5f) return North;
        return South;
    }

    static FacingVector to_vector(Type facing) {
        switch (facing) {
            case East:      return FacingVector{ 1.0f,  0.0f };
            case NorthEast: return FacingVector{ 0.70710678f, -0.70710678f };
            case North:     return FacingVector{ 0.0f, -1.0f };
            case NorthWest: return FacingVector{-0.70710678f, -0.70710678f };
            case West:      return FacingVector{-1.0f,  0.0f };
            case SouthWest: return FacingVector{-0.70710678f,  0.70710678f };
            case South:     return FacingVector{ 0.0f,  1.0f };
            case SouthEast: return FacingVector{ 0.70710678f,  0.70710678f };
            default:        return FacingVector{ 0.0f,  1.0f };
        }
    }

    static int to_degrees(Type facing) {
        switch (facing) {
            case East:      return 0;
            case NorthEast: return 315;
            case North:     return 270;
            case NorthWest: return 225;
            case West:      return 180;
            case SouthWest: return 135;
            case South:     return 90;
            case SouthEast: return 45;
            default:        return 90;
        }
    }

    static bool is_horizontal(Type facing) {
        return facing == East || facing == West;
    }

    static bool is_vertical(Type facing) {
        return facing == North || facing == South;
    }

    static Type opposite(Type facing) {
        switch (facing) {
            case East:      return West;
            case NorthEast: return SouthWest;
            case North:     return South;
            case NorthWest: return SouthEast;
            case West:      return East;
            case SouthWest: return NorthEast;
            case South:     return North;
            case SouthEast: return NorthWest;
            default:        return North;
        }
    }
};

using FacingDir = Facing::Type;
