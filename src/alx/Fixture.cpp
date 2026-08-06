#include "alx/Fixture.h"

namespace alx {

int get_fixture_ports(FixtureType type, PortLocation out_ports[4]) noexcept {
    switch (type) {
        case FixtureType::Refiner:
            out_ports[0] = { 0, 1, -1,  0 }; // West
            out_ports[1] = { 2, 1,  1,  0 }; // East
            out_ports[2] = { 1, 0,  0, -1 }; // North
            out_ports[3] = { 1, 2,  0,  1 }; // South
            return 4;
        case FixtureType::Spire:
            out_ports[0] = { 1, 2,  0,  1 }; // South at (root_x+1, root_y+2)
            return 1;
        case FixtureType::Seep:
            out_ports[0] = { 1, 0,  0, -1 }; // North at (root_x+1, root_y)
            out_ports[1] = { 1, 1,  0,  1 }; // South at (root_x+1, root_y+1)
            return 2;
        default:
            return 0;
    }
}

bool is_fixture_port(FixtureType type, int8_t off_x, int8_t off_y, int8_t face_dx, int8_t face_dy) noexcept {
    PortLocation ports[4];
    int count = get_fixture_ports(type, ports);
    for (int i = 0; i < count; ++i) {
        if (ports[i].off_x == off_x && ports[i].off_y == off_y &&
            ports[i].face_dx == face_dx && ports[i].face_dy == face_dy) {
            return true;
        }
    }
    return false;
}

int max_fixture_footprint_dimension() noexcept {
    int max_dim = 1;
    constexpr FixtureType all_types[] = {
        FixtureType::Pipe,
        FixtureType::Refiner,
        FixtureType::Spire,
        FixtureType::Seep
    };
    for (FixtureType t : all_types) {
        MultiTileFootprint fp = get_fixture_footprint(t);
        if (fp.width > max_dim) max_dim = fp.width;
        if (fp.height > max_dim) max_dim = fp.height;
    }
    return max_dim;
}

Collision::AABB fixture_ground_aabb(int tx, int ty, float tile_size, FixtureType type) {
    MultiTileFootprint fp = get_fixture_footprint(type);
    float total_w = static_cast<float>(fp.width) * tile_size;
    float total_h = static_cast<float>(fp.height) * tile_size;

    if (type == FixtureType::Pipe || type == FixtureType::None) {
        float w = tile_size * FixtureConstants::GROUND_WIDTH_RATIO;
        float h = tile_size * FixtureConstants::GROUND_HEIGHT_RATIO;
        float x = static_cast<float>(tx) * tile_size + (tile_size - w) / 2.0f;
        float y = static_cast<float>(ty) * tile_size + (tile_size * FixtureConstants::GROUND_OFFSET_Y_RATIO);
        return Collision::AABB{ x, y, w, h };
    }

    // Multi-tile solid building (Refiner, Spire): full 100% grid footprint ground AABB
    float x = static_cast<float>(tx) * tile_size;
    float y = static_cast<float>(ty) * tile_size;
    return Collision::AABB{ x, y, total_w, total_h };
}

} // namespace alx
