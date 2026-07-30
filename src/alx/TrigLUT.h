#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace alx {

class TrigLUT {
public:
    static constexpr int SHIFT = 12;
    static constexpr int SCALE = 1 << SHIFT; // 4096

    static inline int32_t cos(int angle_deg) {
        int idx = (angle_deg % 360 + 360) % 360;
        return COS_TABLE[idx];
    }

    static inline int32_t sin(int angle_deg) {
        int idx = (angle_deg % 360 + 360) % 360;
        return SIN_TABLE[idx];
    }

private:
    static inline std::array<int32_t, 360> generate_cos() {
        std::array<int32_t, 360> table{};
        constexpr double pi = 3.14159265358979323846;
        for (int i = 0; i < 360; ++i) {
            double rad = static_cast<double>(i) * pi / 180.0;
            table[i] = static_cast<int32_t>(std::round(std::cos(rad) * SCALE));
        }
        return table;
    }

    static inline std::array<int32_t, 360> generate_sin() {
        std::array<int32_t, 360> table{};
        constexpr double pi = 3.14159265358979323846;
        for (int i = 0; i < 360; ++i) {
            double rad = static_cast<double>(i) * pi / 180.0;
            table[i] = static_cast<int32_t>(std::round(std::sin(rad) * SCALE));
        }
        return table;
    }

    static inline const std::array<int32_t, 360> COS_TABLE = generate_cos();
    static inline const std::array<int32_t, 360> SIN_TABLE = generate_sin();
};

} // namespace alx
